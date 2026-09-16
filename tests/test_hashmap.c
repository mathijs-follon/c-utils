#include "c_utils/hashmap.h"
#include "c_utils/hashset.h"
#include "c_utils/result.h"
#include "test.h"

static void test_result(void) {
    TEST_CHECK(utils_status_str(UTILS_OK) != NULL);
    TEST_CHECK(utils_status_str(UTILS_ERR_NOMEM) != NULL);
}

static void test_hashmap_int(void) {
    HashMap m = HASHMAP_NEW(int, int);
    TEST_CHECK(hashmap_valid(&m));
    TEST_CHECK(hashmap_empty(&m));

    TEST_CHECK(HASHMAP_PUT(int, int, &m, 1, 10));
    TEST_CHECK(HASHMAP_PUT(int, int, &m, 2, 20));
    TEST_CHECK(HASHMAP_PUT(int, int, &m, 3, 30));
    TEST_EQ_USIZE(hashmap_length(&m), 3);

    int v = 0;
    TEST_CHECK(HASHMAP_GET(int, int, &m, 2, &v));
    TEST_EQ_INT(v, 20);
    TEST_CHECK(HASHMAP_CONTAINS(int, &m, 1));
    TEST_CHECK(!HASHMAP_CONTAINS(int, &m, 99));

    TEST_CHECK(HASHMAP_PUT(int, int, &m, 2, 22));
    TEST_EQ_USIZE(hashmap_length(&m), 3);
    TEST_CHECK(HASHMAP_GET(int, int, &m, 2, &v));
    TEST_EQ_INT(v, 22);

    TEST_CHECK(HASHMAP_REMOVE(int, int, &m, 1, &v));
    TEST_EQ_INT(v, 10);
    TEST_EQ_USIZE(hashmap_length(&m), 2);
    TEST_CHECK(!HASHMAP_CONTAINS(int, &m, 1));

    hashmap_clear(&m);
    TEST_CHECK(hashmap_empty(&m));
    hashmap_delete(&m);
}

static void test_hashmap_grow(void) {
    HashMap m = HASHMAP_NEW(int, int);
    for (int i = 0; i < 200; i++)
        TEST_CHECK(HASHMAP_PUT(int, int, &m, i, i * 10));
    TEST_EQ_USIZE(hashmap_length(&m), 200);

    for (int i = 0; i < 200; i++) {
        int v = -1;
        TEST_CHECK(HASHMAP_GET(int, int, &m, i, &v));
        TEST_EQ_INT(v, i * 10);
    }

    for (int i = 0; i < 100; i++)
        TEST_CHECK(HASHMAP_REMOVE(int, int, &m, i, NULL));
    TEST_EQ_USIZE(hashmap_length(&m), 100);

    for (int i = 100; i < 200; i++)
        TEST_CHECK(HASHMAP_CONTAINS(int, &m, i));

    hashmap_delete(&m);
}

static void test_hashmap_cstr(void) {
    HashMap m = hashmap_new(sizeof(const char *), sizeof(int), hashmap_hash_cstr, hashmap_eq_cstr,
                            NULL);
    TEST_CHECK(hashmap_valid(&m));

    const char *ka = "alpha";
    const char *kb = "beta";
    int va = 1, vb = 2;
    TEST_CHECK(hashmap_put(&m, &ka, &va));
    TEST_CHECK(hashmap_put(&m, &kb, &vb));

    const char *lookup = "alpha";
    int out = 0;
    TEST_CHECK(hashmap_get(&m, &lookup, &out));
    TEST_EQ_INT(out, 1);

    hashmap_delete(&m);
}

static void test_hashmap_u64(void) {
    HashMap m = hashmap_new(sizeof(u64), sizeof(int), hashmap_hash_u64, hashmap_eq_u64, NULL);
    u64 k = 42;
    int v = 7;
    TEST_CHECK(hashmap_put(&m, &k, &v));
    int out = 0;
    TEST_CHECK(hashmap_get(&m, &k, &out));
    TEST_EQ_INT(out, 7);
    hashmap_delete(&m);
}

static void test_hashset(void) {
    HashSet s = HASHSET_NEW(int);
    TEST_CHECK(hashset_valid(&s));
    TEST_CHECK(HASHSET_ADD(int, &s, 5));
    TEST_CHECK(HASHSET_ADD(int, &s, 5));
    TEST_EQ_USIZE(hashset_length(&s), 1);
    TEST_CHECK(HASHSET_CONTAINS(int, &s, 5));
    TEST_CHECK(HASHSET_REMOVE(int, &s, 5));
    TEST_CHECK(hashset_empty(&s));
    hashset_delete(&s);
}

int main(void) {
    test_result();
    test_hashmap_int();
    test_hashmap_grow();
    test_hashmap_cstr();
    test_hashmap_u64();
    test_hashset();
    return test_report("hashmap");
}
