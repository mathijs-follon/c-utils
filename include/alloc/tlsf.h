#ifndef ALLOC_TLSF_H
#define ALLOC_TLSF_H

#include "alloc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * TLSF (Two-Level Segregated Fit) allocator module.
 *
 * Dynamic (grows via system malloc):
 *     UtilsTlsf *heap = utils_tlsf_create(256 * 1024);
 *     ...
 *     utils_tlsf_destroy(heap);
 *
 * Static buffer (no system malloc — STM32-friendly):
 *     alignas(8) static u8 pool[32 * 1024];
 *     UtilsTlsf heap;
 *     utils_tlsf_init_static(&heap, pool, sizeof(pool));
 *     ...
 *     utils_tlsf_deinit(&heap);  // does not free pool
 *
 * Install as global default:
 *     utils_alloc_module_use("tlsf");
 *     // or -DUTILS_DEFAULT_ALLOCATOR=tlsf
 */

typedef struct UtilsTlsf UtilsTlsf;

struct UtilsTlsf {
    void *tlsf;     /* tlsf_t */
    void *control;  /* owned control block, or NULL if inside user buffer */
    void *pools;    /* dynamic pool list, or NULL for static */
    void *buffer;   /* user buffer for static mode (not freed) */
    usize next_pool_bytes;
    u32 flags;
};

#define UTILS_TLSF_STATIC ((u32)1u << 0)
#define UTILS_TLSF_GROWABLE ((u32)1u << 1)
#define UTILS_TLSF_OWNS_SELF ((u32)1u << 2)

usize utils_tlsf_align(void);
usize utils_tlsf_min_buffer(void);

UtilsTlsf *utils_tlsf_create(usize initial_pool_bytes);
bool utils_tlsf_init_static(UtilsTlsf *heap, void *buffer, usize size);

void utils_tlsf_destroy(UtilsTlsf *heap); /* dynamic heaps from create() */
void utils_tlsf_deinit(UtilsTlsf *heap);  /* static or dynamic; frees only owned mem */

UtilsAllocator utils_tlsf_allocator(UtilsTlsf *heap);

void *utils_tlsf_malloc(UtilsTlsf *heap, usize size);
void *utils_tlsf_realloc(UtilsTlsf *heap, void *ptr, usize size);
void utils_tlsf_free(UtilsTlsf *heap, void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* ALLOC_TLSF_H */
