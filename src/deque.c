#include "deque.h"

#include "alloc.h"

#include <stdint.h>
#include <string.h>

#define DEQUE_MIN_CAP 8

static bool mul_overflow(usize a, usize b, usize *out) {
    if (a != 0 && b > SIZE_MAX / a)
        return true;
    *out = a * b;
    return false;
}

static usize phys_index(const Deque *deque, usize logical) {
    return (deque->head + logical) % deque->capacity;
}

static void *elem_ptr(Deque *deque, usize logical) {
    return (u8 *)deque->data + phys_index(deque, logical) * deque->stride;
}

static const void *elem_ptr_c(const Deque *deque, usize logical) {
    return (const u8 *)deque->data + phys_index(deque, logical) * deque->stride;
}

static bool deque_grow(Deque *deque, usize min_cap) {
    usize new_cap = deque->capacity ? deque->capacity : DEQUE_MIN_CAP;
    while (new_cap < min_cap) {
        if (new_cap > SIZE_MAX / 2) {
            new_cap = min_cap;
            break;
        }
        new_cap *= 2;
    }

    if (deque->capacity >= new_cap)
        return true;

    usize bytes;
    if (mul_overflow(new_cap, deque->stride, &bytes))
        return false;

    void *fresh = utils_malloc(bytes);
    if (!fresh)
        return false;

    if (deque->length && deque->data) {
        usize first = deque->capacity - deque->head;
        if (first > deque->length)
            first = deque->length;
        usize second = deque->length - first;

        memcpy(fresh, (const u8 *)deque->data + deque->head * deque->stride,
               first * deque->stride);
        if (second)
            memcpy((u8 *)fresh + first * deque->stride, deque->data, second * deque->stride);
    }

    utils_free(deque->data);
    deque->data = fresh;
    deque->capacity = new_cap;
    deque->head = 0;
    return true;
}

Deque deque_new(usize stride) {
    if (!stride)
        return DEQUE_NULL;
    return (Deque){NULL, stride, 0, 0, 0};
}

Deque deque_new_reserve(usize stride, usize capacity) {
    Deque d = deque_new(stride);
    if (!deque_valid(&d))
        return DEQUE_NULL;
    if (capacity && !deque_reserve(&d, capacity))
        return DEQUE_NULL;
    return d;
}

void deque_delete(Deque *deque) {
    if (!deque)
        return;
    utils_free(deque->data);
    *deque = DEQUE_NULL;
}

void deque_clear(Deque *deque) {
    if (!deque)
        return;
    deque->length = 0;
    deque->head = 0;
}

usize deque_length(const Deque *deque) { return deque ? deque->length : 0; }

usize deque_capacity(const Deque *deque) { return deque ? deque->capacity : 0; }

usize deque_stride(const Deque *deque) { return deque ? deque->stride : 0; }

bool deque_empty(const Deque *deque) { return deque_length(deque) == 0; }

bool deque_valid(const Deque *deque) { return deque && deque->stride; }

bool deque_reserve(Deque *deque, usize capacity) {
    if (!deque_valid(deque))
        return false;
    if (capacity <= deque->capacity)
        return true;
    return deque_grow(deque, capacity);
}

bool deque_push_front(Deque *deque, const void *element) {
    if (!deque_valid(deque) || !element)
        return false;
    if (deque->length == deque->capacity && !deque_grow(deque, deque->length + 1))
        return false;

    deque->head = deque->capacity ? (deque->head + deque->capacity - 1) % deque->capacity : 0;
    memcpy(elem_ptr(deque, 0), element, deque->stride);
    deque->length++;
    return true;
}

bool deque_push_back(Deque *deque, const void *element) {
    if (!deque_valid(deque) || !element)
        return false;
    if (deque->length == deque->capacity && !deque_grow(deque, deque->length + 1))
        return false;

    memcpy(elem_ptr(deque, deque->length), element, deque->stride);
    deque->length++;
    return true;
}

bool deque_pop_front(Deque *deque, void *out) {
    if (!deque || deque_empty(deque))
        return false;

    if (out)
        memcpy(out, elem_ptr(deque, 0), deque->stride);

    deque->head = (deque->head + 1) % deque->capacity;
    deque->length--;
    if (deque->length == 0)
        deque->head = 0;
    return true;
}

bool deque_pop_back(Deque *deque, void *out) {
    if (!deque || deque_empty(deque))
        return false;

    if (out)
        memcpy(out, elem_ptr(deque, deque->length - 1), deque->stride);

    deque->length--;
    if (deque->length == 0)
        deque->head = 0;
    return true;
}

void *deque_at(Deque *deque, usize index) {
    if (!deque || index >= deque->length)
        return NULL;
    return elem_ptr(deque, index);
}

const void *deque_at_const(const Deque *deque, usize index) {
    if (!deque || index >= deque->length)
        return NULL;
    return elem_ptr_c(deque, index);
}

bool deque_get(const Deque *deque, usize index, void *out) {
    const void *p = deque_at_const(deque, index);
    if (!p || !out)
        return false;
    memcpy(out, p, deque->stride);
    return true;
}

bool deque_set(Deque *deque, usize index, const void *element) {
    void *p = deque_at(deque, index);
    if (!p || !element)
        return false;
    memcpy(p, element, deque->stride);
    return true;
}

void *deque_front(Deque *deque) { return deque_at(deque, 0); }

const void *deque_front_const(const Deque *deque) { return deque_at_const(deque, 0); }

void *deque_back(Deque *deque) {
    if (!deque || deque_empty(deque))
        return NULL;
    return deque_at(deque, deque->length - 1);
}

const void *deque_back_const(const Deque *deque) {
    if (!deque || deque_empty(deque))
        return NULL;
    return deque_at_const(deque, deque->length - 1);
}
