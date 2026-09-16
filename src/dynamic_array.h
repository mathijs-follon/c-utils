#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include "c_utils/types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Internal growable buffer used by Vec. Not part of the public API. */

#define DYN_ARR_NULL ((DynArray){NULL, 0, 0, 0})
#define DYN_ARR_NOT_FOUND ((usize) - 1)

typedef struct {
    void *data;
    usize stride;
    usize length;
    usize capacity;
} DynArray;

DynArray dyn_arr_new(usize stride);
DynArray dyn_arr_new_reserve(usize stride, usize capacity);
DynArray dyn_arr_new_copy(const DynArray *other);
DynArray dyn_arr_new_from(usize stride, const void *elements, usize count);

void dyn_arr_delete(DynArray *arr);

usize dyn_arr_length(const DynArray *arr);
usize dyn_arr_capacity(const DynArray *arr);
usize dyn_arr_stride(const DynArray *arr);
usize dyn_arr_size_bytes(const DynArray *arr);
bool dyn_arr_empty(const DynArray *arr);
bool dyn_arr_valid(const DynArray *arr);

void *dyn_arr_at(DynArray *arr, usize index);
const void *dyn_arr_at_const(const DynArray *arr, usize index);

void *dyn_arr_data(DynArray *arr);
const void *dyn_arr_data_const(const DynArray *arr);

void *dyn_arr_first(DynArray *arr);
const void *dyn_arr_first_const(const DynArray *arr);
void *dyn_arr_last(DynArray *arr);
const void *dyn_arr_last_const(const DynArray *arr);

bool dyn_arr_get(const DynArray *arr, usize index, void *out);
bool dyn_arr_set(DynArray *arr, usize index, const void *element);

bool dyn_arr_reserve(DynArray *arr, usize capacity);
bool dyn_arr_resize(DynArray *arr, usize length, const void *fill);
bool dyn_arr_shrink_to_fit(DynArray *arr);
void dyn_arr_clear(DynArray *arr);

bool dyn_arr_push(DynArray *arr, const void *element);
bool dyn_arr_push_other(DynArray *arr, const DynArray *other);
bool dyn_arr_insert(DynArray *arr, usize index, const void *element);
bool dyn_arr_insert_range(DynArray *arr, usize index, const void *elements, usize count);

bool dyn_arr_pop(DynArray *arr, void *out);
bool dyn_arr_remove_at(DynArray *arr, usize index, void *out);
bool dyn_arr_remove_range(DynArray *arr, usize index, usize count);

bool dyn_arr_remove(DynArray *arr, const void *element);
bool dyn_arr_remove_all(DynArray *arr, const void *element);

usize dyn_arr_find(const DynArray *arr, const void *element);
usize dyn_arr_rfind(const DynArray *arr, const void *element);
bool dyn_arr_contains(const DynArray *arr, const void *element);
bool dyn_arr_contains_other(const DynArray *arr, const DynArray *other);

void dyn_arr_swap(DynArray *a, DynArray *b);
bool dyn_arr_swap_at(DynArray *arr, usize i, usize j);
void dyn_arr_reverse(DynArray *arr);

bool dyn_arr_assign(DynArray *arr, const DynArray *other);

#ifdef __cplusplus
}
#endif

#endif /* DYNAMIC_ARRAY_H */
