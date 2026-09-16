#include "deque.h"
#include "test.h"

static void test_deque_ends(void) {
    Deque d = DEQUE_NEW(int);
    TEST_CHECK(deque_valid(&d));
    TEST_CHECK(deque_empty(&d));

    TEST_CHECK(DEQUE_PUSH_BACK(int, &d, 1));
    TEST_CHECK(DEQUE_PUSH_BACK(int, &d, 2));
    TEST_CHECK(DEQUE_PUSH_FRONT(int, &d, 0));
    TEST_EQ_USIZE(deque_length(&d), 3);
    TEST_EQ_INT(DEQUE_AT(int, &d, 0), 0);
    TEST_EQ_INT(DEQUE_AT(int, &d, 1), 1);
    TEST_EQ_INT(DEQUE_AT(int, &d, 2), 2);

    int v = 0;
    TEST_CHECK(DEQUE_POP_FRONT(int, &d, &v));
    TEST_EQ_INT(v, 0);
    TEST_CHECK(DEQUE_POP_BACK(int, &d, &v));
    TEST_EQ_INT(v, 2);
    TEST_EQ_USIZE(deque_length(&d), 1);
    TEST_EQ_INT(DEQUE_AT(int, &d, 0), 1);

    deque_delete(&d);
}

static void test_deque_ring(void) {
    Deque d = DEQUE_NEW_RESERVE(int, 4);
    for (int i = 0; i < 3; i++)
        TEST_CHECK(DEQUE_PUSH_BACK(int, &d, i));

    int v = 0;
    TEST_CHECK(DEQUE_POP_FRONT(int, &d, &v));
    TEST_EQ_INT(v, 0);
    TEST_CHECK(DEQUE_POP_FRONT(int, &d, &v));
    TEST_EQ_INT(v, 1);

    TEST_CHECK(DEQUE_PUSH_BACK(int, &d, 10));
    TEST_CHECK(DEQUE_PUSH_BACK(int, &d, 11));
    TEST_CHECK(DEQUE_PUSH_FRONT(int, &d, 9));

    TEST_EQ_USIZE(deque_length(&d), 4);
    TEST_EQ_INT(DEQUE_AT(int, &d, 0), 9);
    TEST_EQ_INT(DEQUE_AT(int, &d, 1), 2);
    TEST_EQ_INT(DEQUE_AT(int, &d, 2), 10);
    TEST_EQ_INT(DEQUE_AT(int, &d, 3), 11);

    for (int i = 0; i < 100; i++)
        TEST_CHECK(DEQUE_PUSH_BACK(int, &d, i));
    TEST_EQ_USIZE(deque_length(&d), 104);

    deque_clear(&d);
    TEST_CHECK(deque_empty(&d));
    deque_delete(&d);
}

int main(void) {
    test_deque_ends();
    test_deque_ring();
    return test_report("deque");
}
