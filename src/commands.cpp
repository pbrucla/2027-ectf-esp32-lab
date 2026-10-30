#include "commands.hpp"

int encode(const cmd_msg_t* in_msg, uint8_t* out_buf, size_t out_cap) {
    /// TODO: choose a byte layout and write each field of `in_msg` to
    /// `out_buf` in it.
    return -1;
}

int decode(cmd_msg_t* out_msg, const uint8_t* in_buf, size_t in_len) {
    /// TODO: check the command type and length, then read each field from
    /// `in_buf` into `out_msg`.
    return -1;
}
