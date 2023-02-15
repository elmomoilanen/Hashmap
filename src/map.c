#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#ifdef __linux__
#include <sys/random.h>
#elif __APPLE__
#include <sys/random.h>
#endif

#include "map.h"
#include "siphash.h"

#define MAP_LOAD_FACTOR_LOWER 0.4
#define MAP_LOAD_FACTOR_UPPER 0.9
#define MAP_MAX_KEY_BYTES 20
#define MAP_TEMP_SLOTS 2

#define BUCKET_HASH_ORIG_BITS 64
#define BUCKET_TOTAL_BITS 32
#define BUCKET_HASH_BITS 20
#define BUCKET_PSL_BITS 11
#define BUCKET_HASH_TRUNC_SIZE ((BUCKET_HASH_ORIG_BITS) - (BUCKET_HASH_BITS))

#define BUCKET_TAKEN_OFFSET 0x0u
#define BUCKET_TAKEN_MASK 0x1u
#define BUCKET_PSL_OFFSET 0x1u
#define BUCKET_PSL_MASK 0x00000FFEu
#define BUCKET_HASH_OFFSET 0xCu
#define BUCKET_HASH_MASK 0xFFFFF000u

#define BUCKET_IS_TAKEN(meta_data) \
    (META_VALUE_GET((meta_data), BUCKET_TAKEN_OFFSET, BUCKET_TAKEN_MASK) & 1U)

#define META_GET_HASH(meta_data) \
    (META_VALUE_GET((meta_data), BUCKET_HASH_OFFSET, BUCKET_HASH_MASK))

#define META_GET_PSL(meta_data) \
    (META_VALUE_GET((meta_data), BUCKET_PSL_OFFSET, BUCKET_PSL_MASK))

#define META_SET_HASH(meta_data, value) \
    (META_VALUE_SET((meta_data), (value), BUCKET_HASH_OFFSET, BUCKET_HASH_MASK))

#define META_SET_PSL(meta_data, value) \
    (META_VALUE_SET((meta_data), (value), BUCKET_PSL_OFFSET, BUCKET_PSL_MASK))

#define META_SET_TAKEN(meta_data, value) \
    (META_VALUE_SET((meta_data), (value), BUCKET_TAKEN_OFFSET, BUCKET_TAKEN_MASK))

#define META_ADD_ONE_TO_PSL(meta_data) \
    (META_SET_PSL((meta_data), META_GET_PSL((meta_data)) + 1U))

#define META_SUBTRACT_ONE_FROM_PSL(meta_data) \
    (META_SET_PSL((meta_data), META_GET_PSL((meta_data)) - 1U))

/*
`BUCKET_TOTAL_BITS`, which is the total bit count of struct `Bucket`
must be compatible with the u32 type.

LSB bit is the "taken" value, 0 if bucket is free and 1 if taken.
Following `BUCKET_PSL_BITS` bits are reserved for the probe sequence length value.
Last `BUCKET_HASH_BITS` bits are reserved for the (truncated) hash value.
*/
struct Bucket {
    u32 meta_data;
};

static u8 randkey[MAP_RAND_KEY_LEN];
static u32 const max_psl = (1U << BUCKET_PSL_BITS) - 1;

static void _clean_up(u32 allocs, void *ptr, ...) {
    va_list args;

    free(ptr);
    va_start(args, ptr);

    for (u32 i=1; i<allocs; ++i) {
        void *p = va_arg(args, void *);
        if (p != NULL) free(p);
    }

    va_end(args);
}

