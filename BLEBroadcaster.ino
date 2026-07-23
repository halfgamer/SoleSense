/*
 * SoleSense - BLE pressure broadcaster (Arduino Nano 33 BLE Sense Rev2)
 *
 * Reads 6 FSR channels and broadcasts them over BLE as a single
 * 12-byte packet (6 sensors x 2 bytes each, uint16, little-endian).
 *
 * ANALOG PIN ASSIGNMENT (sensor -> pin):
 *   A0 = Middle Joint
 *   A1 = Big Toe
 *   A2 = Right Joint
 *   A3 = Right Heel
 *   A4 = Left Heel
 *   A5 = Left Joint
 */

#include <ArduinoBLE.h>

BLEService pressureService("19B10000-E8F2-537E-4F6C-D104768A1214");
BLECharacteristic pressureChar(
  "19B10001-E8F2-537E-4F6C-D104768A1214",
  BLERead | BLENotify,
  12  // 6 sensors x 2 bytes
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
      uint16_t middleJoint = analogRead(A0);
      uint16_t bigToe      = analogRead(A1);
      uint16_t rightJoint  = analogRead(A2);
      uint16_t rightHeel   = analogRead(A3);
      uint16_t leftHeel    = analogRead(A4);
      uint16_t leftJoint   = analogRead(A5);

      uint8_t packet[12];
      packet[0]  = middleJoint & 0xFF; packet[1]  = middleJoint >> 8;
      packet[2]  = bigToe      & 0xFF; packet[3]  = bigToe      >> 8;
      packet[4]  = rightJoint  & 0xFF; packet[5]  = rightJoint  >> 8;
      packet[6]  = rightHeel   & 0xFF; packet[7]  = rightHeel   >> 8;
      packet[8]  = leftHeel    & 0xFF; packet[9]  = leftHeel    >> 8;
      packet[10] = leftJoint   & 0xFF; packet[11] = leftJoint   >> 8;

      pressureChar.writeValue(packet, 12);
      delay(50); // ~20Hz
    }

    Serial.println("Disconnected - resuming advertising");
  }
}
