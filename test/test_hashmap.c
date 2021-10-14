#include "common.h"
#include "map.h"
#include "hashmap.h"


struct Measurement {
    char name[32];
    i32 val_x;
    i32 val_y;
    i32 val_z;
    bool normal;
};


static void test_complete_hashmap() {
    size_t const type_size = sizeof(struct Measurement);    

    struct HashMap *hashmap = hashmap_init(type_size, NULL);
    assert(hashmap != NULL);

    // insert 200 "measurements" to the hashmap
    u32 const elems = 200;

    for (u32 i=1; i<=elems; ++i) {
        char key[10];
        snprintf(key, sizeof key, "%s_%u", "key", i);

        struct Measurement measurement = {
            .name="condition",
            .val_x=i,
            .val_y=i*2,
            .val_z=0,
            .normal=true
        };
        assert(hashmap_insert(hashmap, key, &measurement) == true);
    }

    // remove few measurements
    u32 key_i = 50, key_i2 = 100;
    char key_rem[10], key_rem2[10];
    snprintf(key_rem, sizeof key_rem, "%s_%u", "key", key_i);
    snprintf(key_rem2, sizeof key_rem2, "%s_%u", "key", key_i2);

    struct Measurement *m_back = hashmap_remove(hashmap, key_rem);
    assert(m_back != NULL);
    assert(strcmp(m_back->name, "condition") == 0);
    assert(m_back->val_x == 50);

    assert(hashmap_remove(hashmap, key_rem2) != NULL);

    // run get operation for the rest of the measurement
    for (u32 j=1; j<=elems; ++j) {
        char key[10];
        snprintf(key, sizeof key, "%s_%u", "key", j);

        if (j == key_i || j == key_i2) {
            // removed items shouldn't been there anynore
            assert(hashmap_get(hashmap, key) == NULL);
            continue;
        }
        struct Measurement *m_back = hashmap_get(hashmap, key);
        assert(m_back != NULL);
        assert(m_back->val_y == (i32)(j * 2));
        assert(strcmp(m_back->name, "condition") == 0);
    }

    hashmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_complete_hashmap_mid_size() {
    size_t const type_size = sizeof(struct Measurement);
    size_t const init_elems = 500;

    struct HashMap *hashmap = hashmap_init_with_size(type_size, init_elems, NULL);
    assert(hashmap != NULL);
    // 500 < 2^9, thus capacity exponent should be nine
    assert(hashmap->ex_capa == 9);

    // insert 500 "measurements" to the hashmap
    // resize should occur after threshold value floor(0.9 * 500) is reached
    u32 const elems = 500;

    for (u32 i=1; i<=elems; ++i) {
        char key[10];
        snprintf(key, sizeof key, "%s_%u", "key", i);

        struct Measurement measurement = {
            .name="condition",
            .val_x=i,
            .val_y=i,
            .val_z=0,
            .normal=true
        };
        assert(hashmap_insert(hashmap, key, &measurement) == true);
    }
    assert(hashmap->ex_capa == 10);
    assert(hashmap->occ_slots == elems);

    // run get operations
    for (i32 i=elems; i>0; --i) {
        char key[10];
        snprintf(key, sizeof key, "%s_%d", "key", i);

        struct Measurement *m_back = hashmap_get(hashmap, key);
        assert(m_back != NULL);
        assert(m_back->val_x == i);
        assert(m_back->val_y == i);
        assert(strcmp(m_back->name, "condition") == 0);
        assert(m_back->normal == true);
    }

    // remove all measurements from the hashmap
    for (i32 i=elems; i>0; --i) {
        char key[10];
        snprintf(key, sizeof key, "%s_%d", "key", i);

        struct Measurement *m_back = hashmap_remove(hashmap, key);
        assert(m_back != NULL);
        assert(m_back->val_x == i);
        assert(m_back->val_y == i);
        assert(strcmp(m_back->name, "condition") == 0);

        // measurement should have removed by now
        assert(hashmap_get(hashmap, key) == NULL);
    }
    assert(hashmap->occ_slots == 0);
    // no elements, the capacity should be now the smallest allowed
    assert(hashmap->ex_capa == MAP_INIT_EXP_CAPACITY);

    hashmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_complete_hashmap_oversize_init() {
    size_t const type_size = sizeof(struct Measurement);
    size_t const init_elems = 1050000;

    // requested storage (element count) larger than maximal allowed capacity

    struct HashMap *hashmap = hashmap_init_with_size(type_size, init_elems, NULL);
    assert(hashmap == NULL);

    PRINT_SUCCESS(__func__);
}


test_func hashmap_tests[] = {
    {"complete_hashmap", test_complete_hashmap},
    {"complete_hashmap_mid_size", test_complete_hashmap_mid_size},
    {"complete_hashmap_oversize_init", test_complete_hashmap_oversize_init},
    {NULL, NULL},
};
