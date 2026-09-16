#include "c_utils/hashset.h"

HashSet hashset_new(usize key_stride, HashMapHashFn hash, HashMapEqFn eq, void *ctx) {
    return (HashSet){hashmap_new(key_stride, 0, hash, eq, ctx)};
}

HashSet hashset_new_reserve(usize key_stride, usize capacity, HashMapHashFn hash, HashMapEqFn eq,
                            void *ctx) {
    return (HashSet){hashmap_new_reserve(key_stride, 0, capacity, hash, eq, ctx)};
}

void hashset_delete(HashSet *set) {
    if (!set)
        return;
    hashmap_delete(&set->map);
}

void hashset_clear(HashSet *set) {
    if (!set)
        return;
    hashmap_clear(&set->map);
}

usize hashset_length(const HashSet *set) { return set ? hashmap_length(&set->map) : 0; }

usize hashset_capacity(const HashSet *set) { return set ? hashmap_capacity(&set->map) : 0; }

usize hashset_key_stride(const HashSet *set) { return set ? hashmap_key_stride(&set->map) : 0; }

bool hashset_empty(const HashSet *set) { return hashset_length(set) == 0; }

bool hashset_valid(const HashSet *set) { return set && hashmap_valid(&set->map); }

bool hashset_reserve(HashSet *set, usize capacity) {
    if (!set)
        return false;
    return hashmap_reserve(&set->map, capacity);
}

bool hashset_add(HashSet *set, const void *key) {
    if (!set)
        return false;
    return hashmap_put(&set->map, key, NULL);
}

bool hashset_contains(const HashSet *set, const void *key) {
    if (!set)
        return false;
    return hashmap_contains(&set->map, key);
}

bool hashset_remove(HashSet *set, const void *key) {
    if (!set)
        return false;
    return hashmap_remove(&set->map, key, NULL);
}
