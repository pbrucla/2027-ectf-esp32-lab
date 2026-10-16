#include <SecureUart.h>

void setup() {
  Serial.begin(115200);
  secureBegin(Serial2, 16, 17);  // RX pin, TX pin
}

void loop() {
  String message;
  if (secureReceive(message)) {
    Serial.println("Verified Message: " + message);
  }
}
