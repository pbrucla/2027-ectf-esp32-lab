#define LINK_BAUD    9600
#define LINK_RX_PIN  16
#define LINK_TX_PIN  17
#define MAX_MESSAGE  128   // longest line in bytes; extra characters are dropped

// `Serial`  = USB connection to your computer (the Serial Monitor).
// `peer`    = UART wired to the other board. Serial2 is the ESP32's third UART.
HardwareSerial& peer = Serial2;

// Collects characters from `port` until '\n'. Returns true when a full line is
// in `line`. Never blocks, so the board can send and receive at the same time.
// This function is given to you: read it and make sure you understand it.
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

String typedBuffer;     // what the user is typing in the Serial Monitor
String receivedBuffer;  // what is arriving from the other board

void setup() {
  Serial.begin(115200);

  // TODO 1: Start the `peer` UART at LINK_BAUD using pins LINK_RX_PIN and
  // LINK_TX_PIN. Hint: HardwareSerial::begin(baud, config, rxPin, txPin),
  // where config is SERIAL_8N1 (8 data bits, no parity, 1 stop bit).

  Serial.println("Type a message and press Enter to send it.");
}

void loop() {
  String line;

  if (readLine(Serial, typedBuffer, line) && line.length() > 0) {
    // TODO 2: Send "0:" followed by `line` to the other board, then '\n' so
    // the other board knows where the message ends. The 0 is a placeholder
    // sequence number (used in Part 3). Then print "Sent: <line>" to Serial.
  }

  // TODO 3: Use readLine() to check for a complete line from `peer`.
  // If one arrived (and it is not empty), print "Received: <line>" to Serial.
}
