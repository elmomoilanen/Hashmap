#include "common.h"
#include "map.h"


static void test_getting_random_bytes() {
    assert(MAP_RAND_KEY_LEN > 0);
    assert(MAP_RAND_KEY_LEN < MAP_MAX_RAND_BUF_LEN);

    u8 randkey[MAP_RAND_KEY_LEN] = {0};
    size_t const key_len = sizeof(randkey) / sizeof(randkey[0]);

    bool good_response = get_random_key(randkey, key_len);
    assert(good_response == true);

    PRINT_SUCCESS(__func__);
}

static void test_getting_random_bytes_zero_length_buffer() {
    u8 randkey[0];
    size_t const key_len = 0;

    bool good_response = get_random_key(randkey, key_len);
    assert(good_response == false);

    PRINT_SUCCESS(__func__);
}

static void test_getting_random_bytes_max_allowed_buffer() {
    u8 randkey[MAP_MAX_RAND_BUF_LEN] = {0};
    size_t const key_len = sizeof(randkey) / sizeof(randkey[0]);

    bool good_response = get_random_key(randkey, key_len);
    assert(good_response == true);

    PRINT_SUCCESS(__func__);
}

static void test_getting_random_bytes_oversized_buffer() {
    #define BUFFER_SIZE ((MAP_MAX_RAND_BUF_LEN) + 1)

    u8 randkey[BUFFER_SIZE] = {0};
    size_t const key_len = sizeof(randkey) / sizeof(randkey[0]);

    bool good_response = get_random_key(randkey, key_len);
    assert(good_response == false);

    PRINT_SUCCESS(__func__);
}


test_func random_tests[] = {
    {"getting_random_bytes", test_getting_random_bytes},
    {"getting_random_bytes_zero_length_buffer", test_getting_random_bytes_zero_length_buffer},
    {"getting_random_bytes_max_allowed_buffer", test_getting_random_bytes_max_allowed_buffer},
    {"getting_random_bytes_oversized_buffer", test_getting_random_bytes_oversized_buffer},
    {NULL, NULL},
};
