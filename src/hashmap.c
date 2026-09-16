#include "c_utils/hashmap.h"

#include "c_utils/alloc.h"

#include <stdint.h>
#include <string.h>

enum {
    HASHMAP_CTRL_EMPTY = 0,
    HASHMAP_CTRL_FULL = 1,
    HASHMAP_CTRL_TOMB = 2,
};

#define HASHMAP_LOAD_NUM 3
#define HASHMAP_LOAD_DEN 4
#define HASHMAP_MIN_CAP 8

static bool mul_overflow(usize a, usize b, usize *out) {
    if (a != 0 && b > SIZE_MAX / a)
        return true;
    *out = a * b;
    return false;
}

static usize next_pow2(usize n) {
    usize cap = HASHMAP_MIN_CAP;
    while (cap < n) {
        if (cap > SIZE_MAX / 2)
            return n;
        cap *= 2;
    }
    return cap;
}

static u64 mix64(u64 x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ull;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebull;
    x ^= x >> 31;
    return x;
}

static void *key_at(HashMap *map, usize i) {
    return (u8 *)map->keys + i * map->key_stride;
}

static const void *key_at_c(const HashMap *map, usize i) {
    return (const u8 *)map->keys + i * map->key_stride;
}

static void *val_at(HashMap *map, usize i) {
    if (!map->val_stride || !map->vals)
        return NULL;
    return (u8 *)map->vals + i * map->val_stride;
}

static const void *val_at_c(const HashMap *map, usize i) {
    if (!map->val_stride || !map->vals)
        return NULL;
    return (const u8 *)map->vals + i * map->val_stride;
}

static void hashmap_free_buffers(HashMap *map) {
    utils_free(map->keys);
    utils_free(map->vals);
    utils_free(map->ctrl);
    map->keys = NULL;
    map->vals = NULL;
    map->ctrl = NULL;
    map->capacity = 0;
    map->length = 0;
    map->tombstones = 0;
}

static bool hashmap_alloc_buffers(HashMap *map, usize capacity) {
    usize key_bytes = 0;
    usize val_bytes = 0;

    if (mul_overflow(capacity, map->key_stride, &key_bytes))
        return false;
    if (map->val_stride && mul_overflow(capacity, map->val_stride, &val_bytes))
        return false;

    void *keys = utils_malloc(key_bytes);
    void *vals = NULL;
    u8 *ctrl = utils_calloc(capacity, 1);

    if (!keys || !ctrl) {
        utils_free(keys);
        utils_free(ctrl);
        return false;
    }

    if (map->val_stride) {
        vals = utils_malloc(val_bytes);
        if (!vals) {
            utils_free(keys);
            utils_free(ctrl);
            return false;
        }
    }

    map->keys = keys;
    map->vals = vals;
    map->ctrl = ctrl;
    map->capacity = capacity;
    map->length = 0;
    map->tombstones = 0;
    return true;
}

static isize hashmap_find_slot(const HashMap *map, const void *key, bool *found) {
    if (found)
        *found = false;
    if (!map || !map->capacity || !map->hash || !map->eq || !key)
        return -1;

    u64 h = map->hash(key, map->key_stride, map->ctx);
    usize mask = map->capacity - 1;
    usize idx = (usize)h & mask;
    isize first_tomb = -1;

    for (usize n = 0; n < map->capacity; n++) {
        u8 c = map->ctrl[idx];
        if (c == HASHMAP_CTRL_EMPTY) {
            if (found)
                *found = false;
            return first_tomb >= 0 ? first_tomb : (isize)idx;
        }
        if (c == HASHMAP_CTRL_TOMB) {
            if (first_tomb < 0)
                first_tomb = (isize)idx;
        } else if (map->eq(key_at_c(map, idx), key, map->key_stride, map->ctx)) {
            if (found)
                *found = true;
            return (isize)idx;
        }
        idx = (idx + 1) & mask;
    }

    return first_tomb;
}

