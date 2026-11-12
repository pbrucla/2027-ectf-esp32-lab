// The Phase 2 hash functions are given (the reference solution). Your Phase 3
// code goes at the bottom.
#include <string.h>
#include "security.hpp"
#include "crypto.hpp"

// The hash covers cmd (1 byte) followed by the payload (17 bytes).
#define HASH_INPUT_LEN  18

int add_integrity_hash(cmd_msg_t* msg) {
    /// TODO: get the hash from sha256() and store it followed by zeros in
    /// `msg->signature`.
    return -1;
}

int verify_integrity_hash(const cmd_msg_t* msg) {
    /// TODO: recompute the hash with sha256() and compare it with
    /// `msg->signature`. Also check that the unused bytes are zero.    
    return -1;
}

// ---------- Phase 3: your code ----------

int add_signature(cmd_msg_t* msg, const uint8_t private_key[ED25519_PRIVATE_KEY_LEN]) {
    /// TODO: sign `cmd` followed by `payload` (the same 18 bytes as the hash)
    /// with ed25519_sign(), writing the signature to `msg->signature`.
    return -1;
}

int verify_signature(const cmd_msg_t* msg, const uint8_t public_key[ED25519_PUBLIC_KEY_LEN]) {
    /// TODO: rebuild the same 18 bytes and check `msg->signature` with
    /// ed25519_verify().
    return -1;
}
