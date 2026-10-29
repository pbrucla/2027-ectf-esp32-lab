#include "uart.h"

// UART lab reference solution: two-way text messages between two ESP32s over UART.
//
// Flash this same sketch to both boards. Wiring (cross TX/RX, share ground):
//
//   Board A GPIO17 (TX) ---> Board B GPIO16 (RX)
//   Board A GPIO16 (RX) <--- Board B GPIO17 (TX)
//   Board A GND         ---- Board B GND
//
// Type a line in the Serial Monitor (115200 baud) and press Enter to send it.
// Lines arriving from the other board are printed as "Received #<n>: ...".
//
// This is the finished Part 3 version. Messages on the wire look like
// "<sequence number>:<text>\n", and the receiver warns about gaps.

// ---------- Settings (must match on both boards) ----------
#define LINK_BAUD    9600
#define LINK_RX_PIN  16
#define LINK_TX_PIN  17
#define MAX_MESSAGE  128   // longest line in bytes; extra characters are dropped

HardwareSerial& peer = Serial2;  // UART wired to the other board

// Collects characters from `port` until '\n'. Returns true when a full line is
// in `line`. Never blocks, so the board can send and receive at the same time.
bool readLine(Stream& port, String& buffer, String& line) {
  while (port.available()) {
    int c = port.read();
    if (c == '\n') {
      line = buffer;
      line.trim();  // remove extra spaces
      buffer = "";
      return true;
    }
    // Keep only printable text. This drops noise (e.g. while the other board
    // resets or is being uploaded to) and control characters like '\r'.
    if (c < ' ' || c > '~') continue;
    if (buffer.length() < MAX_MESSAGE) buffer += (char)c;
  }
  return false;
}

// True if `s` is 1 to 9 digits and nothing else (9 digits always fits in toInt()).
bool isNumber(const String& s) {
  if (s.length() == 0 || s.length() > 9) return false;
  for (size_t i = 0; i < s.length(); i++) {
    if (!isDigit(s[i])) return false;
  }
  return true;
}

String typedBuffer;     // what the user is typing in the Serial Monitor
String receivedBuffer;  // what is arriving from the other board

uint32_t sentSeq = 0;      // number of the last message this board sent
uint32_t expectedSeq = 1;  // number we expect on the next received message

void setup() {
  Serial.begin(115200);
  peer.begin(LINK_BAUD, SERIAL_8N1, LINK_RX_PIN, LINK_TX_PIN);
  Serial.println("Type a message and press Enter to send it.");
}

void loop() {
  String line;

  if (readLine(Serial, typedBuffer, line) && line.length() > 0) {
    sentSeq++;
    peer.print(String(sentSeq) + ":" + line + "\n");
    Serial.println("Sent #" + String(sentSeq) + ": " + line);
  }

  if (readLine(peer, receivedBuffer, line) && line.length() > 0) {
    // Noise on the wire can add junk to the front of a line, so check the
    // number is really a number before trusting it.
    int colon = line.indexOf(':');
    if (colon < 0 || !isNumber(line.substring(0, colon))) {
      Serial.println("Warning: message has no valid sequence number: " + line);
      return;
    }
    uint32_t seq = line.substring(0, colon).toInt();
    String text = line.substring(colon + 1);

    if (seq == 0) {  // 0 means "no sequence number" (sent by a Part 1 sketch)
      Serial.println("Received: " + text);
      return;
    }

    if (seq > expectedSeq) {
      Serial.println("Warning: missed " + String(seq - expectedSeq) + " message(s)");
    } else if (seq < expectedSeq) {
      Serial.println("Warning: duplicate or out-of-order message");
    }
    expectedSeq = seq + 1;

    Serial.println("Received #" + String(seq) + ": " + text);
  }
}
