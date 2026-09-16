#include "dynamic_array.h"
#include "c_utils/alloc.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void *elem_ptr(void *data, usize stride, usize index) { return (u8 *)data + index * stride; }

static const void *elem_ptr_const(const void *data, usize stride, usize index) {
    return (const u8 *)data + index * stride;
}

static bool mul_overflow(usize a, usize b, usize *out) {
    if (a != 0 && b > SIZE_MAX / a)
        return true;
    *out = a * b;
    return false;
}

static bool add_overflow(usize a, usize b, usize *out) {
    if (a > SIZE_MAX - b)
        return true;
    *out = a + b;
    return false;
}

static bool dyn_arr_ensure(DynArray *arr, usize required) {
    if (arr->capacity >= required)
        return true;

    usize capacity = arr->capacity ? arr->capacity : 8;
    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            capacity = required;
            break;
        }
        capacity *= 2;
    }

    return dyn_arr_reserve(arr, capacity);
}

DynArray dyn_arr_new(usize stride) {
    if (stride == 0)
        return DYN_ARR_NULL;
    return (DynArray){NULL, stride, 0, 0};
}

DynArray dyn_arr_new_reserve(usize stride, usize capacity) {
    DynArray arr = dyn_arr_new(stride);
    if (!dyn_arr_valid(&arr))
        return DYN_ARR_NULL;
    if (capacity && !dyn_arr_reserve(&arr, capacity))
        return DYN_ARR_NULL;
    return arr;
}

DynArray dyn_arr_new_copy(const DynArray *other) {
    if (!other || !dyn_arr_valid(other))
        return DYN_ARR_NULL;
    return dyn_arr_new_from(other->stride, other->data, other->length);
}

DynArray dyn_arr_new_from(usize stride, const void *elements, usize count) {
    if (stride == 0 || (count && !elements))
        return DYN_ARR_NULL;

    DynArray arr = dyn_arr_new_reserve(stride, count);
    if (!dyn_arr_valid(&arr))
        return DYN_ARR_NULL;

    if (count) {
        usize bytes;
        if (mul_overflow(count, stride, &bytes)) {
            dyn_arr_delete(&arr);
            return DYN_ARR_NULL;
        }
        memcpy(arr.data, elements, bytes);
        arr.length = count;
    }

    return arr;
}

void dyn_arr_delete(DynArray *arr) {
    if (!arr)
        return;
    utils_free(arr->data);
    *arr = DYN_ARR_NULL;
}

usize dyn_arr_length(const DynArray *arr) { return arr ? arr->length : 0; }

usize dyn_arr_capacity(const DynArray *arr) { return arr ? arr->capacity : 0; }

usize dyn_arr_stride(const DynArray *arr) { return arr ? arr->stride : 0; }

usize dyn_arr_size_bytes(const DynArray *arr) {
    if (!arr || !arr->stride)
        return 0;
    usize bytes = 0;
    if (mul_overflow(arr->length, arr->stride, &bytes))
        return 0;
    return bytes;
}

bool dyn_arr_empty(const DynArray *arr) { return dyn_arr_length(arr) == 0; }

bool dyn_arr_valid(const DynArray *arr) { return arr && arr->stride != 0; }

void *dyn_arr_at(DynArray *arr, usize index) {
    if (!arr || !arr->data || index >= arr->length)
        return NULL;
    return elem_ptr(arr->data, arr->stride, index);
}

const void *dyn_arr_at_const(const DynArray *arr, usize index) {
    if (!arr || !arr->data || index >= arr->length)
        return NULL;
    return elem_ptr_const(arr->data, arr->stride, index);
}

void *dyn_arr_data(DynArray *arr) { return arr ? arr->data : NULL; }

const void *dyn_arr_data_const(const DynArray *arr) { return arr ? arr->data : NULL; }

void *dyn_arr_first(DynArray *arr) { return dyn_arr_at(arr, 0); }

const void *dyn_arr_first_const(const DynArray *arr) { return dyn_arr_at_const(arr, 0); }

void *dyn_arr_last(DynArray *arr) {
    if (!arr || arr->length == 0)
        return NULL;
    return dyn_arr_at(arr, arr->length - 1);
}

const void *dyn_arr_last_const(const DynArray *arr) {
    if (!arr || arr->length == 0)
        return NULL;
    return dyn_arr_at_const(arr, arr->length - 1);
}

bool dyn_arr_get(const DynArray *arr, usize index, void *out) {
    const void *src = dyn_arr_at_const(arr, index);
    if (!src || !out)
        return false;
    memcpy(out, src, arr->stride);
    return true;
}

bool dyn_arr_set(DynArray *arr, usize index, const void *element) {
    void *dst = dyn_arr_at(arr, index);
    if (!dst || !element)
        return false;
    memmove(dst, element, arr->stride);
    return true;
}

