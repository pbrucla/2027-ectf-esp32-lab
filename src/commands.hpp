#pragma once

#include <stddef.h>
#include <stdint.h>

// Room for an Ed25519 signature (64 bytes) in a later phase.
// A SHA-256 hash only fills the first 32 bytes; the rest must be zero.
#define NUM_BYTES_IN_SIGNATURE  64
#define MAX_FIRMWARE_SIZE       16

// A fixed uint8_t base type, so any byte (even an unknown command) is a valid
// cmd_type_t value that encode() and decode() can check and reject.
enum cmd_type_t : uint8_t {
    UPDATE = 0,
};

struct firmware_payload_t {
    uint8_t version;
    char firmware[MAX_FIRMWARE_SIZE];
};

union cmd_payload_t {
    firmware_payload_t firmware_payload;
};

// A command in memory. Use encode() and decode() to convert it to and from
// bytes on the wire.
struct cmd_msg_t {
    cmd_type_t cmd;
    cmd_payload_t payload;
    uint8_t signature[NUM_BYTES_IN_SIGNATURE];
};

// You choose the byte layout (the wire format). decode() just has to
// understand what encode() writes, and an encoded command must fit in
// MAX_ENCODED_LEN bytes.
#define MAX_ENCODED_LEN     256

/// TODO: implement this
// Serializes a cmd_msg_t into a U8 array in your wire format.
// Returns how many bytes were encoded, or -1 if the command type is unknown or
// `out_cap` (the size of out_buf) is too small.
int encode(const cmd_msg_t* in_msg, uint8_t* out_buf, size_t out_cap);

/// TODO: implement this
// Deserializes the `in_len` bytes at in_buf into a cmd_msg_t.
// Returns how many bytes were decoded, or -1 if the command type is unknown or
// `in_len` is not the exact length encode() produces for that command.
int decode(cmd_msg_t* out_msg, const uint8_t* in_buf, size_t in_len);
