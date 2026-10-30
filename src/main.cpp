#include <stdio.h>
#include <string.h>
#include "uart.hpp"

// UART: two-way text messages between two ESP32s.
//
// Flash this same sketch to both boards. Wiring (cross TX/RX, share ground):
//
//   Board A GPIO17 (TX) ---> Board B GPIO16 (RX)
//   Board A GPIO16 (RX) <--- Board B GPIO17 (TX)
//   Board A GND         ---- Board B GND
//
// Messages on the wire look like "<text>\n".

void setup() {
    initialize_user_serial();
    initialize_peer_serial();

    Serial.println("Type a message and press Enter to send it.");
}

void loop() {
    handle_user_input();
    handle_peer_input();
}