static bool hashmap_rehash(HashMap *map, usize new_cap) {
    if (!map || new_cap < HASHMAP_MIN_CAP)
        return false;
    new_cap = next_pow2(new_cap);

    HashMap fresh = *map;
    fresh.keys = NULL;
    fresh.vals = NULL;
    fresh.ctrl = NULL;
    fresh.capacity = 0;
    fresh.length = 0;
    fresh.tombstones = 0;

    if (!hashmap_alloc_buffers(&fresh, new_cap))
        return false;

    for (usize i = 0; i < map->capacity; i++) {
        if (map->ctrl[i] != HASHMAP_CTRL_FULL)
            continue;
        if (!hashmap_put(&fresh, key_at(map, i), val_at(map, i))) {
            hashmap_free_buffers(&fresh);
            return false;
        }
    }

    hashmap_free_buffers(map);
    map->keys = fresh.keys;
    map->vals = fresh.vals;
    map->ctrl = fresh.ctrl;
    map->capacity = fresh.capacity;
    map->length = fresh.length;
    map->tombstones = fresh.tombstones;
    return true;
}

static bool hashmap_ensure_insert(HashMap *map) {
    usize used = map->length + map->tombstones;
    usize limit = (map->capacity * HASHMAP_LOAD_NUM) / HASHMAP_LOAD_DEN;

    if (map->capacity == 0)
        return hashmap_rehash(map, HASHMAP_MIN_CAP);

    if (used + 1 > limit) {
        usize grow = map->capacity > SIZE_MAX / 2 ? map->capacity : map->capacity * 2;
        /* If tombstones dominate, rehash at same size to reclaim slots. */
        if (map->tombstones > map->length && map->capacity >= HASHMAP_MIN_CAP)
            grow = map->capacity;
        return hashmap_rehash(map, grow);
    }
    return true;
}

HashMap hashmap_new(usize key_stride, usize val_stride, HashMapHashFn hash, HashMapEqFn eq,
                    void *ctx) {
    if (!key_stride || !hash || !eq)
        return HASHMAP_NULL;
    return (HashMap){NULL, NULL, NULL, key_stride, val_stride, 0, 0, 0, hash, eq, ctx};
}

HashMap hashmap_new_reserve(usize key_stride, usize val_stride, usize capacity, HashMapHashFn hash,
                            HashMapEqFn eq, void *ctx) {
    HashMap map = hashmap_new(key_stride, val_stride, hash, eq, ctx);
    if (!hashmap_valid(&map))
        return HASHMAP_NULL;
    if (capacity && !hashmap_reserve(&map, capacity))
        return HASHMAP_NULL;
    return map;
}

void hashmap_delete(HashMap *map) {
    if (!map)
        return;
    hashmap_free_buffers(map);
    map->hash = NULL;
    map->eq = NULL;
    map->ctx = NULL;
    map->key_stride = 0;
    map->val_stride = 0;
}

void hashmap_clear(HashMap *map) {
    if (!map || !map->ctrl || !map->capacity)
        return;
    memset(map->ctrl, 0, map->capacity);
    map->length = 0;
    map->tombstones = 0;
}

usize hashmap_length(const HashMap *map) { return map ? map->length : 0; }

usize hashmap_capacity(const HashMap *map) { return map ? map->capacity : 0; }

usize hashmap_key_stride(const HashMap *map) { return map ? map->key_stride : 0; }

usize hashmap_val_stride(const HashMap *map) { return map ? map->val_stride : 0; }

bool hashmap_empty(const HashMap *map) { return hashmap_length(map) == 0; }

bool hashmap_valid(const HashMap *map) {
    return map && map->key_stride && map->hash && map->eq;
}

