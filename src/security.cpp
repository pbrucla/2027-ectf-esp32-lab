#include "security.hpp"
#include "crypto.hpp"
#include <stdlib.h>

int add_integrity_hash(cmd_msg_t* msg) {
    /// TODO: get the hash from sha256() and store it followed by zeros in
    /// `msg->signature`.
    return -1;
}

bool verify_integrity_hash(const cmd_msg_t* msg) {
    /// TODO: recompute the hash with sha_256() and compare it with
    /// `msg->signature`. Also check that the unused bytes are zero.
    return false;
}
