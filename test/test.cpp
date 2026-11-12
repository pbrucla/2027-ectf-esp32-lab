// Host-side unit tests for the Phase 2 and Phase 3 code. Run with `make test`.
#include <string.h>
#include "test.hpp"
#include "interop_vectors.hpp"
#include "../src/commands.hpp"
#include "../src/crypto.hpp"
#include "../src/hex.hpp"
#include "../src/security.hpp"

// ---------- Helpers ----------

// An UPDATE command with a known version and firmware, and no hash.
static cmd_msg_t sample_update(void) {
    cmd_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.cmd = UPDATE;
    msg.payload.firmware_payload.version = 3;
    memcpy(msg.payload.firmware_payload.firmware, "hello, firmware!", MAX_FIRMWARE_SIZE);
    return msg;
}

// A fixed private key for the tests, and a second one to play the attacker.
// The real key comes from test/keygen.py and never appears in the code.
static const uint8_t TEST_PRIVATE_KEY[ED25519_PRIVATE_KEY_LEN] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
    0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
};
static const uint8_t ATTACKER_PRIVATE_KEY[ED25519_PRIVATE_KEY_LEN] = {
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa,
    0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5,
    0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
};

static void test_public_key(uint8_t out[ED25519_PUBLIC_KEY_LEN]) {
    ed25519_public_key(TEST_PRIVATE_KEY, out);
}

// Parse a hex string the test knows is valid.
static void from_hex(const char* hex, uint8_t* out, size_t len) {
    hex_decode(hex, out, len);
}

// ---------- Integrity hash ----------
TEST_CASE(integrity_valid_hash) {
    cmd_msg_t msg = sample_update();
    TEST_ASSERT(add_integrity_hash(&msg) == 0);
    TEST_ASSERT(verify_integrity_hash(&msg) == 0);
    TEST_PASS();
}

TEST_CASE(integrity_round_trip) {
    cmd_msg_t msg = sample_update();
    TEST_ASSERT(add_integrity_hash(&msg) == 0);
    TEST_ASSERT(verify_integrity_hash(&msg) == 0);

    // The hash still checks out after a trip through the wire format.
    uint8_t buf[MAX_ENCODED_LEN];
    int length = encode(&msg, buf, sizeof(buf));
    TEST_ASSERT(length > 0);
    cmd_msg_t received;
    TEST_ASSERT(decode(&received, buf, (size_t)length) == length);
    TEST_ASSERT(verify_integrity_hash(&received) == 0);
    TEST_PASS();
}

TEST_CASE(integrity_detects_any_flipped_bit) {
    cmd_msg_t msg = sample_update();
    TEST_ASSERT(add_integrity_hash(&msg) == 0);

    uint8_t original[MAX_ENCODED_LEN];
    int length = encode(&msg, original, sizeof(original));
    TEST_ASSERT(length > 0);

    for (int byte = 0; byte < length; byte++) {
        for (int bit = 0; bit < 8; bit++) {
            uint8_t corrupted[MAX_ENCODED_LEN];
            memcpy(corrupted, original, (size_t)length);
            corrupted[byte] ^= (uint8_t)(1 << bit);

            // Either decode() rejects it, or the hash check must.
            cmd_msg_t received;
            if (decode(&received, corrupted, (size_t)length) != -1) {
                TEST_ASSERT(verify_integrity_hash(&received) == -1);
            }
        }
    }
    TEST_PASS();
}

TEST_CASE(integrity_rejects_missing_hash) {
    cmd_msg_t msg = sample_update();
    TEST_ASSERT(verify_integrity_hash(&msg) == -1);
    TEST_PASS();
}

TEST_CASE(integrity_rejects_nonzero_padding) {
    cmd_msg_t msg = sample_update();
    TEST_ASSERT(add_integrity_hash(&msg) == 0);
    msg.signature[NUM_BYTES_IN_SIGNATURE - 1] = 1;
    TEST_ASSERT(verify_integrity_hash(&msg) == -1);
    TEST_PASS();
}

TEST_CASE(integrity_add_rejects_unknown_command) {
    cmd_msg_t msg = sample_update();
    msg.cmd = (cmd_type_t)7;
    TEST_ASSERT(add_integrity_hash(&msg) == -1);
    TEST_PASS();
}

// ---------- encode() and decode() ----------
// Given in Phase 3. These check behavior that any byte layout must have.
TEST_CASE(encode_fits_buffer) {
    cmd_msg_t msg = sample_update();
    uint8_t buf[MAX_ENCODED_LEN];
    int length = encode(&msg, buf, sizeof(buf));
    TEST_ASSERT(length > 0);
    TEST_ASSERT(length <= MAX_ENCODED_LEN);
    TEST_PASS();
}

