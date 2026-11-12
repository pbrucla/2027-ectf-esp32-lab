#include <string.h>
#include "hex.hpp"

static int hex_value(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

int hex_encode(const uint8_t* in, size_t len, char* out, size_t out_cap) {
    static const char DIGITS[] = "0123456789abcdef";

    if (out == NULL || (in == NULL && len > 0) || out_cap < 2 * len + 1) {
        return -1;
    }

    for (size_t i = 0; i < len; i++) {
        out[2 * i] = DIGITS[in[i] >> 4];
        out[2 * i + 1] = DIGITS[in[i] & 0x0f];
    }
    out[2 * len] = '\0';
    return (int)(2 * len);
}

int hex_decode(const char* in, uint8_t* out, size_t out_cap) {
    if (in == NULL || out == NULL) {
        return -1;
    }

    size_t chars = strlen(in);
    if (chars % 2 != 0 || chars / 2 > out_cap) {
        return -1;
    }

    for (size_t i = 0; i < chars / 2; i++) {
        int high = hex_value(in[2 * i]);
        int low = hex_value(in[2 * i + 1]);
        if (high < 0 || low < 0) {
            return -1;
        }
        out[i] = (uint8_t)((high << 4) | low);
    }
    return (int)(chars / 2);
}
