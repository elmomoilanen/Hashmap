#ifndef __MAP__
#define __MAP__

#include "common.h"
#include "siphash.h"

#define MAP_INIT_EXP_CAPACITY 4
#define MAP_MAX_EXP_CAPACITY 20

#define META_VALUE_SET(meta_data, value, offset, mask) \
    (((meta_data) & ~(mask)) | ((value) << (offset)))

#define META_VALUE_GET(meta_data, offset, mask) (((meta_data) & (mask)) >> (offset))

typedef void (*clean_func_type)(void *);

/*
Memory layout: meta data (bucket) | key | user data ... | meta data | key | user data.

A slot consists of one meta data unit, key and user data item. Hash map will have 
N slots, 2**`MAP_INIT_EXP_CAPACITY` by default and 2**`MAP_MAX_EXP_CAPACITY` at max.
Meta data struct size is fixed to 4 bytes and key (which the end user uses to map to 
the data) to `MAP_MAX_KEY_BYTES` bytes.

Members of HashMap struct:

ex_capa: exponent e for the power of two (2^e) which gives the total capacity.
occ_slots: count of occupied slots.
sz_bucket: size of the meta data struct in bytes.
sz_key: maximal size of the key in bytes (null terminator must be included for this size).
sz_item: data size, defined at initialization.
sz_slot: slot size in bytes (a slot is given by one meta data unit, key and user data item).
rand_key: random key used for the hash function.
slots: starting address for the slots.
_temp: starting address for the garbage data used internally by the hash map.
clean_func: a function pointer doing necessary cleaning for user data. By default,
    this will be internally NULL and the hashmap will use basic `free` to do the cleaning.
*/
struct HashMap {
    u32 ex_capa;
    u32 occ_slots;
    u32 sz_bucket;
    u32 sz_key;
    u32 sz_item;
    u32 sz_slot;
    u8 rand_key[HASH_RAND_KEY_LEN];
    void *slots;
    void *_temp;
    void (*clean_func)(void *);
};

struct HashMap* hmap_init(size_t item_size, u32 init_capa, void (*clean_func)(void *));
void hmap_free(struct HashMap *hashmap);

void* hmap_get(struct HashMap *hashmap, char const *key);
bool hmap_insert(struct HashMap *hashmap, char const *key, void const *data);
void* hmap_remove(struct HashMap *hashmap, char const *key);
bool hmap_iter_apply(struct HashMap *hashmap, bool (*callback)(char const *, void *));
u32 hmap_len(struct HashMap *hashmap);

void traverse_hashmap_slots(struct HashMap *hashmap);
void hmap_show_stats(struct HashMap *hashmap);

// Following are meant only for testing the hash map
bool get_random_key(u8 *buffer, size_t buffer_len);
struct HashMap* hmap_init_with_key(size_t item_size, void (*clean_func)(void *));
u32 get_occupied_slot_count(struct HashMap *hashmap);

#endif // __MAP__