TEST_CASE(encode_rejects_small_buffer) {
    cmd_msg_t msg = sample_update();
    uint8_t buf[MAX_ENCODED_LEN];
    int length = encode(&msg, buf, sizeof(buf));
    TEST_ASSERT(length > 0);
    TEST_ASSERT(encode(&msg, buf, (size_t)length - 1) == -1);
    TEST_ASSERT(encode(&msg, buf, 0) == -1);
    TEST_PASS();
}

TEST_CASE(encode_rejects_unknown_command) {
    cmd_msg_t msg = sample_update();
    msg.cmd = (cmd_type_t)7;
    uint8_t buf[MAX_ENCODED_LEN];
    TEST_ASSERT(encode(&msg, buf, sizeof(buf)) == -1);
    TEST_PASS();
}

TEST_CASE(decode_round_trip) {
    cmd_msg_t msg = sample_update();
    msg.signature[0] = 0xAB;
    msg.signature[NUM_BYTES_IN_SIGNATURE - 1] = 0xCD;

    uint8_t buf[MAX_ENCODED_LEN];
    int length = encode(&msg, buf, sizeof(buf));
    TEST_ASSERT(length > 0);

    cmd_msg_t decoded;
    TEST_ASSERT(decode(&decoded, buf, (size_t)length) == length);
    TEST_ASSERT(decoded.cmd == UPDATE);
    TEST_ASSERT(decoded.payload.firmware_payload.version == 3);
    TEST_ASSERT(memcmp(decoded.payload.firmware_payload.firmware,
                       "hello, firmware!", MAX_FIRMWARE_SIZE) == 0);
    TEST_ASSERT(memcmp(decoded.signature, msg.signature, NUM_BYTES_IN_SIGNATURE) == 0);
    TEST_PASS();
}

TEST_CASE(decode_rejects_wrong_length) {
    cmd_msg_t msg = sample_update();
    uint8_t buf[MAX_ENCODED_LEN] = {0};
    int length = encode(&msg, buf, sizeof(buf) - 1);
    TEST_ASSERT(length > 0);

    // A message cut short, one with an extra byte, and an empty one.
    cmd_msg_t decoded;
    TEST_ASSERT(decode(&decoded, buf, (size_t)length - 1) == -1);
    TEST_ASSERT(decode(&decoded, buf, (size_t)length + 1) == -1);
    TEST_ASSERT(decode(&decoded, buf, 0) == -1);
    TEST_PASS();
}

// ---------- Given crypto: SHA-512 and Ed25519 ----------
// These check the given code in crypto.cpp and ed25519.cpp against published
// test vectors. They should pass before you write anything.
TEST_CASE(crypto_sha512_known_answers) {
    uint8_t expected[SHA512_DIGEST_LEN];
    uint8_t digest[SHA512_DIGEST_LEN];

    // FIPS 180-4 example: SHA-512("abc")
    from_hex("ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
             "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f",
             expected, sizeof(expected));
    sha512((const uint8_t*)"abc", 3, digest);
    TEST_ASSERT(memcmp(digest, expected, SHA512_DIGEST_LEN) == 0);

    // SHA-512 of nothing
    from_hex("cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce"
             "47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e",
             expected, sizeof(expected));
    sha512(NULL, 0, digest);
    TEST_ASSERT(memcmp(digest, expected, SHA512_DIGEST_LEN) == 0);
    TEST_PASS();
}

TEST_CASE(crypto_ed25519_rfc8032_vectors) {
    // RFC 8032 section 7.1, TEST 1 (empty message) and TEST 2 (one byte).
    struct {
        const char* private_key;
        const char* public_key;
        const char* message;
        const char* signature;
    } vectors[] = {
        {"9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60",
         "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a", "",
         "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e06522490155"
         "5fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b"},
        {"4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb",
         "3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c", "72",
         "92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da"
         "085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00"},
    };

    for (size_t v = 0; v < sizeof(vectors) / sizeof(*vectors); v++) {
        uint8_t private_key[ED25519_PRIVATE_KEY_LEN], public_key[ED25519_PUBLIC_KEY_LEN];
        uint8_t expected_public[ED25519_PUBLIC_KEY_LEN];
        uint8_t message[1];
        uint8_t expected_sig[ED25519_SIGNATURE_LEN], sig[ED25519_SIGNATURE_LEN];
        size_t message_len = strlen(vectors[v].message) / 2;

        from_hex(vectors[v].private_key, private_key, sizeof(private_key));
        from_hex(vectors[v].public_key, expected_public, sizeof(expected_public));
        from_hex(vectors[v].message, message, sizeof(message));
        from_hex(vectors[v].signature, expected_sig, sizeof(expected_sig));

        ed25519_public_key(private_key, public_key);
        TEST_ASSERT(memcmp(public_key, expected_public, ED25519_PUBLIC_KEY_LEN) == 0);

        ed25519_sign(message, message_len, private_key, sig);
        TEST_ASSERT(memcmp(sig, expected_sig, ED25519_SIGNATURE_LEN) == 0);
        TEST_ASSERT(ed25519_verify(message, message_len, public_key, sig) == 0);

        sig[0] ^= 1;
        TEST_ASSERT(ed25519_verify(message, message_len, public_key, sig) == -1);
    }
    TEST_PASS();
}

