#include <stdint.h>

// Ed25519 takes 64 bytes
// SHA-256 only takes 32 bytes, so let's just do zeroes for padding
#define NUM_BYTES_IN_SIGNATURE  64
#define MAX_FIRMWARE_SIZE       16

enum cmd_type_t {
    UPDATE = 0,
};

struct firmware_payload_t {
    uint8_t version;
    char firmware[MAX_FIRMWARE_SIZE];
};

union cmd_payload_t {
    firmware_payload_t firmware_payload;
};

struct cmd_msg_t {
    cmd_type_t cmd;
    cmd_payload_t payload;
    uint8_t signature[NUM_BYTES_IN_SIGNATURE];
};

#define MAX_DECODE_LEN 256
#define MAX_ENCODE_LEN 256

/// TODO: implement this
// returns -1 for error or returns how many bytes were encoded (length of out_buf)
// Serializes a cmd_msg_t into a U8 array, potentially adding security measures (like hashes)
int encode(struct cmd_msg_t *in_msg, uint8_t* out_buf);

/// TODO: implement this
// return -1 for error or how many bytes were decoded (length of in_buf)

int decode(struct cmd_msg_t *out_msg, uint8_t* in_buf);
