#include <stdio.h>

#define TEST_ASSERT(_CONDITION_) \
    if (!(_CONDITION_)) { \
        fprintf(stderr, "%s -- %s (%s:%u)\n", __func__, #_CONDITION_, __FILE__, __LINE__); \
        return -1; \
    }

#define TEST_PASS() return 0

#define TEST_CASE(_NAME_) static int _NAME_(void)

#define TEST_SUITE(...)  \
    int main(void) {  \
        int result = 0; \
        int (*const TESTS[])(void) = { __VA_ARGS__ }; \
        for (uint32_t index = 0; index < sizeof(TESTS) / sizeof(*TESTS); ++index) { \
            if (TESTS[index]() != 0) { \
                result = -1; \
            } \
        } \
        return result; \
    }
