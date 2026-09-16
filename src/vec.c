#include "c_utils/vec.h"

#include "dynamic_array.h"

#include <stddef.h>

_Static_assert(sizeof(Vec) == sizeof(DynArray), "Vec must match DynArray layout");
_Static_assert(offsetof(Vec, data) == offsetof(DynArray, data), "Vec.data mismatch");
_Static_assert(offsetof(Vec, stride) == offsetof(DynArray, stride), "Vec.stride mismatch");
_Static_assert(offsetof(Vec, length) == offsetof(DynArray, length), "Vec.length mismatch");
_Static_assert(offsetof(Vec, capacity) == offsetof(DynArray, capacity), "Vec.capacity mismatch");

static DynArray *arr(Vec *vec) { return (DynArray *)vec; }

static const DynArray *arr_c(const Vec *vec) { return (const DynArray *)vec; }

static Vec from_arr(DynArray a) {
    Vec v;
    __builtin_memcpy(&v, &a, sizeof(v));
    return v;
}

Vec vec_new(usize stride) { return from_arr(dyn_arr_new(stride)); }

Vec vec_new_reserve(usize stride, usize capacity) {
    return from_arr(dyn_arr_new_reserve(stride, capacity));
}

Vec vec_new_copy(const Vec *other) {
    if (!other)
        return VEC_NULL;
    return from_arr(dyn_arr_new_copy(arr_c(other)));
}

Vec vec_new_from(usize stride, const void *elements, usize count) {
    return from_arr(dyn_arr_new_from(stride, elements, count));
}

void vec_delete(Vec *vec) {
    if (!vec)
        return;
    dyn_arr_delete(arr(vec));
}

usize vec_length(const Vec *vec) { return vec ? dyn_arr_length(arr_c(vec)) : 0; }

usize vec_capacity(const Vec *vec) { return vec ? dyn_arr_capacity(arr_c(vec)) : 0; }

usize vec_stride(const Vec *vec) { return vec ? dyn_arr_stride(arr_c(vec)) : 0; }

usize vec_size_bytes(const Vec *vec) { return vec ? dyn_arr_size_bytes(arr_c(vec)) : 0; }

bool vec_empty(const Vec *vec) { return vec_length(vec) == 0; }

bool vec_valid(const Vec *vec) { return vec && dyn_arr_valid(arr_c(vec)); }

void *vec_at(Vec *vec, usize index) { return vec ? dyn_arr_at(arr(vec), index) : NULL; }

const void *vec_at_const(const Vec *vec, usize index) {
    return vec ? dyn_arr_at_const(arr_c(vec), index) : NULL;
}

void *vec_data(Vec *vec) { return vec ? dyn_arr_data(arr(vec)) : NULL; }

const void *vec_data_const(const Vec *vec) { return vec ? dyn_arr_data_const(arr_c(vec)) : NULL; }

void *vec_first(Vec *vec) { return vec ? dyn_arr_first(arr(vec)) : NULL; }

const void *vec_first_const(const Vec *vec) { return vec ? dyn_arr_first_const(arr_c(vec)) : NULL; }

void *vec_last(Vec *vec) { return vec ? dyn_arr_last(arr(vec)) : NULL; }

const void *vec_last_const(const Vec *vec) { return vec ? dyn_arr_last_const(arr_c(vec)) : NULL; }

bool vec_get(const Vec *vec, usize index, void *out) {
    if (!vec)
        return false;
    return dyn_arr_get(arr_c(vec), index, out);
}

bool vec_set(Vec *vec, usize index, const void *element) {
    if (!vec)
        return false;
    return dyn_arr_set(arr(vec), index, element);
}

bool vec_reserve(Vec *vec, usize capacity) {
    if (!vec)
        return false;
    return dyn_arr_reserve(arr(vec), capacity);
}

bool vec_resize(Vec *vec, usize length, const void *fill) {
    if (!vec)
        return false;
    return dyn_arr_resize(arr(vec), length, fill);
}

bool vec_shrink_to_fit(Vec *vec) {
    if (!vec)
        return false;
    return dyn_arr_shrink_to_fit(arr(vec));
}

void vec_clear(Vec *vec) {
    if (!vec)
        return;
    dyn_arr_clear(arr(vec));
}

bool vec_push(Vec *vec, const void *element) {
    if (!vec)
        return false;
    return dyn_arr_push(arr(vec), element);
}

bool vec_push_other(Vec *vec, const Vec *other) {
    if (!vec || !other)
        return false;
    return dyn_arr_push_other(arr(vec), arr_c(other));
}

bool vec_insert(Vec *vec, usize index, const void *element) {
    if (!vec)
        return false;
    return dyn_arr_insert(arr(vec), index, element);
}

bool vec_insert_range(Vec *vec, usize index, const void *elements, usize count) {
    if (!vec)
        return false;
    return dyn_arr_insert_range(arr(vec), index, elements, count);
}

bool vec_pop(Vec *vec, void *out) {
    if (!vec)
        return false;
    return dyn_arr_pop(arr(vec), out);
}

bool vec_remove_at(Vec *vec, usize index, void *out) {
    if (!vec)
        return false;
    return dyn_arr_remove_at(arr(vec), index, out);
}

bool vec_remove_range(Vec *vec, usize index, usize count) {
    if (!vec)
        return false;
    return dyn_arr_remove_range(arr(vec), index, count);
}

bool vec_remove(Vec *vec, const void *element) {
    if (!vec)
        return false;
    return dyn_arr_remove(arr(vec), element);
}

bool vec_remove_all(Vec *vec, const void *element) {
    if (!vec)
        return false;
    return dyn_arr_remove_all(arr(vec), element);
}

usize vec_find(const Vec *vec, const void *element) {
    if (!vec)
        return VEC_NOT_FOUND;
    return dyn_arr_find(arr_c(vec), element);
}

usize vec_rfind(const Vec *vec, const void *element) {
    if (!vec)
        return VEC_NOT_FOUND;
    return dyn_arr_rfind(arr_c(vec), element);
}

bool vec_contains(const Vec *vec, const void *element) {
    if (!vec)
        return false;
    return dyn_arr_contains(arr_c(vec), element);
}

bool vec_contains_other(const Vec *vec, const Vec *other) {
    if (!vec || !other)
        return false;
    return dyn_arr_contains_other(arr_c(vec), arr_c(other));
}

void vec_swap(Vec *a, Vec *b) {
    if (!a || !b || a == b)
        return;
    dyn_arr_swap(arr(a), arr(b));
}

bool vec_swap_at(Vec *vec, usize i, usize j) {
    if (!vec)
        return false;
    return dyn_arr_swap_at(arr(vec), i, j);
}

void vec_reverse(Vec *vec) {
    if (!vec)
        return;
    dyn_arr_reverse(arr(vec));
}

bool vec_assign(Vec *vec, const Vec *other) {
    if (!vec || !other)
        return false;
    return dyn_arr_assign(arr(vec), arr_c(other));
}
