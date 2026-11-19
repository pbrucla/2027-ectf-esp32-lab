#include "utilities.hpp"
#include <stdint.h>
#include <string.h>

int const_memcmp(const void* a, const void* b, size_t n) {
    bool same = 0;
    for (size_t i = 0; i < n; i++) {
        same |= ((const uint8_t*) a)[i] ^ ((const uint8_t*) b)[i];
    }
    return (same == 0) ? 0 : -1;
}

/// TO EDIT: this is a bit silly, maybe see if this is necessary at all?
int not_safe_memcmp(const void* a, const void* b, size_t n) {
    return memcmp(a, b, n);
}