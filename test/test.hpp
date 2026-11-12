#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------- Colors ----------
// PASS is printed in green and FAIL in red, using ANSI escape codes, when the
// output is a terminal that understands them (macOS, Linux, the MSYS2
// terminal on Windows, Windows Terminal, and Windows 10+ consoles).
// Set NO_COLOR=1 to turn colors off, or FORCE_COLOR=1 to force them on.
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <unistd.h>
#endif

static int test_env_set(const char* name) {
    const char* value = getenv(name);
    return value != NULL && value[0] != '\0' && strcmp(value, "0") != 0;
}

static int test_use_color(void) {
    static int cached = -1;
    if (cached != -1) {
        return cached;
    }

    const char* term = getenv("TERM");
    if (test_env_set("NO_COLOR") || (term != NULL && strcmp(term, "dumb") == 0)) {
        cached = 0;
    } else if (test_env_set("FORCE_COLOR")) {
        cached = 1;
    } else {
#ifdef _WIN32
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        if (GetConsoleMode(out, &mode)) {
            // A Windows console: ask it to understand ANSI colors.
            cached = SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) ? 1 : 0;
        } else {
            // Not a console. The MSYS2 terminal (mintty) looks like a pipe to
            // Windows programs, but it understands colors and sets TERM.
            cached = (term != NULL && term[0] != '\0') ? 1 : 0;
        }
#else
        cached = isatty(fileno(stdout)) ? 1 : 0;
#endif
    }
    return cached;
}

#define TEST_GREEN (test_use_color() ? "\033[32m" : "")
#define TEST_RED   (test_use_color() ? "\033[31m" : "")
#define TEST_BOLD  (test_use_color() ? "\033[1m" : "")
#define TEST_RESET (test_use_color() ? "\033[0m" : "")

// ---------- Tests ----------

// Fail the current test if _CONDITION_ is false.
#define TEST_ASSERT(_CONDITION_) \
    do { \
        if (!(_CONDITION_)) { \
            printf("%sFAIL%s %s -- %s (%s:%d)\n", TEST_RED, TEST_RESET, \
                   __func__, #_CONDITION_, __FILE__, __LINE__); \
            return -1; \
        } \
    } while (0)

#define TEST_PASS() return 0

#define TEST_CASE(_NAME_) static int _NAME_(void)

// Copy the next name from a comma-separated list like "a, b,\n c" into `out`
// and move `*cursor` past it. TEST_SUITE uses this to print each test's name.
static void test_next_name(const char** cursor, char* out, size_t cap) {
    const char* p = *cursor;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
        p++;
    }
    size_t n = 0;
    while (*p != '\0' && *p != ',' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
        if (n + 1 < cap) {
            out[n++] = *p;
        }
        p++;
    }
    out[n] = '\0';
    while (*p != '\0' && *p != ',') {
        p++;
    }
    if (*p == ',') {
        p++;
    }
    *cursor = p;
}

#define TEST_STRINGIFY(...) #__VA_ARGS__

// Define main() to run every test listed, print PASS or FAIL for each one, and
// report how many passed.
#define TEST_SUITE(...)  \
    int main(void) {  \
        int (*const TESTS[])(void) = { __VA_ARGS__ }; \
        const char* names = TEST_STRINGIFY(__VA_ARGS__); \
        uint32_t count = sizeof(TESTS) / sizeof(*TESTS); \
        uint32_t passed = 0; \
        for (uint32_t index = 0; index < count; ++index) { \
            char name[128]; \
            test_next_name(&names, name, sizeof(name)); \
            if (TESTS[index]() == 0) { \
                printf("%sPASS%s %s\n", TEST_GREEN, TEST_RESET, name); \
                ++passed; \
            } \
        } \
        printf("\n%s%s%u/%u tests passed%s\n", TEST_BOLD, \
               passed == count ? TEST_GREEN : TEST_RED, \
               (unsigned)passed, (unsigned)count, TEST_RESET); \
        return passed == count ? 0 : 1; \
    }
