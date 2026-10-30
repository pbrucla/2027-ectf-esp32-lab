#pragma once

#include "commands.hpp"
#include "crypto.hpp"

// Fill `msg->signature` with a SHA-256 hash followed by zeros. The hash covers
// the whole message encoded with encode(), with the signature field set to all
// zeros while hashing (so the hash doesn't depend on itself).
// Returns 0 on success, or -1 if the message can't be encoded.
/// TODO: implement this
int add_integrity_hash(cmd_msg_t* msg);

// Return true if `msg->signature` holds the correct hash (as written by
// add_integrity_hash()) and the rest of the field is all zeros.
/// TODO: implement this
bool verify_integrity_hash(const cmd_msg_t* msg);

// Later phases:
// bool verify_firmware_signature(const cmd_msg_t* msg);
// bool verify_firmware_counter(const cmd_msg_t* msg);