// ---------- Hex helpers ----------
TEST_CASE(hex_round_trip) {
    const uint8_t bytes[] = {0x00, 0x01, 0x7f, 0x80, 0xab, 0xff};
    char text[2 * sizeof(bytes) + 1];
    TEST_ASSERT(hex_encode(bytes, sizeof(bytes), text, sizeof(text)) == 12);
    TEST_ASSERT(strcmp(text, "00017f80abff") == 0);

    uint8_t back[sizeof(bytes)];
    TEST_ASSERT(hex_decode(text, back, sizeof(back)) == (int)sizeof(bytes));
    TEST_ASSERT(memcmp(back, bytes, sizeof(bytes)) == 0);
    TEST_ASSERT(hex_decode("ABff", back, sizeof(back)) == 2);
    TEST_ASSERT(back[0] == 0xab && back[1] == 0xff);
    TEST_PASS();
}

TEST_CASE(hex_rejects_bad_input) {
    uint8_t out[4];
    char text[8];
    TEST_ASSERT(hex_decode("abc", out, sizeof(out)) == -1);        // odd length
    TEST_ASSERT(hex_decode("zz", out, sizeof(out)) == -1);         // not hex
    TEST_ASSERT(hex_decode("0011223344", out, sizeof(out)) == -1); // too long
    TEST_ASSERT(hex_encode(out, 4, text, sizeof(text)) == -1);     // no room for '\0'
    TEST_PASS();
}

// ---------- Signatures ----------
TEST_CASE(signature_valid) {
    uint8_t public_key[ED25519_PUBLIC_KEY_LEN];
    test_public_key(public_key);

    cmd_msg_t msg = sample_update();
    TEST_ASSERT(add_signature(&msg, TEST_PRIVATE_KEY) == 0);
    TEST_ASSERT(verify_signature(&msg, public_key) == 0);
    TEST_PASS();
}

TEST_CASE(signature_signs_cmd_and_payload) {
    // add_signature() must sign exactly `cmd` then `payload` (18 bytes), the
    // same bytes test/server.py signs. Otherwise the board rejects its updates.
    cmd_msg_t msg = sample_update();
    TEST_ASSERT(add_signature(&msg, TEST_PRIVATE_KEY) == 0);

    uint8_t data[18];
    data[0] = UPDATE;
    data[1] = 3;
    memcpy(&data[2], "hello, firmware!", MAX_FIRMWARE_SIZE);
    uint8_t expected[ED25519_SIGNATURE_LEN];
    ed25519_sign(data, sizeof(data), TEST_PRIVATE_KEY, expected);

    TEST_ASSERT(memcmp(msg.signature, expected, NUM_BYTES_IN_SIGNATURE) == 0);
    TEST_PASS();
}

TEST_CASE(signature_round_trip) {
    uint8_t public_key[ED25519_PUBLIC_KEY_LEN];
    test_public_key(public_key);

    cmd_msg_t msg = sample_update();
    TEST_ASSERT(add_signature(&msg, TEST_PRIVATE_KEY) == 0);

    // The signature still checks out after a trip through the wire format.
    uint8_t buf[MAX_ENCODED_LEN];
    int length = encode(&msg, buf, sizeof(buf));
    TEST_ASSERT(length > 0);
    cmd_msg_t received;
    TEST_ASSERT(decode(&received, buf, (size_t)length) == length);
    TEST_ASSERT(verify_signature(&received, public_key) == 0);
    TEST_PASS();
}

TEST_CASE(signature_detects_any_flipped_bit) {
    uint8_t public_key[ED25519_PUBLIC_KEY_LEN];
    test_public_key(public_key);

    cmd_msg_t msg = sample_update();
    TEST_ASSERT(add_signature(&msg, TEST_PRIVATE_KEY) == 0);

    uint8_t original[MAX_ENCODED_LEN];
    int length = encode(&msg, original, sizeof(original));
    TEST_ASSERT(length > 0);

    for (int byte = 0; byte < length; byte++) {
        for (int bit = 0; bit < 8; bit++) {
            uint8_t corrupted[MAX_ENCODED_LEN];
            memcpy(corrupted, original, (size_t)length);
            corrupted[byte] ^= (uint8_t)(1 << bit);

            // Either decode() rejects it, or the signature check must.
            cmd_msg_t received;
            if (decode(&received, corrupted, (size_t)length) != -1) {
                TEST_ASSERT(verify_signature(&received, public_key) == -1);
            }
        }
    }
    TEST_PASS();
}

