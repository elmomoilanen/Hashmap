# Hashmap #

[![main](https://github.com/elmomoilanen/Hashmap/actions/workflows/main.yml/badge.svg)](https://github.com/elmomoilanen/Hashmap/actions/workflows/main.yml)

Library implementing the hash map data structure with open addressing, robin hood hashing more presicely, as a collision resolution strategy. Library uses strings as keys that are internally mapped to the data via SipHash-2-4 hashing function, see e.g. the reference C implementation [SipHash](https://github.com/veorq/SipHash) for more information of this hashing. Size of the keys is restricted to 19 bytes, the 20th byte being reserved internally for the null character. Reason for this choice is that the library is mainly designed for small scale needs and long keys would seem redundant with respect to this purpose. Underlying memory layout for the hashmap can also be tighter when allowing key sizes only up to a specific boundary. This memory layout is formed by slots that each have four first bytes reserved for meta data, following 20 bytes reserved for the key and the next x bytes for one data item which size must be known when initialising the hashmap in the first place. Slot count (i.e., capacity of the hash map) can be set at the beginning or left to be configured internally by the library. Notice that there are some other size restrictions but these are checked when needed and they shouldn't restrict too much if any the user experience, see the *usage* section below for more information. Notice also that this library is not thread-safe and thus shouldn't be used with multi-threaded code.

Precise memory layout for one slot: meta data (4 bytes; 1 bit to mark whether the slot is reserved, 11 bits for probe sequence length and 20 bits for truncated hash value) | key (20 bytes) | data (x byte). Meta data is implemented as a normal unsigned int data type and specific bits inside it are modified by bitwise operations.

## Build ##

Expected to work in most common Linux distros (e.g. Ubuntu) and macOS, notice however that for macOS one might need to change the compiler to clang (CC=clang). Library uses C11 standard.

Following shell command builds the library, runs unit tests and lastly cleans up unneeded object files
```bash
make && make test && make clean
```
On a successful build, the static library file `libhashmap.a` is formed in this level of the folder.

## Usage ##

Header file `include/hashmap.h` defines public APIs for the library.

Indicate to the compiler the include path (-I) for the header file `include/hashmap.h` and library path (-L) and name (-l) for the static library file `libhashmap.a`. E.g. the following shell command

```bash
gcc test_prog.c -I./include -L. -lhashmap -o test_prog -Wall -Wextra -Werror -std=c11 -g
```

would compile a `test_prog.c` source code file that uses the hashmap library. Contents of this source code file could be e.g. the following

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

    // insert temperature data (pointer to it) to hashmap, using dates as keys
    hashmap_insert(hashmap, "1.8.2021", &temp_18);
    hashmap_insert(hashmap, "2.8.2021", &temp_28);

    // show some internal hashmap statistics, members of the hashmap struct aren't directly accessible
    // e.g. the load factor would be 2/16 at this point
    hashmap_stats(hashmap);
    hashmap_traverse(hashmap);

    // ...

    // get some data and check that it has remained correct
    Temperature *t_18 = hashmap_get(hashmap, "1.8.2021");
    assert(t_18->kelvin - temp_18.kelvin < 0.01);

    // remove the same data from the hashmap (ignore return value for now)
    hashmap_remove(hashmap, "1.8.2021");

    // data not there anymore
    assert(hashmap_get(hashmap, "1.8.2021") == NULL);

    // clean up memory used by the hashmap struct
    hashmap_free(hashmap);
}

```

Here is a short summary for some important details related to this hash map implementation:

- new hashmap can be initialised to default size by `hashmap_init` and by `hashmap_init_with_size` to meet some initial size requirement
- size of one data item is passed as an argument when initialising the hashmap and this must not exceed approx 2^32 bytes
- custom clean up function can be passed when initialising if specific memory clean up is needed when calling later `hashmap_free`
- hashmap struct has an upper bound for its capacity but this bound is over one million slots
- capacity increases exponentially (powers of two) if the load factor increases over 90 %
- if the load factor falls below 40 %, the capacity is shrunk