static bool _init_random_key(u8 *buf, size_t buflen) {
    if (buflen == 0) {
        fprintf(stderr, "cannot init random key for zero bytes.\n");
        return false;
    }
    if (buflen > MAP_MAX_RAND_BUF_LEN) {
        fprintf(
            stderr,
            "cannot init random key with more than %u bytes.\n",
            MAP_MAX_RAND_BUF_LEN
        );
        return false;
    }
    errno = 0;
    bool init_success = true;

#ifdef __linux__
    ssize_t result = getrandom(buf, buflen, GRND_NONBLOCK);

    if (result < 0) {
        if (errno) {
            fprintf(stderr, "error when requesting random bytes: %s.\n", strerror(errno));
        } else {
            fprintf(stderr, "unknown error when requesting random bytes.\n");
        }
        init_success = false;
    }
    // buflen is some small positive number (from 1 to MAP_MAX_RAND_BUF_LEN)
    else if (result != (ssize_t)buflen) {
        // should never end up here
        fprintf(stderr, "received less random bytes than requested.\n");
        init_success = false;
    }
#elif __APPLE__
    ssize_t result = getentropy(buf, buflen);

    if (result < 0) {
        if (errno) {
            fprintf(stderr, "error when requesting random bytes: %s.\n", strerror(errno));
        } else {
            fprintf(stderr, "unknown error when requesting random bytes.\n");
        }
        init_success = false;
    }
#else
    fprintf(stderr, "os not recognised to be Linux or MacOS, cannot init random key.");
    init_success = false;
#endif
    return init_success;
}

static u32 get_truncated_hash(char const *key) {
    u64 hash = siphash(key, strlen(key), randkey);
    return hash << BUCKET_HASH_TRUNC_SIZE >> BUCKET_HASH_TRUNC_SIZE;
}

static void _update_bucket_meta(struct Bucket *bucket, u32 psl, u32 hash) {
    bucket->meta_data = META_SET_TAKEN(bucket->meta_data, 1U);
    bucket->meta_data = META_SET_PSL(bucket->meta_data, psl);
    bucket->meta_data = META_SET_HASH(bucket->meta_data, hash);
}

static bool _keys_are_equal(char const *left, char const *right) {
    return strncmp(left, right, MAP_MAX_KEY_BYTES) == 0;
}

static void _hmap_init_set_size_members(struct HashMap *hashmap, u32 item_size, u32 ex_capa) {
    hashmap->sz_bucket = sizeof(struct Bucket);
    hashmap->sz_key = MAP_MAX_KEY_BYTES;
    hashmap->sz_item = item_size;

    u32 slot_size = hashmap->sz_bucket + hashmap->sz_key + hashmap->sz_item;
    slot_size += (slot_size % sizeof(void *));
    hashmap->sz_slot = slot_size;

    hashmap->ex_capa = ex_capa;
}

static struct HashMap* _hmap_init_common(u32 item_size, u32 ex_capa) {
    struct HashMap *hashmap = calloc(1, sizeof *hashmap);

    if (hashmap == NULL) {
        return NULL;
    }

    _hmap_init_set_size_members(hashmap, item_size, ex_capa);
    size_t const init_slot_count = 1U << hashmap->ex_capa;

    hashmap->slots = calloc(init_slot_count, hashmap->sz_slot);

    if (hashmap->slots == NULL) {
        free(hashmap);
        return NULL;
    }

    hashmap->_temp = calloc(MAP_TEMP_SLOTS, hashmap->sz_slot);

    if (hashmap->_temp == NULL) {
        _clean_up(MAP_TEMP_SLOTS, hashmap->slots, hashmap);
        return NULL;
    }

    return hashmap;
}

static struct HashMap* _hmap_init(
    u32 item_size,
    u32 init_capa,
    void (*clean_func)(void *),
    bool use_random_key)
{
    size_t const rkey_len = sizeof(randkey) / sizeof(randkey[0]);

    if (use_random_key) {
        if (!_init_random_key(randkey, rkey_len)) {
            return NULL; 
        }
    } else {
        for (u32 i=0; i<rkey_len; ++i) {
            randkey[i] = 0;
        }
    }

    struct HashMap *hashmap = _hmap_init_common(item_size, init_capa);
    if (hashmap == NULL) return NULL;

    hashmap->occ_slots = 0;
    hashmap->clean_func = clean_func;

    return hashmap;
}

static struct HashMap* _hmap_init_resized(u32 item_size, u32 ex_capa) {
    return _hmap_init_common(item_size, ex_capa);
}

