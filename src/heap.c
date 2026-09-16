#include "c_utils/heap.h"

#include "c_utils/alloc.h"

#include <stdint.h>
#include <string.h>

#define HEAP_MIN_CAP 8

static bool mul_overflow(usize a, usize b, usize *out) {
    if (a != 0 && b > SIZE_MAX / a)
        return true;
    *out = a * b;
    return false;
}

static void *elem_ptr(Heap *heap, usize index) {
    return (u8 *)heap->data + index * heap->stride;
}

static const void *elem_ptr_c(const Heap *heap, usize index) {
    return (const u8 *)heap->data + index * heap->stride;
}

static void heap_swap_at(Heap *heap, usize i, usize j) {
    u8 *a = (u8 *)elem_ptr(heap, i);
    u8 *b = (u8 *)elem_ptr(heap, j);
    for (usize k = 0; k < heap->stride; k++) {
        u8 t = a[k];
        a[k] = b[k];
        b[k] = t;
    }
}

static bool heap_grow(Heap *heap, usize min_cap) {
    usize new_cap = heap->capacity ? heap->capacity : HEAP_MIN_CAP;
    while (new_cap < min_cap) {
        if (new_cap > SIZE_MAX / 2) {
            new_cap = min_cap;
            break;
        }
        new_cap *= 2;
    }

    if (heap->capacity >= new_cap)
        return true;

    usize bytes;
    if (mul_overflow(new_cap, heap->stride, &bytes))
        return false;

    void *data = utils_realloc(heap->data, bytes);
    if (!data)
        return false;

    heap->data = data;
    heap->capacity = new_cap;
    return true;
}

static void sift_up(Heap *heap, usize index) {
    while (index > 0) {
        usize parent = (index - 1) / 2;
        if (heap->cmp(elem_ptr(heap, index), elem_ptr(heap, parent), heap->ctx) >= 0)
            break;
        heap_swap_at(heap, index, parent);
        index = parent;
    }
}

static void sift_down(Heap *heap, usize index) {
    for (;;) {
        usize left = index * 2 + 1;
        usize right = left + 1;
        usize best = index;

        if (left < heap->length &&
            heap->cmp(elem_ptr(heap, left), elem_ptr(heap, best), heap->ctx) < 0)
            best = left;
        if (right < heap->length &&
            heap->cmp(elem_ptr(heap, right), elem_ptr(heap, best), heap->ctx) < 0)
            best = right;
        if (best == index)
            break;
        heap_swap_at(heap, index, best);
        index = best;
    }
}

Heap heap_new(usize stride, HeapCmpFn cmp, void *ctx) {
    if (!stride || !cmp)
        return HEAP_NULL;
    return (Heap){NULL, stride, 0, 0, cmp, ctx};
}

Heap heap_new_reserve(usize stride, usize capacity, HeapCmpFn cmp, void *ctx) {
    Heap h = heap_new(stride, cmp, ctx);
    if (!heap_valid(&h))
        return HEAP_NULL;
    if (capacity && !heap_reserve(&h, capacity))
        return HEAP_NULL;
    return h;
}

void heap_delete(Heap *heap) {
    if (!heap)
        return;
    utils_free(heap->data);
    *heap = HEAP_NULL;
}

void heap_clear(Heap *heap) {
    if (!heap)
        return;
    heap->length = 0;
}

usize heap_length(const Heap *heap) { return heap ? heap->length : 0; }

usize heap_capacity(const Heap *heap) { return heap ? heap->capacity : 0; }

usize heap_stride(const Heap *heap) { return heap ? heap->stride : 0; }

bool heap_empty(const Heap *heap) { return heap_length(heap) == 0; }

bool heap_valid(const Heap *heap) { return heap && heap->stride && heap->cmp; }

bool heap_reserve(Heap *heap, usize capacity) {
    if (!heap_valid(heap))
        return false;
    if (capacity <= heap->capacity)
        return true;
    return heap_grow(heap, capacity);
}

bool heap_push(Heap *heap, const void *element) {
    if (!heap_valid(heap) || !element)
        return false;
    if (heap->length == heap->capacity && !heap_grow(heap, heap->length + 1))
        return false;

    memcpy(elem_ptr(heap, heap->length), element, heap->stride);
    sift_up(heap, heap->length);
    heap->length++;
    return true;
}

bool heap_pop(Heap *heap, void *out) {
    if (!heap || heap_empty(heap))
        return false;

    if (out)
        memcpy(out, elem_ptr(heap, 0), heap->stride);

    heap->length--;
    if (heap->length > 0) {
        memcpy(elem_ptr(heap, 0), elem_ptr(heap, heap->length), heap->stride);
        sift_down(heap, 0);
    }
    return true;
}

bool heap_peek(const Heap *heap, void *out) {
    const void *top = heap_top_const(heap);
    if (!top || !out)
        return false;
    memcpy(out, top, heap->stride);
    return true;
}

const void *heap_top_const(const Heap *heap) {
    if (!heap || heap_empty(heap))
        return NULL;
    return elem_ptr_c(heap, 0);
}

void *heap_top(Heap *heap) {
    if (!heap || heap_empty(heap))
        return NULL;
    return elem_ptr(heap, 0);
}
