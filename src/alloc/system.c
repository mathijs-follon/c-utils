#include "alloc_module.h"

#include <stdlib.h>

#ifndef UTILS_SYSTEM_MALLOC
#define UTILS_SYSTEM_MALLOC malloc
#endif

#ifndef UTILS_SYSTEM_REALLOC
#define UTILS_SYSTEM_REALLOC realloc
#endif

#ifndef UTILS_SYSTEM_FREE
#define UTILS_SYSTEM_FREE free
#endif

static void *system_malloc_fn(usize size, void *ctx) {
    (void)ctx;
    return UTILS_SYSTEM_MALLOC((size_t)size);
}

static void *system_realloc_fn(void *ptr, usize size, void *ctx) {
    (void)ctx;
    return UTILS_SYSTEM_REALLOC(ptr, (size_t)size);
}

static void system_free_fn(void *ptr, void *ctx) {
    (void)ctx;
    UTILS_SYSTEM_FREE(ptr);
}

static bool system_init(void) { return true; }

static void system_shutdown(void) {}

static UtilsAllocator system_allocator(void) {
    return (UtilsAllocator){
        .malloc = system_malloc_fn,
        .realloc = system_realloc_fn,
        .free = system_free_fn,
        .ctx = NULL,
    };
}

const UtilsAllocModule utils_alloc_module_system = {
    .name = "system",
    .init = system_init,
    .shutdown = system_shutdown,
    .allocator = system_allocator,
};
