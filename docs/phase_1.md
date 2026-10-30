# Phase 1: UART Communication!

## Part 1: Implement UART

Open [src/uart.cpp](../src/uart.cpp) and read the whole file first, along
with [src/uart.hpp](../src/uart.hpp) (the settings) and
[src/main.cpp](../src/main.cpp) (which calls your functions from `setup()` and
`loop()`). The `readLine()` helper is given to you. Make sure you understand
how it builds up a line one character at a time and why it **never waits** for
data.

Then complete the three TODOs:

1. **TODO 1, `initialize_peer_serial()`:** Start the `peer` UART at
   `LINK_BAUD` with the `SERIAL_8N1` format on pins `LINK_RX_PIN` and
   `LINK_TX_PIN`.
2. **TODO 2, `handle_user_input()`:** When the user types a line (read it from
   `Serial` into `typedBuffer` with `readLine()`, passing `&typedLength`), send `<line>` to `peer`
   followed by `'\n'`, then print `Sent: <line>` to `Serial`.
3. **TODO 3, `handle_peer_input()`:** Check for a complete line from `peer`
   with `readLine()` into `receivedBuffer`, passing `&receivedLength`. If one
   arrived, print
   `Received: <line>` to `Serial`.

Useful functions: `peer.begin(...)`, `peer.write(...)`, `peer.print(...)`,
`strlen(...)`, `Serial.printf(...)`.

## Part 2: Test with boards

1. Plug in both boards and run `pio device list` to find both ports. Upload the
   **same code** to each one:

   ```bash
   pio run -t upload --upload-port <port A>
   pio run -t upload --upload-port <port B>
   ```

2. Wire them together:

   ```
   Board A GPIO17 (TX) ───► Board B GPIO16 (RX)
   Board A GPIO16 (RX) ◄─── Board B GPIO17 (TX)
   Board A GND         ──── Board B GND
   ```

   ⚠️ Do **not** connect the 3V3, 5V or VIN pins between boards. Each board is
   powered by its own USB cable.

3. Open a serial monitor for each board, each in its own terminal, from the
   repository folder:

   ```bash
   pio device monitor -p <port A>
   pio device monitor -p <port B>
   ```

   `platformio.ini` already sets the speed to 115200 baud and makes Enter send
   a newline. Close both monitors (**Ctrl+C**) before you upload again.
4. Type `hello` in one monitor. The other should show:

   ```
   Received: hello
   ```

   Try both directions, and try both boards at the same time.

## Troubleshooting

| Symptom | Likely cause |
| --- | --- |
| Nothing happens when you press Enter | The monitor isn't sending a newline. Start it with `pio device monitor` from the repository folder so it uses the settings in `platformio.ini`. |
| Garbage characters in the serial monitor | The monitor isn't at 115200 baud. Start it from the repository folder, without a `-b` option. |
| Upload fails because the port is busy | A serial monitor still has the port open. Close it with **Ctrl+C** and upload again. |
| `Received:` shows garbled text, or nothing arrives at all | Baud rates differ between the boards, or GND isn't connected. (`readLine()` drops non-printable bytes, so bad data often shows up as nothing.) |
| `Sent:` appears but nothing is received | TX/RX not crossed, wrong pins, loose wire, or the other board isn't running the same code. |
| Upload fails with "Failed to connect" | Hold the **BOOT** button while the upload starts. |
| Board reboots or crashes on start | Some modules (e.g. ESP32-WROVER) use GPIO16/17 internally. Change `LINK_RX_PIN` and `LINK_TX_PIN` in `src/uart.hpp` to 25 and 26 on both boards. |

## Pinout Reference
![image](pinout_reference.png) for wiring