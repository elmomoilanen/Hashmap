#include "common.h"
#include "map.h"


struct HashMap* hashmap_init(size_t item_size, void (*clean_func)(void *)) {
    u32 default_init_capa = MAP_INIT_EXP_CAPACITY;

    return hmap_init(item_size, default_init_capa, clean_func);
}

static u32 _get_init_capa(size_t elems) {
    u32 exp = MAP_INIT_EXP_CAPACITY;

    do {
        if (elems <= (1U << exp)) break;
        exp += 1;
    } while (exp <= MAP_MAX_EXP_CAPACITY);

    return exp;
}

struct HashMap* hashmap_init_with_size(
    size_t item_size,
    size_t elems,
    void (*clean_func)(void *))
{
    u32 init_capa = _get_init_capa(elems);

    return hmap_init(item_size, init_capa, clean_func);
}

void hashmap_insert() {

}

void hashmap_get() {

}

void hashmap_remove() {

}

void hashmap_free() {

}

void hashmap_traverse() {

}
