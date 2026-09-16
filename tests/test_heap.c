#include "c_utils/heap.h"
#include "test.h"

static int cmp_int_min(const void *a, const void *b, void *ctx) {
    (void)ctx;
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y);
}

static int cmp_int_max(const void *a, const void *b, void *ctx) {
    (void)ctx;
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (y > x) - (y < x);
}

static void test_heap_min(void) {
    Heap h = HEAP_NEW(int, cmp_int_min, NULL);
    TEST_CHECK(heap_valid(&h));
    TEST_CHECK(heap_empty(&h));

    TEST_CHECK(HEAP_PUSH(int, &h, 5));
    TEST_CHECK(HEAP_PUSH(int, &h, 1));
    TEST_CHECK(HEAP_PUSH(int, &h, 9));
    TEST_CHECK(HEAP_PUSH(int, &h, 3));
    TEST_EQ_USIZE(heap_length(&h), 4);

    int v = 0;
    TEST_CHECK(HEAP_PEEK(int, &h, &v));
    TEST_EQ_INT(v, 1);

    TEST_CHECK(HEAP_POP(int, &h, &v));
    TEST_EQ_INT(v, 1);
    TEST_CHECK(HEAP_POP(int, &h, &v));
    TEST_EQ_INT(v, 3);
    TEST_CHECK(HEAP_POP(int, &h, &v));
    TEST_EQ_INT(v, 5);
    TEST_CHECK(HEAP_POP(int, &h, &v));
    TEST_EQ_INT(v, 9);
    TEST_CHECK(heap_empty(&h));
    TEST_CHECK(!HEAP_POP(int, &h, &v));

    heap_delete(&h);
}

static void test_heap_max(void) {
    Heap h = heap_new(sizeof(int), cmp_int_max, NULL);
    int vals[] = {4, 8, 2, 7, 1};
    for (usize i = 0; i < sizeof(vals) / sizeof(vals[0]); i++)
        TEST_CHECK(heap_push(&h, &vals[i]));

    int prev = 100;
    while (!heap_empty(&h)) {
        int v = 0;
        TEST_CHECK(heap_pop(&h, &v));
        TEST_CHECK(v <= prev);
        prev = v;
    }
    heap_delete(&h);
}

static void test_heap_grow(void) {
    Heap h = HEAP_NEW(int, cmp_int_min, NULL);
    for (int i = 100; i >= 0; i--)
        TEST_CHECK(HEAP_PUSH(int, &h, i));
    TEST_EQ_USIZE(heap_length(&h), 101);

    for (int expect = 0; expect <= 100; expect++) {
        int v = -1;
        TEST_CHECK(HEAP_POP(int, &h, &v));
        TEST_EQ_INT(v, expect);
    }
    heap_delete(&h);
}

int main(void) {
    test_heap_min();
    test_heap_max();
    test_heap_grow();
    return test_report("heap");
}
