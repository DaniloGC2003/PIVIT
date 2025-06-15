#define RXD2 16
#define TXD2 17

void setup() {
  Serial.begin(115200);  // For debugging on USB
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);  // UART2
  Serial.println("UART Two-Way Chat started...");
}

void loop() {
  // 1. Send a message every 2 seconds
  static unsigned long lastSendTime = 0;
  if (millis() - lastSendTime > 2000) {
    lastSendTime = millis();
    String msg = "Hello from ESP32 at " + String(millis()/1000) + "s";
    Serial2.println(msg);
    Serial.println("Sent: " + msg);
  }

  // 2. Check for incoming messages
  while (Serial2.available()) {
    String incoming = Serial2.readStringUntil('\n');
    Serial.println("Received: " + incoming);
  }
}
