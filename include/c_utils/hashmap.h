#ifndef HASHMAP_H
#define HASHMAP_H

#include "c_utils/types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Type-erased open-addressing hash map (linear probing).
 *
 *     HashMap m = HASHMAP_NEW(int, int);
 *     HASHMAP_PUT(int, int, &m, 1, 100);
 *     int v = 0;
 *     HASHMAP_GET(int, int, &m, 1, &v);
 *     hashmap_delete(&m);
 *
 * Custom hash/eq: pass hashmap_new with your own HashMapHashFn / HashMapEqFn.
 * key_size is the map's key_stride. ctx is the user pointer from hashmap_new.
 */

#define HASHMAP_NULL ((HashMap){NULL, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL})

typedef u64 (*HashMapHashFn)(const void *key, usize key_size, void *ctx);
typedef bool (*HashMapEqFn)(const void *a, const void *b, usize key_size, void *ctx);

typedef struct {
    void *keys;
    void *vals;
    u8 *ctrl; /* 0 empty, 1 occupied, 2 tombstone */
    usize key_stride;
    usize val_stride;
    usize length;
    usize capacity;
    usize tombstones;
    HashMapHashFn hash;
    HashMapEqFn eq;
    void *ctx;
} HashMap;

HashMap hashmap_new(usize key_stride, usize val_stride, HashMapHashFn hash, HashMapEqFn eq,
                    void *ctx);
HashMap hashmap_new_reserve(usize key_stride, usize val_stride, usize capacity, HashMapHashFn hash,
                            HashMapEqFn eq, void *ctx);

void hashmap_delete(HashMap *map);
void hashmap_clear(HashMap *map);

usize hashmap_length(const HashMap *map);
usize hashmap_capacity(const HashMap *map);
usize hashmap_key_stride(const HashMap *map);
usize hashmap_val_stride(const HashMap *map);
bool hashmap_empty(const HashMap *map);
bool hashmap_valid(const HashMap *map);

bool hashmap_reserve(HashMap *map, usize capacity);

/* Insert or overwrite. Returns false on OOM / invalid args. */
bool hashmap_put(HashMap *map, const void *key, const void *value);

/* Copy value into out if found (out may be NULL). Returns whether key exists. */
bool hashmap_get(const HashMap *map, const void *key, void *out);

bool hashmap_contains(const HashMap *map, const void *key);

/* Remove key. Copies removed value into out if non-NULL. Returns whether key existed. */
bool hashmap_remove(HashMap *map, const void *key, void *out);

/* Default helpers usable as HashMapHashFn / HashMapEqFn (ctx ignored). */
u64 hashmap_hash_bytes(const void *key, usize key_size, void *ctx);
bool hashmap_eq_bytes(const void *a, const void *b, usize key_size, void *ctx);

u64 hashmap_hash_u64(const void *key, usize key_size, void *ctx);
bool hashmap_eq_u64(const void *a, const void *b, usize key_size, void *ctx);

/* Keys are const char * (key_stride == sizeof(char *)). */
u64 hashmap_hash_cstr(const void *key, usize key_size, void *ctx);
bool hashmap_eq_cstr(const void *a, const void *b, usize key_size, void *ctx);

/* Typed helpers.
 *
 *   HASHMAP_NEW(K, V)                                     -> HashMap (bytes hash/eq)
 *   HASHMAP_PUT(K, V, HashMap *map, key, ...)             -> bool
 *   HASHMAP_GET(K, V, const HashMap *map, key, V *out)    -> bool
 *   HASHMAP_CONTAINS(K, HashMap *map, key)                -> bool
 *   HASHMAP_REMOVE(K, V, HashMap *map, key, V *out)       -> bool
 *
 * key / value accept compound-literal initializers like vec macros.
 */

#define HASHMAP_NEW(K, V)                                                                          \
    hashmap_new(sizeof(K), sizeof(V), hashmap_hash_bytes, hashmap_eq_bytes, NULL)

#define HASHMAP_PUT(K, V, map, key, ...)                                                           \
    (hashmap_put((map), &(K){key}, &(V){__VA_ARGS__}))

#define HASHMAP_GET(K, V, map, key, out) (hashmap_get((map), &(K){key}, (out)))

#define HASHMAP_CONTAINS(K, map, key) (hashmap_contains((map), &(K){key}))

#define HASHMAP_REMOVE(K, V, map, key, out) (hashmap_remove((map), &(K){key}, (out)))

#ifdef __cplusplus
}
#endif

#endif /* HASHMAP_H */
