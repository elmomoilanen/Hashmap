#ifndef __HASHMAP__
#define __HASHMAP__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct HashMap;

/*
Initialise a new hash map struct with the default capacity of 16 storage slots.

HashMap struct uses internally the following memory layout:

(0) meta data (bucket) | key | data ... (N) meta data (bucket) | key | data,

where the number inside the brackets represents a slot number, which in the default
case of 16 storage slots goes from 0 to 15. Size of one data item must be known
when initialising the hash map and kept same during usage of the hash map struct.

Meta data consumes 4 bytes each, a key 20 bytes and the user data item x bytes. Size of
one data item is restricted approx below 2^32 bytes. Also the slot count cannot
exceed the upper bound of 2^20 slots.

Params:
    item_size: size of one data item
    clean_func: a function pointer if custom cleaning functionality is needed.
        If such is not needed, set this to NULL.

Returns:
    struct HashMap*: a pointer to created hash map struct. If the initialisation 
        failed for some reason (not enough memory available, or too large item size),
        NULL is returned.
*/
struct HashMap* hashmap_init(size_t item_size, void (*clean_func)(void *));

/*
Initialise a new hash map struct to a specific size.

This is convenient (more so than the `hashmap_init`) if the user wants straight from
the start a specific storage count for the hash map struct. E.g., to insert 10 000 elements
to the hash map, this initialising option can allocate the needed storage size from the start.

Params:
    item_size: size of one data item
    elems: initial storage count for the hash map
    clean_func: a function pointer if custom cleaning functionality is needed.
        If such is not needed, set this to NULL.

Returns:
    struct HashMap*: a pointer to created hash map struct. If the initialisation failed for 
        some reason (not enough memory available, or too large item size), NULL is returned.
*/
struct HashMap* hashmap_init_with_size(size_t item_size, size_t elems, void (*clean_func)(void *));

/*
Insert data item to the hash map.

Size of the key is limited to 19 bytes and the insertion will fail (return false) if the
used key is too large. It's recommended that the key consists only of characters
that consume one byte of memory (ascii characters).

For every insertion, the hash map makes itself a shallow copy of the passed data item and key.

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
to the memory address where t->ptr_to_u is pointing to. This has implications for other
hash map operations.

In these more complicated cases (e.g. a struct containing a pointer to another memory address) 
it must be ensured that the memory at those addresses gets properly freed either manually or 
by providing a custom clean up function for the hashmap_init in the first place.

Params:
    hashmap: HashMap struct
    key: for which the passed data will be mapped to
    data: data item

Returns:
    bool: true if the insertion succeeded, false otherwise. Latter case can occur
        if and only if probe sequence length reaches its upper bound.
*/
bool hashmap_insert(struct HashMap *hashmap, char const *key, void const *data);

/*
Get data item from the hash map.

Returned value will be a reference to data that was stored to the hash map as a shallow copy
of the original data item during a preceding insertion operation call. This reference has
lifetime until the next hash map insertion or removal operation (when the hash map might resize)
or when the whole hash map is deleted from memory.

Also in a bit more complicated cases, lifetime ends when the original data item is freed. 
See the example given for `hashmap_insert` for more info about this case.

Params:
    hashmap: HashMap struct
    key: for which the data item has been mapped to

Returns:
    pointer to the data item: if a data item with the passed key is found, otherwise NULL.
*/
void* hashmap_get(struct HashMap *hashmap, char const *key);

/*
Remove data item from the hash map.

Data mapped to by the provided key will be removed if found from the hash map.
In this case a reference to the data item is returned as a response though the reference
points to a temporary location (used internally by the hash map struct) which 
lifetime ends upon the next hash map operation call.

Depending on the current hash map capacity, remove call may trigger resizing of the hash map.

Params:
    hashmap: HashMap struct
    key: for which the data item has been mapped to

Returns:
    pointer to the removed data item: this if such a data item is found, otherwise NULL.
*/
void* hashmap_remove(struct HashMap *hashmap, char const *key);

/*
Free the memory allocated for the hash map.

After calling this function, subsequent attempts to access the hash map struct
will result in undefined behaviour.

If no custom clean up function were passed during initialisation, cleaning
process will be kind of dummy, i.e., just freeing the slots, temporary storage
and the HashMap struct itself. Thus, in this case, if the data items contained
pointers to other memory addresses with allocated objects/data, the user remains
responsible for cleaning up those.

Otherwise, with custom clean up function given, this function is called for each
data item stored in the hash map. E.g., in the example given for `hashmap_insert`
operation, one could define the following cleaning function

void clean(void *data_item) {
    if (((struct T *)data_item)->ptr_to_u) {
        free(((struct T *)data_item)->ptr_to_u);
    }
}

in order to free allocated memory starting at address data_item->ptr_to_u.

Params:
    hashmap: HashMap struct
*/
void hashmap_free(struct HashMap *hashmap);

/*
Iterate the hash map and apply a callback to the keys and data items.

Iteration continues as long as the callback keeps returning true. That is,
the iteration stops for the first false return value from the callback or
when the hash map has been iterated through.

Params:
    hashmap: HashMap struct
    callback: a function pointer that is to be applied for data items. This function
        must take two arguments for the key and data item and return a boolean value.

Returns:
    bool: true if the hash map was completely iterated through, false otherwise.
*/
bool hashmap_iter_apply(struct HashMap *hashmap, bool (*callback)(char const *, void *));

/*
Get the current length of the hash map.

This is the count of occupied slots in the hash map.

Params:
    hashmap: HashMap struct

Returns:
    uint32_t: count of occupied slots in the hash map
*/
uint32_t hashmap_len(struct HashMap *hashmap);

/*
Traverse slots of the hash map.

Prints the starting address of each slot (in virtual memory) and whether the slot
is occupied or not.

Params:
    hashmap: HashMap struct
*/
void hashmap_stats_traverse(struct HashMap *hashmap);

/*
Prints a summary of the hash map internal statistics.

Current total capacity, occupied slot count, size of each slot and 
the load factor (occupied slots / total capacity) are printed to stdout.

Params:
    hashmap: HashMap struct
*/
void hashmap_stats_summary(struct HashMap *hashmap);


#endif /* __HASHMAP__ */
