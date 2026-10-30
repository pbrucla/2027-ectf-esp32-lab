# Phase 2: Integrity Checks!

In Phase 1, the receiver printed whatever arrived. If noise flipped a bit on
the wire, it printed the wrong text and had no way to know. That's annoying
for chat messages. It's dangerous for a **firmware update**, where one wrong
byte can brick a device.

Like seen before in the [Python Crypto lab](https://github.com/pbrucla/2027-ectf-crypto-lab), we want to implement hashes to verify integrity! For this week, we're only focused on hashes since I figured C/C++ development is a bit of a learning curve. If you're more familiar with C/C++, feel free to go ahead and implement signatures, and replay protection as we have done in the Python lab.

## The code

Here we're focusing on `add_integrity_hash()` and `verify_integrity_hash()` in [security.cpp](../src/security.cpp). Implement these functions and also implement `encode()` and `decode()` in [commands.cpp](../src/commands.cpp).

| File | What it holds |
| --- | --- |
| [src/commands.hpp](../src/commands.hpp) | The `cmd_msg_t` command struct, its wire format, `encode()` and `decode()` |
| [src/crypto.hpp](../src/crypto.hpp) | `sha256()` (given, in `crypto.cpp`) |
| [src/security.hpp](../src/security.hpp) | `add_integrity_hash()` and `verify_integrity_hash()` |
| [test/test.cpp](../test/test.cpp) | Unit tests |

Read the headers first. Each function you write has a `TODO`.

## Running the tests

You have already implemented hash checks before in the cryptography lab, but implementing serialization and deserialization in `encode()` and `decode()` will certainly be more challenging. We wrote some unit tests so you may verify your solution.

The tests build with your computer's C++ compiler, not the ESP32 toolchain.
You need `make` and a C++ compiler:

- **macOS:** run `xcode-select --install`.
- **Ubuntu / WSL:** run `sudo apt install build-essential`.
- **Windows:** install [MSYS2](https://www.msys2.org/), open the
  **MSYS2 UCRT64** terminal from the Start menu, and run:

  ```bash
  pacman -S make mingw-w64-ucrt-x86_64-gcc
  ```

  Run `make test` from that same MSYS2 UCRT64 terminal. Your Windows drives
  are under `/c/`, `/d/` and so on, so `cd` there first, for example
  `cd /c/Users/<you>/Documents/2027-ectf-esp32-lab`. PowerShell and Command
  Prompt won't work, because the Makefile uses Unix commands.

Then, from the repository folder:

```bash
make test
```

Each failing check prints a `FAIL` line with the test name and the line
number, and the last line says how many tests passed. Right now some of them
fail. By the end of this phase, all of them should pass.

## Part 1: SHA-256

A **hash function** turns any amount of data into a short, fixed-size value
(32 bytes for SHA-256). Changing even one bit of the input changes the output
completely, and nobody knows how to find two inputs with the same hash.

`sha256()` is already written for you in [src/crypto.cpp](../src/crypto.cpp).
You only need to call it:

```cpp
uint8_t digest[SHA256_DIGEST_LEN];
sha256(data, length, digest);
```

If you're curious how it works, read it alongside section 6.2 of the standard,
[FIPS 180-4](https://doi.org/10.6028/NIST.FIPS.180-4).

## Part 2: `encode()` and `decode()`

```cpp
// Returns the number of bytes written, or -1.
int encode(const cmd_msg_t* in_msg, uint8_t* out_buf, size_t out_cap);
// Returns the number of bytes decoded, or -1 if the bytes aren't a valid command.
int decode(cmd_msg_t* out_msg, const uint8_t* in_buf, size_t in_len);
```

Some thoughts about `encode()` and `decode()`:
- You are converting a logical, structured format (`struct`) into an array of bytes. Maybe consider `memcpy`?
- Honestly the way you serialize is not super important rn, moreso, your decode has to understand your encode. The byte layout is up to you, as long as an encoded command fits in `MAX_ENCODED_LEN` (256) bytes.
- If you `memcpy` the whole struct at once, you also copy the **padding** the compiler adds between fields. That works when both sides run the same code, but think about what happens if they don't. Copying field by field avoids the question.

`encode()` returns -1 if the command type is unknown or `out_cap` (the
buffer's size) is too small. The bytes `decode()` reads come from the wire, so
trust nothing: return -1 for an unknown command type or if `in_len` isn't
exactly the right length, and never read past `in_len`.

`encode()` and `decode()` only convert; they don't touch the hash.

## Part 3: The integrity hash

Hashing lives in [security.cpp](../src/security.cpp):

- `add_integrity_hash()` should remember to compute the SHA256 hash and include
  it in the message! Encode a copy of the message with its `signature` field
  set to all zeros, hash those bytes, then write the hash into
  `msg->signature`. (Why zero the signature field before hashing?)
- `verify_integrity_hash()` should compute the SHA256 hash the same way and
  compare it to *the hash provided in the message*.

The `signature` field has room for 64 bytes, enough for a real signature
later. A SHA-256 hash only fills the first 32, so set the rest to zero, and
have `verify_integrity_hash()` reject a message where they aren't.

On the receiving side, call `decode()` and then `verify_integrity_hash()`.
Only trust the message if both succeed.

## What the tests check

Besides `encode()` and `decode()` working together, the tests in
[test/test.cpp](../test/test.cpp) check that:

1. A message passes `verify_integrity_hash()` right after
   `add_integrity_hash()`, including after going through `encode()` and
   `decode()`.
2. Flipping **any single bit** of the encoded message makes `decode()` or
   `verify_integrity_hash()` reject it.
3. A message that never had a hash added fails.
4. A message whose unused signature bytes aren't zero fails.

One rule the tests can't check, because they don't know your layout:
`decode()` must reject bytes whose command type is unknown. Make sure yours
does.