static void _clean_hashmap_slots(struct HashMap *hashmap) {
    clean_func_type clean_data_func = hashmap->clean_func ? hashmap->clean_func : NULL;

    if (clean_data_func) {
        u32 const total_capacity = 1U << hashmap->ex_capa;

        for (u32 j=0; j<total_capacity; ++j) {
            struct Bucket *bucket = (struct Bucket *)
                ((char *)hashmap->slots + hashmap->sz_slot * j);

            if(BUCKET_IS_TAKEN(bucket->meta_data)) {
                clean_data_func((char *)bucket + hashmap->sz_bucket + hashmap->sz_key);
            }
        }
    }

    free(hashmap->slots);
}

static void _hmap_free(struct HashMap *hashmap) {
    _clean_hashmap_slots(hashmap);
    free(hashmap->_temp);
    free(hashmap);
}

static bool _hmap_resize(struct HashMap *hashmap, u32 new_ex_capa) {
    struct HashMap *new_hashmap = _hmap_init_resized(hashmap->sz_item, new_ex_capa);
    if (new_hashmap == NULL) {
        return false;
    }

    u32 const current_capacity = 1U << hashmap->ex_capa;
    u32 const new_mask = (1U << new_hashmap->ex_capa) - 1;

    for (u32 j=0; j<current_capacity; ++j) {
        struct Bucket *bucket = (struct Bucket *)
            ((char *)hashmap->slots + hashmap->sz_slot * j);

        if (!BUCKET_IS_TAKEN(bucket->meta_data)) continue;

        u32 idx = META_GET_HASH(bucket->meta_data) & new_mask;
        bucket->meta_data = META_SET_PSL(bucket->meta_data, 0U);

        while (true) {
            struct Bucket *new_bucket = (struct Bucket *)
                ((char *)new_hashmap->slots + new_hashmap->sz_slot * idx);

            if (!BUCKET_IS_TAKEN(new_bucket->meta_data)) {
                memcpy(new_bucket, bucket, hashmap->sz_slot);
                break;
            }
            if (META_GET_PSL(bucket->meta_data) > META_GET_PSL(new_bucket->meta_data)) {
                // Occupied slot but the key in this slot is "richer", so make a swap
                memcpy(
                    (char *)new_hashmap->_temp + new_hashmap->sz_slot,
                    new_bucket,
                    new_hashmap->sz_slot
                );
                memcpy(new_bucket, bucket, hashmap->sz_slot);
                memcpy(
                    bucket,
                    (char *)new_hashmap->_temp + new_hashmap->sz_slot,
                    new_hashmap->sz_slot
                );
            }
            if (META_GET_PSL(bucket->meta_data) >= max_psl) {
                // Maximal probe sequence length reached, unable to resize
                free(new_hashmap->slots);
                free(new_hashmap->_temp);
                free(new_hashmap);
                return false;
            }
            bucket->meta_data = META_ADD_ONE_TO_PSL(bucket->meta_data);
            idx = (idx + 1) & new_mask;
        }
    }
    // Clean memory from old hashmap but do not follow possible pointers as
    // new_hashmap->slots points then also to those same locations.
    // There is no need to touch hashmap->_temp and also hmap_remove needs that memory.
    free(hashmap->slots);
    hashmap->slots = new_hashmap->slots;
    hashmap->ex_capa = new_hashmap->ex_capa;

    free(new_hashmap->_temp);
    free(new_hashmap);

    return true;
}

static void* _hmap_get(struct HashMap *hashmap, char const *key) {
    u32 const hash_trunc = get_truncated_hash(key);
    u32 const mask = (1U << hashmap->ex_capa) - 1;
    u32 idx = hash_trunc & mask, psl = 0;

    while (true) {
        struct Bucket *bucket = (struct Bucket *)
            ((char *)hashmap->slots + hashmap->sz_slot * idx);

        if (!BUCKET_IS_TAKEN(bucket->meta_data) || META_GET_PSL(bucket->meta_data) < psl) {
            return NULL;
        }
        if (META_GET_HASH(bucket->meta_data) == hash_trunc &&
            _keys_are_equal(key, (char *)bucket + hashmap->sz_bucket))
        {
            return (char *)bucket + hashmap->sz_bucket + hashmap->sz_key;
        }
        psl++;
        idx = (idx + 1) & mask;
    }
}

