#ifndef STACK_H
#define STACK_H

#include "c_utils/vec.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * LIFO stack backed by Vec (top = last element).
 *
 *     Stack s = STACK_NEW(int);
 *     STACK_PUSH(int, &s, 1);
 *     int v;
 *     STACK_POP(int, &s, &v);
 *     stack_delete(&s);
 */

#define STACK_NULL ((Stack){VEC_NULL})

typedef struct {
    Vec data;
} Stack;

Stack stack_new(usize stride);
Stack stack_new_reserve(usize stride, usize capacity);
Stack stack_new_copy(const Stack *other);

void stack_delete(Stack *stack);

usize stack_length(const Stack *stack);
usize stack_capacity(const Stack *stack);
usize stack_stride(const Stack *stack);
bool stack_empty(const Stack *stack);
bool stack_valid(const Stack *stack);

bool stack_reserve(Stack *stack, usize capacity);
bool stack_shrink_to_fit(Stack *stack);
void stack_clear(Stack *stack);

bool stack_push(Stack *stack, const void *element);
bool stack_pop(Stack *stack, void *out);
bool stack_peek(const Stack *stack, void *out);

void *stack_top(Stack *stack);
const void *stack_top_const(const Stack *stack);

void stack_swap(Stack *a, Stack *b);
bool stack_assign(Stack *stack, const Stack *other);

/* Typed helpers. T is the element type.
 *
 *   STACK_NEW(T)                                          -> Stack
 *   STACK_NEW_RESERVE(T, usize capacity)                  -> Stack
 *   STACK_PUSH(T, Stack *stack, T element)                -> bool
 *   STACK_POP(T, Stack *stack, T *out)                    -> bool
 *   STACK_PEEK(T, const Stack *stack, T *out)             -> bool
 *   STACK_TOP(T, Stack *stack)                            -> T
 *   STACK_TOP_CONST(T, const Stack *stack)                -> const T
 *
 * element accepts a compound-literal initializer, e.g. STACK_PUSH(int, &s, 1).
 */

#define STACK_NEW(T) stack_new(sizeof(T))
#define STACK_NEW_RESERVE(T, capacity) stack_new_reserve(sizeof(T), (capacity))

#define STACK_PUSH(T, stack, ...) (stack_push((stack), &(T){__VA_ARGS__}))
#define STACK_POP(T, stack, out) (stack_pop((stack), (out)))
#define STACK_PEEK(T, stack, out) (stack_peek((stack), (out)))
#define STACK_TOP(T, stack) (*(T *)stack_top(stack))
#define STACK_TOP_CONST(T, stack) (*(const T *)stack_top_const(stack))

#ifdef __cplusplus
}
#endif

#endif /* STACK_H */
