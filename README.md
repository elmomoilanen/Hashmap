# Hashmap #

[![main](https://github.com/elmomoilanen/Hashmap/actions/workflows/main.yml/badge.svg)](https://github.com/elmomoilanen/Hashmap/actions/workflows/main.yml)

Library implementing the hash map data structure with open addressing, robin hood hashing more presicely, as a collision resolution strategy. Library uses strings as keys that are internally mapped to the data via SipHash-2-4 hashing function, see e.g. the reference C implementation [SipHash](https://github.com/veorq/SipHash) for more information of this hashing algorithm.

Library design restricts size of the keys to 19 bytes, the 20th byte being reserved internally for the null character. One of the reasons for this implementation choice was that the library is targeted mainly for small scale needs and long keys would seem redundant with respect to this purpose. Underlying memory layout for the hash map can also be implemented more tightly when allowing key sizes only up to a specific boundary. This memory layout is formed by so called slots that each have four first bytes reserved for meta data, following 20 bytes reserved for the key (as described earlier) and the next x bytes for one actual data item which size must be known when initialising the hash map in the first place. Slot count (i.e., total capacity of the hash map) can be set at the beginning or left to be configured internally by the library. Notice that there are some other size restrictions in place but these are checked by the library when needed and they shouldn't restrict too much, if any, the user experience; please see the *usage* section below for more information of these limitations. Notice also that this library is not thread-safe and thus shouldn't be used with multi-threaded code.

Precise memory layout for one slot is following: meta data (4 bytes in total; 1 bit to mark whether the slot is reserved, 11 bits for probe sequence length (PSL) and 20 bits for truncated hash value) | key (20 bytes) | data (x bytes). Meta data is implemented as a normal unsigned integer data type and the specific bits inside it are modified by bitwise operations. As the maximal capacity of the hash map is limited, 11 bits for PSL and 20 bits for hash are sufficient.

## Build ##

Expected to work in most common Linux distros (e.g. Ubuntu) and macOS, notice however that for macOS one might need to change the compiler to clang (CC=clang). Library uses C11 standard.

Following shell command builds the library, runs unit tests and lastly cleans up unneeded object files
```bash
make && make test && make clean
```
On a successful build, the static library file *libhashmap.a* is formed in this level of the folder.

## Usage ##

Header file *include/hashmap.h* defines public APIs for the library.

Indicate to the compiler the include path (-I) for the header file *hashmap.h* and library path (-L) and name (-l) for the static library file *libhashmap.a*. E.g. the following shell command

```bash
gcc test_prog.c -I./include -L. -lhashmap -o test_prog -Wall -Wextra -Werror -std=c11 -g
```

would compile a *test_prog.c* source code file that uses the hashmap library. Contents of this source code file could be e.g. the following

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
    // using default init size, 16 open slots and no specific clean up function (hence pass NULL)
    struct HashMap *hashmap = hashmap_init(sizeof(Temperature), NULL);

    Temperature temp_18 = {.kelvin=293.15, .hour=12, .mins=0};
    Temperature temp_28 = {.kelvin=298.15, .hour=12, .mins=0};

    // insert temperature data (actually, pointer to it) to hashmap, using dates as keys
    // for every insertion, hashmap makes itself a copy of the data 
    hashmap_insert(hashmap, "1.8.2021", &temp_18);
    hashmap_insert(hashmap, "2.8.2021", &temp_28);

    // show some internal hashmap statistics, e.g. the load factor would be 2/16 at this point
    hashmap_stats(hashmap);
    hashmap_traverse(hashmap);

    // get data (reference) back from hashmap and check that it has remained correct
    Temperature *t_18 = hashmap_get(hashmap, "1.8.2021");
    assert(t_18->kelvin - temp_18.kelvin < 0.01);

    // remove the same data from the hashmap (ignore return value for now)
    hashmap_remove(hashmap, "1.8.2021");
    
    assert(hashmap_get(hashmap, "1.8.2021") == NULL);

    // finally, clean up memory used by the hashmap struct
    hashmap_free(hashmap);
}
```

Here is a short summary for some of the most important details related to this hash map implementation:

- initialise a new hash map by calling `hashmap_init` or `hashmap_init_with_size`

    New hash map can be initialised to default size by *hashmap_init* or to meet some initial size requirement by *hashmap_init_with_size*. Size of one data item is passed as an argument when initialising the hashmap and this must not exceed approx 2^32 bytes. Custom clean up function can be passed when initialising if specific memory clean up is needed when calling later *hashmap_free*. Returned hash map struct has an upper bound for its total capacity but this bound is over one million slots. Capacity will grow exponentially (powers of two) if the load factor increases over 90 %. If load factor of the hash map falls below 40 %, the capacity will shrink (but can occur only when removing items from the hash map).

- insert data items to hash map by `hashmap_insert`

    For every insertion, the hash map makes itself a copy of the passed data and key. A boolean value is returned as a response to indicate whether the insertion was successful.

- show internal hashmap struct statistics by `hashmap_stats` and `hashmap_traverse`

    For the former call, current total capacity, occupied slot count, size of each slot and the load factor (occupied slots / total capacity) are printed to stdout. For the latter call, the whole hash map will be traversed and meta data information for each slot gets printed to stdout.

- get data from hash map or check only its existence by `hashmap_get`

    Returned data is a reference to data (NULL if not found) that was copied and stored during a preceding insertion operation.

- remove data from hash map by `hashmap_remove`

    Data mapped to by the provided key will be removed if found from the hash map. If the data is found, also a reference to it is returned as a response though the reference points to a temporary location (used internally by the hash map struct) which lifetime ends upon the next hash map operation call. 
    
- clean up used memory of the hash map by `hashmap_free`

See the *hashmap.h* header file for more precise introductions of the public APIs.
