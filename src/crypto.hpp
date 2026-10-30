#pragma once

#include <stddef.h>
#include <stdint.h>

#define SHA256_DIGEST_LEN 32

// Write the SHA-256 hash of the `len` bytes at `data` to `out`.
// Spec: FIPS 180-4, https://doi.org/10.6028/NIST.FIPS.180-4
void sha256(const uint8_t* data, size_t len, uint8_t out[SHA256_DIGEST_LEN]);
