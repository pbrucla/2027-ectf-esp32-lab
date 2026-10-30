#include "security.hpp"

int hash_message(const cmd_msg_t* msg, uint8_t digest[SHA256_DIGEST_LEN]) {
    /// TODO: copy `msg`, zero the copy's signature field, encode the copy, and
    /// hash the encoded bytes into `digest`.
    return -1;
}

int add_integrity_hash(cmd_msg_t* msg) {
    /// TODO: get the hash from hash_message() and store it followed by zeros in
    /// `msg->signature`.
    return -1;
}

bool verify_integrity_hash(const cmd_msg_t* msg) {
    /// TODO: recompute the hash with hash_message() and compare it with
    /// `msg->signature`. Also check that the unused bytes are zero.
    return false;
}