static bool _hmap_insert(struct HashMap *hashmap, char const *key, void const *data) {
    u32 hash_trunc = get_truncated_hash(key);
    u32 const mask = (1U << hashmap->ex_capa) - 1;
    u32 idx = hash_trunc & mask, psl = 0;

    char key_buffer[MAP_MAX_KEY_BYTES] = {0};
    strncpy(key_buffer, key, MAP_MAX_KEY_BYTES - 1);
    memcpy((char *)hashmap->_temp + hashmap->sz_bucket, key_buffer, hashmap->sz_key);

    memcpy(
        (char *)hashmap->_temp + hashmap->sz_bucket + hashmap->sz_key,
        data,
        hashmap->sz_item
    );

    while (true) {
        struct Bucket *bucket = (struct Bucket *)
            ((char *)hashmap->slots + hashmap->sz_slot * idx);

        if (!BUCKET_IS_TAKEN(bucket->meta_data)) {
            _update_bucket_meta(bucket, psl, hash_trunc);
            memcpy(
                (char *)bucket + hashmap->sz_bucket,
                (char *)hashmap->_temp + hashmap->sz_bucket,
                hashmap->sz_key + hashmap->sz_item
            );
            hashmap->occ_slots += 1;
            return true;
        }
        if (META_GET_HASH(bucket->meta_data) == hash_trunc &&
            _keys_are_equal(
                (char *)hashmap->_temp + hashmap->sz_bucket,
                (char *)bucket + hashmap->sz_bucket
            ))
        {
            // Keys have the same hash, replace data
            bucket->meta_data = META_SET_PSL(bucket->meta_data, psl);
            memcpy(
                (char *)bucket + hashmap->sz_bucket + hashmap->sz_key,
                (char *)hashmap->_temp + hashmap->sz_bucket + hashmap->sz_key,
                hashmap->sz_item
            );
            return true;
        }
        if (psl > META_GET_PSL(bucket->meta_data)) {
            // Occupied slot but the key in this slot is "richer", so make a swap
            memcpy((char *)hashmap->_temp + hashmap->sz_slot, (char *)bucket, hashmap->sz_slot);

            _update_bucket_meta(bucket, psl, hash_trunc);
            memcpy(
                (char *)bucket + hashmap->sz_bucket,
                (char *)hashmap->_temp + hashmap->sz_bucket,
                hashmap->sz_key + hashmap->sz_item
            );
            memcpy(
                hashmap->_temp,
                (char *)hashmap->_temp + hashmap->sz_slot,
                hashmap->sz_slot
            );

            hash_trunc = META_GET_HASH(((struct Bucket *)(hashmap->_temp))->meta_data);
            psl = META_GET_PSL(((struct Bucket *)(hashmap->_temp))->meta_data);
        }
        if (psl >= max_psl) {
            // Maximal probe sequence length reached
            fprintf(
                stderr,
                "maximal probe sequence length %u reached, cannot insert key %s",
                max_psl,
                key
            );
            return false;
        }
        psl++;
        idx = (idx + 1) & mask;
    }
}

