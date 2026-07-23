/*
 * SoleSense - BLE pressure broadcaster (Arduino Nano 33 BLE Sense Rev2)
 *
 * Reads 5 FSR channels and broadcasts them over BLE as a single
 * 10-byte packet (5 sensors x 2 bytes each, uint16, little-endian).
 *
 * ANALOG PIN ASSIGNMENT (sensor -> pin):
 *   A0 = Left Joint
 *   A1 = Right Joint
 *   A2 = Middle Joint
 *   A3 = Heel
 *   A5 = Big Toe      (A4 unused - was Right Heel, now removed)
 */

#include <ArduinoBLE.h>

BLEService pressureService("19B10000-E8F2-537E-4F6C-D104768A1214");
BLECharacteristic pressureChar(
  "19B10001-E8F2-537E-4F6C-D104768A1214",
  BLERead | BLENotify,
  10  // 5 sensors x 2 bytes
);

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  if (!BLE.begin()) {
    Serial.println("BLE init failed!");
    while (1);
  }

  BLE.setLocalName("SoleSense");
  BLE.setAdvertisedService(pressureService);
  pressureService.addCharacteristic(pressureChar);
  BLE.addService(pressureService);
  BLE.advertise();

  Serial.println("SoleSense BLE broadcasting - waiting for phone...");
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("Connected to: ");
    Serial.println(central.address());

    while (central.connected()) {
      uint16_t leftJoint   = analogRead(A0);
      uint16_t rightJoint  = analogRead(A1);
      uint16_t middleJoint = analogRead(A2);
      uint16_t heel        = analogRead(A3);
      uint16_t bigToe      = analogRead(A5);

      uint8_t packet[10];
      packet[0] = leftJoint   & 0xFF; packet[1] = leftJoint   >> 8;
      packet[2] = rightJoint  & 0xFF; packet[3] = rightJoint  >> 8;
      packet[4] = middleJoint & 0xFF; packet[5] = middleJoint >> 8;
      packet[6] = heel        & 0xFF; packet[7] = heel        >> 8;
      packet[8] = bigToe      & 0xFF; packet[9] = bigToe      >> 8;

      pressureChar.writeValue(packet, 10);
      delay(50); // ~20Hz
    }

    Serial.println("Disconnected - resuming advertising");
  }
}
