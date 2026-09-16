#ifndef DEQUE_H
#define DEQUE_H

#include "c_utils/types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Ring-buffer deque (double-ended queue).
 *
 *     Deque d = DEQUE_NEW(int);
 *     DEQUE_PUSH_BACK(int, &d, 1);
 *     DEQUE_PUSH_FRONT(int, &d, 0);
 *     int v;
 *     DEQUE_POP_FRONT(int, &d, &v);
 *     deque_delete(&d);
 */

#define DEQUE_NULL ((Deque){NULL, 0, 0, 0, 0})

typedef struct {
    void *data;
    usize stride;
    usize length;
    usize capacity;
    usize head; /* index of front element in the ring */
} Deque;

Deque deque_new(usize stride);
Deque deque_new_reserve(usize stride, usize capacity);

void deque_delete(Deque *deque);
void deque_clear(Deque *deque);

usize deque_length(const Deque *deque);
usize deque_capacity(const Deque *deque);
usize deque_stride(const Deque *deque);
bool deque_empty(const Deque *deque);
bool deque_valid(const Deque *deque);

bool deque_reserve(Deque *deque, usize capacity);

bool deque_push_front(Deque *deque, const void *element);
bool deque_push_back(Deque *deque, const void *element);
bool deque_pop_front(Deque *deque, void *out);
bool deque_pop_back(Deque *deque, void *out);

void *deque_at(Deque *deque, usize index);
const void *deque_at_const(const Deque *deque, usize index);
bool deque_get(const Deque *deque, usize index, void *out);
bool deque_set(Deque *deque, usize index, const void *element);

void *deque_front(Deque *deque);
const void *deque_front_const(const Deque *deque);
void *deque_back(Deque *deque);
const void *deque_back_const(const Deque *deque);

/* Typed helpers. T is the element type.
 *
 *   DEQUE_NEW(T)                                          -> Deque
 *   DEQUE_NEW_RESERVE(T, usize capacity)                  -> Deque
 *   DEQUE_PUSH_FRONT(T, Deque *d, T element)              -> bool
 *   DEQUE_PUSH_BACK(T, Deque *d, T element)               -> bool
 *   DEQUE_POP_FRONT(T, Deque *d, T *out)                  -> bool
 *   DEQUE_POP_BACK(T, Deque *d, T *out)                   -> bool
 *   DEQUE_AT(T, Deque *d, usize index)                    -> T
 *   DEQUE_GET(T, const Deque *d, usize index, T *out)     -> bool
 *   DEQUE_SET(T, Deque *d, usize index, T element)        -> bool
 */

#define DEQUE_NEW(T) deque_new(sizeof(T))
#define DEQUE_NEW_RESERVE(T, capacity) deque_new_reserve(sizeof(T), (capacity))

#define DEQUE_PUSH_FRONT(T, deque, ...) (deque_push_front((deque), &(T){__VA_ARGS__}))
#define DEQUE_PUSH_BACK(T, deque, ...) (deque_push_back((deque), &(T){__VA_ARGS__}))
#define DEQUE_POP_FRONT(T, deque, out) (deque_pop_front((deque), (out)))
#define DEQUE_POP_BACK(T, deque, out) (deque_pop_back((deque), (out)))

#define DEQUE_AT(T, deque, index) (*(T *)deque_at((deque), (index)))
#define DEQUE_GET(T, deque, index, out) (deque_get((deque), (index), (out)))
#define DEQUE_SET(T, deque, index, ...) (deque_set((deque), (index), &(T){__VA_ARGS__}))

#ifdef __cplusplus
}
#endif

#endif /* DEQUE_H */
