#include <SecureUart.h>

void setup() {
  Serial.begin(115200);
  secureBegin(Serial2, 16, 17);  // RX pin, TX pin
  Serial.println("Type a message and press Enter to send it.");
}

void loop() {
  if (Serial.available()) {
    String message = Serial.readStringUntil('\n');
    message.trim();  // remove the Enter key (\r or \n) and extra spaces

    if (message.length() > 0 && secureSend(message)) {
      Serial.println("Sent: " + message);
    }
  }
}
