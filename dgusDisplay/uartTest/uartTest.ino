void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Ready. Listening to DWIN...");
}

void loop() {
  if (Serial.available()) {
    int incomingByte = Serial.read();
    if (incomingByte < 0x10) Serial.print("0");
    Serial.print(incomingByte, HEX);
    Serial.print(" ");
  }
}
