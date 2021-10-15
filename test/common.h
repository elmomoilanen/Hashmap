#ifndef __COMMON__
#define __COMMON__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define PRINT_SUCCESS(func) printf("%s() %spassed%s\n", (func),\
    ANSI_COLOR_GREEN, ANSI_COLOR_RESET)

typedef struct {
    char *name;
    void (*func)();
} test_func;

extern test_func siphash_tests[];
extern test_func random_tests[];
extern test_func map_tests[];

extern test_func hashmap_tests[];
extern test_func hashset_tests[];

#endif // __COMMON__
