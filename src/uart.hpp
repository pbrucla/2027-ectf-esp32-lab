#pragma once

#include <Arduino.h>
#include <stdlib.h>

#include "commands.hpp"

#define USER_BAUD 115200
// ---------- Settings (must match on both boards) ----------
#define LINK_BAUD 9600
#define LINK_RX_PIN 16
#define LINK_TX_PIN 17

// Phase 3: a line starting with CMD_PREFIX carries a command, hex-encoded:
// "CMD <hex of the encoded cmd_msg_t>\n". Any other line is a text message.
#define CMD_PREFIX "CMD "
// Long enough for the longest command line.
#define MAX_MESSAGE (sizeof(CMD_PREFIX) - 1 + 2 * MAX_ENCODED_LEN)

void initialize_user_serial();
void initialize_peer_serial();
bool readLine(Stream& port, char* buffer, size_t capacity, size_t* length);
void handle_user_input();
void handle_peer_input();
