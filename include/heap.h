#ifndef HEAP_H
#define HEAP_H

#include "types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Binary heap (priority queue). cmp is like qsort_r:
 *   < 0 if a should be ordered before b (higher priority for min-heap style),
 *   0 if equal, > 0 if a after b.
 *
 * Default usage (min-heap of int):
 *
 *     static int cmp_int(const void *a, const void *b, void *ctx) {
 *         (void)ctx;
 *         int x = *(const int *)a, y = *(const int *)b;
 *         return (x > y) - (x < y);
 *     }
 *     Heap h = heap_new(sizeof(int), cmp_int, NULL);
 *     HEAP_PUSH(int, &h, 3);
 *     int v;
 *     HEAP_POP(int, &h, &v);
 *     heap_delete(&h);
 */

#define HEAP_NULL ((Heap){NULL, 0, 0, 0, NULL, NULL})

typedef int (*HeapCmpFn)(const void *a, const void *b, void *ctx);

typedef struct {
    void *data;
    usize stride;
    usize length;
    usize capacity;
    HeapCmpFn cmp;
    void *ctx;
} Heap;

Heap heap_new(usize stride, HeapCmpFn cmp, void *ctx);
Heap heap_new_reserve(usize stride, usize capacity, HeapCmpFn cmp, void *ctx);

void heap_delete(Heap *heap);
void heap_clear(Heap *heap);

usize heap_length(const Heap *heap);
usize heap_capacity(const Heap *heap);
usize heap_stride(const Heap *heap);
bool heap_empty(const Heap *heap);
bool heap_valid(const Heap *heap);

bool heap_reserve(Heap *heap, usize capacity);

bool heap_push(Heap *heap, const void *element);
bool heap_pop(Heap *heap, void *out);
bool heap_peek(const Heap *heap, void *out);

const void *heap_top_const(const Heap *heap);
void *heap_top(Heap *heap);

/* Typed helpers.
 *
 *   HEAP_NEW(T, cmp, ctx)                                 -> Heap
 *   HEAP_PUSH(T, Heap *h, T element)                      -> bool
 *   HEAP_POP(T, Heap *h, T *out)                          -> bool
 *   HEAP_PEEK(T, const Heap *h, T *out)                   -> bool
 */

#define HEAP_NEW(T, cmp, ctx) heap_new(sizeof(T), (cmp), (ctx))
#define HEAP_PUSH(T, heap, ...) (heap_push((heap), &(T){__VA_ARGS__}))
#define HEAP_POP(T, heap, out) (heap_pop((heap), (out)))
#define HEAP_PEEK(T, heap, out) (heap_peek((heap), (out)))

#ifdef __cplusplus
}
#endif

#endif /* HEAP_H */
