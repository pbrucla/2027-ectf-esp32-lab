#include "SecureUart.h"
#include "secrets.h"
#include <Preferences.h>
#include <psa/crypto.h>

// How a packet looks on the wire (sent as hex text, one packet per line):
//
//   | counter (4 bytes) | encrypted message (n bytes) | tag (16 bytes) |
//
// - counter: goes up by one every message. Used to build the nonce and to
//   reject replayed (re-sent) old packets.
// - tag: a checksum only someone with the key can make. If any bit changes in
//   transit, decryption fails.
// Internal details, hidden from sketches so names like `ready` can't clash.
namespace {

const size_t COUNTER_SIZE = 4;
const size_t TAG_SIZE = 16;
const size_t MAX_PACKET = COUNTER_SIZE + SECURE_UART_MAX_MESSAGE + TAG_SIZE;

HardwareSerial* uart = nullptr;
Preferences storage;  // flash storage that survives reboots
mbedtls_svc_key_id_t aesKey = MBEDTLS_SVC_KEY_ID_INIT;
bool ready = false;

uint32_t sentCounter = 0;      // last counter this board sent
uint32_t receivedCounter = 0;  // highest counter this board accepted

// This library is one-way: a board may send OR receive, not both. Two boards
// sending with the same key could pick the same counter, which breaks AES-GCM.
enum Role { NONE, SENDER, RECEIVER };
Role role = NONE;

bool claimRole(Role wanted) {
  if (role == NONE) role = wanted;
  if (role != wanted) {
    Serial.println("SecureUart: a board can only send OR receive, not both");
    return false;
  }
  return true;
}

// The nonce must never repeat for the same key: 8 zero bytes + the 4-byte counter.
void makeNonce(const uint8_t counterBytes[COUNTER_SIZE], uint8_t nonce[12]) {
  memset(nonce, 0, 12);
  memcpy(nonce + 8, counterBytes, COUNTER_SIZE);
}

}  // namespace

bool secureBegin(HardwareSerial& port, int rxPin, int txPin) {
  uart = &port;
  uart->begin(SECURE_UART_BAUD, SERIAL_8N1, rxPin, txPin);

  if (psa_crypto_init() != PSA_SUCCESS) {
    Serial.println("SecureUart: crypto init failed");
    return false;
  }

  psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
  psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
  psa_set_key_bits(&attributes, 256);
  psa_set_key_algorithm(&attributes, PSA_ALG_GCM);
  psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
  if (psa_import_key(&attributes, PSK, sizeof(PSK), &aesKey) != PSA_SUCCESS) {
    Serial.println("SecureUart: key import failed");
    return false;
  }

  storage.begin("secureuart", false);
  sentCounter = storage.getUInt("sent", 0);
  receivedCounter = storage.getUInt("received", 0);

  ready = true;
  return true;
}

bool secureSend(const String& message) {
  if (!ready || !claimRole(SENDER)) return false;

  size_t length = message.length();
  if (length > SECURE_UART_MAX_MESSAGE) {
    Serial.println("SecureUart: message too long");
    return false;
  }

  // Save the new counter BEFORE sending, so a reboot can never reuse it.
  // (Fine at a few messages per second; for much faster sending, save in blocks.)
  sentCounter++;
  storage.putUInt("sent", sentCounter);

  uint8_t packet[MAX_PACKET];
  uint8_t* counterPart = packet;
  uint8_t* encryptedPart = packet + COUNTER_SIZE;  // encrypted message + tag

  counterPart[0] = sentCounter >> 24;
  counterPart[1] = sentCounter >> 16;
  counterPart[2] = sentCounter >> 8;
  counterPart[3] = sentCounter;

  uint8_t nonce[12];
  makeNonce(counterPart, nonce);

  size_t encryptedLength = 0;
  psa_status_t status = psa_aead_encrypt(aesKey, PSA_ALG_GCM,
                                         nonce, sizeof(nonce),
                                         NULL, 0,
                                         (const uint8_t*)message.c_str(), length,
                                         encryptedPart, SECURE_UART_MAX_MESSAGE + TAG_SIZE,
                                         &encryptedLength);
  if (status != PSA_SUCCESS) {
    Serial.println("SecureUart: encryption failed");
    return false;
  }

  // Send as hex text so the packet can never contain a stray '\n'
  size_t packetLength = COUNTER_SIZE + encryptedLength;
  for (size_t i = 0; i < packetLength; i++) {
    uart->printf("%02x", packet[i]);
  }
  uart->print('\n');
  return true;
}

bool secureReceive(String& message) {
  if (!ready || !claimRole(RECEIVER)) return false;
  if (!uart->available()) return false;

  String line = uart->readStringUntil('\n');
  line.trim();

  // 1. Check the size: every byte is 2 hex characters
  size_t hexLength = line.length();
  if (hexLength % 2 != 0 ||
      hexLength < 2 * (COUNTER_SIZE + TAG_SIZE) ||
      hexLength > 2 * MAX_PACKET) {
    Serial.println("SecureUart: rejected (wrong size)");
    return false;
  }

  // 2. Turn the hex text back into bytes
  uint8_t packet[MAX_PACKET];
  size_t packetLength = hexLength / 2;
  for (size_t i = 0; i < packetLength; i++) {
    char pair[3] = { line[2 * i], line[2 * i + 1], '\0' };
    if (!isxdigit(pair[0]) || !isxdigit(pair[1])) {
      Serial.println("SecureUart: rejected (not hex)");
      return false;
    }
    packet[i] = strtoul(pair, NULL, 16);
  }

  uint8_t* counterPart = packet;
  uint8_t* encryptedPart = packet + COUNTER_SIZE;  // encrypted message + tag
  size_t encryptedLength = packetLength - COUNTER_SIZE;

  // 3. Reject old counters (someone re-sending a recorded packet)
  uint32_t counter = ((uint32_t)counterPart[0] << 24) |
                     ((uint32_t)counterPart[1] << 16) |
                     ((uint32_t)counterPart[2] << 8) |
                     counterPart[3];
  if (counter <= receivedCounter) {
    Serial.println("SecureUart: rejected (replayed or old packet)");
    return false;
  }

  // 4. Decrypt. This also checks the tag, so a wrong key or changed data fails.
  uint8_t nonce[12];
  makeNonce(counterPart, nonce);

  uint8_t plaintext[SECURE_UART_MAX_MESSAGE + 1];
  size_t plaintextLength = 0;
  psa_status_t status = psa_aead_decrypt(aesKey, PSA_ALG_GCM,
                                         nonce, sizeof(nonce),
                                         NULL, 0,
                                         encryptedPart, encryptedLength,
                                         plaintext, SECURE_UART_MAX_MESSAGE,
                                         &plaintextLength);
  if (status != PSA_SUCCESS) {
    Serial.println("SecureUart: rejected (tampered data or wrong key)");
    return false;
  }

  // 5. Genuine and new: remember the counter (in flash, so it survives reboots)
  receivedCounter = counter;
  storage.putUInt("received", receivedCounter);

  plaintext[plaintextLength] = '\0';
  message = (const char*)plaintext;
  return true;
}
