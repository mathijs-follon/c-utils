#include "c_utils/stack.h"
#include "test.h"

static void test_stack_lifo(void) {
    Stack s = STACK_NEW(int);
    TEST_CHECK(stack_valid(&s));
    TEST_CHECK(stack_empty(&s));

    TEST_CHECK(STACK_PUSH(int, &s, 1));
    TEST_CHECK(STACK_PUSH(int, &s, 2));
    TEST_CHECK(STACK_PUSH(int, &s, 3));
    TEST_EQ_USIZE(stack_length(&s), 3);
    TEST_EQ_INT(STACK_TOP(int, &s), 3);

    int v = 0;
    TEST_CHECK(STACK_PEEK(int, &s, &v));
    TEST_EQ_INT(v, 3);
    TEST_EQ_USIZE(stack_length(&s), 3);

    TEST_CHECK(STACK_POP(int, &s, &v));
    TEST_EQ_INT(v, 3);
    TEST_CHECK(STACK_POP(int, &s, &v));
    TEST_EQ_INT(v, 2);
    TEST_CHECK(STACK_POP(int, &s, &v));
    TEST_EQ_INT(v, 1);
    TEST_CHECK(stack_empty(&s));
    TEST_CHECK(!STACK_POP(int, &s, &v));

    stack_delete(&s);
}

static void test_stack_copy(void) {
    Stack s = STACK_NEW(int);
    STACK_PUSH(int, &s, 10);
    STACK_PUSH(int, &s, 20);

    Stack c = stack_new_copy(&s);
    TEST_EQ_USIZE(stack_length(&c), 2);
    TEST_EQ_INT(STACK_TOP(int, &c), 20);

    int v = 0;
    STACK_POP(int, &s, &v);
    TEST_EQ_USIZE(stack_length(&s), 1);
    TEST_EQ_USIZE(stack_length(&c), 2);

    stack_delete(&s);
    stack_delete(&c);
}

int main(void) {
    test_stack_lifo();
    test_stack_copy();
    return test_report("stack");
}
