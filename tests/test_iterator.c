#include "iterator.h"
#include "test.h"

typedef struct {
    const int *items;
    usize len;
    usize i;
} IntSource;

static bool int_source_next(void *ctx, void *out) {
    IntSource *s = ctx;
    if (s->i >= s->len)
        return false;
    *(int *)out = s->items[s->i++];
    return true;
}

static bool stop_after_n(void *ctx, usize produced, const void *last) {
    (void)last;
    usize n = *(const usize *)ctx;
    return produced >= n;
}

static void test_iter_vec_forward_reverse(void) {
    Vec v = VEC_NEW(int);
    VEC_PUSH(int, &v, 10);
    VEC_PUSH(int, &v, 20);
    VEC_PUSH(int, &v, 30);

    int sum = 0;
    ITER_FOR(it, iter_from_vec(&v)) { sum += ITER_GET(int, &it); }
    TEST_EQ_INT(sum, 60);

    Iterator it = iter_from_vec(&v);
    iter_reverse(&it);
    TEST_EQ_INT(ITER_GET(int, &it), 30);
    TEST_CHECK(iter_next(&it));
    TEST_EQ_INT(ITER_GET(int, &it), 20);
    TEST_CHECK(iter_next(&it));
    TEST_EQ_INT(ITER_GET(int, &it), 10);
    TEST_CHECK(!iter_next(&it));
    TEST_CHECK(iter_done(&it));

    iter_reset(&it);
    TEST_EQ_INT(ITER_GET(int, &it), 30);
    TEST_CHECK(iter_seek(&it, 0));
    TEST_EQ_INT(ITER_GET(int, &it), 10);

    vec_delete(&v);
}

static void test_iter_queue(void) {
    Queue q = QUEUE_NEW(int);
    QUEUE_PUSH(int, &q, 1);
    QUEUE_PUSH(int, &q, 2);
    QUEUE_PUSH(int, &q, 3);
    int dropped = 0;
    QUEUE_POP(int, &q, &dropped);
    TEST_EQ_INT(dropped, 1);

    Iterator it = iter_from_queue(&q);
    TEST_EQ_INT(ITER_GET(int, &it), 2);
    TEST_CHECK(iter_next(&it));
    TEST_EQ_INT(ITER_GET(int, &it), 3);
    TEST_CHECK(!iter_next(&it));

    queue_delete(&q);
}

static void test_consumer_queue(void) {
    Queue q = QUEUE_NEW(int);
    QUEUE_PUSH(int, &q, 1);
    QUEUE_PUSH(int, &q, 2);
    QUEUE_PUSH(int, &q, 3);

    Consumer c = consumer_from_queue(&q);
    int x = 0;
    TEST_CHECK(CONSUMER_NEXT(int, &c, &x));
    TEST_EQ_INT(x, 1);
    TEST_CHECK(CONSUMER_NEXT(int, &c, &x));
    TEST_EQ_INT(x, 2);
    TEST_CHECK(CONSUMER_PEEK(int, &c, &x));
    TEST_EQ_INT(x, 3);
    TEST_CHECK(CONSUMER_NEXT(int, &c, &x));
    TEST_EQ_INT(x, 3);
    TEST_CHECK(consumer_done(&c));
    TEST_CHECK(queue_empty(&q));

    queue_delete(&q);
}

static void test_consumer_vec_reverse(void) {
    Vec v = VEC_NEW(int);
    VEC_PUSH(int, &v, 1);
    VEC_PUSH(int, &v, 2);
    VEC_PUSH(int, &v, 3);

    Consumer c = consumer_from_vec(&v);
    consumer_reverse(&c);

    int x = 0;
    TEST_CHECK(CONSUMER_NEXT(int, &c, &x));
    TEST_EQ_INT(x, 3);
    TEST_CHECK(CONSUMER_NEXT(int, &c, &x));
    TEST_EQ_INT(x, 2);
    TEST_CHECK(CONSUMER_NEXT(int, &c, &x));
    TEST_EQ_INT(x, 1);
    TEST_CHECK(vec_empty(&v));

    vec_delete(&v);
}

static void test_producer_until(void) {
    Vec v = VEC_NEW(int);
    const int items[] = {1, 2, 3, 4, 5};
    IntSource src = {items, 5, 0};
    usize limit = 3;

    Producer p = producer_to_vec(&v, int_source_next, &src);
    usize n = producer_run_until(&p, stop_after_n, &limit);
    TEST_EQ_USIZE(n, 3);
    TEST_EQ_USIZE(vec_length(&v), 3);
    TEST_EQ_INT(VEC_AT(int, &v, 0), 1);
    TEST_EQ_INT(VEC_AT(int, &v, 2), 3);
    TEST_EQ_USIZE(src.i, 3);

    n = producer_run(&p);
    TEST_EQ_USIZE(n, 2);
    TEST_EQ_USIZE(vec_length(&v), 5);

    vec_delete(&v);
}

int main(void) {
    test_iter_vec_forward_reverse();
    test_iter_queue();
    test_consumer_queue();
    test_consumer_vec_reverse();
    test_producer_until();
    return test_report("iterator");
}
