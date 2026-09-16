#include "queue.h"
#include "test.h"

static void test_queue_fifo(void) {
    Queue q = QUEUE_NEW(int);
    TEST_CHECK(queue_valid(&q));
    TEST_CHECK(queue_empty(&q));

    TEST_CHECK(QUEUE_PUSH(int, &q, 1));
    TEST_CHECK(QUEUE_PUSH(int, &q, 2));
    TEST_CHECK(QUEUE_PUSH(int, &q, 3));
    TEST_EQ_USIZE(queue_length(&q), 3);
    TEST_EQ_INT(QUEUE_FRONT(int, &q), 1);
    TEST_EQ_INT(QUEUE_BACK(int, &q), 3);

    int v = 0;
    TEST_CHECK(QUEUE_PEEK(int, &q, &v));
    TEST_EQ_INT(v, 1);

    TEST_CHECK(QUEUE_POP(int, &q, &v));
    TEST_EQ_INT(v, 1);
    TEST_CHECK(QUEUE_POP(int, &q, &v));
    TEST_EQ_INT(v, 2);
    TEST_CHECK(QUEUE_POP(int, &q, &v));
    TEST_EQ_INT(v, 3);
    TEST_CHECK(queue_empty(&q));
    TEST_CHECK(!QUEUE_POP(int, &q, &v));

    queue_delete(&q);
}

static void test_queue_compact(void) {
    Queue q = QUEUE_NEW(int);
    for (int i = 0; i < 40; i++)
        TEST_CHECK(QUEUE_PUSH(int, &q, i));

    int v = 0;
    for (int i = 0; i < 30; i++) {
        TEST_CHECK(QUEUE_POP(int, &q, &v));
        TEST_EQ_INT(v, i);
    }

    TEST_EQ_USIZE(queue_length(&q), 10);
    TEST_EQ_INT(QUEUE_FRONT(int, &q), 30);
    TEST_EQ_INT(QUEUE_BACK(int, &q), 39);

    Queue copy = queue_new_copy(&q);
    TEST_EQ_USIZE(queue_length(&copy), 10);
    TEST_EQ_INT(QUEUE_FRONT(int, &copy), 30);
    TEST_CHECK(copy.head == 0);

    queue_delete(&q);
    queue_delete(&copy);
}

int main(void) {
    test_queue_fifo();
    test_queue_compact();
    return test_report("queue");
}
