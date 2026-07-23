/*
 * SoleSense - Data Collection Sketch (Arduino GIGA R1 WiFi)
 *
 * Reads 6 FSR pressure sensors and logs timestamped rows to data.txt on a
 * USB mass storage device plugged into the GIGA's USB-A host port.
 *
 * Label control via Serial Monitor (type and press Enter):
 *   Middle / Left / Right / Front / Back  -> start or switch to that label,
 *                                            recording continuously
 *   Stop                                  -> stop recording entirely
 *
 * ANALOG PIN ASSIGNMENT (sensor -> pin):
 *   A0 = Left Joint
 *   A1 = Right Joint
 *   A2 = Middle Joint
 *   A3 = Left Heel
 *   A4 = Right Heel
 *   A5 = Big Toe
 *
 * Row format written to data.txt (no header row, appends across sessions):
 *   leftJoint,rightJoint,middleJoint,leftHeel,rightHeel,bigToe,label
 */

#include <Arduino_USBHostMbed5.h>
#include "FATFileSystem.h"

USBHostMSD msd;
mbed::FATFileSystem usb("usb");

const unsigned long SAMPLE_INTERVAL_MS = 50; // ~20Hz

bool isRecording = false;
String currentLabel = "";
FILE *dataFile = nullptr;

unsigned long lastSampleTime = 0;
String inputBuffer = "";

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println();
  Serial.println("==================== NEW RUN ====================");
  Serial.println();

  analogReadResolution(12); // match Nano 33 BLE's 12-bit resolution exactly

  Serial.println("Waiting for USB mass storage device...");
  while (!msd.connect()) {
    delay(1000);
  }
  Serial.println("USB device connected.");

  int err = usb.mount(&msd);
  if (err) {
    Serial.print("Failed to mount USB filesystem, error: ");
    Serial.println(err);
    return;
  }
  Serial.println("USB filesystem mounted.");
  Serial.println("Type a label (Middle / Left / Right / Front / Back) to start/switch recording.");
  Serial.println("Type Stop to stop recording.");
}

void loop() {
  readSerialInput();

  if (isRecording && millis() - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = millis();
    writeSample();
  }
}

void readSerialInput() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        handleCommand(inputBuffer);
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }
}

void handleCommand(const String &raw) {
  String command = raw;
  command.trim();

  if (command == "Stop") {
    stopRecording();
  } else if (command == "Middle" || command == "Left" || command == "Right" ||
             command == "Front" || command == "Back") {
    startOrSwitchLabel(command);
  } else {
    Serial.print("Unrecognized input: ");
    Serial.println(command);
  }
}

void startOrSwitchLabel(const String &label) {
  if (!isRecording) {
    dataFile = fopen("/usb/data.txt", "a");
    if (dataFile == nullptr) {
      Serial.println("Failed to open data.txt for writing.");
      return;
    }
    isRecording = true;
  }
  currentLabel = label;
  Serial.print(">>> Recording label: ");
  Serial.println(label);
}

void stopRecording() {
  if (dataFile != nullptr) {
    fclose(dataFile);
    dataFile = nullptr;
  }
  isRecording = false;
  Serial.println(">>> Stopped recording.");
  currentLabel = "";
}

void writeSample() {
  if (dataFile == nullptr) return;

  uint16_t leftJoint   = analogRead(A0);
  uint16_t rightJoint  = analogRead(A1);
  uint16_t middleJoint = analogRead(A2);
  uint16_t leftHeel    = analogRead(A3);
  uint16_t rightHeel   = analogRead(A4);
  uint16_t bigToe      = analogRead(A5);

  fprintf(dataFile, "%u,%u,%u,%u,%u,%u,%s\n",
          leftJoint, rightJoint, middleJoint,
          leftHeel, rightHeel, bigToe,
          currentLabel.c_str());
  fflush(dataFile); // force write to actually hit the drive
}
