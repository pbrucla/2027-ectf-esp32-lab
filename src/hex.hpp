#pragma once

#include <stddef.h>
#include <stdint.h>

// Helpers for sending bytes over a text link. readLine() in uart.cpp drops
// bytes that aren't printable, so commands go over the wire as hex text: each
// byte becomes two characters from 0-9 and a-f.

// Write the `len` bytes at `in` as 2 * `len` lowercase hex characters plus a
// '\0' to `out`. Returns the number of characters written (without the '\0'),
// or -1 if `out_cap` (the size of out) is too small.
int hex_encode(const uint8_t* in, size_t len, char* out, size_t out_cap);

// Parse the hex string `in` (upper or lower case) into bytes. Returns the
// number of bytes written to `out`, or -1 if `in` has an odd length, a
// character that isn't hex, or more bytes than `out_cap`.
int hex_decode(const char* in, uint8_t* out, size_t out_cap);
