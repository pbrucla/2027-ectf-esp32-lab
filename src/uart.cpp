#include "uart.hpp"

// Fixed-size buffers. No dynamic allocation.
char typedBuffer[MAX_MESSAGE + 1] = {0};
char receivedBuffer[MAX_MESSAGE + 1] = {0};

uint32_t sentSeq = 0;
uint32_t expectedSeq = 1;

HardwareSerial& peer = Serial2;

void initialize_user_serial() {
    Serial.begin(115200);
}
void initialize_peer_serial() {
    peer.begin(LINK_BAUD, SERIAL_8N1, LINK_RX_PIN, LINK_TX_PIN);
}

// Collect characters from `port` until '\n'.
// Returns true when a complete line is available.
// Never blocks.
bool readLine(Stream& port, char* buffer, size_t capacity) {
    static size_t typedLength = 0;
    static size_t receivedLength = 0;

    // Select the appropriate length tracker for each buffer.
    size_t* length;

    if (buffer == typedBuffer) {
        length = &typedLength;
    } else {
        length = &receivedLength;
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

// True if s contains 1 to 9 decimal digits.
bool isNumber(const char* s) {
    size_t length = strlen(s);

    if (length == 0 || length > 9) {
        return false;
    }

    for (size_t i = 0; i < length; i++) {
        if (s[i] < '0' || s[i] > '9') {
            return false;
        }
    }

    return true;
}

void handle_user_input() {
    if (readLine(Serial, typedBuffer, sizeof(typedBuffer))
        && typedBuffer[0] != '\0') {

        sentSeq++;

        char message[MAX_MESSAGE + 16];

        int length = snprintf(
            message,
            sizeof(message),
            "%lu:%s\n",
            (unsigned long)sentSeq,
            typedBuffer
        );

        if (length > 0 && (size_t)length < sizeof(message)) {
            peer.write((const uint8_t*)message, length);
        }

        Serial.printf("Sent #%lu: %s\n",
                      (unsigned long)sentSeq, typedBuffer);
    }
}

void handle_peer_input() {
    // Read messages from the other board.
    if (readLine(peer, receivedBuffer, sizeof(receivedBuffer))
        && receivedBuffer[0] != '\0') {

        // Find the sequence-number delimiter.
        char* colon = strchr(receivedBuffer, ':');

        if (colon == NULL) {
            Serial.printf(
                "Warning: message has no valid sequence number: %s\n",
                receivedBuffer
            );
            return;
        }

        // Separate sequence number and message text.
        *colon = '\0';
        char* text = colon + 1;

        if (!isNumber(receivedBuffer)) {
            Serial.println("Warning: invalid sequence number");
            return;
        }

        uint32_t seq = (uint32_t)strtoul(receivedBuffer, NULL, 10);

        if (seq == 0) {
            Serial.printf("Received: %s\n", text);
            return;
        }

        if (seq > expectedSeq) {
            Serial.printf(
                "Warning: missed %lu message(s)\n",
                (unsigned long)(seq - expectedSeq)
            );
        } else if (seq < expectedSeq) {
            Serial.println("Warning: duplicate or out-of-order message");
        }

        expectedSeq = seq + 1;

        Serial.printf("Received #%lu: %s\n",
                      (unsigned long)seq, text);
    }

}