#ifndef TEST_H
#define TEST_H

#include "c_utils/types.h"

#include <stdio.h>
#include <stdlib.h>

static int g_test_failures = 0;
static int g_test_checks = 0;

#define TEST_CHECK(cond)                                                                           \
    do {                                                                                           \
        g_test_checks++;                                                                           \
        if (!(cond)) {                                                                             \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                         \
            g_test_failures++;                                                                     \
        }                                                                                          \
    } while (0)

#define TEST_CHECK_MSG(cond, msg)                                                                  \
    do {                                                                                           \
        g_test_checks++;                                                                           \
        if (!(cond)) {                                                                             \
            fprintf(stderr, "FAIL %s:%d: %s (%s)\n", __FILE__, __LINE__, #cond, (msg));             \
            g_test_failures++;                                                                     \
        }                                                                                          \
    } while (0)

#define TEST_EQ_USIZE(a, b)                                                                        \
    do {                                                                                           \
        usize _a = (a);                                                                            \
        usize _b = (b);                                                                            \
        g_test_checks++;                                                                           \
        if (_a != _b) {                                                                            \
            fprintf(stderr, "FAIL %s:%d: %s == %s (%llu != %llu)\n", __FILE__, __LINE__, #a, #b,    \
                    (unsigned long long)_a, (unsigned long long)_b);                               \
            g_test_failures++;                                                                     \
        }                                                                                          \
    } while (0)

#define TEST_EQ_INT(a, b)                                                                          \
    do {                                                                                           \
        int _a = (int)(a);                                                                         \
        int _b = (int)(b);                                                                         \
        g_test_checks++;                                                                           \
        if (_a != _b) {                                                                            \
            fprintf(stderr, "FAIL %s:%d: %s == %s (%d != %d)\n", __FILE__, __LINE__, #a, #b, _a,    \
                    _b);                                                                           \
            g_test_failures++;                                                                     \
        }                                                                                          \
    } while (0)

static int test_report(const char *suite) {
    if (g_test_failures == 0) {
        printf("OK   %s (%d checks)\n", suite, g_test_checks);
        return 0;
    }
    printf("FAIL %s (%d/%d checks failed)\n", suite, g_test_failures, g_test_checks);
    return 1;
}

#endif /* TEST_H */
