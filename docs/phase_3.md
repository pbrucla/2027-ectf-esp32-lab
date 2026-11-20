# Phase 3: Signatures

Like seen in the [Python Crypto lab](https://github.com/pbrucla/2027-ectf-crypto-lab), we want to now implement signatures!

## Why a hash isn't enough

In Phase 2, the board accepts an `UPDATE` if `signature` holds the SHA-256 hash of `cmd` and `payload`. That catches a wire that flips bits by accident.

However, anyone can run `sha256()`, so an attacker who wants to install their own firmware can just:
1. Write their own `payload`,
2. Compute the hash, the same way `add_integrity_hash()` does,
3. Send it. The board checks the hash, it matches, and the board installs it.

The test `signature_stops_forged_update` does exactly this.

A **digital signature** fixes this by adding authentication to the hash:
| Key | Who has it | What it can do |
| --- | --- | --- |
| **Private key** | Only the update server (`test/server.py`) | Make signatures |
| **Public key** | Every board, compiled into the firmware | Check signatures |

Checking a signature needs only the public key, but making one needs the private key. So even someone who pulls the public key out of a board's flash can't sign their own firmware.

We use **Ed25519**. Its signatures are 64 bytes, which is why `signature` has been 64 bytes all along.

## The code

| File | What it holds |
| --- | --- |
| [src/security.cpp](../src/security.cpp) | `add_signature()` and `verify_signature()`, **which you write**, below the Phase 2 hash functions (now given) |
| [src/commands.cpp](../src/commands.cpp) | `encode()` and `decode()`, now given (the Phase 2 solution) |
| [src/crypto.hpp](../src/crypto.hpp) | `ed25519_sign()`, `ed25519_verify()` and `sha512()`, given |
| [src/ed25519.cpp](../src/ed25519.cpp) | The Ed25519 code, given. |
| [src/hex.hpp](../src/hex.hpp) | `hex_encode()` and `hex_decode()`, given |
| [src/uart.cpp](../src/uart.cpp) | Now runs commands it receives, given |
| [test/keygen.py](../test/keygen.py) | Makes the keypair |
| [test/server.py](../test/server.py) | Signs and sends commands to a board |
| [test/test.cpp](../test/test.cpp) | Unit tests, with new signature tests |

## Part 1: `add_signature()` and `verify_signature()`

```cpp
// Returns 0 on success, or -1 if `msg` or `private_key` is NULL or `msg->cmd` is unknown.
int add_signature(cmd_msg_t* msg, const uint8_t private_key[ED25519_PRIVATE_KEY_LEN]);
// Returns 0 if the signature is valid, or -1 if it is not.
int verify_signature(const cmd_msg_t* msg, const uint8_t public_key[ED25519_PUBLIC_KEY_LEN]);
```

These look a lot like the Phase 2 functions, so start from `add_integrity_hash()` and `verify_integrity_hash()`, which are now given at the top of [security.cpp](../src/security.cpp). If you finished Phase 2, you can compare them with yours.

- Sign the **same 18 bytes** as the hash: the 1 byte of `cmd` followed by the
  17 bytes of `payload`. `test/server.py` signs exactly these bytes, so if you
  sign anything else, the board will reject the server's updates. The test
  `signature_signs_cmd_and_payload` checks this.
- `add_signature()` calls `ed25519_sign()`, which writes all 64 bytes of the
  signature. There's no zero padding this time.
- `verify_signature()` rebuilds the same 18 bytes and calls
  `ed25519_verify()`. Don't compare signatures yourself: the only way to check
  one is `ed25519_verify()`.
- Return -1 for a NULL `msg`, a NULL key, or an unknown `cmd`.

The functions you call:

```cpp
void ed25519_sign(const uint8_t* msg, size_t len,
                  const uint8_t private_key[ED25519_PRIVATE_KEY_LEN],
                  uint8_t sig[ED25519_SIGNATURE_LEN]);
// Returns 0 if valid, -1 if not.
int ed25519_verify(const uint8_t* msg, size_t len,
                   const uint8_t public_key[ED25519_PUBLIC_KEY_LEN],
                   const uint8_t sig[ED25519_SIGNATURE_LEN]);
```

### Running the tests

Same as Phase 2:

```bash
make test
```

The Phase 2 tests and the `crypto_*` and `hex_*` tests check given code, so
they pass from the start. Most of the `signature_*` and `interop_*` tests fail
until you finish Part 1. A few, like `signature_stops_forged_update`, pass
even now, because a stub that always returns -1 happens to reject everything.
Only once all of them pass do you know your code accepts good signatures too.

Passing tests print a green `PASS` and failing ones a red `FAIL`, with the
check that failed and its line number. Notice what the new tests check:

1. A signed message verifies, including after `encode()` and `decode()`.
2. Flipping **any single bit** of the encoded message makes it fail.
3. A message with no signature, or signed with **a different private key**,
   fails.
4. A message with a correct Phase 2 hash, but no signature, fails. That's the
   attack from the top of this page.
5. Signing and encoding a message in C gives exactly the bytes
   `test/server.py` sends for it (`interop_encode_matches_server`, using a
   line stored in [test/interop_vectors.hpp](../test/interop_vectors.hpp)).
   If you sign the right 18 bytes, this passes.

## Part 2: Make your keys

The tests use a made-up key. For the boards, make a real one.

The Python scripts in `test/` run with [uv](https://docs.astral.sh/uv/). Each
script lists the packages it needs at the top, and uv installs them (and a
Python, if you need one) the first time you run it. Install uv once:

- **macOS:** `brew install uv`
- **Linux:** `curl -LsSf https://astral.sh/uv/install.sh | sh`
- **Windows:** in PowerShell, run
  `powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/install.ps1 | iex"`

Close and reopen your terminals so they find `uv`. Then:

```bash
make keys
```

**Windows:** run `make keys` from the MSYS2 UCRT64 terminal, like
`make test`. The MSYS2 terminal doesn't search the folder uv installs to, so
the Makefile looks for uv there itself. Run `uv` commands of your own, like
`uv run test/server.py` below, from PowerShell, where `uv` is on the PATH.

This writes two files, and `.gitignore` keeps both out of git:

- `keys/private_key.hex`: the **private** key. Only `test/server.py` reads
  it. It never goes on a board.
- `src/public_key.hpp`: the **public** key, as a C array named `PUBLIC_KEY`.
  `uart.cpp` includes it, so the firmware won't build until you run
  `make keys`.

Running `make keys` again keeps the same key. `uv run test/keygen.py --force`
makes a new one. After that, re-upload the firmware to every board, or they'll
keep trusting the old key.

## Part 3: Commands over UART

Text messages work like before. A line that starts with `CMD ` is now a command, sent as hex because `readLine()` drops bytes that aren't printable:

```
CMD 0001686578...<164 hex characters in all>
```

When a board receives one, [uart.cpp](../src/uart.cpp) does this:

1. `hex_decode()` the text after `CMD `,
2. `decode()` the bytes into a `cmd_msg_t`,
3. `verify_signature()` with `PUBLIC_KEY`,
4. only if all three succeed, run the command. For `UPDATE`, that means
   storing the new firmware.

The board then sends a reply line back to the sender, starting with either
`ACCEPTED:` or `REJECTED:`.

### The wire format

`encode()` and `decode()` are given this phase, so you don't need to match
anything yourself. For reference, `test/server.py` builds the same layout they
use (82 bytes for `UPDATE`):

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 1 | `cmd` (0 = `UPDATE`) |
| 1 | 1 | `payload.firmware_payload.version` |
| 2 | 16 | `payload.firmware_payload.firmware` |
| 18 | 64 | `signature` |

If you ever change the format, change `commands.cpp` and `build_update()` in
`test/server.py` together, then run `uv run test/gen_interop_vectors.py` to
update the stored line.

## Part 4: Try it on the boards

1. Build and upload to **both** boards, the same as in Phase 1. Both need the
   same `src/public_key.hpp`, so build both from the same folder.
2. Wire them together as before. Plug both into your computer.
3. Open a serial monitor on **board B** only:

   ```bash
   pio device monitor -p <port B>
   ```

4. In another terminal, send an update through **board A**. Board A forwards
   the line to board B, just as it does with lines you type:

   ```bash
   uv run test/server.py --port <port A> --version 2 --firmware "new code"
   ```

   Board B's monitor should show `ACCEPTED: firmware v2 "new code"`, and
   `server.py` prints the same reply after board A passes it back.
5. Now try to break in. Each of these should print `REJECTED: bad signature`:

   ```bash
   uv run test/server.py --port <port A> --attack tamper     # firmware changed after signing
   uv run test/server.py --port <port A> --attack wrong-key  # signed with someone else's key
   uv run test/server.py --port <port A> --attack hash       # the Phase 2 hash
   uv run test/server.py --port <port A> --attack unsigned   # no signature at all
   ```

   Always pass `--port`: with both boards plugged in, `server.py` can't tell
   which one is board A. Run `pio device list` to see the ports. On Windows
   they look like `COM3`.

`server.py` needs the port to itself, so close any `pio device monitor` on
board A first.

## Something to think about

Send the same valid update twice, or an old one after a newer one:

```bash
uv run test/server.py --port <port A> --version 5 --firmware "new code"
uv run test/server.py --port <port A> --version 1 --firmware "old buggy code"
```

Both are accepted. Each one really was signed by the server, so the signature
checks out. But an attacker who recorded the old update off the wire can
**replay** it later to roll the board back to buggy firmware. Signatures prove
who made a message, not when.

## Troubleshooting

| Symptom | Likely cause |
| --- | --- |
| Build fails with `src/public_key.hpp is missing` | Run `make keys` first. |
| `uv: command not found` | Install uv (Part 2), then close and reopen the terminal. On Windows, run `uv` from PowerShell; `make keys` works from MSYS2. |
| `ModuleNotFoundError: No module named 'cryptography'` (or `serial`) | You ran the script with `python3` instead of `uv run`. |
| Every update says `REJECTED: bad signature` | Did you finish `verify_signature()` in `src/`? Do the `signature_*` and `interop_*` tests pass? Did you run `keygen.py --force` without re-uploading? |
| `REJECTED: not a valid command` | `commands.cpp` and `server.py` disagree on the wire format. Did someone edit one of them? `interop_encode_matches_server` should fail too. |
| `server.py` says the port is busy | Close the serial monitor on that port. |
| `server.py` prints nothing back | Wrong port, the boards aren't wired together, or the listening time was too short (try `--listen 5`). |
