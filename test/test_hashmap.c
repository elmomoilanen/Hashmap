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

typedef struct {
    i32 x;
    i32 y;
    bool found;
} i32_pair;

typedef struct {
    f32 kelvin;
    u32 hour;
    u32 mins;
} Temperature;


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

static i32_pair find_pair_that_sum_to_specific_target(i32 *array, u32 arr_len, i32 target) {
    struct HashMap *hashmap = hashmap_init_with_size(sizeof *array, arr_len, NULL);
    assert(hashmap != NULL);

    i32_pair pair = {0};
    pair.found = false;
    i32 placeholder = 0;

    for (u32 j=0; j<arr_len; ++j) {
        i32 num = array[j];
        i32 search_num = target - num;

        i32 const buf_size = snprintf(NULL, 0, "%d", search_num);
        char *search_key = calloc(1, buf_size + 1);
        snprintf(search_key, buf_size + 1, "%d", search_num);

        if (hashmap_get(hashmap, search_key) == NULL) {
            i32 const buf_size2 = snprintf(NULL, 0, "%d", num);
            char *key = calloc(1, buf_size2 + 1);
            snprintf(key, buf_size2 + 1, "%d", num);
            hashmap_insert(hashmap, key, &placeholder);
            free(key);
        } else {
            pair.x = num;
            pair.y = search_num;
            pair.found = true;
            free(search_key);
            break;
        }
        free(search_key);
    }
    hashmap_free(hashmap);

    return pair;
}

static void test_hashmap_usage_in_search_algorithm() {
    i32 array[] = {-1, -11, 2, 3, -2, -1, 0, 1, -7, 3};
    u32 const arr_size = sizeof(array) / sizeof(array[0]);

    // check whether there are array elements for which the sum equals -11
    i32_pair result = find_pair_that_sum_to_specific_target(array, arr_size, -11);

    assert(result.found == true);
    assert(result.x == 0);
    assert(result.y == -11);

    PRINT_SUCCESS(__func__);
}

static u32 iter_visited_counter = 0;

bool custom_callback(char const *key, void const *data) {
    assert(key != NULL);

    u32 const mins = ((Temperature *)data)->mins;
    if (mins > 0) {
        iter_visited_counter += mins;
        return true;
    }
    return false;
}

static void test_hashmap_iter_apply() {
    iter_visited_counter = 0;
    struct HashMap *hashmap = hashmap_init(sizeof(Temperature), NULL);

    u32 const data_items = 10;

    for (u32 j=1; j<=data_items; ++j) {
        char key[10];
        snprintf(key, sizeof key, "%s_%u", "key", j);

        Temperature temp = {
            .kelvin=(f32)j,
            .hour=j,
            .mins=1
        };

        assert(hashmap_insert(hashmap, key, &temp) == true);
    }
    // Test that all slots were visited and returned true
    assert(hashmap_iter_apply(hashmap, custom_callback) == true);
    assert(iter_visited_counter == data_items);

    hashmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_iter_apply_early_termination() {
    iter_visited_counter = 0;
    struct HashMap *hashmap = hashmap_init(sizeof(Temperature), NULL);

    hashmap_insert(hashmap, "1.8.2021", &(Temperature){.kelvin=250.0, .hour=12, .mins=1});
    hashmap_insert(hashmap, "2.8.2021", &(Temperature){.kelvin=257.0, .hour=12, .mins=0});
    hashmap_insert(hashmap, "3.8.2021", &(Temperature){.kelvin=244.0, .hour=12, .mins=1});

    // Test that the return value is correct
    assert(hashmap_iter_apply(hashmap, custom_callback) == false);

    hashmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

bool custom_callback_keys(char const *key, void const *data) {
    assert(data != NULL);

    // MAP_MAX_KEY_BYTES == 20
    if (strncmp(key, "15.8.2021", 20) == 0 || strncmp(key, "16.8.2021", 20) == 0) {
        iter_visited_counter += 1;
        return true;
    }
    return false;
}

static void test_hashmap_iter_apply_keys() {
    iter_visited_counter = 0;
    struct HashMap *hashmap = hashmap_init(sizeof(Temperature), NULL);

    hashmap_insert(hashmap, "15.8.2021", &(Temperature){.kelvin=251.0, .hour=12, .mins=0});
    hashmap_insert(hashmap, "16.8.2021", &(Temperature){.kelvin=254.0, .hour=12, .mins=0});

    assert(hashmap_iter_apply(hashmap, custom_callback_keys) == true);
    assert(iter_visited_counter == 2);

    hashmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_readme_example() {
    struct HashMap *hashmap = hashmap_init(sizeof(Temperature), NULL);

    Temperature temp_18 = {.kelvin=293.15, .hour=12, .mins=0};
    Temperature temp_28 = {.kelvin=298.15, .hour=12, .mins=0};

    assert(hashmap_insert(hashmap, "1.8.2021", &temp_18) == true);
    assert(hashmap_insert(hashmap, "2.8.2021", &temp_28) == true);

    Temperature *t_18 = hashmap_get(hashmap, "1.8.2021");
    assert(t_18 != NULL);
    assert(t_18->kelvin - temp_18.kelvin < 0.01);
    assert(t_18->hour == 12);
    assert(t_18->mins == 0);

    assert(hashmap_remove(hashmap, "1.8.2021") != NULL);
    assert(hashmap_get(hashmap, "1.8.2021") == NULL);

    Temperature *t_28 = hashmap_get(hashmap, "2.8.2021");
    assert(t_28 != NULL);
    assert(t_28->kelvin - temp_28.kelvin < 0.01);
    assert(t_28->hour == 12);
    assert(t_28->mins == 0);

    assert(hashmap_remove(hashmap, "2.8.2021") != NULL);
    assert(hashmap_get(hashmap, "2.8.2021") == NULL);
    
    hashmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}


test_func hashmap_tests[] = {
    {"complete_hashmap", test_complete_hashmap},
    {"complete_hashmap_mid_size", test_complete_hashmap_mid_size},
    {"complete_hashmap_oversize_init", test_complete_hashmap_oversize_init},
    {"hashmap_usage_in_search_algorithm", test_hashmap_usage_in_search_algorithm},
    {"hashmap_iter_apply", test_hashmap_iter_apply},
    {"hashmap_iter_apply_early_termination", test_hashmap_iter_apply_early_termination},
    {"hashmap_iter_apply_keys", test_hashmap_iter_apply_keys},
    {"hashmap_readme_example", test_hashmap_readme_example},
    {NULL, NULL},
};
