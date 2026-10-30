#include "uart.hpp"

// Fixed-size buffers. No dynamic allocation.
char typedBuffer[MAX_MESSAGE + 1] = {0};
char receivedBuffer[MAX_MESSAGE + 1] = {0};

// How many characters each buffer holds so far. readLine() updates these.
size_t typedLength = 0;
size_t receivedLength = 0;

HardwareSerial& peer = Serial2;

void initialize_user_serial() {
    Serial.begin(115200);
}
void initialize_peer_serial() {
    peer.begin(LINK_BAUD, SERIAL_8N1, LINK_RX_PIN, LINK_TX_PIN);
}

// Collect characters from `port` into `buffer` until '\n'.
// `length` counts the characters in `buffer` so far. Each buffer needs its own
// length variable that keeps its value between calls.
// Returns true when a complete line is available.
// Never blocks.
bool readLine(Stream& port, char* buffer, size_t capacity, size_t* length) {
    if (capacity == 0) {
        return false;
    }

    while (port.available()) {
        int c = port.read();

        if (c == '\n') {
            // Remove trailing carriage return and terminate.
            while (*length > 0 && buffer[*length - 1] == '\r') {
                (*length)--;
            }

            buffer[*length] = '\0';
            *length = 0;
            return true;
        }

        // Keep only printable ASCII characters.
        if (c < ' ' || c > '~') {
            continue;
        }

        // Leave space for the null terminator.
        if (*length < capacity - 1) {
            buffer[(*length)++] = (char)c;
        }
    }

    return false;
}

void handle_user_input() {
    if (readLine(Serial, typedBuffer, sizeof(typedBuffer), &typedLength)
        && typedBuffer[0] != '\0') {

        peer.write((const uint8_t*)typedBuffer, strlen(typedBuffer));
        peer.write('\n');

        Serial.printf("Sent: %s\n", typedBuffer);
    }
}

void handle_peer_input() {
    // Read messages from the other board.
    if (readLine(peer, receivedBuffer, sizeof(receivedBuffer), &receivedLength)
        && receivedBuffer[0] != '\0') {

        Serial.printf("Received: %s\n", receivedBuffer);
    }
}
