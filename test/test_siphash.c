#include "common.h"
#include "siphash.h"

#define TEST_SET_SIZE 32


u8 const key[] = {
    0x0, 0x1, 0x2, 0x3,
    0x4, 0x5, 0x6, 0x7,
    0x8, 0x9, 0xa, 0xb,
    0xc, 0xd, 0xe, 0xf,
};

u8 const test_set[][2] = {
    "\x20", "\x21", "\x22", "\x23",
    "\x24", "\x25", "\x26", "\x27",
    "\x28", "\x29", "\x2a", "\x2b",
    "\x2c", "\x2d", "\x2e", "\x2f",
    "\x30", "\x31", "\x32", "\x33",
    "\x34", "\x35", "\x36", "\x37",
    "\x38", "\x39", "\x3a", "\x3b",
    "\x3c", "\x3d", "\x3e", "\x3f",
};

u64 const correct_hashes_test_set[] = {
    0x21bd0cab435c8c79ULL, 0x6e50ede395d65a46ULL, 0x572bf3ca9a47158ULL, 0xd2bcd5254fc978adULL,
    0x8a1d0b1ea809514cULL, 0x730387bdc4f327e3ULL, 0x2e31e526b451c719ULL, 0xe535adaadf4158b1ULL,
    0xadef2948d21bc86cULL, 0x408dae3f830a2888ULL, 0x18a2866298a494e2ULL, 0xeeb1303e324cc958ULL,
    0xa6ceccf2e0a4f94eULL, 0x9d2b038de36ea196ULL, 0xbd321b758a057a29ULL, 0x9872eb0c8b9a0a30ULL,
    0x4ec57f76eb9f068fULL, 0x3943c8fcfccf7ce0ULL, 0xe542b1b716b820dcULL, 0x67d6d8c8413eba27ULL,
    0x3b95f58bdab79630ULL, 0xdbd4a63992cdc07aULL, 0xd5472de600064a92ULL, 0x18733d77b7f7e614ULL,
    0x2d86b5bb6a3c0cf2ULL, 0xa57aebd4075acff5ULL, 0x2da625fdf6d7c4caULL, 0x4148ae80da82bd0eULL,
    0xb0f4d346d72da699ULL, 0xebb5b33bbdbad7a0ULL, 0x73be792ca75eae4dULL, 0x714ddbefc9d4b97cULL,
};


static void test_siphash_ascii_chars() {
    u64 corr_hashes = sizeof(correct_hashes_test_set) / sizeof(correct_hashes_test_set[0]);
    assert(corr_hashes == TEST_SET_SIZE);

    assert(sizeof(key)/sizeof(key[0]) == 16);

    for (u8 i=0; i<TEST_SET_SIZE; ++i) {
        u64 hash = siphash(test_set[i], 1, key);
        assert(hash == correct_hashes_test_set[i]);
    }

    PRINT_SUCCESS(__func__);
}

static void test_siphash_string() {
    char string[] = "Hello, this is a siphash test!";

    size_t s_len = strlen(string);

    u64 hash = siphash(string, s_len, key);
    assert(hash == 0xb4721902258a7432ULL);

    PRINT_SUCCESS(__func__);
}

static void test_siphash_long_string() {
    char string[] = "Hello, this is a very very very very very very long data for testing siphash!";

    size_t s_len = strlen(string);

    u64 hash = siphash(string, s_len, key);
    assert(hash == 0xcbf88d2deb16d829ULL);

    PRINT_SUCCESS(__func__);
}

static void test_siphash_equal_data() {
    u8 data[] = "\x68\x65\x6c\x6c\x6f";

    u64 hash = siphash(data, 5, key);
    siphash("temp", strlen("temp"), key);
    u64 hash_second = siphash(data, 5, key);

    assert(hash == hash_second);

    PRINT_SUCCESS(__func__);
}


test_func siphash_tests[] = {
    {"siphash_ascii_chars", test_siphash_ascii_chars},
    {"siphash_string", test_siphash_string},
    {"siphash_long_string", test_siphash_long_string},
    {"siphash_equal_data", test_siphash_equal_data},
    {NULL, NULL},
};