TEST_CASE(signature_rejects_missing_signature) {
    uint8_t public_key[ED25519_PUBLIC_KEY_LEN];
    test_public_key(public_key);

    cmd_msg_t msg = sample_update();
    TEST_ASSERT(verify_signature(&msg, public_key) == -1);
    TEST_PASS();
}

TEST_CASE(signature_rejects_wrong_key) {
    uint8_t public_key[ED25519_PUBLIC_KEY_LEN];
    test_public_key(public_key);

    // Someone without our private key signs an update with their own key.
    cmd_msg_t msg = sample_update();
    TEST_ASSERT(add_signature(&msg, ATTACKER_PRIVATE_KEY) == 0);
    TEST_ASSERT(verify_signature(&msg, public_key) == -1);
    TEST_PASS();
}

TEST_CASE(signature_stops_forged_update) {
    // The point of Phase 3: anyone can compute a hash, but only the holder of
    // the private key can make a signature.
    uint8_t public_key[ED25519_PUBLIC_KEY_LEN];
    test_public_key(public_key);

    cmd_msg_t msg = sample_update();
    memcpy(msg.payload.firmware_payload.firmware, "evil firmware!!!", MAX_FIRMWARE_SIZE);

    // The attacker's hash passes the Phase 2 check...
    TEST_ASSERT(add_integrity_hash(&msg) == 0);
    TEST_ASSERT(verify_integrity_hash(&msg) == 0);
    // ...but not the signature check.
    TEST_ASSERT(verify_signature(&msg, public_key) == -1);
    TEST_PASS();
}

TEST_CASE(signature_rejects_unknown_command) {
    uint8_t public_key[ED25519_PUBLIC_KEY_LEN];
    test_public_key(public_key);

    cmd_msg_t msg = sample_update();
    msg.cmd = (cmd_type_t)7;
    TEST_ASSERT(add_signature(&msg, TEST_PRIVATE_KEY) == -1);
    TEST_ASSERT(verify_signature(&msg, public_key) == -1);
    TEST_PASS();
}

TEST_CASE(signature_rejects_null) {
    uint8_t public_key[ED25519_PUBLIC_KEY_LEN];
    test_public_key(public_key);

    cmd_msg_t msg = sample_update();
    TEST_ASSERT(add_signature(NULL, TEST_PRIVATE_KEY) == -1);
    TEST_ASSERT(add_signature(&msg, NULL) == -1);
    TEST_ASSERT(verify_signature(NULL, public_key) == -1);
    TEST_ASSERT(add_signature(&msg, TEST_PRIVATE_KEY) == 0);
    TEST_ASSERT(verify_signature(&msg, NULL) == -1);
    TEST_PASS();
}

// ---------- server.py to board ----------
// SERVER_VALID_UPDATE (interop_vectors.hpp) is the exact line test/server.py
// sends for sample_update(), signed with TEST_PRIVATE_KEY. Signing and
// encoding the same message in C must give the same bytes, or the board would
// reject server.py's updates.
TEST_CASE(interop_encode_matches_server) {
    // Ed25519 signatures are deterministic, so the bytes match exactly.
    cmd_msg_t msg = sample_update();
    TEST_ASSERT(add_signature(&msg, TEST_PRIVATE_KEY) == 0);

    uint8_t ours[MAX_ENCODED_LEN];
    int length = encode(&msg, ours, sizeof(ours));
    TEST_ASSERT(length > 0);

    uint8_t theirs[MAX_ENCODED_LEN];
    int server_length = hex_decode(SERVER_VALID_UPDATE + strlen("CMD "),
                                   theirs, sizeof(theirs));
    TEST_ASSERT(length == server_length);
    TEST_ASSERT(memcmp(ours, theirs, (size_t)length) == 0);
    TEST_PASS();
}

TEST_SUITE(
    integrity_valid_hash,
    integrity_round_trip,
    integrity_detects_any_flipped_bit,
    integrity_rejects_missing_hash,
    integrity_rejects_nonzero_padding,
    integrity_add_rejects_unknown_command,
    encode_fits_buffer,
    encode_rejects_small_buffer,
    encode_rejects_unknown_command,
    decode_round_trip,
    decode_rejects_wrong_length,
    crypto_sha512_known_answers,
    crypto_ed25519_rfc8032_vectors,
    hex_round_trip,
    hex_rejects_bad_input,
    signature_valid,
    signature_signs_cmd_and_payload,
    signature_round_trip,
    signature_detects_any_flipped_bit,
    signature_rejects_missing_signature,
    signature_rejects_wrong_key,
    signature_stops_forged_update,
    signature_rejects_unknown_command,
    signature_rejects_null,
    interop_encode_matches_server,
)
