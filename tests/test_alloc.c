#include "c_utils/alloc.h"
#include "c_utils/alloc_module.h"
#include "test.h"
#include "c_utils/vec.h"

#include <stdlib.h>
#include <string.h>

#if defined(UTILS_ALLOC_TLSF) && UTILS_ALLOC_TLSF
#include "c_utils/alloc/tlsf.h"
#endif

typedef struct {
    int mallocs;
    int reallocs;
    int frees;
} AllocStats;

static void *counting_malloc(usize size, void *ctx) {
    AllocStats *s = ctx;
    s->mallocs++;
    return malloc((size_t)size);
}

static void *counting_realloc(void *ptr, usize size, void *ctx) {
    AllocStats *s = ctx;
    s->reallocs++;
    return realloc(ptr, (size_t)size);
}

static void counting_free(void *ptr, void *ctx) {
    AllocStats *s = ctx;
    s->frees++;
    free(ptr);
}

static void test_custom_allocator(void) {
    AllocStats stats = {0};
    UtilsAllocator a = {
        .malloc = counting_malloc,
        .realloc = counting_realloc,
        .free = counting_free,
        .ctx = &stats,
    };
    utils_allocator_set(&a);

    Vec v = VEC_NEW(int);
    TEST_CHECK(VEC_PUSH(int, &v, 1));
    TEST_CHECK(VEC_PUSH(int, &v, 2));
    TEST_CHECK(stats.mallocs + stats.reallocs > 0);

    int before_frees = stats.frees;
    vec_delete(&v);
    TEST_CHECK(stats.frees > before_frees);

    utils_allocator_reset();
}

static void test_system_module(void) {
    TEST_CHECK(utils_alloc_module_use("system"));
    TEST_CHECK(utils_alloc_module_current() != NULL);
    TEST_CHECK(strcmp(utils_alloc_module_current()->name, "system") == 0);

    void *p = utils_malloc(32);
    TEST_CHECK(p != NULL);
    utils_free(p);
    utils_allocator_reset();
}

#if defined(UTILS_ALLOC_TLSF) && UTILS_ALLOC_TLSF
static void test_tlsf_module(void) {
    TEST_CHECK(utils_alloc_module_find("tlsf") != NULL);
    TEST_CHECK(utils_alloc_module_use("tlsf"));
    TEST_CHECK(strcmp(utils_alloc_module_current()->name, "tlsf") == 0);

    Vec v = VEC_NEW(int);
    for (int i = 0; i < 100; i++)
        TEST_CHECK(VEC_PUSH(int, &v, i));
    TEST_EQ_USIZE(vec_length(&v), 100);
    TEST_EQ_INT(VEC_AT(int, &v, 50), 50);
    vec_delete(&v);

    UtilsTlsf *heap = utils_tlsf_create(8 * 1024);
    TEST_CHECK(heap != NULL);
    void *a = utils_tlsf_malloc(heap, 128);
    void *b = utils_tlsf_malloc(heap, 256);
    TEST_CHECK(a && b);
    a = utils_tlsf_realloc(heap, a, 512);
    TEST_CHECK(a != NULL);
    utils_tlsf_free(heap, a);
    utils_tlsf_free(heap, b);
    utils_tlsf_destroy(heap);

    utils_allocator_reset();
}

static void test_tlsf_init_static(void) {
    enum { POOL = 32 * 1024 };
    _Alignas(16) static u8 pool[POOL];
    UtilsTlsf heap;

    TEST_CHECK(utils_tlsf_min_buffer() > 0);
    TEST_CHECK(utils_tlsf_init_static(&heap, pool, sizeof(pool)));

    void *p = utils_tlsf_malloc(&heap, 64);
    void *q = utils_tlsf_malloc(&heap, 128);
    TEST_CHECK(p && q);
    TEST_CHECK(p != q);

    memset(p, 0xAB, 64);
    p = utils_tlsf_realloc(&heap, p, 256);
    TEST_CHECK(p != NULL);

    utils_tlsf_free(&heap, p);
    utils_tlsf_free(&heap, q);
    utils_tlsf_deinit(&heap);

    /* Too small must fail. */
    TEST_CHECK(!utils_tlsf_init_static(&heap, pool, 8));
}
#endif

int main(void) {
    test_custom_allocator();
    test_system_module();
#if defined(UTILS_ALLOC_TLSF) && UTILS_ALLOC_TLSF
    test_tlsf_module();
    test_tlsf_init_static();
#endif
    return test_report("alloc");
}
