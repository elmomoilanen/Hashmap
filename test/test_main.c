#include "common.h"


static void run_siphash_tests() {
    test_func *test = &siphash_tests[0];

    for (; test->name; test++) {
        test->func();
    }
}

static void run_random_tests() {
    test_func *test = &random_tests[0];

    for (; test->name; test++) {
        test->func();
    }
}

static void run_map_tests() {
    test_func *test = &map_tests[0];

    for (; test->name; test++) {
        test->func();
    }
}

static void run_hashmap_tests() {
    test_func *test = &hashmap_tests[0];

    for (; test->name; test++) {
        test->func();
    }
}

static void run_hashset_tests() {
    test_func *test = &hashset_tests[0];

    for (; test->name; test++) {
        test->func();
    }
}


int main() {
    fprintf(stdout, "\nrunning tests...\n\n");

    fprintf(stdout, "running siphash tests...\n");
    run_siphash_tests();

    fprintf(stdout, "\nrunning random tests...\n");
    run_random_tests();

    fprintf(stdout, "\nrunning map tests...\n");
    run_map_tests();

    fprintf(stdout, "\nrunning hashmap tests...\n");
    run_hashmap_tests();

    fprintf(stdout, "\nrunning hashset tests...\n");
    run_hashset_tests();

    fprintf(stdout, "\n");
}