static void* _hmap_remove(struct HashMap *hashmap, char const *key) {
    u32 const hash_trunc = get_truncated_hash(key);
    u32 const mask = (1U << hashmap->ex_capa) - 1;
    u32 idx = hash_trunc & mask, psl = 0;

    struct Bucket *prev_bucket;

    while (true) {
        struct Bucket *bucket = (struct Bucket *)
            ((char *)hashmap->slots + hashmap->sz_slot * idx);

        if (!BUCKET_IS_TAKEN(bucket->meta_data) || META_GET_PSL(bucket->meta_data) < psl) {
            // Targeted key not in the hash map, nothing to remove
            return NULL;
        }
        if (META_GET_HASH(bucket->meta_data) == hash_trunc &&
            _keys_are_equal(key, (char *)bucket + hashmap->sz_bucket))
        {
            // Target key found, copy slot contents to temp location
            memcpy(hashmap->_temp, bucket, hashmap->sz_slot);
            prev_bucket = bucket;
            break;
        }
        psl++;
        idx = (idx + 1) & mask;
    }
    hashmap->occ_slots -= 1;

    // Start backward shifting
    while (true) {
        idx = (idx + 1) & mask;
        struct Bucket *bucket = (struct Bucket *)
            ((char *)hashmap->slots + hashmap->sz_slot * idx);

        if (!BUCKET_IS_TAKEN(bucket->meta_data) || META_GET_PSL(bucket->meta_data) == 0) {
            // Nothing to shift anymore
            prev_bucket->meta_data = META_SET_TAKEN(prev_bucket->meta_data, 0U);
            break;
        }
        memcpy(prev_bucket, bucket, hashmap->sz_slot);
        prev_bucket->meta_data = META_SUBTRACT_ONE_FROM_PSL(prev_bucket->meta_data);
        prev_bucket = bucket;
    }

    if (hashmap->ex_capa > MAP_INIT_EXP_CAPACITY &&
        hashmap->occ_slots <= (1U << hashmap->ex_capa) * MAP_LOAD_FACTOR_LOWER)
    {
        // Hash map too sparse, resize down as much as possible
        u32 new_ex_capa = hashmap->ex_capa - 1;
        while (
            new_ex_capa > MAP_INIT_EXP_CAPACITY &&
            hashmap->occ_slots <= (1U << new_ex_capa) * MAP_LOAD_FACTOR_LOWER
        )
        {
            new_ex_capa -= 1;
        }
        _hmap_resize(hashmap, new_ex_capa);
    }

    return (char *)hashmap->_temp + hashmap->sz_bucket + hashmap->sz_key;
}

bool get_random_key(u8 *buffer, size_t buffer_len) {
    return _init_random_key(buffer, buffer_len);
}

struct HashMap* hmap_init(size_t item_size, u32 init_capa, void (*clean_func)(void *)) {
    if (init_capa < MAP_INIT_EXP_CAPACITY) {
        init_capa = MAP_INIT_EXP_CAPACITY;
    }
    else if (init_capa > MAP_MAX_EXP_CAPACITY) {
        fprintf(
            stderr,
            "cannot allocate a hash map with capacity over 2^%u.\n",
            MAP_MAX_EXP_CAPACITY
        );
        return NULL;
    }

    size_t const sz_meta_chunk = sizeof(struct Bucket) + MAP_MAX_KEY_BYTES;

    if (item_size < UINT32_MAX - sz_meta_chunk) {
        u32 const sz_slot_raw = item_size + sz_meta_chunk;

        if (sz_slot_raw < UINT32_MAX - sz_slot_raw % sizeof(void *)) {
            return _hmap_init(item_size, init_capa, clean_func, true);
        }
    }
    return NULL;
}

struct HashMap* hmap_init_with_key(size_t item_size, void (*clean_func)(void *)) {
    size_t const sz_meta_chunk = sizeof(struct Bucket) + MAP_MAX_KEY_BYTES;

    if (item_size < UINT32_MAX - sz_meta_chunk) {
        u32 const sz_slot_raw = item_size + sz_meta_chunk;

        if (sz_slot_raw < UINT32_MAX - sz_slot_raw % sizeof(void *)) {
            // Init with a deterministic key, use only for testing
            return _hmap_init(item_size, MAP_INIT_EXP_CAPACITY, clean_func, false);
        }
    }
    return NULL;
}

void hmap_free(struct HashMap *hashmap) {
    if (hashmap != NULL) {
        _hmap_free(hashmap);
    }
}

