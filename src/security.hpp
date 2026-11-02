#pragma once

#include "commands.hpp"

// Fill `msg->signature` with the hash from sha256() followed by zeros.
// Returns 0 on success, or -1 if the message can't be encoded.
/// TODO: implement this
int add_integrity_hash(cmd_msg_t* msg);

// Return true if `msg->signature` holds the hash from sha_256() and the
// rest of the field is all zeros.
/// TODO: implement this
bool verify_integrity_hash(const cmd_msg_t* msg);

// Later phases:
// bool verify_firmware_signature(const cmd_msg_t* msg);
// bool verify_firmware_counter(const cmd_msg_t* msg);
