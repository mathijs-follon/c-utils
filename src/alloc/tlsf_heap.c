#include "alloc/tlsf.h"
#include "alloc_module.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "tlsf.h"

#ifndef UTILS_SYSTEM_MALLOC
#define UTILS_SYSTEM_MALLOC malloc
#endif

#ifndef UTILS_SYSTEM_FREE
#define UTILS_SYSTEM_FREE free
#endif

#ifndef UTILS_TLSF_INITIAL_POOL
#define UTILS_TLSF_INITIAL_POOL ((usize)256 * 1024)
#endif

typedef struct TlsfPoolNode {
    void *mem;
    struct TlsfPoolNode *next;
} TlsfPoolNode;

usize utils_tlsf_align(void) { return (usize)tlsf_align_size(); }

usize utils_tlsf_min_buffer(void) {
    return (usize)tlsf_size() + (usize)tlsf_pool_overhead() + (usize)tlsf_block_size_min();
}

static void *align_up_ptr(void *ptr, usize align) {
    uintptr_t p = (uintptr_t)ptr;
    uintptr_t a = (uintptr_t)align;
    return (void *)((p + a - 1) / a * a);
}

static bool tlsf_add_grown_pool(UtilsTlsf *heap, usize bytes) {
    if (!heap || !(heap->flags & UTILS_TLSF_GROWABLE) || bytes == 0)
        return false;

    usize need = bytes;
    usize overhead = (usize)tlsf_pool_overhead();
    if (need < overhead + (usize)tlsf_block_size_min())
        need = overhead + (usize)tlsf_block_size_min() + 64;

    void *mem = UTILS_SYSTEM_MALLOC((size_t)need);
    if (!mem)
        return false;

    if (!tlsf_add_pool(heap->tlsf, mem, (size_t)need)) {
        UTILS_SYSTEM_FREE(mem);
        return false;
    }

    TlsfPoolNode *node = (TlsfPoolNode *)UTILS_SYSTEM_MALLOC(sizeof(TlsfPoolNode));
    if (!node) {
        tlsf_remove_pool(heap->tlsf, mem);
        UTILS_SYSTEM_FREE(mem);
        return false;
    }

    node->mem = mem;
    node->next = (TlsfPoolNode *)heap->pools;
    heap->pools = node;

    if (heap->next_pool_bytes < need * 2 && need < ((usize)1 << 26))
        heap->next_pool_bytes = need * 2;

    return true;
}

bool utils_tlsf_init_static(UtilsTlsf *heap, void *buffer, usize size) {
    if (!heap || !buffer || size < utils_tlsf_min_buffer())
        return false;

    usize align = utils_tlsf_align();
    void *aligned = align_up_ptr(buffer, align);
    usize lead = (usize)((u8 *)aligned - (u8 *)buffer);
    if (size <= lead || size - lead < utils_tlsf_min_buffer())
        return false;

    usize usable = size - lead;
    tlsf_t tlsf = tlsf_create_with_pool(aligned, (size_t)usable);
    if (!tlsf)
        return false;

    memset(heap, 0, sizeof(*heap));
    heap->tlsf = tlsf;
    heap->control = NULL;
    heap->pools = NULL;
    heap->buffer = buffer;
    heap->next_pool_bytes = 0;
    heap->flags = UTILS_TLSF_STATIC;
    return true;
}

UtilsTlsf *utils_tlsf_create(usize initial_pool_bytes) {
    usize control_bytes = (usize)tlsf_size();
    void *control = UTILS_SYSTEM_MALLOC((size_t)control_bytes);
    if (!control)
        return NULL;

    UtilsTlsf *heap = (UtilsTlsf *)UTILS_SYSTEM_MALLOC(sizeof(UtilsTlsf));
    if (!heap) {
        UTILS_SYSTEM_FREE(control);
        return NULL;
    }

    memset(heap, 0, sizeof(*heap));
    heap->control = control;
    heap->tlsf = tlsf_create(control);
    if (!heap->tlsf) {
        UTILS_SYSTEM_FREE(control);
        UTILS_SYSTEM_FREE(heap);
        return NULL;
    }

    heap->flags = UTILS_TLSF_GROWABLE | UTILS_TLSF_OWNS_SELF;
    heap->next_pool_bytes =
        initial_pool_bytes ? initial_pool_bytes : UTILS_TLSF_INITIAL_POOL;

    if (!tlsf_add_grown_pool(heap, heap->next_pool_bytes)) {
        tlsf_destroy(heap->tlsf);
        UTILS_SYSTEM_FREE(control);
        UTILS_SYSTEM_FREE(heap);
        return NULL;
    }

    return heap;
}

