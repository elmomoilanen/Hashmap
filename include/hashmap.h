#ifndef __HASHMAP__
#define __HASHMAP__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct HashMap;

/*
Initialise a new hash map struct with the default capacity of 16 storage slots.

HashMap struct uses internally the following memory layout:

(0) meta data (bucket) | key | user data ... (N) meta data (bucket) | key | user data,

where the number inside the brackets represents a slot number, which in the default
case of 16 storage slots goes from 0 to 15. Size of one (user) data item must be known
when initialising the hash map and kept same during usage of the hash map struct.

Meta data consumes 4 bytes each, a key 20 bytes and the user data item x bytes. Size of 
one user data item is restricted approx below 2^32 bytes. Also the slot count cannot 
exceed the upper bound of 2^20 slots.

Params:
    item_size: size of one (user) data item
    clean_func: a function pointer if custom cleaning functionality is needed.
        In most cases, set this to NULL.

Returns:
    struct HashMap*: a pointer to created hash map struct. If the initialisation 
        failed for some reason (not enough memory available, or too large item size),
        NULL is returned.
*/
struct HashMap* hashmap_init(size_t item_size, void (*clean_func)(void *));

/*
Initialise a new hash map struct to a specific size.

This is convenient (more so than the `hashmap_init`) if the user wants straight from
the start a specific storage count for the hash map struct, e.g. the user would like to
insert 10 000 elements to the hash map, this initialising option can allocate the needed
storage size from the start.

Params:
    item_size: size of one (user) data item
    elems: initial storage count for the hash map
    clean_func: a function pointer if custom cleaning functionality is needed.
        In most cases, set this to NULL.

Returns:
    struct HashMap*: a pointer to created hash map struct. If the initialisation failed for 
        some reason (not enough memory available, or too large item size), NULL is returned.
*/
struct HashMap* hashmap_init_with_size(size_t item_size, size_t elems, void (*clean_func)(void *));

/*
Insert data item to the hash map.

Size of the key is restricted and the insertion will fail (return false) if the
used key is too large. It's recommended that the key consists only of characters
that consume one byte of memory (ascii characters).

For every insertion, the hash map makes itself a copy of the passed data item and key.

Notice that for a bit more complicated data types e.g.,

struct U {
    // primitive member declarations here...
}

struct T {
    U *ptr_to_u;
    // other primitive members...
}

struct T *t = alloc(...)
t->ptr_to_u = alloc(...)

inserting a data item of struct T to the hash map causes there to be two references
to the memory address where t->ptr_to_u is pointing to. This has implications to other
hash map operations.

Params:
    hashmap: pointer to the HashMap struct
    key: string type key for which the passed data will be mapped to
    data: a pointer to the data item

Returns:
    bool: true if the insertion succeeded, false otherwise. Latter case can occur
        only if probe sequence length reaches its upper bound but this is unlikely.
*/
bool hashmap_insert(struct HashMap *hashmap, char const *key, void const *data);

/*
Get data item from the hash map.

Returned data item will be a reference to data that was copied and stored to the hash map
in a preceding insertion operation. This reference has lifetime until next hash map insertion
or removal operation (where the hash map might resize) or when the whole hash map is deleted
from memory.

Also in a bit more complicated cases, lifetime ends when the original data item is freed. 
See the example above for `hashmap_insert` for more info about this case.

Params:
    hashmap: pointer to the HashMap struct
    key: string type key for which the data item has been mapped to

Returns:
    pointer to the data: if a data item with passed key is found, otherwise NULL.
*/
void* hashmap_get(struct HashMap *hashmap, char const *key);

/*
Remove data from the hash map.

Data mapped to by the provided key will be removed if found from the hash map.
If the data is found, also a reference to it is returned though the reference 
points to a temporary location (used internally by the hash map struct) which 
lifetime ends upon the next hash map operation call.

Params:
    hashmap: the HashMap struct
    key: string type key for which the data item has been mapped to

Returns:
    pointer to the removed data: if a data item with passed key is found,
        otherwise NULL is returned.
*/
void* hashmap_remove(struct HashMap *hashmap, char const *key);

/*
Clean up memory used by the hash map.

If no custom clean up function was passed during initialisation, cleaning
process will be kind of dummy, i.e., just freeing the slots, temporary storage
and the HashMap struct itself. Thus, in this case, if the data items contained
pointers to other memory addresses with allocated objects/data, the user remains
responsible to clean up those.

Otherwise, with custom clean up function given, this function is called for each
data item stored in the hash map. E.g., in the case illustrated above before
`hashmap_insert` operation, one could define following cleaning function

void clean(void *data_item) {
    if (((struct T *)data_item)->ptr_to_u) {
        free(((struct T *)data_item)->ptr_to_u)
    }
}

in order to free allocated memory starting at address data_item->ptr_to_u.

Params:
    hashmap: the HashMap struct
*/
void hashmap_free(struct HashMap *hashmap);

/*
Traverse slots of the hash map.

Will print the starting address of each slot and indicate whether each slot
contains a data item. Mostly useful for debugging.

Params:
    hashmap: the HashMap struct
*/
void hashmap_traverse(struct HashMap *hashmap);

/*
Print hash map internal statistics.

For this call current total capacity, occupied slot count, size of each slot 
and the load factor (occupied slots / total capacity) are printed to stdout.

Params:
    hashmap: the HashMap struct
*/
void hashmap_stats(struct HashMap *hashmap);


#endif /* __HASHMAP__ */
