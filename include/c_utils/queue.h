#ifndef QUEUE_H
#define QUEUE_H

#include "c_utils/vec.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * FIFO queue backed by Vec with a front index (amortized O(1) ops).
 *
 *     Queue q = QUEUE_NEW(int);
 *     QUEUE_PUSH(int, &q, 1);
 *     int v;
 *     QUEUE_POP(int, &q, &v);
 *     queue_delete(&q);
 */

#define QUEUE_NULL ((Queue){VEC_NULL, 0})

typedef struct {
    Vec data;
    usize head;
} Queue;

Queue queue_new(usize stride);
Queue queue_new_reserve(usize stride, usize capacity);
Queue queue_new_copy(const Queue *other);

void queue_delete(Queue *queue);

usize queue_length(const Queue *queue);
usize queue_capacity(const Queue *queue);
usize queue_stride(const Queue *queue);
bool queue_empty(const Queue *queue);
bool queue_valid(const Queue *queue);

bool queue_reserve(Queue *queue, usize capacity);
bool queue_shrink_to_fit(Queue *queue);
void queue_clear(Queue *queue);

bool queue_push(Queue *queue, const void *element);
bool queue_pop(Queue *queue, void *out);
bool queue_peek(const Queue *queue, void *out);

void *queue_front(Queue *queue);
const void *queue_front_const(const Queue *queue);
void *queue_back(Queue *queue);
const void *queue_back_const(const Queue *queue);

void queue_swap(Queue *a, Queue *b);
bool queue_assign(Queue *queue, const Queue *other);

/* Typed helpers. T is the element type.
 *
 *   QUEUE_NEW(T)                                          -> Queue
 *   QUEUE_NEW_RESERVE(T, usize capacity)                  -> Queue
 *   QUEUE_PUSH(T, Queue *queue, T element)                -> bool
 *   QUEUE_POP(T, Queue *queue, T *out)                    -> bool
 *   QUEUE_PEEK(T, const Queue *queue, T *out)             -> bool
 *   QUEUE_FRONT(T, Queue *queue)                          -> T
 *   QUEUE_FRONT_CONST(T, const Queue *queue)              -> const T
 *   QUEUE_BACK(T, Queue *queue)                           -> T
 *   QUEUE_BACK_CONST(T, const Queue *queue)               -> const T
 *
 * element accepts a compound-literal initializer, e.g. QUEUE_PUSH(int, &q, 1).
 */

#define QUEUE_NEW(T) queue_new(sizeof(T))
#define QUEUE_NEW_RESERVE(T, capacity) queue_new_reserve(sizeof(T), (capacity))

#define QUEUE_PUSH(T, queue, ...) (queue_push((queue), &(T){__VA_ARGS__}))
#define QUEUE_POP(T, queue, out) (queue_pop((queue), (out)))
#define QUEUE_PEEK(T, queue, out) (queue_peek((queue), (out)))
#define QUEUE_FRONT(T, queue) (*(T *)queue_front(queue))
#define QUEUE_FRONT_CONST(T, queue) (*(const T *)queue_front_const(queue))
#define QUEUE_BACK(T, queue) (*(T *)queue_back(queue))
#define QUEUE_BACK_CONST(T, queue) (*(const T *)queue_back_const(queue))

#ifdef __cplusplus
}
#endif

#endif /* QUEUE_H */
