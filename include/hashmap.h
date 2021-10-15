#ifndef __HASHMAP__
#define __HASHMAP__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct HashMap;

/*
Initialise a new hashmap struct with default capacity of 16 storage slots.

HashMap struct uses internally the following memory layout:
(0) meta data (bucket) | key | user data ... (N) meta data (bucket) | key | user data,
where the number inside the brackets represents a slot number, which in the default
case of 16 storage slots goes from 0 to 15. Size of one (user) data item must be known
when initialising the hashmap and kept the same during usage of the same hashmap struct.

Meta data consumes 4 bytes each, a key 20 bytes and the user data x bytes. Size of one user
data item is restricted approx below 2^32 bytes. Also the slot count cannot exceed the
upper bound of 2^20 slots.

Params:
    item_size, size of one (user) data item
    clean_func, a function pointer if a custom clean up functionality is needed.
        In most cases, set this to NULL.

Returns:
    struct HashMap, a hashmap struct which must be passed to each hashmap operations,
        e.g. insert or get. If the initialisation failed for some reason (not enough memory
        available, or too large item size), a NULL is returned.
*/
struct HashMap* hashmap_init(size_t item_size, void (*clean_func)(void *));

/*
Initialise a new hashmap struct to a specific size.

This is convenient (more so than the `hashmap_init`) if the user wants straight from
the start a specific storage count for the hashmap, e.g. the user would like to
insert 10 000 elements to the hashmap, this initialising option can allocate the needed
storage size from the start.

Params:
    item_size, size of one (user) data item
    elems, initial storage count for the hashmap
    clean_func, a function pointer if a custom clean up functionality is needed.
        In most cases, set this to NULL.

Returns:
    struct HashMap, a hashmap struct which must be passed to each hashmap operations,
        e.g. insert or get. If the initialisation failed for some reason (not enough memory
        available, or too large item size), a NULL is returned.
*/
struct HashMap* hashmap_init_with_size(size_t item_size, size_t elems, void (*clean_func)(void *));


/*
Insert data item to the hashmap.

Size of the key is restricted and the insertion will fail (return false) if the
used key is too large. It's recommended that the key consists only of characters
that consume one byte of memory (ascii characters).

Params:
    hashmap, the HashMap struct
    key, string type key for which the passed data will be mapped
    data, a pointer to the data item

Returns:
    bool, true if the insertion succeeded and false otherwise
*/
bool hashmap_insert(struct HashMap *hashmap, char const *key, void const *data);

/*
Get data from the hashmap.

Params:
    hashmap, the HashMap struct
    key, string type key

Returns:
    pointer to the data, if a data item with passed key is found and otherwise
        a NULL pointer is returned.
*/
void* hashmap_get(struct HashMap *hashmap, char const *key);

/*
Remove data from the hashmap.

Params:
    hashmap, the HashMap struct
    key, string type key

Returns:
    pointer to the removed data, if a data item with passed key is found and otherwise
        a NULL pointer is returned. After a successful remove call, the hashmap
        doesn't store the returned data anymore.
*/
void* hashmap_remove(struct HashMap *hashmap, char const *key);

/*
Clean up memory used by the hashmap.

Params:
    hashmap, the HashMap struct
*/
void hashmap_free(struct HashMap *hashmap);

/*
Traverse slots of the hashmap.

Will print the starting address of each slot and indicate whether each slot
contains a data item. Mostly useful for debugging.

Params:
    hashmap, the HashMap struct
*/
void hashmap_traverse(struct HashMap *hashmap);

/*
Print hashmap internal statistics, e.g. the load factor.

Params:
    hashmap, the HashMap struct
*/
void hashmap_stats(struct HashMap *hashmap);


#endif /* __HASHMAP__ */
