# Hashmap #

Library implementing the hash map data structure with open addressing, robin hood hashing more presicely, as a collision resolution strategy. Uses C-style strings as keys that are internally mapped to the data via SipHash-2-4 hashing function, see e.g. the reference C implementation [SipHash](https://github.com/veorq/SipHash) for more information. Size of the keys is restricted to 19 bytes, the 20th byte being reserved internally for the null character. Reason for this choice is that the library is mainly designed for small scale needs and long keys would seem redundant with respect to this purpose. Underlying memory layout for the hashmap can also be tighter when allowing keys sizes only up to a specific boundary. This memory layout is formed by slots that each have four first bytes for meta data, following 20 bytes for the key and the next x bytes for the data. Slot count (i.e., capacity of the hash map) can be set at the beginning or left to be configured internally by the library. Notice that this library is not thread-safe and thus should not be used with multi-threaded code.

Precise memory layout for one slot: meta data (4 bytes; 1 bit to mark whether slot is reserved, 11 bits for probe sequence length and 20 bits for truncated hash value) | key (20 bytes) | data (x byte). Meta data is implemented as a normal unsigned int data type and specific bits inside it are modified by bitwise operations.

## Build ##

Expected to work in most common Linux distros (e.g. Ubuntu) and macOS, notice however that for macOS one might need to change the compiler to clang (CC=clang). Library uses C11 standard.

Following shell command builds the library, runs unit tests and lastly cleans up unneeded object files
```bash
make && make test && make clean
```
On a successful build, the static library file `libhashmap.a` is formed in this level of the folder.

## Usage ##

