#include "common.h"
#include "map.h"


typedef struct {
    i32 value_x;
    i32 value_y;
    char *text;
} test_type_a;


struct vector {
    i32 *data;
    u32 size;
    u32 capacity;
};


static void test_value_set_macro_lsb() {
    u32 const offset = 0x0u;
    u32 const mask = 0x1u;
    u32 const offset_mid = 0x1u;
    u32 const mask_mid = 0x00000FFEu;

    u32 number = 0;
    number = META_VALUE_SET(number, 1U, offset, mask);
    assert(number == 1U);

    number = META_VALUE_SET(number, 0U, offset, mask);
    assert(number == 0U);

    number = META_VALUE_SET(number, 1U, offset_mid, mask_mid);
    assert(number == 2U);

    number = META_VALUE_SET(number, 1U, offset, mask);

    number = META_VALUE_SET(number, 4U, offset_mid, mask_mid);
    assert(number == 9U);

    number = META_VALUE_SET(number, 2047U, offset_mid, mask_mid);
    assert(number == 4095U);

    number = META_VALUE_SET(number, 0U, offset_mid, mask_mid);
    assert(number == 1U);

    PRINT_SUCCESS(__func__);
}

static void test_value_set_macro_msb() {
    u32 const offset_mid = 0x1u;
    u32 const mask_mid = 0x00000FFEu;
    u32 const offset_high = 0xCu;
    u32 const mask_high = 0xFFFFF000u;

    u32 number = 0;
    number = META_VALUE_SET(number, 1000U, offset_mid, mask_mid);
    assert(number == 2000U);

    number = META_VALUE_SET(number, 1U, offset_high, mask_high);
    assert(number == 6096U);

    number = META_VALUE_SET(number, 2047U, offset_mid, mask_mid);
    assert(number == 8190U);

    number = META_VALUE_SET(number, 1048575U, offset_high, mask_high);
    assert(number == 4294967294U);

    number = META_VALUE_SET(number, 0U, offset_high, mask_high);
    assert(number == 4094U);

    PRINT_SUCCESS(__func__);
}

static void test_value_get_macro() {
    u32 const mask = 0x00000FFEu;
    u32 const offset = 0x1u;
    u32 const offset_high = 0xCu;
    u32 const mask_high = 0xFFFFF000u;

    u32 number = 0;
    number = META_VALUE_SET(number, 2047U, offset, mask);
    assert(META_VALUE_GET(number, offset, mask) == 2047U);

    number = META_VALUE_SET(number, 1U, 0x0u, 0x1u);

    number = META_VALUE_SET(number, 1000U, offset, mask);
    assert(META_VALUE_GET(number, offset, mask) == 1000U);

    number = META_VALUE_SET(number, 500000U, offset_high, mask_high);
    assert(META_VALUE_GET(number, offset_high, mask_high) == 500000U);
    
    PRINT_SUCCESS(__func__);
}

