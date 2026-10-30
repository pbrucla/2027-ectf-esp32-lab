#pragma once

#include "commands.hpp"
#include "crypto.hpp"

// Write to `digest` the SHA-256 hash of `msg` encoded with encode(), with the
// signature field set to all zeros while hashing (so the hash doesn't depend on
// itself). `msg` itself is not changed.
// Returns 0 on success, or -1 if the message can't be encoded.
/// TODO: implement this
int hash_message(const cmd_msg_t* msg, uint8_t digest[SHA256_DIGEST_LEN]);

// Fill `msg->signature` with the hash from hash_message() followed by zeros.
// Returns 0 on success, or -1 if the message can't be encoded.
/// TODO: implement this
int add_integrity_hash(cmd_msg_t* msg);

// Return true if `msg->signature` holds the hash from hash_message() and the
// rest of the field is all zeros.
/// TODO: implement this
bool verify_integrity_hash(const cmd_msg_t* msg);

// Later phases:
// bool verify_firmware_signature(const cmd_msg_t* msg);
// bool verify_firmware_counter(const cmd_msg_t* msg);