bool dyn_arr_reserve(DynArray *arr, usize capacity) {
    if (!dyn_arr_valid(arr) || capacity < arr->length)
        return false;
    if (arr->capacity >= capacity && (capacity == 0 || arr->data))
        return true;

    if (capacity == 0) {
        utils_free(arr->data);
        arr->data = NULL;
        arr->capacity = 0;
        return true;
    }

    usize bytes;
    if (mul_overflow(capacity, arr->stride, &bytes))
        return false;

    void *data = utils_realloc(arr->data, bytes);
    if (!data)
        return false;

    arr->data = data;
    arr->capacity = capacity;
    return true;
}

bool dyn_arr_resize(DynArray *arr, usize length, const void *fill) {
    if (!dyn_arr_valid(arr))
        return false;

    if (length > arr->length) {
        if (!dyn_arr_ensure(arr, length))
            return false;
        if (fill) {
            for (usize i = arr->length; i < length; i++)
                memcpy(elem_ptr(arr->data, arr->stride, i), fill, arr->stride);
        } else {
            usize bytes;
            if (mul_overflow(length - arr->length, arr->stride, &bytes))
                return false;
            memset(elem_ptr(arr->data, arr->stride, arr->length), 0, bytes);
        }
    }

    arr->length = length;
    return true;
}

bool dyn_arr_shrink_to_fit(DynArray *arr) {
    if (!dyn_arr_valid(arr) || arr->length > arr->capacity)
        return false;
    return dyn_arr_reserve(arr, arr->length);
}

void dyn_arr_clear(DynArray *arr) {
    if (!arr)
        return;
    arr->length = 0;
}

bool dyn_arr_push(DynArray *arr, const void *element) {
    if (!dyn_arr_valid(arr) || !element)
        return false;

    usize new_len;
    if (add_overflow(arr->length, 1, &new_len))
        return false;
    if (!dyn_arr_ensure(arr, new_len))
        return false;

    memcpy(elem_ptr(arr->data, arr->stride, arr->length), element, arr->stride);
    arr->length = new_len;
    return true;
}

bool dyn_arr_push_other(DynArray *arr, const DynArray *other) {
    if (!dyn_arr_valid(arr) || !other)
        return false;
    if (other->length == 0)
        return true;
    if (!dyn_arr_valid(other) || other->stride != arr->stride)
        return false;

    return dyn_arr_insert_range(arr, arr->length, other->data, other->length);
}

bool dyn_arr_insert(DynArray *arr, usize index, const void *element) {
    if (!element)
        return false;
    return dyn_arr_insert_range(arr, index, element, 1);
}

bool dyn_arr_insert_range(DynArray *arr, usize index, const void *elements, usize count) {
    if (!dyn_arr_valid(arr) || index > arr->length || (!elements && count))
        return false;
    if (!count)
        return true;

    usize new_len;
    if (add_overflow(arr->length, count, &new_len))
        return false;

    /* Copy source if it aliases our buffer (e.g. push_other of self). */
    void *temp = NULL;
    const void *src = elements;
    if (arr->data) {
        uintptr_t base = (uintptr_t)arr->data;
        uintptr_t end = base + arr->capacity * arr->stride;
        uintptr_t p = (uintptr_t)elements;
        if (p >= base && p < end) {
            usize bytes;
            if (mul_overflow(count, arr->stride, &bytes))
                return false;
            temp = utils_malloc(bytes);
            if (!temp)
                return false;
            memcpy(temp, elements, bytes);
            src = temp;
        }
    }

    bool ok = false;
    if (dyn_arr_ensure(arr, new_len)) {
        usize move_count = arr->length - index;
        if (move_count) {
            memmove(elem_ptr(arr->data, arr->stride, index + count),
                    elem_ptr(arr->data, arr->stride, index), move_count * arr->stride);
        }
        memcpy(elem_ptr(arr->data, arr->stride, index), src, count * arr->stride);
        arr->length = new_len;
        ok = true;
    }

    utils_free(temp);
    return ok;
}

bool dyn_arr_pop(DynArray *arr, void *out) {
    if (!dyn_arr_valid(arr) || arr->length == 0)
        return false;
    return dyn_arr_remove_at(arr, arr->length - 1, out);
}

bool dyn_arr_remove_at(DynArray *arr, usize index, void *out) {
    if (!dyn_arr_valid(arr) || index >= arr->length)
        return false;

    if (out)
        memcpy(out, elem_ptr(arr->data, arr->stride, index), arr->stride);

    usize move_count = arr->length - index - 1;
    if (move_count) {
        memmove(elem_ptr(arr->data, arr->stride, index),
                elem_ptr(arr->data, arr->stride, index + 1), move_count * arr->stride);
    }

    arr->length--;
    return true;
}

