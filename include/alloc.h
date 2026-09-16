#ifndef ALLOC_H
#define ALLOC_H

#include "types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Global allocator used by the library.
 *
 * Default backend is selected at build time:
 *   -DUTILS_DEFAULT_ALLOCATOR=system   (libc malloc; default)
 *   -DUTILS_DEFAULT_ALLOCATOR=tlsf     (Two-Level Segregated Fit)
 *
 * Switch at runtime (see alloc_module.h):
 *     utils_alloc_module_use("tlsf");
 *     utils_alloc_module_use("system");
 *
 * Or install any UtilsAllocator:
 *     utils_allocator_set(&a);
 *
 * Memory returned by the library (e.g. fs_read_all) must be released with
 * utils_free (or the matching custom free).
 *
 * Override libc entry points used by the system/TLSF bootstrap with:
 *   -DUTILS_SYSTEM_MALLOC=... -DUTILS_SYSTEM_REALLOC=... -DUTILS_SYSTEM_FREE=...
 */

typedef void *(*UtilsMallocFn)(usize size, void *ctx);
typedef void *(*UtilsReallocFn)(void *ptr, usize size, void *ctx);
typedef void (*UtilsFreeFn)(void *ptr, void *ctx);

typedef struct {
    UtilsMallocFn malloc;
    UtilsReallocFn realloc;
    UtilsFreeFn free;
    void *ctx;
} UtilsAllocator;

void utils_allocator_set(const UtilsAllocator *allocator);
void utils_allocator_reset(void);
UtilsAllocator utils_allocator_get(void);

void *utils_malloc(usize size);
void *utils_calloc(usize count, usize size);
void *utils_realloc(void *ptr, usize size);
void utils_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* ALLOC_H */
