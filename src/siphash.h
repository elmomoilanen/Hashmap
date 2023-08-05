#include <inttypes.h>

#define HASH_RAND_KEY_LEN 16
#define HASH_MAX_RAND_BUF_LEN 256

u64 siphash(void const *data, size_t data_len, u8 const key[HASH_RAND_KEY_LEN]);
