#include "test.h"
#include "vec.h"

typedef struct {
    int x;
    int y;
} Point;

static void test_vec_int(void) {
    Vec v = VEC_NEW(int);
    TEST_CHECK(vec_valid(&v));
    TEST_CHECK(vec_empty(&v));

    TEST_CHECK(VEC_PUSH(int, &v, 10));
    TEST_CHECK(VEC_PUSH(int, &v, 20));
    TEST_CHECK(VEC_PUSH(int, &v, 30));
    TEST_EQ_USIZE(vec_length(&v), 3);
    TEST_EQ_INT(VEC_AT(int, &v, 0), 10);
    TEST_EQ_INT(VEC_AT(int, &v, 2), 30);

    TEST_CHECK(VEC_INSERT(int, &v, 1, 15));
    TEST_EQ_INT(VEC_AT(int, &v, 1), 15);
    TEST_EQ_USIZE(vec_length(&v), 4);

    TEST_CHECK(VEC_CONTAINS(int, &v, 20));
    TEST_EQ_USIZE(VEC_FIND(int, &v, 30), 3);

    int popped = 0;
    TEST_CHECK(VEC_POP(int, &v, &popped));
    TEST_EQ_INT(popped, 30);
    TEST_EQ_USIZE(vec_length(&v), 3);

    TEST_CHECK(vec_remove_at(&v, 1, NULL));
    TEST_EQ_INT(VEC_AT(int, &v, 1), 20);

    vec_reverse(&v);
    TEST_EQ_INT(VEC_AT(int, &v, 0), 20);
    TEST_EQ_INT(VEC_AT(int, &v, 1), 10);

    Vec copy = vec_new_copy(&v);
    TEST_EQ_USIZE(vec_length(&copy), 2);
    TEST_EQ_INT(VEC_AT(int, &copy, 0), 20);

    vec_delete(&v);
    vec_delete(&copy);
}

static void test_vec_struct(void) {
    Vec pts = VEC_NEW(Point);
    TEST_CHECK(VEC_PUSH(Point, &pts, .x = 1, .y = 2));
    TEST_CHECK(VEC_PUSH(Point, &pts, .x = 3, .y = 4));

    Point p = VEC_AT(Point, &pts, 0);
    TEST_EQ_INT(p.x, 1);
    TEST_EQ_INT(p.y, 2);

    TEST_CHECK(VEC_SET(Point, &pts, 0, .x = 9, .y = 8));
    TEST_EQ_INT(VEC_AT(Point, &pts, 0).x, 9);

    TEST_CHECK(VEC_CONTAINS(Point, &pts, .x = 3, .y = 4));
    TEST_CHECK(VEC_REMOVE(Point, &pts, .x = 3, .y = 4));
    TEST_EQ_USIZE(vec_length(&pts), 1);

    vec_delete(&pts);
}

static void test_vec_push_other_self(void) {
    Vec v = VEC_NEW(int);
    TEST_CHECK(VEC_PUSH(int, &v, 1));
    TEST_CHECK(VEC_PUSH(int, &v, 2));
    TEST_CHECK(vec_push_other(&v, &v));
    TEST_EQ_USIZE(vec_length(&v), 4);
    TEST_EQ_INT(VEC_AT(int, &v, 0), 1);
    TEST_EQ_INT(VEC_AT(int, &v, 1), 2);
    TEST_EQ_INT(VEC_AT(int, &v, 2), 1);
    TEST_EQ_INT(VEC_AT(int, &v, 3), 2);
    vec_delete(&v);
}

int main(void) {
    test_vec_int();
    test_vec_struct();
    test_vec_push_other_self();
    return test_report("vec");
}
