#include "uart.hpp"

#include <string.h>

#include "commands.hpp"
#include "hex.hpp"
#include "security.hpp"

// The public key the boards trust, made by `make keys` (test/keygen.py).
#if __has_include("public_key.hpp")
#include "public_key.hpp"
#else
#error \
    "src/public_key.hpp is missing. Run `make keys` first (see docs/phase_3.md)."
#endif

// Fixed-size buffers. No dynamic allocation.
char typedBuffer[MAX_MESSAGE + 1] = {0};
char receivedBuffer[MAX_MESSAGE + 1] = {0};

// How many characters each buffer holds so far. readLine() updates these.
size_t typedLength = 0;
size_t receivedLength = 0;

HardwareSerial& peer = Serial2;

// The firmware this board is "running": the payload of the last accepted
// UPDATE. A real board would write it to flash; we just keep it in memory.
static firmware_payload_t current_firmware = {0, {0}};

void initialize_user_serial() {
    // Commands are long lines, and checking a signature takes a moment, so
    // give the receive buffers room to hold a whole line meanwhile.
    Serial.setRxBufferSize(1024);
    Serial.begin(USER_BAUD);
}
void initialize_peer_serial() {
    peer.setRxBufferSize(1024);
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
    if (readLine(Serial, typedBuffer, sizeof(typedBuffer), &typedLength) &&
        typedBuffer[0] != '\0') {
        peer.write((const uint8_t*)typedBuffer, strlen(typedBuffer));
        peer.write('\n');

        Serial.printf("Sent: %s\n", typedBuffer);
    }
}

// Print `text` locally and send it back to whoever sent the command, so the
// sender (another board, or test/server.py) learns what happened.
static void reply(const char* text) {
    Serial.println(text);
    peer.write((const uint8_t*)text, strlen(text));
    peer.write('\n');
}

static void apply_update(const firmware_payload_t* update) {
    current_firmware = *update;

    char line[64];
    snprintf(line, sizeof(line), "ACCEPTED: firmware v%u \"%.*s\"",
             (unsigned)current_firmware.version, MAX_FIRMWARE_SIZE,
             current_firmware.firmware);
    reply(line);
}

static void auth_check(const auth_check_payload_t* payload) {
    if (memcmp(PASSCODE, payload->passcode, PASSCODE_SIZE) == 0) {
        reply("SUCCESS.");
    } else {
        reply("REJECTED.");
    }
}

// Run a command line's hex text (the part after CMD_PREFIX). Nothing in it is
// trusted until decode() and verify_signature() both succeed.
static void handle_command(const char* hex) {
    uint8_t bytes[MAX_ENCODED_LEN];
    int length = hex_decode(hex, bytes, sizeof(bytes));
    if (length < 0) {
        reply("REJECTED: not valid hex");
        return;
    }

    cmd_msg_t msg = {};
    if (decode(&msg, bytes, (size_t)length) < 0) {
        reply("REJECTED: not a valid command");
        return;
    }

    if (verify_signature(&msg, PUBLIC_KEY) != 0) {
        reply("REJECTED: bad signature");
        return;
    }

    switch (msg.cmd) {
        case UPDATE:
            apply_update(&msg.payload.firmware_payload);
            break;
        case AUTH_CHECK:
            auth_check(&msg.payload.auth_payload);
            break;
        default:
            // decode() already rejects unknown commands, but stay safe.
            reply("REJECTED: unknown command");
            break;
    }
}

void handle_peer_input() {
    // Read messages from the other board.
    if (readLine(peer, receivedBuffer, sizeof(receivedBuffer),
                 &receivedLength) &&
        receivedBuffer[0] != '\0') {
        Serial.printf("Received: %s\n", receivedBuffer);

        if (strncmp(receivedBuffer, CMD_PREFIX, strlen(CMD_PREFIX)) == 0) {
            handle_command(receivedBuffer + strlen(CMD_PREFIX));
        }
    }
}
