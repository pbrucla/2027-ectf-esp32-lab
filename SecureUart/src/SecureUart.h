// SecureUart: encrypted, tamper-proof messages between two ESP32s over UART.
//
// One board sends, the other receives. Both need the same key in secrets.h.
//
//   Sender:    secureBegin(Serial2, 16, 17);  secureSend("hello");
//   Receiver:  secureBegin(Serial2, 16, 17);  String msg; if (secureReceive(msg)) { ... }

#pragma once
#include <Arduino.h>

// ---------- Settings (must match on both boards) ----------
#define SECURE_UART_BAUD         9600
#define SECURE_UART_MAX_MESSAGE  128   // longest message in bytes

// Start the UART and load the key and saved counter.
// Returns false (and prints why) if something went wrong.
bool secureBegin(HardwareSerial& port, int rxPin, int txPin);

// Encrypt and send one message. Returns false if it could not be sent.
bool secureSend(const String& message);

// Check for an incoming message. Returns true and fills `message` only when a
// genuine, new message arrived. Rejected packets print the reason to Serial.
bool secureReceive(String& message);
