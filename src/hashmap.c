#include "common.h"
#include "map.h"

struct HashMap* hashmap_init(size_t item_size, void (*clean_func)(void *)) {
    return hmap_init(item_size, MAP_INIT_EXP_CAPACITY, clean_func);
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

bool hashmap_insert(
    struct HashMap *hashmap,
    char const *key,
    void const *data)
{
    return hmap_insert(hashmap, key, data);
}

void* hashmap_get(struct HashMap *hashmap, char const *key) {
    return hmap_get(hashmap, key);
}

void* hashmap_remove(struct HashMap *hashmap, char const *key) {
    return hmap_remove(hashmap, key);
}

void hashmap_free(struct HashMap *hashmap) {
    hmap_free(hashmap);
}

bool hashmap_iter_apply(
    struct HashMap *hashmap,
    bool (*callback)(char const *, void *))
{
    return hmap_iter_apply(hashmap, callback);
}

u32 hashmap_len(struct HashMap *hashmap) {
    return hmap_len(hashmap);
}

void hashmap_stats_traverse(struct HashMap *hashmap) {
    traverse_hashmap_slots(hashmap);
}

void hashmap_stats_summary(struct HashMap *hashmap) {
    hmap_show_stats(hashmap);
}
