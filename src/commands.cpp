// Given in Phase 3: the Phase 2 reference solution for encode() and decode().
#include <string.h>
#include "commands.hpp"

// The wire format for UPDATE (82 bytes). test/server.py builds the same bytes.
//
//   offset  size  field
//   0       1     cmd (0 = UPDATE)
//   1       17    payload (1 byte version + 16 bytes firmware)
//   18      64    signature
#define ENCODED_UPDATE_LEN  82

int encode(const cmd_msg_t* in_msg, uint8_t* out_buf, size_t out_cap) {
    if (in_msg == NULL || out_buf == NULL || in_msg->cmd != UPDATE) {
        return -1;
    }

    if (out_cap < ENCODED_UPDATE_LEN) {
        return -1;
    }

    memcpy(&out_buf[0], &in_msg->cmd, 1);
    memcpy(&out_buf[1], &in_msg->payload, 17);
    memcpy(&out_buf[18], in_msg->signature, NUM_BYTES_IN_SIGNATURE);

    return ENCODED_UPDATE_LEN;
}

int decode(cmd_msg_t* out_msg, const uint8_t* in_buf, size_t in_len) {
    if (out_msg == NULL || in_buf == NULL) {
        return -1;
    }

    // Check the command type and the exact length before reading anything.
    if (in_len != ENCODED_UPDATE_LEN || in_buf[0] != UPDATE) {
        return -1;
    }

    memcpy(&out_msg->cmd, &in_buf[0], 1);
    memcpy(&out_msg->payload, &in_buf[1], 17);
    memcpy(out_msg->signature, &in_buf[18], NUM_BYTES_IN_SIGNATURE);

    return ENCODED_UPDATE_LEN;
}