static void test_hashmap_init() {
    struct HashMap *hashmap = hmap_init_with_key(sizeof(test_type_a), NULL);

    assert(hashmap != NULL);
    assert(hashmap->slots != NULL);
    assert(hashmap->_temp != NULL);

    assert(hashmap->occ_slots == 0);
    assert(hashmap->clean_func == NULL);

    assert(hashmap->ex_capa == MAP_INIT_EXP_CAPACITY);

    hmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_init_with_random_key() {
    struct HashMap *hashmap = hmap_init(sizeof(test_type_a), MAP_INIT_EXP_CAPACITY, NULL);

    assert(hashmap != NULL);
    assert(hashmap->slots != NULL);
    assert(hashmap->_temp != NULL);

    assert(hashmap->occ_slots == 0);
    assert(hashmap->clean_func == NULL);

    assert(hashmap->ex_capa == MAP_INIT_EXP_CAPACITY);

    hmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_init_invalid_item_size() {
    u64 const item_size = (u64)UINT32_MAX + 1ULL;
    struct HashMap *hashmap = hmap_init_with_key(item_size, NULL);

    assert(hashmap == NULL);

    PRINT_SUCCESS(__func__);
}

static void test_empty_hashmap_metadata() {
    struct HashMap *hashmap = hmap_init_with_key(sizeof(test_type_a), NULL);

    assert(hashmap != NULL);
    assert(hashmap->occ_slots == 0);
    assert(get_occupied_slot_count(hashmap) == 0);

    hmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_operations_small_size() {
    struct HashMap *hashmap = hmap_init_with_key(sizeof(test_type_a), NULL);
    assert(hashmap != NULL);

    test_type_a test_struct1 = {.value_x=2, .value_y=1, .text="test"};
    test_type_a test_struct2 = {.value_x=-1, .value_y=-5, .text="other"};

    bool response = hmap_insert(hashmap, "elem1", &test_struct1);
    assert(response == true);
    response = hmap_insert(hashmap, "elem2", &test_struct2);
    assert(response == true);
    assert(hashmap->occ_slots == 2);

    test_type_a *resp_struct;

    resp_struct = hmap_get(hashmap, "elem1");
    assert(resp_struct != NULL);
    assert(resp_struct->value_x == 2);
    assert(strcmp(resp_struct->text, "test") == 0);

    resp_struct = hmap_get(hashmap, "elem2");
    assert(resp_struct != NULL);
    assert(resp_struct->value_x == -1);
    assert(strcmp(resp_struct->text, "other") == 0);

    hmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_misc_operations_small_size() {
    struct HashMap *hashmap = hmap_init_with_key(sizeof(test_type_a), NULL);
    assert(hashmap != NULL);

    assert(hmap_get(hashmap, "key_not_there") == NULL);
    assert(
        hmap_insert(hashmap, "key_is_there", &(test_type_a){.value_x=-1, .value_y=0, .text="testing"})
    );
    // next key is the longest allowed
    assert(
        hmap_insert(hashmap, "key_is_there_other_", &(test_type_a){.value_x=0, .value_y=0, .text="testing"})
    );

    assert(hashmap->occ_slots == 2);
    assert(get_occupied_slot_count(hashmap) == hashmap->occ_slots);

    assert(hmap_get(hashmap, "key_is_there_other_") != NULL);

    // next key should be one character too long to fit (key length restriction)
    assert(
        hmap_insert(hashmap, "key_is_there_other__", &(test_type_a){.value_x=0, .value_y=0, .text="test"})
        == false
    );
    // invalid keys, should't been anything for return
    assert(hmap_get(hashmap, "key_is_there_other__") == NULL);
    assert(hmap_get(hashmap, NULL) == NULL);

    test_type_a *test_struct = hmap_get(hashmap, "key_is_there");
    assert(test_struct != NULL);
    assert(strcmp(test_struct->text, "testing") == 0);
    assert(test_struct->value_x == -1);

    hmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_duplicate_insertions() {
    struct HashMap *hashmap = hmap_init_with_key(sizeof(test_type_a), NULL);
    assert(hashmap != NULL);

    assert(
        hmap_insert(hashmap, "this_is_key1", &(test_type_a){.value_x=1, .value_y=0, .text="test"})
    );
    assert(
        hmap_insert(hashmap, "this_is_key1", &(test_type_a){.value_x=-1, .value_y=-1, .text="test_2"})
    );
    assert(hashmap->occ_slots == 1);

    test_type_a *test_struct = hmap_get(hashmap, "this_is_key1");
    assert(test_struct != NULL);
    assert(test_struct->value_y == -1);

    assert(
        hmap_insert(hashmap, "this_is_key1", &(test_type_a){.value_x=-2, .value_y=-2, .text="test"})
    );
    assert(hashmap->occ_slots == 1);

    test_struct = hmap_get(hashmap, "this_is_key1");    
    assert(test_struct != NULL);
    assert(test_struct->value_y == -2);

    hmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_operations_small_size_many_insertions() {
    struct HashMap *hashmap = hmap_init_with_key(sizeof(test_type_a), NULL);
    assert(hashmap != NULL);

    u32 const elems = 10;
    for (u32 i=1; i<=elems; ++i) {
        char key[10];
        snprintf(key, sizeof key, "%s_%u", "key", i);

        bool response = hmap_insert(
            hashmap,
            key,
            &(test_type_a){.value_x=i, .value_y=i, .text="test"}
        );
        assert(response == true);
    }

    assert(hashmap->occ_slots == elems);
    assert(hashmap->occ_slots == get_occupied_slot_count(hashmap));

    for (u32 i=1; i<=elems; ++i) {
        char key[10];
        snprintf(key, sizeof key, "%s_%u", "key", i);

        test_type_a *resp = hmap_get(
            hashmap,
            key
        );
        assert(resp != NULL);
    }

    test_type_a *resp_struct;
    resp_struct = hmap_get(hashmap, "key_1");
    assert(resp_struct != NULL);
    assert(resp_struct->value_x == 1);
    assert(strcmp(resp_struct->text, "test") == 0);

    char last_key[10];
    snprintf(last_key, sizeof last_key, "%s_%u", "key", elems);
    resp_struct = hmap_get(hashmap, last_key);
    assert(resp_struct != NULL);
    assert(resp_struct->value_x == elems);

    hmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_init_to_specific_size() {
    u32 const init_exp = 5;
    struct HashMap *hashmap = hmap_init(sizeof(test_type_a), init_exp, NULL);

    assert(hashmap != NULL);
    assert(hashmap->slots != NULL);
    assert(hashmap->_temp != NULL);
    assert(hashmap->occ_slots == 0);
    assert(hashmap->clean_func == NULL);

    assert(hashmap->ex_capa == init_exp);

    hmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_init_for_too_large_size() {
    u32 const init_exp = MAP_MAX_EXP_CAPACITY + 1;
    struct HashMap *hashmap = hmap_init(sizeof(test_type_a), init_exp, NULL);

    assert(hashmap == NULL);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_init_for_too_large_item_size() {
    size_t const type_size = UINT32_MAX;
    struct HashMap *hashmap = hmap_init(type_size, MAP_INIT_EXP_CAPACITY, NULL);

    assert(hashmap == NULL);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_element_removal() {
    struct HashMap *hashmap = hmap_init_with_key(sizeof(test_type_a), NULL);
    assert(hashmap != NULL);

    hmap_insert(hashmap, "key_1", &(test_type_a){.value_x=1, .value_y=1, .text="test"});
    hmap_insert(hashmap, "key_2", &(test_type_a){.value_x=2, .value_y=2, .text="test"});
    hmap_insert(hashmap, "key_3", &(test_type_a){.value_x=3, .value_y=3, .text="test"});

    assert(hashmap->occ_slots == 3);

    test_type_a *test_struct = hmap_remove(hashmap, "key_2");
    assert(test_struct != NULL);
    assert(test_struct->value_x == 2);
    assert(hashmap->occ_slots == 2);
    assert(hashmap->occ_slots == get_occupied_slot_count(hashmap));

    test_struct = hmap_remove(hashmap, "key_2");
    assert(test_struct == NULL);
    assert(hashmap->occ_slots == 2);

    test_struct = hmap_remove(hashmap, "key_1");
    assert(test_struct != NULL);
    assert(test_struct->value_x == 1);
    assert(hashmap->occ_slots == 1);

    assert(hmap_get(hashmap, "key_2") == NULL);
    assert(hmap_get(hashmap, "key_1") == NULL);
    assert(hmap_get(hashmap, "key_3") != NULL);

    hmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_resizing_up() {
    struct HashMap *hashmap = hmap_init_with_key(sizeof(test_type_a), NULL);
    assert(hashmap != NULL);

    // insert 16 elements, resizing should happen at 14th insertion
    u32 const elems = 16;
    for (u32 i=1; i<=elems; ++i) {
        char key[10];
        snprintf(key, sizeof key, "%s_%u", "key", i);

        bool response = hmap_insert(
            hashmap,
            key,
            &(test_type_a){.value_x=i, .value_y=i, .text="test"}
        );
        assert(response == true);
    }

    assert(hashmap->occ_slots == elems);
    assert(hashmap->ex_capa == MAP_INIT_EXP_CAPACITY + 1);

    hmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_resizing_up_and_down() {
    u32 const init_exp = 5;
    struct HashMap *hashmap = hmap_init(sizeof(test_type_a), init_exp, NULL);

    assert(hashmap != NULL);
    assert(hashmap->ex_capa == init_exp);

    hmap_insert(hashmap, "key", &(test_type_a){.value_x=0, .value_y=0, .text="test"});
    assert(hashmap->occ_slots == 1);

    test_type_a *test_struct = hmap_remove(hashmap, "key");
    assert(test_struct != NULL);

    assert(hashmap->occ_slots == 0);
    assert(hashmap->ex_capa == init_exp - 1);

    hmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_resizing_down() {
    u32 const init_exp = 8;
    struct HashMap *hashmap = hmap_init(sizeof(test_type_a), init_exp, NULL);

    assert(hashmap != NULL);
    assert(hashmap->ex_capa == init_exp);

    hmap_insert(hashmap, "key", &(test_type_a){.value_x=0, .value_y=0, .text="test"});

    assert(hashmap->occ_slots == 1);
    assert(hashmap->ex_capa == init_exp);

    // following removal should drop the size of the hashmap to lowest possible

    test_type_a *test_struct = hmap_remove(hashmap, "key");
    assert(test_struct != NULL);

    assert(hashmap->occ_slots == 0);
    assert(hashmap->ex_capa == MAP_INIT_EXP_CAPACITY);

    hmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_removing_and_resizing() {
    u32 const init_exp = 6;
    struct HashMap *hashmap = hmap_init(sizeof(test_type_a), init_exp, NULL);

    assert(hashmap != NULL);
    assert(hashmap->ex_capa == init_exp);

    u32 const elems_orig = 20;
    u32 elems = elems_orig;
    for (u32 i=1; i<=elems; ++i) {
        char key[10];
        snprintf(key, sizeof key, "%s_%u", "key", i);
        hmap_insert(hashmap, key, &(test_type_a){.value_x=i, .value_y=i, .text="test"});
    }
    assert(hashmap->ex_capa == init_exp);
    assert(hashmap->occ_slots == elems);

    u32 const elems_rvm = 5;
    for (u32 j=10; j<10+elems_rvm; ++j) {
        char key[10];
        snprintf(key, sizeof key, "%s_%u", "key", j);

        test_type_a *test_struct = hmap_remove(hashmap, key);
        assert(test_struct != NULL);
        assert(strcmp(test_struct->text, "test") == 0);
        assert(test_struct->value_y == (i32)j);
    }
    elems -= elems_rvm;
    assert(hashmap->ex_capa == init_exp - 1);
    assert(hashmap->occ_slots == elems);

    assert(hmap_remove(hashmap, "key_10") == NULL);
    assert(hmap_get(hashmap, "key_10") == NULL);

    for (u32 j=1; j<=elems_orig; ++j) {
        char key[10];
        snprintf(key, sizeof key, "%s_%u", "key", j);

        if (j >= 10 && j < 10+elems_rvm) {
            assert(hmap_get(hashmap, key) == NULL);
            continue;
        }
        test_type_a *test_struct = hmap_remove(hashmap, key);
        assert(test_struct != NULL);
        assert(strcmp(test_struct->text, "test") == 0);
        assert(test_struct->value_y == (i32)j);
    }
    assert(MAP_INIT_EXP_CAPACITY == init_exp - 2);
    assert(hashmap->ex_capa == MAP_INIT_EXP_CAPACITY);
    assert(hashmap->occ_slots == 0);

    hmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_invalid_keys() {
    struct HashMap *hashmap = hmap_init_with_key(sizeof(test_type_a), NULL);
    assert(hashmap != NULL);

    assert(hmap_get(hashmap, NULL) == NULL);
    assert(hmap_get(hashmap, "this_key_is_too_long") == NULL);

    assert(hmap_remove(hashmap, NULL) == NULL);
    assert(hmap_remove(hashmap, "this_key_is_too_long") == NULL);

    test_type_a placeholder = {0};

    assert(hmap_insert(hashmap, NULL, &placeholder) == false);
    assert(hmap_insert(hashmap, "this_key_is_too_long", &placeholder) == false);
    assert(hmap_insert(hashmap, "valid_key", NULL) == false);

    hmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_misc_operations_mid_size() {
    u32 const init_exp = 6;
    struct HashMap *hashmap = hmap_init(sizeof(test_type_a), init_exp, NULL);

    assert(hashmap != NULL);
    assert(hashmap->ex_capa == init_exp);

    u32 const elems = 50;

    for (u32 i=1; i<=elems; ++i) {
        char key[10];
        snprintf(key, sizeof key, "%s_%u", "key", i);

        hmap_insert(
            hashmap,
            key,
            &(test_type_a){.value_x=i, .value_y=i, .text="test"}
        );
    }
    assert(hashmap->occ_slots == elems);
    assert(hashmap->ex_capa == init_exp);

    // remove elems such that the size will drop down

    for (u32 i=21; i<=elems; ++i) {
        char key[10];
        snprintf(key, sizeof key, "%s_%u", "key", i);

        test_type_a *resp = hmap_remove(hashmap, key);
        assert(resp != NULL);
    }
    assert(hashmap->ex_capa == init_exp - 1);
    assert(elems == 50); // just to make sure that following correct
    // test few keys
    assert(hmap_get(hashmap, "key_30") == NULL);
    assert(hmap_get(hashmap, "key_50") == NULL);

    assert(hmap_get(hashmap, "key_10") != NULL);

    hmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_integer_data() {
    // test hashmap for type int32_t
    struct HashMap *hashmap = hmap_init_with_key(sizeof(i32), NULL);
    assert(hashmap != NULL);

    i32 x = 10;
    i32 x2 = 20;

    assert(hmap_insert(hashmap, "key_1", &x) == true);
    assert(hmap_insert(hashmap, "key_2", &x2) == true);

    i32 *int_ptr = hmap_get(hashmap, "key_1");
    assert(int_ptr != NULL);
    assert(*int_ptr == x);

    int_ptr = hmap_remove(hashmap, "key_1");
    assert(int_ptr != NULL);
    assert(*int_ptr == x);

    int_ptr = hmap_get(hashmap, "key_2");
    assert(int_ptr != NULL);
    assert(*int_ptr == x2);

    assert(hmap_get(hashmap, "key_1") == NULL);

    hmap_free(hashmap);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_array_data() {
    u32 const elems = 15;
    i32 *arr = calloc(elems, sizeof *arr);
    assert(arr != NULL);

    i32 *arr2 = calloc(elems, sizeof *arr2);
    assert(arr2 != NULL);

    for (u32 i=0; i<elems; ++i) arr[i] = 1;
    for (u32 i=0; i<elems; ++i) arr2[i] = -1;

    struct HashMap *hashmap = hmap_init_with_key(elems * sizeof *arr, NULL);
    assert(hashmap != NULL);

    assert(hmap_insert(hashmap, "key", arr) == true);
    assert(hmap_insert(hashmap, "key2", arr2) == true);
    assert(hashmap->occ_slots == 2);

    i32 *arr_back = hmap_get(hashmap, "key");
    assert(arr_back != NULL);
    for (u32 i=0; i<elems; ++i) assert(arr_back[i] == 1);

    assert(hmap_remove(hashmap, "key") != NULL);

    i32 *arr2_back = hmap_get(hashmap, "key2");
    assert(arr2_back != NULL);
    for (u32 i=0; i<elems; ++i) assert(arr2_back[i] == -1);

    hmap_free(hashmap);

    free(arr);
    free(arr2);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_custom_allocation() {
    struct vector *vec = malloc(1 * sizeof *vec);
    assert(vec != NULL);

    u32 const data_size = 25;
    vec->data = malloc(data_size * sizeof(*vec->data));
    assert(vec->data != NULL);

    vec->capacity = data_size * 2;
    vec->size = data_size;
    for (u32 i=0; i<vec->size; ++i) vec->data[i] = i;

    struct HashMap *hashmap = hmap_init_with_key(sizeof *vec, NULL);
    assert(hashmap != NULL);

    assert(hmap_insert(hashmap, "key", vec) == true);
    assert(hashmap->occ_slots == 1);

    struct vector *vec_back = hmap_get(hashmap, "key");
    assert(vec_back != NULL);
    assert(vec_back->size == data_size);
    for (i32 i=0; i<(i32)vec_back->size; ++i) assert(vec_back->data[i] == i);

    hmap_free(hashmap);

    // hmap_free did not follow vec->data as it shouldn't in this case
    free(vec->data);
    free(vec);

    PRINT_SUCCESS(__func__);
}

static u8 custom_clean_vec_call_counter = 0;

void custom_clean_vec(void *data_item) {
    // following condition check is mandatory
    if (((struct vector *)data_item)->data) {
        custom_clean_vec_call_counter += 1;
        free(((struct vector *)data_item)->data);
    }
}

static void test_hashmap_custom_allocation_and_free() {
    struct vector *vec = malloc(1 * sizeof *vec);
    assert(vec != NULL);

    u32 const data_size = 5;
    vec->data = malloc(data_size * sizeof(*vec->data));
    assert(vec->data != NULL);

    vec->capacity = data_size * 2;
    vec->size = data_size;
    for (u32 i=0; i<vec->size; ++i) vec->data[i] = i;

    // pass a custom clean up function
    struct HashMap *hashmap = hmap_init_with_key(sizeof *vec, custom_clean_vec);
    assert(hashmap != NULL);

    assert(hmap_insert(hashmap, "key_to_vec", vec) == true);
    assert(hashmap->occ_slots == 1);

    assert(custom_clean_vec_call_counter == 0);

    hmap_free(hashmap);

    // check that the custom clean up was really called
    assert(custom_clean_vec_call_counter == 1);

    // hmap_free followed vec->data and freed it, must not free it anymore
    free(vec);

    PRINT_SUCCESS(__func__);
}

static void test_hashmap_custom_allocation_with_remove() {
    struct vector *vec = malloc(1 * sizeof *vec);
    assert(vec != NULL);

    u32 const data_size = 3;
    vec->data = malloc(data_size * sizeof(*vec->data));
    assert(vec->data != NULL);

    vec->capacity = data_size * 2;
    vec->size = data_size;
    for (u32 i=0; i<vec->size; ++i) vec->data[i] = i;

    custom_clean_vec_call_counter = 0;

    // pass a custom clean up function
    struct HashMap *hashmap = hmap_init_with_key(sizeof *vec, custom_clean_vec);
    assert(hashmap != NULL);

    assert(hmap_insert(hashmap, "again_key_to_vec", vec) == true);
    assert(hashmap->occ_slots == 1);

    assert(hmap_remove(hashmap, "again_key_to_vec") != NULL);

    assert(custom_clean_vec_call_counter == 0);
    hmap_free(hashmap);

    // there shouldn't been clean up call as the remove operation was called before
    assert(custom_clean_vec_call_counter == 0);

    // it's our responsibility to clean the data
    free(vec->data);
    free(vec);

    PRINT_SUCCESS(__func__);
}


test_func map_tests[] = {
    {"value_set_macro_lsb", test_value_set_macro_lsb},
    {"value_set_macro_msb", test_value_set_macro_msb},
    {"value_get_macro", test_value_get_macro},
    {"hashmap_init", test_hashmap_init},
    {"hashmap_init_with_random_key", test_hashmap_init_with_random_key},
    {"hashmap_init_invalid_item_size", test_hashmap_init_invalid_item_size},
    {"empty_hashmap_metadata", test_empty_hashmap_metadata},
    {"hashmap_operations_small_size", test_hashmap_operations_small_size},
    {"hashmap_misc_operations_small_size", test_hashmap_misc_operations_small_size},
    {"hashmap_duplicate_insertions", test_hashmap_duplicate_insertions},
    {"hashmap_operations_small_size_many_insertions", test_hashmap_operations_small_size_many_insertions},
    {"hashmap_init_to_specific_size", test_hashmap_init_to_specific_size},
    {"hashmap_init_for_too_large_size", test_hashmap_init_for_too_large_size},
    {"hashmap_init_for_too_large_item_size", test_hashmap_init_for_too_large_item_size},
    {"hashmap_element_removal", test_hashmap_element_removal},
    {"hashmap_resizing_up", test_hashmap_resizing_up},
    {"hashmap_resizing_up_and_down", test_hashmap_resizing_up_and_down},
    {"hashmap_resizing_down", test_hashmap_resizing_down},
    {"hashmap_removing_and_resizing", test_hashmap_removing_and_resizing},
    {"hashmap_invalid_keys", test_hashmap_invalid_keys},
    {"hashmap_misc_operations_mid_size", test_hashmap_misc_operations_mid_size},
    {"hashmap_integer_data", test_hashmap_integer_data},
    {"hashmap_array_data", test_hashmap_array_data},
    {"hashmap_custom_allocation", test_hashmap_custom_allocation},
    {"test_hashmap_custom_allocation_and_free", test_hashmap_custom_allocation_and_free},
    {"test_hashmap_custom_allocation_with_remove", test_hashmap_custom_allocation_with_remove},
    {NULL, NULL},
};
