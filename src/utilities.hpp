#pragma once
#include <stddef.h>

// void safe_memcpy();
// Functionally the same as std::memcmp, but completes in constant time.
int const_memcmp(const void* a, const void* b, size_t n);

/// void safe_memcpy();
// Functionally the same as std::memcmp, but completes in constant time.
int not_safe_memcmp(const void* a, const void* b, size_t n);
