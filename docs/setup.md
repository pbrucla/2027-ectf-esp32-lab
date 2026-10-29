# Setting up the ESP32

## What you need

- 2 × ESP32 dev boards (ESP32-WROOM "DevKit" style)
- 2 × USB cables
- 3 × female-to-female jumper wires (or a breadboard and male-to-male wires)
- A computer with the [Arduino IDE](https://www.arduino.cc/en/software) 2.x

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
| `Serial` | USB connection to your computer (the Serial Monitor) |
| `Serial1` | Free (not used in this lab) |
| `Serial2` | **Our link to the other board**. We put it on GPIO16 (RX) and GPIO17 (TX). |

## Setup

1. In the Arduino IDE, open **Boards Manager** and install **esp32 by Espressif
   Systems**.
2. Select **Tools → Board → esp32 → ESP32-WROOM-DA Module**.
3. Open [uart/uart.ino](uart/uart.ino).
4. Plug in one board, select its port under **Tools → Port**, and click
   **Upload** to check that everything works. The starter code compiles; it
   just doesn't do anything useful yet.
