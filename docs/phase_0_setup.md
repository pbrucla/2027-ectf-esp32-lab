# Setting up the ESP32

## What you need

- 2 × ESP32 dev boards (ESP32-WROOM "DevKit" style)
- 2 × USB cables
- 3 × female-to-female jumper wires (or a breadboard and male-to-male wires)
- A computer with [PlatformIO](https://platformio.org/): either the
  **PlatformIO IDE** extension for [VS Code](https://code.visualstudio.com/),
  or the command-line tool (`pip install platformio` or
  `brew install platformio`)

## Background: how UART works

UART (Universal Asynchronous Receiver/Transmitter) sends data one bit at a time
over a single wire. Each device has two data pins:

| Pin | Meaning | Direction |
| --- | --- | --- |
| **TX** | Transmit | Data goes **out** of this pin |
| **RX** | Receive | Data comes **in** on this pin |

Because one board's output must go into the other board's input, the wires
**cross**: TX of board A goes to RX of board B, and the other way around.

The boards also need a shared **ground (GND)**. A voltage is only "high" or
"low" compared to something else, and the shared ground gives both boards the
same reference point.

There is no clock wire (that's the "asynchronous" part), so both sides must
agree on the speed in advance. This is the **baud rate**: bits per second. In
this lab we use **9600 baud** with the **8N1** format, meaning 8 data bits,
no parity bit and 1 stop bit. Each character takes 10 bits on the wire (start
bit, 8 data bits, stop bit).

The ESP32 has three hardware UARTs:

| Arduino name | Used for |
| --- | --- |
| `Serial` | USB connection to your computer (the serial monitor) |
| `Serial1` | Free (not used in this lab) |
| `Serial2` | **Our link to the other board**. We put it on GPIO16 (RX) and GPIO17 (TX). |

## Setup

The board, framework and serial monitor settings are already in
[platformio.ini](../platformio.ini). PlatformIO downloads the ESP32 toolchain
the first time you build, which can take a few minutes.

1. Open this repository's folder (the one containing `platformio.ini`). In
   VS Code use **File → Open Folder**; on the command line, `cd` into it. Run
   every `pio` command below from this folder.

   If you installed the VS Code extension, `pio` only works in a terminal
   opened inside VS Code (**Terminal → New Terminal**). In any other terminal
   you get `pio: command not found`. `make build` finds it either way.
2. Build the code:

   ```bash
   pio run
   ```

   In VS Code, click the **✓ (Build)** button in the blue status bar instead.
   `make build` does the same thing.
3. Plug in one board and find its port:

   ```bash
   pio device list
   ```

   It looks like `/dev/cu.usbserial-0001` on macOS, `/dev/ttyUSB0` on Linux or
   `COM3` on Windows.
4. Upload to check that everything works:

   ```bash
   pio run -t upload --upload-port <port>
   ```

   In VS Code, click the **→ (Upload)** button. With only one board plugged
   in, you can leave out `--upload-port`.
5. Open the serial monitor (115200 baud, set in `platformio.ini`):

   ```bash
   pio device monitor -p <port>
   ```

   In VS Code, click the **🔌 (Serial Monitor)** button. Press **Ctrl+C** to
   exit. Close the monitor before uploading again, because only one program
   can use the port at a time.

The code is in [src/](../src): `main.cpp` holds `setup()` and `loop()`, and
`uart.cpp` holds the UART code you will work on in Phase 1.
