#include "alloc.h"
#include "alloc_module.h"

#include <string.h>

#if defined(UTILS_ALLOC_TLSF) && UTILS_ALLOC_TLSF
#include "alloc/tlsf.h"
#endif

extern const UtilsAllocModule utils_alloc_module_system;

#if defined(UTILS_ALLOC_TLSF) && UTILS_ALLOC_TLSF
extern const UtilsAllocModule utils_alloc_module_tlsf;
#endif

static const UtilsAllocModule *const g_modules[] = {
    &utils_alloc_module_system,
#if defined(UTILS_ALLOC_TLSF) && UTILS_ALLOC_TLSF
    &utils_alloc_module_tlsf,
#endif
};

static const UtilsAllocModule *g_current_module = NULL;
static UtilsAllocator g_allocator;
static bool g_started = false;

static const UtilsAllocModule *default_module(void) {
#if defined(UTILS_DEFAULT_ALLOCATOR_TLSF) && UTILS_DEFAULT_ALLOCATOR_TLSF
#if defined(UTILS_ALLOC_TLSF) && UTILS_ALLOC_TLSF
    return &utils_alloc_module_tlsf;
#else
#error "UTILS_DEFAULT_ALLOCATOR=tlsf requires UTILS_ALLOC_TLSF"
#endif
#else
    return &utils_alloc_module_system;
#endif
}

static bool ensure_started(void) {
    if (g_started)
        return true;

    g_current_module = default_module();
    if (!g_current_module->init()) {
        g_current_module = &utils_alloc_module_system;
        if (!g_current_module->init())
            return false;
    }

    g_allocator = g_current_module->allocator();
    g_started = true;
    return true;
}

const UtilsAllocModule *utils_alloc_module_find(const char *name) {
    if (!name)
        return NULL;

    for (usize i = 0; i < sizeof(g_modules) / sizeof(g_modules[0]); i++) {
        if (g_modules[i] && g_modules[i]->name && strcmp(g_modules[i]->name, name) == 0)
            return g_modules[i];
    }
    return NULL;
}

bool utils_alloc_module_use(const char *name) {
    const UtilsAllocModule *mod = utils_alloc_module_find(name);
    if (!mod || !mod->init || !mod->allocator)
        return false;

    if (g_started && g_current_module && g_current_module != mod && g_current_module->shutdown)
        g_current_module->shutdown();

    if (!mod->init())
        return false;

    g_current_module = mod;
    g_allocator = mod->allocator();
    g_started = true;
    return true;
}

const UtilsAllocModule *utils_alloc_module_current(void) {
    ensure_started();
    return g_current_module;
}

void utils_allocator_set(const UtilsAllocator *allocator) {
    ensure_started();
    if (!allocator || !allocator->malloc || !allocator->realloc || !allocator->free) {
        utils_allocator_reset();
        return;
    }
    g_allocator = *allocator;
}

void utils_allocator_reset(void) {
    if (g_started && g_current_module && g_current_module->shutdown)
        g_current_module->shutdown();

    g_started = false;
    g_current_module = NULL;
    ensure_started();
}

UtilsAllocator utils_allocator_get(void) {
    ensure_started();
    return g_allocator;
}

void *utils_malloc(usize size) {
    ensure_started();
    return g_allocator.malloc(size, g_allocator.ctx);
}

void *utils_calloc(usize count, usize size) {
    usize bytes;
    if (count != 0 && size > (usize)-1 / count)
        return NULL;
    bytes = count * size;

    void *ptr = utils_malloc(bytes);
    if (ptr && bytes)
        memset(ptr, 0, (size_t)bytes);
    return ptr;
}

void *utils_realloc(void *ptr, usize size) {
    ensure_started();
    return g_allocator.realloc(ptr, size, g_allocator.ctx);
}

void utils_free(void *ptr) {
    if (!ptr)
        return;
    ensure_started();
    g_allocator.free(ptr, g_allocator.ctx);
}
