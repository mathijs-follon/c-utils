#ifndef ALLOC_MODULE_H
#define ALLOC_MODULE_H

#include "alloc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Allocator backend modules.
 *
 * Built-ins: "system", "tlsf" (if enabled).
 * Add more by defining a UtilsAllocModule and registering it, or by extending
 * the built-in table in src/alloc.c.
 *
 *     utils_alloc_module_use("tlsf");
 *     utils_alloc_module_use("system");
 */

typedef struct UtilsAllocModule {
    const char *name;
    bool (*init)(void);
    void (*shutdown)(void);
    UtilsAllocator (*allocator)(void);
} UtilsAllocModule;

const UtilsAllocModule *utils_alloc_module_find(const char *name);
bool utils_alloc_module_use(const char *name);
const UtilsAllocModule *utils_alloc_module_current(void);

extern const UtilsAllocModule utils_alloc_module_system;

#if defined(UTILS_ALLOC_TLSF) && UTILS_ALLOC_TLSF
extern const UtilsAllocModule utils_alloc_module_tlsf;
#endif

#ifdef __cplusplus
}
#endif

#endif /* ALLOC_MODULE_H */
