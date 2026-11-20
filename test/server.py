#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.9"
# dependencies = [
#     "cryptography>=42",
#     "pyserial>=3.5",
# ]
# ///
"""Send signed commands to a board over serial, the way a real update server would.

This script holds the private key (from test/keygen.py), so it is the only
thing that can make valid signatures. Boards hold just the public key.

Two ways to connect:
  1. Through board A's USB port (the default, 115200 baud). Board A forwards
     every line you send to board B over UART, exactly as if you'd typed it in
     the serial monitor. Board B checks the command and replies, and board A
     prints the reply back to us as "Received: ...".
  2. With a USB-to-UART adapter wired straight to a board's GPIO16/17 (use
     --baud 9600). Then this script *is* the other board.

Examples:
  uv run test/server.py                                   # send v1 "hello, firmware!"
  uv run test/server.py --version 2 --firmware "new code"
  uv run test/server.py --attack tamper                   # change the payload after signing
  uv run test/server.py --attack wrong-key                # sign with someone else's key
  uv run test/server.py --attack unsigned                 # all-zero signature
  uv run test/server.py --attack hash                     # Phase 2 hash instead of a signature
  uv run test/server.py --port /dev/cu.usbserial-0001

Close any `pio device monitor` on the port first; only one program can use it.
"""

import argparse
import hashlib
import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    sys.exit("pyserial is missing: run this script with `uv run test/server.py`")

from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

from keygen import PRIVATE_KEY_PATH, load_private_key

# Must match commands.hpp and the reference wire format in docs/phase_3.md.
CMD_UPDATE = 0
MAX_FIRMWARE_SIZE = 16
SIGNATURE_LEN = 64
CMD_PREFIX = "CMD "

# USB vendor:product IDs of the USB-serial chips on common ESP32 boards and
# adapters, so we can find the port on our own (like `pio device monitor`).
KNOWN_USB_SERIAL = {
    (0x10C4, 0xEA60): "CP210x",
    (0x1A86, 0x7523): "CH340",
    (0x1A86, 0x55D4): "CH9102",
    (0x0403, 0x6001): "FT232",
    (0x0403, 0x6015): "FT231X",
    (0x303A, 0x1001): "ESP32 USB",
}


def find_port() -> str:
    """Return the one ESP32-looking serial port, or exit with a list to choose from."""
    ports = list(list_ports.comports())
    matches = [p for p in ports if (p.vid, p.pid) in KNOWN_USB_SERIAL]
    if len(matches) == 1:
        return matches[0].device

    if not matches:
        print("No ESP32 serial port found. Ports on this computer:", file=sys.stderr)
        listed = ports
    else:
        print(
            "More than one board is plugged in. Pick one with --port:", file=sys.stderr
        )
        listed = matches
    for p in listed:
        print(f"  {p.device}  {p.description}", file=sys.stderr)
    sys.exit(1)


def payload_bytes(version: int, firmware: str) -> bytes:
    try:
        data = firmware.encode("ascii")
    except UnicodeEncodeError:
        sys.exit("--firmware must be plain ASCII characters")
    if len(data) > MAX_FIRMWARE_SIZE:
        sys.exit(f"--firmware is limited to {MAX_FIRMWARE_SIZE} characters")
    return bytes([version]) + data.ljust(MAX_FIRMWARE_SIZE, b"\0")


def build_update(
    key: Ed25519PrivateKey, version: int, firmware: str, attack: str
) -> bytes:
    """The encoded UPDATE: cmd (1) | version (1) | firmware (16) | signature (64)."""
    signed = bytes([CMD_UPDATE]) + payload_bytes(version, firmware)

    if attack == "wrong-key":
        signature = Ed25519PrivateKey.generate().sign(signed)
    elif attack == "unsigned":
        signature = bytes(SIGNATURE_LEN)
    elif attack == "hash":
        # What Phase 2 sent: anyone can compute this, no key needed.
        signature = hashlib.sha256(signed).digest().ljust(SIGNATURE_LEN, b"\0")
    else:
        signature = key.sign(signed)

    if attack == "tamper":
        # Swap in different firmware after signing, as a man-in-the-middle would.
        signed = bytes([CMD_UPDATE]) + payload_bytes(version, "evil firmware!!!")

    return signed + signature


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__.split("\n")[0],
        epilog="See docs/phase_3.md.",
    )
    parser.add_argument(
        "--port",
        help="serial port (found automatically if only one board is plugged in)",
    )
    parser.add_argument(
        "--baud",
        type=int,
        default=115200,
        help="115200 through a board's USB port (default), 9600 for a UART adapter",
    )
    parser.add_argument(
        "--version", type=int, default=1, help="firmware version, 0-255 (default 1)"
    )
    parser.add_argument(
        "--firmware", default="hello, firmware!", help="up to 16 ASCII characters"
    )
    parser.add_argument(
        "--attack",
        default="none",
        choices=["none", "tamper", "wrong-key", "unsigned", "hash"],
        help="send a bad command to check the board rejects it",
    )
    parser.add_argument(
        "--listen",
        type=float,
        default=3.0,
        help="seconds to print what comes back (default 3)",
    )
    args = parser.parse_args()

    if not 0 <= args.version <= 255:
        sys.exit("--version must be 0-255")
    if not PRIVATE_KEY_PATH.exists():
        sys.exit("No private key yet: run `make keys` first")

    message = build_update(load_private_key(), args.version, args.firmware, args.attack)
    line = CMD_PREFIX + message.hex() + "\n"

    port = args.port or find_port()
    link = serial.Serial()
    link.port = port
    link.baudrate = args.baud
    link.timeout = 0.1
    # Opening the port can toggle DTR/RTS, which resets most ESP32 boards.
    # Keep both low so the board keeps running.
    link.dtr = False
    link.rts = False
    link.open()

    with link:
        time.sleep(0.2)
        link.reset_input_buffer()

        print(
            f"Sending to {port} ({args.attack if args.attack != 'none' else 'valid signature'}):"
        )
        print(f"  {line.strip()}")
        link.write(line.encode("ascii"))
        link.flush()

        deadline = time.monotonic() + args.listen
        while time.monotonic() < deadline:
            reply = link.readline().decode("ascii", errors="replace").strip()
            if reply:
                print(f"< {reply}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
