#include <stdint.h>

// Ed25519 takes 64 bytes
// SHA-256 only takes 32 bytes, so let's just do zeroes for padding
#define NUM_BYTES_IN_SIGNATURE  64

enum cmd_type_t {
    UPDATE = 0,
};

union cmd_payload_t {

};
struct cmd_msg_t {
    cmd_type_t cmd;
    cmd_payload_t payload;
    uint8_t signature[NUM_BYTES_IN_SIGNATURE];
};

#define MAX_DECODE_LEN 256
#define MAX_ENCODE_LEN 256

/// TODO: stop using String type, use uint8_t[] or char[]
// return -1 for error
int encode(struct cmd_msg_t *in_msg, uint8_t* out_buf);

// return -1 for error
int decode(struct cmd_msg_t *out_msg, uint8_t* in_buf);

