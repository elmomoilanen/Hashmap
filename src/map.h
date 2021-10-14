#ifndef __MAP__
#define __MAP__

#include <stddef.h>
#include <stdbool.h>

#include "common.h"

#define MAP_MAX_RAND_BUF_LEN 256
#define MAP_RAND_KEY_LEN 16

#define MAP_INIT_EXP_CAPACITY 4
#define MAP_MAX_EXP_CAPACITY 20

#define META_VALUE_SET(meta_data, value, offset, mask) \
    (((meta_data) & ~(mask)) | ((value) << (offset)))

#define META_VALUE_GET(meta_data, offset, mask) (((meta_data) & (mask)) >> (offset))

typedef void (*clean_func_type)(void *);

/*
struct HashMap

Memory layout: meta data (bucket) | key | user data ... | meta data | key | user data.
A slot consists of one meta data, key and user data. Hashmap will have N slots,
2**`MAP_INIT_EXP_CAPACITY` by default and 2**`MAP_MAX_EXP_CAPACITY` at maximum.
Meta data structs are fixed to 4 bytes and key (the key which the end user uses to 
map the data) to `MAP_MAX_KEY_BYTES`.

ex_capa: the power (exponent) of two which gives the total capacity.
occ_slots: occupied slots.
sz_bucket: size of the meta data struct in bytes.
sz_key: maximal size of the key in bytes (null terminator must be included for this size).
sz_item: data size, defined at initialization.
sz_slot: slot size in bytes (a slot is given by one meta data, key and user data items).
slots: starting address for the slots.
_temp: starting address for the garbage data used internally by the hashmap
    (a user won't need for anything and shouldn't access it).
clean_func: a function (pointer) doing necessary cleaning for user data. By default,
    this will be internally NULL and the hashmap will use basic `free` to do clean up.
    Pass this as a argument when initializing hashmap if needed.
*/
struct HashMap {
    u32 ex_capa;
    u32 occ_slots;
    u32 sz_bucket;
    u32 sz_key;
    u32 sz_item;
    u32 sz_slot;
    void *slots;
    void *_temp;
    void (*clean_func)(void *);
};


struct HashMap* hmap_init(size_t item_size, u32 init_capa, void (*clean_func)(void *));
void hmap_free(struct HashMap *hashmap);

void* hmap_get(struct HashMap *hashmap, char const *key);
bool hmap_insert(struct HashMap *hashmap, char const *key, void const *data);
void* hmap_remove(struct HashMap *hashmap, char const *key);

void traverse_hashmap_slots(struct HashMap *hashmap);


// Following are meant only for testing hashmap

bool get_random_key(u8 *buffer, size_t buffer_len);
struct HashMap* hmap_init_with_key(size_t item_size, void (*clean_func)(void *));
u32 get_occupied_slot_count(struct HashMap *hashmap);

#endif // __MAP__
