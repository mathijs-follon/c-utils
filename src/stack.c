#include "stack.h"

Stack stack_new(usize stride) { return (Stack){vec_new(stride)}; }

Stack stack_new_reserve(usize stride, usize capacity) {
    return (Stack){vec_new_reserve(stride, capacity)};
}

Stack stack_new_copy(const Stack *other) {
    if (!other)
        return STACK_NULL;
    return (Stack){vec_new_copy(&other->data)};
}

void stack_delete(Stack *stack) {
    if (!stack)
        return;
    vec_delete(&stack->data);
}

usize stack_length(const Stack *stack) { return stack ? vec_length(&stack->data) : 0; }

usize stack_capacity(const Stack *stack) { return stack ? vec_capacity(&stack->data) : 0; }

usize stack_stride(const Stack *stack) { return stack ? vec_stride(&stack->data) : 0; }

bool stack_empty(const Stack *stack) { return stack_length(stack) == 0; }

bool stack_valid(const Stack *stack) { return stack && vec_valid(&stack->data); }

bool stack_reserve(Stack *stack, usize capacity) {
    if (!stack)
        return false;
    return vec_reserve(&stack->data, capacity);
}

bool stack_shrink_to_fit(Stack *stack) {
    if (!stack)
        return false;
    return vec_shrink_to_fit(&stack->data);
}

void stack_clear(Stack *stack) {
    if (!stack)
        return;
    vec_clear(&stack->data);
}

bool stack_push(Stack *stack, const void *element) {
    if (!stack)
        return false;
    return vec_push(&stack->data, element);
}

bool stack_pop(Stack *stack, void *out) {
    if (!stack)
        return false;
    return vec_pop(&stack->data, out);
}

bool stack_peek(const Stack *stack, void *out) {
    if (!stack || !out || stack_empty(stack))
        return false;
    return vec_get(&stack->data, stack->data.length - 1, out);
}

void *stack_top(Stack *stack) {
    if (!stack)
        return NULL;
    return vec_last(&stack->data);
}

const void *stack_top_const(const Stack *stack) {
    if (!stack)
        return NULL;
    return vec_last_const(&stack->data);
}

void stack_swap(Stack *a, Stack *b) {
    if (!a || !b || a == b)
        return;
    vec_swap(&a->data, &b->data);
}

bool stack_assign(Stack *stack, const Stack *other) {
    if (!stack || !other)
        return false;
    return vec_assign(&stack->data, &other->data);
}
