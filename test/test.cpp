// Host-side unit tests for the Phase 2 code. Run with `make test`.
#include <string.h>
#include "test.hpp"
#include "commands.hpp"
#include "security.hpp"

// ---------- Helpers ----------

// An UPDATE command with a known version and firmware, and no hash.
static cmd_msg_t sample_update(void) {
    cmd_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.cmd = UPDATE;
    msg.payload.firmware_payload.version = 3;
    memcpy(msg.payload.firmware_payload.firmware, "hello, firmware!", MAX_FIRMWARE_SIZE);
    return msg;
}

// ---------- encode() and decode() ----------

// The byte layout is up to you, so these tests only check behavior that any
// layout must have.

TEST_CASE(encode_fits_buffer) {
    cmd_msg_t msg = sample_update();
    uint8_t buf[MAX_ENCODED_LEN];
    int length = encode(&msg, buf, sizeof(buf));
    TEST_ASSERT(length > 0);
    TEST_ASSERT(length <= MAX_ENCODED_LEN);
    TEST_PASS();
}

TEST_CASE(encode_rejects_small_buffer) {
    cmd_msg_t msg = sample_update();
    uint8_t buf[MAX_ENCODED_LEN];
    int length = encode(&msg, buf, sizeof(buf));
    TEST_ASSERT(length > 0);
    TEST_ASSERT(encode(&msg, buf, (size_t)length - 1) == -1);
    TEST_ASSERT(encode(&msg, buf, 0) == -1);
    TEST_PASS();
}

TEST_CASE(encode_rejects_unknown_command) {
    cmd_msg_t msg = sample_update();
    msg.cmd = (cmd_type_t)7;
    uint8_t buf[MAX_ENCODED_LEN];
    TEST_ASSERT(encode(&msg, buf, sizeof(buf)) == -1);
    TEST_PASS();
}

TEST_CASE(decode_round_trip) {
    cmd_msg_t msg = sample_update();
    msg.signature[0] = 0xAB;
    msg.signature[NUM_BYTES_IN_SIGNATURE - 1] = 0xCD;

    uint8_t buf[MAX_ENCODED_LEN];
    int length = encode(&msg, buf, sizeof(buf));
    TEST_ASSERT(length > 0);

    cmd_msg_t decoded;
    TEST_ASSERT(decode(&decoded, buf, (size_t)length) == length);
    TEST_ASSERT(decoded.cmd == UPDATE);
    TEST_ASSERT(decoded.payload.firmware_payload.version == 3);
    TEST_ASSERT(memcmp(decoded.payload.firmware_payload.firmware,
                       "hello, firmware!", MAX_FIRMWARE_SIZE) == 0);
    TEST_ASSERT(memcmp(decoded.signature, msg.signature, NUM_BYTES_IN_SIGNATURE) == 0);
    TEST_PASS();
}

TEST_CASE(decode_rejects_wrong_length) {
    cmd_msg_t msg = sample_update();
    uint8_t buf[MAX_ENCODED_LEN] = {0};
    int length = encode(&msg, buf, sizeof(buf) - 1);
    TEST_ASSERT(length > 0);

    // A message cut short, one with an extra byte, and an empty one.
    cmd_msg_t decoded;
    TEST_ASSERT(decode(&decoded, buf, (size_t)length - 1) == -1);
    TEST_ASSERT(decode(&decoded, buf, (size_t)length + 1) == -1);
    TEST_ASSERT(decode(&decoded, buf, 0) == -1);
    TEST_PASS();
}

// ---------- Integrity hash ----------

TEST_CASE(integrity_round_trip) {
    cmd_msg_t msg = sample_update();
    TEST_ASSERT(add_integrity_hash(&msg) == 0);
    TEST_ASSERT(verify_integrity_hash(&msg));

    // The hash still checks out after a trip through the wire format.
    uint8_t buf[MAX_ENCODED_LEN];
    int length = encode(&msg, buf, sizeof(buf));
    TEST_ASSERT(length > 0);
    cmd_msg_t received;
    TEST_ASSERT(decode(&received, buf, (size_t)length) == length);
    TEST_ASSERT(verify_integrity_hash(&received));
    TEST_PASS();
}

TEST_CASE(integrity_detects_any_flipped_bit) {
    cmd_msg_t msg = sample_update();
    TEST_ASSERT(add_integrity_hash(&msg) == 0);

    uint8_t original[MAX_ENCODED_LEN];
    int length = encode(&msg, original, sizeof(original));
    TEST_ASSERT(length > 0);

    for (int byte = 0; byte < length; byte++) {
        for (int bit = 0; bit < 8; bit++) {
            uint8_t corrupted[MAX_ENCODED_LEN];
            memcpy(corrupted, original, (size_t)length);
            corrupted[byte] ^= (uint8_t)(1 << bit);

            // Either decode() rejects it, or the hash check must.
            cmd_msg_t received;
            if (decode(&received, corrupted, (size_t)length) != -1) {
                TEST_ASSERT(!verify_integrity_hash(&received));
            }
        }
    }
    TEST_PASS();
}

TEST_CASE(integrity_rejects_missing_hash) {
    cmd_msg_t msg = sample_update();
    TEST_ASSERT(!verify_integrity_hash(&msg));
    TEST_PASS();
}

TEST_CASE(integrity_rejects_nonzero_padding) {
    cmd_msg_t msg = sample_update();
    TEST_ASSERT(add_integrity_hash(&msg) == 0);
    msg.signature[NUM_BYTES_IN_SIGNATURE - 1] = 1;
    TEST_ASSERT(!verify_integrity_hash(&msg));
    TEST_PASS();
}

TEST_CASE(integrity_add_rejects_unknown_command) {
    cmd_msg_t msg = sample_update();
    msg.cmd = (cmd_type_t)7;
    TEST_ASSERT(add_integrity_hash(&msg) == -1);
    TEST_PASS();
}

TEST_SUITE(
    encode_fits_buffer,
    encode_rejects_small_buffer,
    encode_rejects_unknown_command,
    decode_round_trip,
    decode_rejects_wrong_length,
    integrity_round_trip,
    integrity_detects_any_flipped_bit,
    integrity_rejects_missing_hash,
    integrity_rejects_nonzero_padding,
    integrity_add_rejects_unknown_command
)