bool dyn_arr_remove_range(DynArray *arr, usize index, usize count) {
    if (!dyn_arr_valid(arr) || index > arr->length)
        return false;
    if (count > arr->length - index)
        count = arr->length - index;
    if (!count)
        return true;

    usize move_count = arr->length - index - count;
    if (move_count) {
        memmove(elem_ptr(arr->data, arr->stride, index),
                elem_ptr(arr->data, arr->stride, index + count), move_count * arr->stride);
    }

    arr->length -= count;
    return true;
}

bool dyn_arr_remove(DynArray *arr, const void *element) {
    usize index = dyn_arr_find(arr, element);
    if (index == DYN_ARR_NOT_FOUND)
        return false;
    return dyn_arr_remove_at(arr, index, NULL);
}

bool dyn_arr_remove_all(DynArray *arr, const void *element) {
    if (!dyn_arr_valid(arr) || !element)
        return false;

    usize write = 0;
    for (usize read = 0; read < arr->length; read++) {
        if (memcmp(elem_ptr(arr->data, arr->stride, read), element, arr->stride) == 0)
            continue;
        if (write != read)
            memcpy(elem_ptr(arr->data, arr->stride, write), elem_ptr(arr->data, arr->stride, read),
                   arr->stride);
        write++;
    }

    bool removed = write != arr->length;
    arr->length = write;
    return removed;
}

usize dyn_arr_find(const DynArray *arr, const void *element) {
    if (!dyn_arr_valid(arr) || !element || !arr->data)
        return DYN_ARR_NOT_FOUND;

    for (usize i = 0; i < arr->length; i++) {
        if (memcmp(elem_ptr(arr->data, arr->stride, i), element, arr->stride) == 0)
            return i;
    }
    return DYN_ARR_NOT_FOUND;
}

usize dyn_arr_rfind(const DynArray *arr, const void *element) {
    if (!dyn_arr_valid(arr) || !element || !arr->data)
        return DYN_ARR_NOT_FOUND;

    for (usize i = arr->length; i > 0; i--) {
        if (memcmp(elem_ptr_const(arr->data, arr->stride, i - 1), element, arr->stride) == 0)
            return i - 1;
    }
    return DYN_ARR_NOT_FOUND;
}

bool dyn_arr_contains(const DynArray *arr, const void *element) {
    return dyn_arr_find(arr, element) != DYN_ARR_NOT_FOUND;
}

bool dyn_arr_contains_other(const DynArray *arr, const DynArray *other) {
    if (!dyn_arr_valid(arr) || !dyn_arr_valid(other) || arr->stride != other->stride)
        return false;
    if (other->length == 0)
        return true;

    for (usize i = 0; i < other->length; i++) {
        if (dyn_arr_find(arr, elem_ptr_const(other->data, other->stride, i)) == DYN_ARR_NOT_FOUND)
            return false;
    }
    return true;
}

void dyn_arr_swap(DynArray *a, DynArray *b) {
    if (!a || !b || a == b)
        return;
    DynArray tmp = *a;
    *a = *b;
    *b = tmp;
}

bool dyn_arr_swap_at(DynArray *arr, usize i, usize j) {
    if (!dyn_arr_valid(arr) || i >= arr->length || j >= arr->length)
        return false;
    if (i == j)
        return true;

    u8 *tmp = (u8 *)utils_malloc(arr->stride);
    if (!tmp)
        return false;

    memcpy(tmp, elem_ptr(arr->data, arr->stride, i), arr->stride);
    memcpy(elem_ptr(arr->data, arr->stride, i), elem_ptr(arr->data, arr->stride, j), arr->stride);
    memcpy(elem_ptr(arr->data, arr->stride, j), tmp, arr->stride);
    utils_free(tmp);
    return true;
}

void dyn_arr_reverse(DynArray *arr) {
    if (!dyn_arr_valid(arr) || arr->length < 2)
        return;

    u8 *tmp = (u8 *)utils_malloc(arr->stride);
    if (!tmp)
        return;

    for (usize i = 0, j = arr->length - 1; i < j; i++, j--) {
        void *a = elem_ptr(arr->data, arr->stride, i);
        void *b = elem_ptr(arr->data, arr->stride, j);
        memcpy(tmp, a, arr->stride);
        memcpy(a, b, arr->stride);
        memcpy(b, tmp, arr->stride);
    }

    utils_free(tmp);
}

bool dyn_arr_assign(DynArray *arr, const DynArray *other) {
    if (!arr || !dyn_arr_valid(other))
        return false;

    if (arr == other)
        return true;

    DynArray copy = dyn_arr_new_copy(other);
    if (!dyn_arr_valid(&copy) && other->length != 0)
        return false;

    dyn_arr_delete(arr);
    *arr = copy;
    return true;
}