void* hmap_get(struct HashMap *hashmap, char const *key) {
    return (key == NULL || strlen(key) > MAP_MAX_KEY_BYTES - 1) ? NULL :
        _hmap_get(hashmap, key);
}

bool hmap_insert(struct HashMap *hashmap, char const *key, void const *data) {
    if (key == NULL || strlen(key) > MAP_MAX_KEY_BYTES - 1 || data == NULL) {
        return false;
    }
    if (hashmap->occ_slots >= (1U << hashmap->ex_capa) * MAP_LOAD_FACTOR_UPPER) {
        if (hashmap->ex_capa == MAP_MAX_EXP_CAPACITY) {
            fprintf(
                stderr,
                "hash map capacity cannot be increased over 2^%u.\n",
                MAP_MAX_EXP_CAPACITY
            );
            return false;
        }
        if (!_hmap_resize(hashmap, hashmap->ex_capa + 1)) {
            return false;
        }
    }
    return _hmap_insert(hashmap, key, data);
}

void* hmap_remove(struct HashMap *hashmap, char const *key) {
    return (key == NULL || strlen(key) > MAP_MAX_KEY_BYTES - 1) ? NULL :
        _hmap_remove(hashmap, key);
}

bool hmap_iter_apply(struct HashMap *hashmap, bool (*callback)(char const *, void *)) {
    u32 const total_capacity = 1U << hashmap->ex_capa;
    u32 const data_offset = hashmap->sz_bucket + hashmap->sz_key;

    for (u32 j=0; j<total_capacity; ++j) {
        struct Bucket *bucket = (struct Bucket *)
            ((char *)hashmap->slots + hashmap->sz_slot * j);

        if (BUCKET_IS_TAKEN(bucket->meta_data)) {
            char key_buffer[MAP_MAX_KEY_BYTES] = {0};
            memcpy(key_buffer, (char *)bucket + hashmap->sz_bucket, hashmap->sz_key - 1);

            if (!callback(key_buffer, (char *)bucket + data_offset)) {
                return false;
            }
        }
    }
    return true;
}

u32 get_occupied_slot_count(struct HashMap *hashmap) {
    u32 const total_capacity = 1U << hashmap->ex_capa;
    u32 occupied = 0;

    for (u32 j=0; j<total_capacity; ++j) {
        struct Bucket *bucket = (struct Bucket *)
            ((char *)hashmap->slots + hashmap->sz_slot * j);

        if (BUCKET_IS_TAKEN(bucket->meta_data)) {
            occupied += 1;
        }
    }
    return occupied;
}

void hmap_show_stats(struct HashMap *hashmap) {
    u32 const total_capacity = 1U << hashmap->ex_capa;

    fprintf(stdout, "Total capacity: %u\n", total_capacity);
    fprintf(stdout, "Occupied slots: %u\n", hashmap->occ_slots);
    fprintf(stdout, "Slot size in bytes: %u\n", hashmap->sz_slot);
    fprintf(stdout, "Load factor: %.2f\n\n", (f32)hashmap->occ_slots / total_capacity);
}

void traverse_hashmap_slots(struct HashMap *hashmap) {
    u32 const total_capacity = 1U << hashmap->ex_capa;

    for (u32 j=0; j<total_capacity; ++j) {
        struct Bucket *bucket = (struct Bucket *)
            ((char *)hashmap->slots + hashmap->sz_slot * j);

        fprintf(stdout, "Bucket address: %p\n", bucket);

        if (BUCKET_IS_TAKEN(bucket->meta_data)) {
            fprintf(stdout, "Bucket taken, psl == %u\n", META_GET_PSL(bucket->meta_data));

            char key_buffer[MAP_MAX_KEY_BYTES] = {0};
            memcpy(key_buffer, (char *)bucket + hashmap->sz_bucket, hashmap->sz_key - 1);

            fprintf(stdout, "Key: %s\n", key_buffer);
        } else {
            fprintf(stdout, "Bucket is free\n");
        }
    }
    
    fprintf(stdout, "\n\n");
}
