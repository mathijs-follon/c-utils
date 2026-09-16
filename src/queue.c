#include "queue.h"

#include <string.h>

static usize queue_logical_length(const Queue *queue) {
    if (!queue || queue->data.length < queue->head)
        return 0;
    return queue->data.length - queue->head;
}

static void queue_compact(Queue *queue) {
    if (!queue || queue->head == 0)
        return;

    usize len = queue_logical_length(queue);
    if (len == 0) {
        vec_clear(&queue->data);
        queue->head = 0;
        return;
    }

    if (queue->data.data && queue->data.stride) {
        u8 *base = (u8 *)queue->data.data;
        memmove(base, base + queue->head * queue->data.stride, len * queue->data.stride);
    }

    queue->data.length = len;
    queue->head = 0;
}

static bool queue_maybe_compact(Queue *queue) {
    if (!queue)
        return false;

    usize len = queue_logical_length(queue);
    if (len == 0) {
        vec_clear(&queue->data);
        queue->head = 0;
        return true;
    }

    /* Compact when wasted front space dominates live elements. */
    if (queue->head >= 16 && queue->head >= len)
        queue_compact(queue);

    return true;
}

Queue queue_new(usize stride) { return (Queue){vec_new(stride), 0}; }

Queue queue_new_reserve(usize stride, usize capacity) {
    return (Queue){vec_new_reserve(stride, capacity), 0};
}

Queue queue_new_copy(const Queue *other) {
    if (!other || !queue_valid(other))
        return QUEUE_NULL;

    usize len = queue_logical_length(other);
    if (len == 0)
        return queue_new(other->data.stride);

    const void *src = (const u8 *)other->data.data + other->head * other->data.stride;
    return (Queue){vec_new_from(other->data.stride, src, len), 0};
}

void queue_delete(Queue *queue) {
    if (!queue)
        return;
    vec_delete(&queue->data);
    queue->head = 0;
}

usize queue_length(const Queue *queue) { return queue_logical_length(queue); }

usize queue_capacity(const Queue *queue) {
    if (!queue)
        return 0;
    usize cap = vec_capacity(&queue->data);
    return cap > queue->head ? cap - queue->head : 0;
}

usize queue_stride(const Queue *queue) { return queue ? vec_stride(&queue->data) : 0; }

bool queue_empty(const Queue *queue) { return queue_length(queue) == 0; }

bool queue_valid(const Queue *queue) { return queue && vec_valid(&queue->data); }

bool queue_reserve(Queue *queue, usize capacity) {
    if (!queue)
        return false;

    queue_compact(queue);
    return vec_reserve(&queue->data, capacity);
}

bool queue_shrink_to_fit(Queue *queue) {
    if (!queue)
        return false;

    queue_compact(queue);
    return vec_shrink_to_fit(&queue->data);
}

void queue_clear(Queue *queue) {
    if (!queue)
        return;
    vec_clear(&queue->data);
    queue->head = 0;
}

bool queue_push(Queue *queue, const void *element) {
    if (!queue)
        return false;
    return vec_push(&queue->data, element);
}

bool queue_pop(Queue *queue, void *out) {
    if (!queue || queue_empty(queue))
        return false;

    if (out) {
        const void *src = (const u8 *)queue->data.data + queue->head * queue->data.stride;
        memcpy(out, src, queue->data.stride);
    }

    queue->head++;
    queue_maybe_compact(queue);
    return true;
}

bool queue_peek(const Queue *queue, void *out) {
    const void *src = queue_front_const(queue);
    if (!src || !out)
        return false;
    memcpy(out, src, queue->data.stride);
    return true;
}

void *queue_front(Queue *queue) {
    if (!queue || queue_empty(queue))
        return NULL;
    return (u8 *)queue->data.data + queue->head * queue->data.stride;
}

const void *queue_front_const(const Queue *queue) {
    if (!queue || queue_empty(queue))
        return NULL;
    return (const u8 *)queue->data.data + queue->head * queue->data.stride;
}

void *queue_back(Queue *queue) {
    if (!queue || queue_empty(queue))
        return NULL;
    return vec_last(&queue->data);
}

const void *queue_back_const(const Queue *queue) {
    if (!queue || queue_empty(queue))
        return NULL;
    return vec_last_const(&queue->data);
}

void queue_swap(Queue *a, Queue *b) {
    if (!a || !b || a == b)
        return;

    Queue tmp = *a;
    *a = *b;
    *b = tmp;
}

bool queue_assign(Queue *queue, const Queue *other) {
    if (!queue || !other || !queue_valid(other))
        return false;
    if (queue == other)
        return true;

    Queue copy = queue_new_copy(other);
    if (!queue_valid(&copy) && !queue_empty(other))
        return false;

    queue_delete(queue);
    *queue = copy;
    return true;
}
