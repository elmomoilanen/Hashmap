# Hashmap #

[![main](https://github.com/elmomoilanen/Hashmap/actions/workflows/main.yml/badge.svg)](https://github.com/elmomoilanen/Hashmap/actions/workflows/main.yml)

This library implements a hash map data structure with open addressing and Robin Hood hashing as the collision resolution strategy. Strings are used as keys that are internally mapped to values through the SipHash-2-4 hashing function (see the reference C implementation [SipHash](https://github.com/veorq/SipHash) for more info). Key size is limited to 19 bytes, with the 20th byte reserved for the null character. This design choice enables a more compact memory layout for the hash map.

The memory layout of the hash map consists of slots, each with 4 bytes reserved for metadata, 20 bytes for a key (as mentioned above), and x bytes for a data item. Size of a data item must be specified when initializing the hash map. The number of slots, or the total capacity of the hash map, can be set by the user or left to be determined internally by the library. There are other size restrictions, like for example the maximal slot count, but they are handled by the library and should not significantly impact the user experience (see the API summary section below for more info).

The memory layout for a slot is as follows: metadata (4 bytes: 1 bit for reserved flag, 11 bits for probe sequence length (PSL), and 20 bits for truncated hash value) | key (20 bytes: last byte reserved for the null character) | data item (x bytes: determined at initialization). Given the restricted maximal capacity of the hash map, 11 bits for PSL and 20 bits for hash value are sufficient.

This library is not thread-safe by default and in case of multithreaded code external synchronization mechanisms should be considered.

## Build ##

This library uses the C11 standard and `calloc` as the memory allocator for allocating memory dynamically. It is expected to work on most common Linux distros and macOS.

To build the library, run 

```bash
make
```

If the build is successful, a static library file named `libhashmap.a` will be created in the current directory.

Tests can be run as follows

```bash
make test
```

Optionally to the previous make command, the following command installs the library and header file in the system directories specified by the PREFIX variable, which defaults to /usr/local in the Makefile

```bash
make install
```

To uninstall, run

```bash
make uninstall
```

## Usage ##

Header file **include/hashmap.h** defines public API for the library.

To compile a source code file that uses this library, specify the include path for the header file `hashmap.h` with the `-I` flag, and the library path and name for the static library file `libhashmap.a` with the `-L` and `-l` flags respectively. For example

```bash
gcc test_prog.c -I./include -L. -lhashmap -o test_prog -Wall -Wextra -Werror -std=c11 -g
```

would compile a `test_prog.c` source code file that uses this library.

Following code section gives a concrete example of how to use this library.

```C
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "hashmap.h"

typedef struct {
    float kelvin;
    uint32_t hour;
    uint32_t mins;
} Temperature;

int main() {
    // Use default initial capacity of 16 open slots and no specific clean up function
    struct HashMap *hashmap = hashmap_init(sizeof(Temperature), NULL);

    Temperature temp_18 = {.kelvin=293.15, .hour=12, .mins=0};
    Temperature temp_28 = {.kelvin=298.15, .hour=12, .mins=0};

    // Insert temperature data to the hash map, using dates as keys
    // For every insertion hash map makes itself a (shallow) copy of the data
    hashmap_insert(hashmap, "1.8.2021", &temp_18);
    hashmap_insert(hashmap, "2.8.2021", &temp_28);

    // Print some internal statistics to stdout, e.g. the load factor is now 2/16
    hashmap_stats_summary(hashmap);
    hashmap_stats_traverse(hashmap);

    // Get back a reference to data and update its kelvin value
    // t_18 is safe to use until the next hash map insertion or removal operation
    Temperature *t_18 = hashmap_get(hashmap, "1.8.2021");
    float updated_kelvin = 291.50;
    t_18->kelvin = updated_kelvin;

    // Remove the first day and get a temporary pointer to the data
    t_18 = hashmap_remove(hashmap, "1.8.2021");
    assert(t_18->kelvin - updated_kelvin < 0.01);

    // Insert new temperature data
    hashmap_insert(hashmap, "3.8.2021", &(Temperature){.kelvin=297.0, .hour=12, .mins=0});

    // Notice that after insert call t_18 is a dangling pointer
    // Verify removal of the first day
    assert(hashmap_get(hashmap, "1.8.2021") == NULL);
    assert(hashmap_len(hashmap) == 2);

    // Finally, clean up all memory allocated by the hash map
    hashmap_free(hashmap);
}
```

In this case, output of the function call `hashmap_stats_summary` is the following

```
Total capacity: 16
Occupied slots: 2
Slot size in bytes: 40
Load factor: 0.12
```

and for `hashmap_stats_traverse` resulted output could start e.g. as follows (showing only the first five meta data buckets of the total 16)

```
Bucket address: 0x130e043c0
Bucket is free
Bucket address: 0x130e043e8
Bucket is free
Bucket address: 0x130e04410
Bucket taken, psl == 0
Key: 1.8.2021
Bucket address: 0x130e04438
Bucket taken, psl == 1
Key: 2.8.2021
Bucket address: 0x130e04460
Bucket is free
...
```

Here it's seen that a collision occurred for the key "2.8.2021" and hence it was inserted to the next available slot.

## API summary ##

Here is a short summary for some of the most important details related to this implementation:

- Initialise a new hash map by `hashmap_init` or `hashmap_init_with_size`

    A new hash map can be initialised to a default size (slot count) by hashmap_init, or to meet an initial size requirement by hashmap_init_with_size. The size of one data item must be passed as an argument during initialisation and cannot exceed approximately 2^32 bytes. If specific memory cleanup is required, a custom cleanup function can be given as argument.

    Returned hash map struct has an upper bound for its total capacity but this bound is over one million (2^20) slots. Capacity will grow exponentially (as powers of two) if the load factor exceeds 90%. Conversely, if the load factor falls below 40%, the capacity of the hash map will shrink, but this can only occur when data items are removed from the hash map (i.e., shrinkage can only happen during removal operation).

- Insert a data item to the hash map by `hashmap_insert`

    For every insertion, the hash map makes itself a shallow copy of the passed data item and key. A successful insertion returns `true`, while a failed insertion returns `false` which occurs if the key size exceeds 19 bytes, the hash map fails to resize due to reaching its maximal capacity or when the maximal probe sequence length is reached as specified in the metadata (11 bits reserved for PSL value).

    For complex data types that contain pointers to memory locations, insertion calls increase the reference count to these memory locations.

- Get a data item from the hash map by `hashmap_get`

    This is a reference to the data item (or NULL, if not found) stored in the hash map as a shallow copy of the original data item. It has a limited lifetime and should only be used prior to the next insertion or removal operation, as the hash map may resize during these operations and the reference may become invalid.    

- Remove a data item from the hash map by `hashmap_remove`

    The data associated with the given key will be removed from the hash map if it is found. In this case, a reference to the data item is returned, but it refers to a temporary location that is used internally by the hash map structure. This reference is only valid until the next operation on the hash map is performed. If the key is not found, NULL is returned.
    
- Free the allocated memory by `hashmap_free`

    Normally this frees the slots, temporary storage and the HashMap struct itself. If a custom cleaning function was provided during initialisation of the hash map, it will be called for each data item stored in the hash map. An example of a custom cleaning function can be found in `hashmap.h`.

- Iterate the hash map and apply a callback to the keys and data items by `hashmap_iter_apply`

    Iteration through the hash map continues as long as the callback keeps returning true. Callback must take two arguments: first for the key and second for the data item.

- Get the current length of the hash map by `hashmap_len`

    This is the count of occupied slots in the hash map.

- Show internal hash map struct statistics by `hashmap_stats_summary` and `hashmap_stats_traverse`

    For the former function, current total capacity, occupied slot count, the size of each slot and the load factor (occupied slots / total capacity) are printed to stdout. For the latter, the whole hash map will be traversed and metadata information for each slot is printed to stdout. Obviously, traversing is slow for large hash maps.

For additional information and examples, refer to the `hashmap.h` header file.
