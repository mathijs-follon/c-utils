#ifndef HASHSET_H
#define HASHSET_H

#include "hashmap.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Type-erased hash set: thin wrapper over HashMap with zero-size values.
 *
 *     HashSet s = HASHSET_NEW(int);
 *     HASHSET_ADD(int, &s, 1);
 *     HASHSET_CONTAINS(int, &s, 1);
 *     hashset_delete(&s);
 */

#define HASHSET_NULL ((HashSet){HASHMAP_NULL})

typedef struct {
    HashMap map;
} HashSet;

HashSet hashset_new(usize key_stride, HashMapHashFn hash, HashMapEqFn eq, void *ctx);
HashSet hashset_new_reserve(usize key_stride, usize capacity, HashMapHashFn hash, HashMapEqFn eq,
                            void *ctx);

void hashset_delete(HashSet *set);
void hashset_clear(HashSet *set);

usize hashset_length(const HashSet *set);
usize hashset_capacity(const HashSet *set);
usize hashset_key_stride(const HashSet *set);
bool hashset_empty(const HashSet *set);
bool hashset_valid(const HashSet *set);

bool hashset_reserve(HashSet *set, usize capacity);

bool hashset_add(HashSet *set, const void *key);
bool hashset_contains(const HashSet *set, const void *key);
bool hashset_remove(HashSet *set, const void *key);

/* Typed helpers.
 *
 *   HASHSET_NEW(K)                                        -> HashSet (bytes hash/eq)
 *   HASHSET_ADD(K, HashSet *set, key)                     -> bool
 *   HASHSET_CONTAINS(K, const HashSet *set, key)          -> bool
 *   HASHSET_REMOVE(K, HashSet *set, key)                  -> bool
 */

#define HASHSET_NEW(K)                                                                             \
    ((HashSet){hashmap_new(sizeof(K), 0, hashmap_hash_bytes, hashmap_eq_bytes, NULL)})

#define HASHSET_ADD(K, set, ...) (hashset_add((set), &(K){__VA_ARGS__}))
#define HASHSET_CONTAINS(K, set, ...) (hashset_contains((set), &(K){__VA_ARGS__}))
#define HASHSET_REMOVE(K, set, ...) (hashset_remove((set), &(K){__VA_ARGS__}))

#ifdef __cplusplus
}
#endif

#endif /* HASHSET_H */