bool hashmap_reserve(HashMap *map, usize capacity) {
    if (!hashmap_valid(map))
        return false;
    if (capacity == 0)
        return true;

    usize need = next_pow2(capacity);
    /* Ensure load factor headroom for `capacity` live entries. */
    usize min_cap = next_pow2((capacity * HASHMAP_LOAD_DEN + HASHMAP_LOAD_NUM - 1) / HASHMAP_LOAD_NUM);
    if (min_cap > need)
        need = min_cap;
    if (need < HASHMAP_MIN_CAP)
        need = HASHMAP_MIN_CAP;

    if (map->capacity >= need)
        return true;
    return hashmap_rehash(map, need);
}

bool hashmap_put(HashMap *map, const void *key, const void *value) {
    if (!hashmap_valid(map) || !key)
        return false;
    if (map->val_stride && !value)
        return false;

    if (!hashmap_ensure_insert(map))
        return false;

    bool found = false;
    isize slot = hashmap_find_slot(map, key, &found);
    if (slot < 0)
        return false;

    usize idx = (usize)slot;
    if (!found) {
        if (map->ctrl[idx] == HASHMAP_CTRL_TOMB) {
            if (map->tombstones)
                map->tombstones--;
        }
        map->ctrl[idx] = HASHMAP_CTRL_FULL;
        memcpy(key_at(map, idx), key, map->key_stride);
        map->length++;
    }

    if (map->val_stride)
        memcpy(val_at(map, idx), value, map->val_stride);
    return true;
}

bool hashmap_get(const HashMap *map, const void *key, void *out) {
    if (!hashmap_valid(map) || !key || !map->capacity)
        return false;

    bool found = false;
    isize slot = hashmap_find_slot(map, key, &found);
    if (!found || slot < 0)
        return false;

    if (out && map->val_stride)
        memcpy(out, val_at_c(map, (usize)slot), map->val_stride);
    return true;
}

bool hashmap_contains(const HashMap *map, const void *key) {
    return hashmap_get(map, key, NULL);
}

bool hashmap_remove(HashMap *map, const void *key, void *out) {
    if (!hashmap_valid(map) || !key || !map->capacity)
        return false;

    bool found = false;
    isize slot = hashmap_find_slot(map, key, &found);
    if (!found || slot < 0)
        return false;

    usize idx = (usize)slot;
    if (out && map->val_stride)
        memcpy(out, val_at(map, idx), map->val_stride);

    map->ctrl[idx] = HASHMAP_CTRL_TOMB;
    map->length--;
    map->tombstones++;
    return true;
}

u64 hashmap_hash_bytes(const void *key, usize key_size, void *ctx) {
    (void)ctx;
    const u8 *p = (const u8 *)key;
    u64 h = 14695981039346656037ull;
    for (usize i = 0; i < key_size; i++) {
        h ^= p[i];
        h *= 1099511628211ull;
    }
    return h;
}

bool hashmap_eq_bytes(const void *a, const void *b, usize key_size, void *ctx) {
    (void)ctx;
    return memcmp(a, b, key_size) == 0;
}

u64 hashmap_hash_u64(const void *key, usize key_size, void *ctx) {
    (void)key_size;
    (void)ctx;
    u64 v;
    memcpy(&v, key, sizeof(v));
    return mix64(v);
}

bool hashmap_eq_u64(const void *a, const void *b, usize key_size, void *ctx) {
    (void)key_size;
    (void)ctx;
    u64 x, y;
    memcpy(&x, a, sizeof(x));
    memcpy(&y, b, sizeof(y));
    return x == y;
}

u64 hashmap_hash_cstr(const void *key, usize key_size, void *ctx) {
    (void)key_size;
    (void)ctx;
    const char *s = *(const char *const *)key;
    if (!s)
        return 0;
    return hashmap_hash_bytes(s, strlen(s), NULL);
}

bool hashmap_eq_cstr(const void *a, const void *b, usize key_size, void *ctx) {
    (void)key_size;
    (void)ctx;
    const char *sa = *(const char *const *)a;
    const char *sb = *(const char *const *)b;
    if (sa == sb)
        return true;
    if (!sa || !sb)
        return false;
    return strcmp(sa, sb) == 0;
}
