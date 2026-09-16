#ifndef VEC_H
#define VEC_H

#include "types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Growable vector.
 *
 *     Vec v = VEC_NEW(int);
 *     VEC_PUSH(int, &v, 1);
 *     int i;
 *     VEC_POP(int, &v, &i);
 *     vec_delete(&v);
 */

#define VEC_NULL ((Vec){NULL, 0, 0, 0})
#define VEC_NOT_FOUND ((usize) - 1)

typedef struct {
  void *data;
  usize stride;
  usize length;
  usize capacity;
} Vec;

Vec vec_new(usize stride);
Vec vec_new_reserve(usize stride, usize capacity);
Vec vec_new_copy(const Vec *other);
Vec vec_new_from(usize stride, const void *elements, usize count);

void vec_delete(Vec *vec);

usize vec_length(const Vec *vec);
usize vec_capacity(const Vec *vec);
usize vec_stride(const Vec *vec);
usize vec_size_bytes(const Vec *vec);
bool vec_empty(const Vec *vec);
bool vec_valid(const Vec *vec);

void *vec_at(Vec *vec, usize index);
const void *vec_at_const(const Vec *vec, usize index);

void *vec_data(Vec *vec);
const void *vec_data_const(const Vec *vec);

void *vec_first(Vec *vec);
const void *vec_first_const(const Vec *vec);
void *vec_last(Vec *vec);
const void *vec_last_const(const Vec *vec);

bool vec_get(const Vec *vec, usize index, void *out);
bool vec_set(Vec *vec, usize index, const void *element);

bool vec_reserve(Vec *vec, usize capacity);
bool vec_resize(Vec *vec, usize length, const void *fill);
bool vec_shrink_to_fit(Vec *vec);
void vec_clear(Vec *vec);

bool vec_push(Vec *vec, const void *element);
bool vec_push_other(Vec *vec, const Vec *other);
bool vec_insert(Vec *vec, usize index, const void *element);
bool vec_insert_range(Vec *vec, usize index, const void *elements, usize count);

bool vec_pop(Vec *vec, void *out);
bool vec_remove_at(Vec *vec, usize index, void *out);
bool vec_remove_range(Vec *vec, usize index, usize count);

bool vec_remove(Vec *vec, const void *element);
bool vec_remove_all(Vec *vec, const void *element);

usize vec_find(const Vec *vec, const void *element);
usize vec_rfind(const Vec *vec, const void *element);
bool vec_contains(const Vec *vec, const void *element);
bool vec_contains_other(const Vec *vec, const Vec *other);

void vec_swap(Vec *a, Vec *b);
bool vec_swap_at(Vec *vec, usize i, usize j);
void vec_reverse(Vec *vec);

bool vec_assign(Vec *vec, const Vec *other);

/* Typed helpers. T is the element type.
 *
 *   VEC_NEW(T)                                            -> Vec
 *   VEC_NEW_RESERVE(T, usize capacity)                    -> Vec
 *
 *   VEC_AT(T, Vec *vec, usize index)                      -> T
 *   VEC_AT_CONST(T, const Vec *vec, usize index)          -> const T
 *   VEC_PTR(T, Vec *vec, usize index)                     -> T *
 *   VEC_PTR_CONST(T, const Vec *vec, usize index)         -> const T *
 *   VEC_DATA(T, Vec *vec)                                 -> T *
 *   VEC_DATA_CONST(T, const Vec *vec)                     -> const T *
 *   VEC_FIRST(T, Vec *vec)                                -> T
 *   VEC_LAST(T, Vec *vec)                                 -> T
 *
 *   VEC_PUSH(T, Vec *vec, T element)                      -> bool
 *   VEC_INSERT(T, Vec *vec, usize index, T element)       -> bool
 *   VEC_SET(T, Vec *vec, usize index, T element)          -> bool
 *   VEC_GET(T, const Vec *vec, usize index, T *out)       -> bool
 *   VEC_POP(T, Vec *vec, T *out)                          -> bool
 *
 *   VEC_FIND(T, const Vec *vec, T element)                -> usize
 *   VEC_CONTAINS(T, const Vec *vec, T element)            -> bool
 *   VEC_REMOVE(T, Vec *vec, T element)                    -> bool
 *
 * element args accept a compound-literal initializer, e.g. VEC_PUSH(int, &v, 1)
 * or VEC_PUSH(Point, &v, .x = 1, .y = 2).
 */

#define VEC_NEW(T) vec_new(sizeof(T))
#define VEC_NEW_RESERVE(T, capacity) vec_new_reserve(sizeof(T), (capacity))

#define VEC_AT(T, vec, index) (*(T *)vec_at((vec), (index)))
#define VEC_AT_CONST(T, vec, index) (*(const T *)vec_at_const((vec), (index)))

#define VEC_PTR(T, vec, index) ((T *)vec_at((vec), (index)))
#define VEC_PTR_CONST(T, vec, index) ((const T *)vec_at_const((vec), (index)))

#define VEC_DATA(T, vec) ((T *)vec_data(vec))
#define VEC_DATA_CONST(T, vec) ((const T *)vec_data_const(vec))

#define VEC_FIRST(T, vec) (*(T *)vec_first(vec))
#define VEC_LAST(T, vec) (*(T *)vec_last(vec))

#define VEC_PUSH(T, vec, ...) (vec_push((vec), &(T){__VA_ARGS__}))
#define VEC_INSERT(T, vec, index, ...)                                         \
  (vec_insert((vec), (index), &(T){__VA_ARGS__}))
#define VEC_SET(T, vec, index, ...) (vec_set((vec), (index), &(T){__VA_ARGS__}))

#define VEC_GET(T, vec, index, out) (vec_get((vec), (index), (out)))
#define VEC_POP(T, vec, out) (vec_pop((vec), (out)))

#define VEC_FIND(T, vec, ...) (vec_find((vec), &(T){__VA_ARGS__}))
#define VEC_CONTAINS(T, vec, ...) (vec_contains((vec), &(T){__VA_ARGS__}))
#define VEC_REMOVE(T, vec, ...) (vec_remove((vec), &(T){__VA_ARGS__}))

#ifdef __cplusplus
}
#endif

#endif /* VEC_H */