void utils_tlsf_deinit(UtilsTlsf *heap) {
    if (!heap)
        return;

    if (heap->flags & UTILS_TLSF_STATIC) {
        if (heap->tlsf)
            tlsf_destroy(heap->tlsf);
        memset(heap, 0, sizeof(*heap));
        return;
    }

    TlsfPoolNode *node = (TlsfPoolNode *)heap->pools;
    while (node) {
        TlsfPoolNode *next = node->next;
        if (heap->tlsf && node->mem)
            tlsf_remove_pool(heap->tlsf, node->mem);
        UTILS_SYSTEM_FREE(node->mem);
        UTILS_SYSTEM_FREE(node);
        node = next;
    }

    if (heap->tlsf)
        tlsf_destroy(heap->tlsf);

    UTILS_SYSTEM_FREE(heap->control);

    if (heap->flags & UTILS_TLSF_OWNS_SELF)
        UTILS_SYSTEM_FREE(heap);
    else
        memset(heap, 0, sizeof(*heap));
}

void utils_tlsf_destroy(UtilsTlsf *heap) {
    if (!heap)
        return;
    /* Static heaps must use deinit — destroy is for create(). */
    if (heap->flags & UTILS_TLSF_STATIC)
        return;
    utils_tlsf_deinit(heap);
}

void *utils_tlsf_malloc(UtilsTlsf *heap, usize size) {
    if (!heap || !heap->tlsf)
        return NULL;

    void *ptr = tlsf_malloc(heap->tlsf, (size_t)size);
    if (ptr)
        return ptr;

    if (!(heap->flags & UTILS_TLSF_GROWABLE))
        return NULL;

    usize grow = heap->next_pool_bytes;
    if (grow < size + 4096)
        grow = size + 4096;

    if (!tlsf_add_grown_pool(heap, grow))
        return NULL;

    return tlsf_malloc(heap->tlsf, (size_t)size);
}

void *utils_tlsf_realloc(UtilsTlsf *heap, void *ptr, usize size) {
    if (!heap || !heap->tlsf)
        return NULL;

    if (!ptr)
        return utils_tlsf_malloc(heap, size);

    if (size == 0) {
        utils_tlsf_free(heap, ptr);
        return NULL;
    }

    void *out = tlsf_realloc(heap->tlsf, ptr, (size_t)size);
    if (out)
        return out;

    if (!(heap->flags & UTILS_TLSF_GROWABLE)) {
        void *fresh = tlsf_malloc(heap->tlsf, (size_t)size);
        if (!fresh)
            return NULL;
        size_t old_size = tlsf_block_size(ptr);
        size_t copy = old_size < (size_t)size ? old_size : (size_t)size;
        memcpy(fresh, ptr, copy);
        tlsf_free(heap->tlsf, ptr);
        return fresh;
    }

    usize grow = heap->next_pool_bytes;
    if (grow < size + 4096)
        grow = size + 4096;
    if (!tlsf_add_grown_pool(heap, grow))
        return NULL;

    out = tlsf_realloc(heap->tlsf, ptr, (size_t)size);
    if (out)
        return out;

    void *fresh = tlsf_malloc(heap->tlsf, (size_t)size);
    if (!fresh)
        return NULL;

    size_t old_size = tlsf_block_size(ptr);
    size_t copy = old_size < (size_t)size ? old_size : (size_t)size;
    memcpy(fresh, ptr, copy);
    tlsf_free(heap->tlsf, ptr);
    return fresh;
}

void utils_tlsf_free(UtilsTlsf *heap, void *ptr) {
    if (!heap || !heap->tlsf || !ptr)
        return;
    tlsf_free(heap->tlsf, ptr);
}

static void *tlsf_malloc_fn(usize size, void *ctx) {
    return utils_tlsf_malloc((UtilsTlsf *)ctx, size);
}

static void *tlsf_realloc_fn(void *ptr, usize size, void *ctx) {
    return utils_tlsf_realloc((UtilsTlsf *)ctx, ptr, size);
}

static void tlsf_free_fn(void *ptr, void *ctx) {
    utils_tlsf_free((UtilsTlsf *)ctx, ptr);
}

UtilsAllocator utils_tlsf_allocator(UtilsTlsf *heap) {
    return (UtilsAllocator){
        .malloc = tlsf_malloc_fn,
        .realloc = tlsf_realloc_fn,
        .free = tlsf_free_fn,
        .ctx = heap,
    };
}

/* --- module (global default) -------------------------------------------- */

static UtilsTlsf *g_tlsf_heap = NULL;

static bool tlsf_module_init(void) {
    if (g_tlsf_heap)
        return true;
    g_tlsf_heap = utils_tlsf_create(UTILS_TLSF_INITIAL_POOL);
    return g_tlsf_heap != NULL;
}

static void tlsf_module_shutdown(void) {
    utils_tlsf_destroy(g_tlsf_heap);
    g_tlsf_heap = NULL;
}

static UtilsAllocator tlsf_module_allocator(void) {
    return utils_tlsf_allocator(g_tlsf_heap);
}

const UtilsAllocModule utils_alloc_module_tlsf = {
    .name = "tlsf",
    .init = tlsf_module_init,
    .shutdown = tlsf_module_shutdown,
    .allocator = tlsf_module_allocator,
};
