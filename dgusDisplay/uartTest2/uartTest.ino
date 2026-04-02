#define DGUS_BAUD 9600
#define RELAY_VP_ADDR 0x1388
#define NUM_RELAYS 10

void setup() {
  Serial.begin(DGUS_BAUD);
  while (!Serial);
  Serial.println("DGUS Relay Monitor Started");
  Serial.println("Reading VP address 5000 (0x1388)");
  Serial.println("--------------------------------");
}

void loop() {
  uint16_t bitmask = readDGUSRegister(RELAY_VP_ADDR, 2);
  printRelayStatus(bitmask);
  delay(1000);
}

uint16_t readDGUSRegister(uint16_t vpAddr, uint8_t length) {
  uint8_t cmd[7] = {
    0x5A, 0xA5,
    0x04,
    0x46,
    (uint8_t)(vpAddr >> 8),
    (uint8_t)(vpAddr & 0xFF),
    length
  };

  uint8_t checksum = 0;
  for (int i = 0; i < 7; i++) checksum += cmd[i];

  Serial.write(cmd, 7);
  Serial.write(checksum);

  uint32_t startTime = millis();
  while (Serial.available() < (length + 5)) {
    if (millis() - startTime > 100) return 0;
  }

  for (int i = 0; i < 5; i++) Serial.read();
  uint16_t result = 0;
  for (int i = 0; i < length; i++) {
    result = (result << 8) | Serial.read();
  }

  return result;
}

void printRelayStatus(uint16_t bitmask) {
  Serial.write(27);
  Serial.print("[2J");
  Serial.write(27);
  Serial.print("[H");
  Serial.println("========== Relay Status ==========");
  for (int i = 1; i <= NUM_RELAYS; i++) {
    bool isOn = (bitmask >> (i - 1)) & 0x01;
    Serial.print("Relay ");
    if (i < 10) Serial.print(" ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(isOn ? "ON " : "OFF");
  }
  Serial.println("==================================");
}

/*
 * ============================================
 * RELAY CONTROL (COMMENTED - FOR FUTURE USE)
 * ============================================
 * Uncomment to enable latching relay control
 *
 * #define RELAY_CONTROL_ENABLED
 * #include <EEPROM.h>
 *
 * #define LATCH_PULSE_MS 100
 * #define SET_PIN 2
 * #define CLR_PIN 3
 * #define CLK_PIN 4
 * #define DATA_PIN 5
 *
 * const uint8_t RELAY_PINS[NUM_RELAYS] = { 6, 7, 8, 9, 10, 11, 12, A0, A1, A2 };
 *
 * uint16_t localRelayState = 0;
 *
 * void initRelays() {
 *   for (int i = 0; i < NUM_RELAYS; i++) {
 *     pinMode(RELAY_PINS[i], OUTPUT);
 *     digitalWrite(RELAY_PINS[i], LOW);
 *   }
 *   pinMode(SET_PIN, OUTPUT);
 *   pinMode(CLR_PIN, OUTPUT);
 *   pinMode(CLK_PIN, OUTPUT);
 *   pinMode(DATA_PIN, OUTPUT);
 *   digitalWrite(SET_PIN, LOW);
 *   digitalWrite(CLR_PIN, LOW);
 *   digitalWrite(CLK_PIN, LOW);
 *   digitalWrite(DATA_PIN, LOW);
 *   localRelayState = EEPROM.read(0) | (EEPROM.read(1) << 8);
 *   syncRelaysToState();
 * }
 *
 * void latchPulse(uint8_t relayNum, bool setHigh) {
 *   digitalWrite(CLK_PIN, HIGH);
 *   digitalWrite(DATA_PIN, setHigh ? HIGH : LOW);
 *   delayMicroseconds(1);
 *   digitalWrite(CLK_PIN, LOW);
 *   delayMicroseconds(1);
 *   digitalWrite(CLK_PIN, HIGH);
 *   digitalWrite(CLK_PIN, LOW);
 *   uint8_t targetPin = RELAY_PINS[relayNum - 1];
 *   digitalWrite(targetPin, HIGH);
 *   delay(LATCH_PULSE_MS);
 *   digitalWrite(targetPin, LOW);
 * }
 *
 * void setRelay(uint8_t relayNum, bool state) {
 *   if (relayNum < 1 || relayNum > NUM_RELAYS) return;
 *   bool currentState = (localRelayState >> (relayNum - 1)) & 0x01;
 *   if (currentState != state) {
 *     latchPulse(relayNum, state);
 *     bitWrite(localRelayState, relayNum - 1, state);
 *     saveRelayState();
 *   }
 * }
 *
 * void toggleRelay(uint8_t relayNum) {
 *   if (relayNum < 1 || relayNum > NUM_RELAYS) return;
 *   bool currentState = (localRelayState >> (relayNum - 1)) & 0x01;
 *   latchPulse(relayNum, !currentState);
 *   bitWrite(localRelayState, relayNum - 1, !currentState);
 *   saveRelayState();
 * }
 *
 * void syncRelaysToState() {
 *   for (int i = 1; i <= NUM_RELAYS; i++) {
 *     bool state = (localRelayState >> (i - 1)) & 0x01;
 *     latchPulse(i, state);
 *   }
 * }
 *
 * void saveRelayState() {
 *   EEPROM.write(0, lowByte(localRelayState));
 *   EEPROM.write(1, highByte(localRelayState));
 * }
 *
 * void handleSerialCommand(String cmd) {
 *   cmd.trim();
 *   cmd.toUpperCase();
 *   if (cmd.startsWith("R") && cmd.length() >= 3) {
 *     int relayNum = cmd.substring(1, 2).toInt();
 *     if (cmd.indexOf("ON") != -1) {
 *       setRelay(relayNum, true);
 *       Serial.print("Relay ");
 *       Serial.print(relayNum);
 *       Serial.println(" set to ON");
 *     } else if (cmd.indexOf("OFF") != -1) {
 *       setRelay(relayNum, false);
 *       Serial.print("Relay ");
 *       Serial.print(relayNum);
 *       Serial.println(" set to OFF");
 *     } else if (cmd.indexOf("TOGGLE") != -1) {
 *       toggleRelay(relayNum);
 *       Serial.print("Relay ");
 *       Serial.print(relayNum);
 *       Serial.println(" toggled");
 *     }
 *   } else if (cmd == "STATUS") {
 *     Serial.print("Current state: 0x");
 *     Serial.println(localRelayState, HEX);
 *   } else if (cmd == "HELP") {
 *     Serial.println("Commands: R1-10 ON/OFF/TOGGLE, STATUS, SYNC");
 *   } else if (cmd == "SYNC") {
 *     syncRelaysToState();
 *     Serial.println("Relays synced to saved state");
 *   }
 * }
 */
