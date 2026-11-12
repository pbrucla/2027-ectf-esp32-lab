#pragma once

#include "commands.hpp"
#include "crypto.hpp"

// ---------- Phase 2: integrity hash ----------

// Fill `msg->signature` with the hash from sha256() followed by zeros.
// Returns 0 on success, or -1 if `msg` is NULL or `msg->cmd` is not a known
// command type.
int add_integrity_hash(cmd_msg_t* msg);

// Returns 0 if `msg->signature` holds the hash from sha256() and the rest of
// the field is all zeros, or -1 if not (including a NULL `msg` or unknown cmd).
int verify_integrity_hash(const cmd_msg_t* msg);

// ---------- Phase 3: signatures ----------

// Sign the same 18 bytes the hash covered (`cmd`, then `payload`) with
// ed25519_sign() and `private_key`, and write the 64-byte signature to
// `msg->signature`.
// Returns 0 on success, or -1 if `msg` or `private_key` is NULL or `msg->cmd`
// is not a known command type.
int add_signature(cmd_msg_t* msg,
                  const uint8_t private_key[ED25519_PRIVATE_KEY_LEN]);

// Returns 0 if `msg->signature` is a valid signature of `cmd` and `payload`
// by the owner of `public_key` (checked with ed25519_verify()), or -1 if not
// (including a NULL `msg` or `public_key`, or an unknown cmd).
int verify_signature(const cmd_msg_t* msg,
                     const uint8_t public_key[ED25519_PUBLIC_KEY_LEN]);

// Later phases:
// bool verify_firmware_counter(const cmd_msg_t* msg);
