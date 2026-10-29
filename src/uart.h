#include <stdint.h>
#include "security.h"

struct cmd_msg {
    uint8_t cmd;
};

#define MAX_DECODE_LEN 256
#define MAX_ENCODE_LEN 256

/// TODO: stop using STring type, use uint8_t[] or char[]
int encode(struct cmd_msg *in_msg, uint8_t* out_buf);

// return -1 for error
int decode(struct cmd_msg *out_msg, uint8_t* in_buf);