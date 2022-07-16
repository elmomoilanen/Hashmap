# Hashmap #

[![main](https://github.com/elmomoilanen/Hashmap/actions/workflows/main.yml/badge.svg)](https://github.com/elmomoilanen/Hashmap/actions/workflows/main.yml)

Library implementing the hash map data structure with open addressing, robin hood hashing more presicely, as a collision resolution strategy. Library uses strings as keys that are internally mapped to values via SipHash-2-4 hashing function, see e.g. the reference C implementation [SipHash](https://github.com/veorq/SipHash) for more information on this hashing algorithm.

Library design restricts size of the keys to 19 bytes, the 20th byte being reserved internally for the null character. One of the reasons for this implementation choice was that the library is targeted mainly for small scale needs and longer keys would seem redundant with respect to this purpose. Underlying memory layout for the hash map can also be implemented more tightly when allowing key sizes only up to a specific boundary.

Memory layout is formed by so called slots that each have four first bytes reserved for meta data, following 20 bytes reserved for the key (as described earlier) and the next x bytes for an actual data item which size must be known when initialising the hash map in the first place. Slot count (i.e., total capacity of the hash map) can be set at the beginning or left to be configured internally by the library. Notice that there are some other size restrictions in place but these are checked by the library when needed and they shouldn't restrict too much, if any, the user experience; please see the *API summary* section below for more information on these limitations. Notice also that this library is not thread-safe and shouldn't be used with multi-threaded code.

Precise memory layout for a slot is the following: meta data (4 bytes in total; 1 bit to mark whether the slot is reserved, 11 bits for probe sequence length (PSL) and 20 bits for truncated hash value) | key (20 bytes, last byte always reserved for the null character) | data item (x bytes, determined at initialisation). Meta data is implemented as a normal unsigned integer data type and the specific bits inside it are modified by bitwise operations. As the maximal capacity of the hash map is limited, 11 bits for PSL and 20 bits for hash are sufficient.

## Build ##

Expected to work in most common Linux distros (e.g. Ubuntu) and macOS, notice however that for macOS one might need to change the compiler to clang (CC=clang). Library uses C11 standard.

Following shell command builds the library, runs unit tests and lastly cleans up unneeded object files
```bash
make && make test && make clean
```
On a successful build, the static library file *libhashmap.a* is formed in this level of the folder.

## Usage ##

Header file *include/hashmap.h* defines public APIs for the library.

Indicate to compiler the include path (-I) for the header file *hashmap.h* and library path (-L) and name (-l) for the static library file *libhashmap.a*. E.g. the following shell command

```bash
gcc test_prog.c -I./include -L. -lhashmap -o test_prog -Wall -Wextra -Werror -std=c11 -g
```

would compile a *test_prog.c* source code file that uses this library. Contents of the source code file could be e.g. the following

```C
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "hashmap.h"

typedef unsigned int u32;
typedef float f32;

typedef struct {
    f32 kelvin;
    u32 hour;
    u32 mins;
} Temperature;

int main() {
    // Use default init size, 16 open slots and no specific clean up function
    struct HashMap *hashmap = hashmap_init(sizeof(Temperature), NULL);

    Temperature temp_18 = {.kelvin=293.15, .hour=12, .mins=0};
    Temperature temp_28 = {.kelvin=298.15, .hour=12, .mins=0};

    // Insert temperature data to the hash map, using dates as keys
    // For every insertion hash map makes itself a (shallow) copy of the data
    hashmap_insert(hashmap, "1.8.2021", &temp_18);
    hashmap_insert(hashmap, "2.8.2021", &temp_28);

    // Print some internal statistics to stdout, e.g. the load factor is 2/16 now
    hashmap_stats(hashmap);
    hashmap_traverse(hashmap);

    // Get data item (reference) back and check that it has remained correct
    // t_18 is safe to use until next hash map insertion or removal call
    Temperature *t_18 = hashmap_get(hashmap, "1.8.2021");
    assert(t_18->kelvin - temp_18.kelvin < 0.01);

    // Remove the same data item (ignoring the return value for now)
    hashmap_remove(hashmap, "1.8.2021");
    
    assert(hashmap_get(hashmap, "1.8.2021") == NULL);
    // Notice that previous t_18 is currently a dangling pointer

    // Finally, clean up memory used by the hash map struct
    hashmap_free(hashmap);
}
```

In this case, output of the function call *hashmap_stats* will be the following

```
Total capacity: 16
Occupied slots: 2
Slot size in bytes: 40
Load factor: 0.12
```

and for *hashmap_traverse* resulted output could start e.g. as follows (showing only the first five meta data buckets of the total 16)

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

- Initialise a new hash map by calling `hashmap_init` or `hashmap_init_with_size`

    New hash map can be initialised to default size by *hashmap_init* or to meet some initial size requirement by *hashmap_init_with_size*. Size of one data item is passed as an argument when initialising the hash map and this must not exceed approx 2^32 bytes. Custom clean up function can be passed if specific memory clean up is needed when calling *hashmap_free* later.

    Returned hash map struct has an upper bound for its total capacity but this bound is over one million slots. Capacity will grow exponentially (as powers of two) if load factor increases over 90 %. Conversely, if the load factor falls below 40 %, the capacity of the hash map will shrink (but can occur only when removing data items from the hash map).

- Insert data item to hash map by `hashmap_insert`

    For every insertion, the hash map makes itself a copy of the passed data item and key. This insert operation returns a boolean value to indicate whether the insertion was successful. Failure occurs if and only if maximal probe sequence length is reached (metadata reserves 11 bits for PSL values).

    For a bit more complicated data types (e.g., contain pointers to manually allocated memory) hash map insertion calls increase the reference count to these memory locations. In this respect, the copying mechanism is shallow.

- Show internal hash map struct statistics by `hashmap_stats` and `hashmap_traverse`

    For the former call, current total capacity, occupied slot count, size of each slot and the load factor (occupied slots / total capacity) are printed to stdout. For the latter call, the whole hash map will be traversed and meta data information for each slot gets printed to stdout.

- Get data item from hash map or check only its existence by `hashmap_get`

    This is a reference to data item (NULL if not found) that was copied and stored during a preceding insertion operation. The reference has kind of limited lifetime and is thus safe to use only prior to next hash map insertion or removal call as the hash map might resize during these operations.

- Remove data item from hash map by `hashmap_remove`

    Data mapped to by the provided key will be removed if found from the hash map. In this case a reference to the data item is returned as a response though the reference points to a temporary location (used internally by the hash map struct) which lifetime ends upon the next hash map operation call.
    
- Clean up used memory of the hash map by `hashmap_free`

    Normally this frees the slots, temporary storage and the HashMap struct itself. If a custom cleaning function was given during initialisation, it is called to for each data item stored in the hash map. Please see an example of a custom cleaning function in the *hashmap.h*.

See the *hashmap.h* header file for more information and examples.
