#pragma once

#include <stdint.h>
#include <stdio.h>

// Fail the current test if _CONDITION_ is false.
#define TEST_ASSERT(_CONDITION_) \
    do { \
        if (!(_CONDITION_)) { \
            fprintf(stderr, "FAIL %s -- %s (%s:%d)\n", __func__, #_CONDITION_, __FILE__, __LINE__); \
            return -1; \
        } \
    } while (0)

#define TEST_PASS() return 0

#define TEST_CASE(_NAME_) static int _NAME_(void)

// Define main() to run every test listed and report how many passed.
#define TEST_SUITE(...)  \
    int main(void) {  \
        int (*const TESTS[])(void) = { __VA_ARGS__ }; \
        uint32_t count = sizeof(TESTS) / sizeof(*TESTS); \
        uint32_t passed = 0; \
        for (uint32_t index = 0; index < count; ++index) { \
            if (TESTS[index]() == 0) { \
                ++passed; \
            } \
        } \
        printf("%u/%u tests passed\n", (unsigned)passed, (unsigned)count); \
        return passed == count ? 0 : 1; \
    }
