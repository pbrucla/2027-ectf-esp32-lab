#pragma once

#include <stdlib.h>
#include <Arduino.h>

#define USER_BAUD    115200
// ---------- Settings (must match on both boards) ----------
#define LINK_BAUD    9600
#define LINK_RX_PIN  16
#define LINK_TX_PIN  17
#define MAX_MESSAGE  128

void initialize_user_serial();
void initialize_peer_serial();
bool readLine(Stream& port, char* buffer, size_t capacity, size_t* length);
void handle_user_input();
void handle_peer_input();
