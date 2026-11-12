#pragma once

#include <stddef.h>
#include <stdint.h>

#define SHA256_DIGEST_LEN 32
#define SHA512_DIGEST_LEN 64

#define ED25519_PRIVATE_KEY_LEN 32
#define ED25519_PUBLIC_KEY_LEN 32
#define ED25519_SIGNATURE_LEN 64

// Write the SHA-256 hash of the `len` bytes at `data` to `out`.
// Spec: FIPS 180-4, https://doi.org/10.6028/NIST.FIPS.180-4
void sha256(const uint8_t* data, size_t len, uint8_t out[SHA256_DIGEST_LEN]);

// Write the SHA-512 hash of the `len` bytes at `data` to `out`.
// Ed25519 uses it internally. Spec: FIPS 180-4 section 6.4.
void sha512(const uint8_t* data, size_t len, uint8_t out[SHA512_DIGEST_LEN]);

// SHA-512 over several pieces of data, for when the input isn't in one buffer:
// call sha512_init(), then sha512_update() once per piece, then sha512_final().
struct sha512_ctx_t {
    uint64_t state[8];
    uint64_t total_len;
    uint8_t block[128];
    size_t block_len;
};
void sha512_init(sha512_ctx_t* ctx);
void sha512_update(sha512_ctx_t* ctx, const uint8_t* data, size_t len);
void sha512_final(sha512_ctx_t* ctx, uint8_t out[SHA512_DIGEST_LEN]);

// ---------- Ed25519 signatures (RFC 8032) ----------
// https://www.rfc-editor.org/rfc/rfc8032
//
// A private key is 32 random bytes. Only the signer has it.
// The public key is computed from the private key and can be shared freely.

// Compute the public key that goes with `private_key`.
void ed25519_public_key(const uint8_t private_key[ED25519_PRIVATE_KEY_LEN],
                        uint8_t public_key[ED25519_PUBLIC_KEY_LEN]);

// Sign the `len` bytes at `msg` with `private_key`, writing 64 bytes to `sig`.
void ed25519_sign(const uint8_t* msg, size_t len,
                  const uint8_t private_key[ED25519_PRIVATE_KEY_LEN],
                  uint8_t sig[ED25519_SIGNATURE_LEN]);

// Returns 0 if `sig` is a valid signature of the `len` bytes at `msg` by the
// owner of `public_key`, or -1 if not.
int ed25519_verify(const uint8_t* msg, size_t len,
                   const uint8_t public_key[ED25519_PUBLIC_KEY_LEN],
                   const uint8_t sig[ED25519_SIGNATURE_LEN]);
