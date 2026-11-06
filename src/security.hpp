#pragma once

#include "commands.hpp"

// Fill `msg->signature` with the hash from sha256() followed by zeros.
// Returns 0 on success, or -1 if `msg` is NULL or `msg->cmd` is not a known
// command type.
/// TODO: implement this
int add_integrity_hash(cmd_msg_t* msg);

// Returns 0 if `msg->signature` holds the hash from sha256() and the rest of
// the field is all zeros, or -1 if not (including a NULL `msg` or unknown cmd).
/// TODO: implement this
int verify_integrity_hash(const cmd_msg_t* msg);

// Later phases:
// bool verify_firmware_signature(const cmd_msg_t* msg);
// bool verify_firmware_counter(const cmd_msg_t* msg);
