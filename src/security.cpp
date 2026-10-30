#include "security.hpp"

int add_integrity_hash(cmd_msg_t* msg) {
    /// TODO: encode a copy of `msg` with its signature field zeroed, hash the
    /// encoded bytes, and store the hash followed by zeros in `msg->signature`.
    return -1;
}

bool verify_integrity_hash(const cmd_msg_t* msg) {
    /// TODO: recompute the hash and compare it with `msg->signature`. Also
    /// check that the unused bytes are zero.
    return false;
}
