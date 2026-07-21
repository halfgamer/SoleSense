/*
 * SoleSense - Data Collection Sketch (Arduino GIGA R1 WiFi)
 *
 * Reads 9 FSR pressure sensors + 5 stance-label buttons, and logs
 * timestamped rows to data.txt on a USB mass storage device plugged
 * into the GIGA's USB-A host port.
 *
 * ANALOG PIN ASSIGNMENT (sensor -> pin):
 *   A0 = Right Heel
 *   A1 = Middle Joint
 *   A2 = Left Joint
 *   A3 = Right Joint
 *   A4 = Big Toe
 *   A5 = Index
 *   A6 = Middle Toe
 *   A7 = Ring
 *   A8 = Left Heel
 *
 * DIGITAL PIN ASSIGNMENT (stance button -> pin):
 *   D0 = Middle
 *   D1 = Forward
 *   D2 = Backward
 *   D3 = Right
 *   D4 = Left
 *
 * Row format written to data.txt (no header row, appends across sessions):
 *   rightHeel,middleJoint,leftJoint,rightJoint,bigToe,index,middleToe,ring,leftHeel,label
 */

#include <Arduino_USBHostMbed5.h>
#include "FATFileSystem.h"

USBHostMSD msd;
mbed::FATFileSystem usb("usb");

#define FSR_A8_PIN A8

const int BUTTON_MIDDLE   = 0; // D0
const int BUTTON_FORWARD  = 1; // D1
const int BUTTON_BACKWARD = 2; // D2
const int BUTTON_RIGHT    = 3; // D3
const int BUTTON_LEFT     = 4; // D4

const unsigned long SAMPLE_INTERVAL_MS = 50; // ~20Hz, matches Nano BLE rate
const unsigned long DEBOUNCE_MS = 300;

bool isRecording = false;
String currentLabel = "";
FILE *dataFile = nullptr;

unsigned long lastSampleTime = 0;
unsigned long lastButtonPressTime = 0;

bool prevMiddle = LOW;
bool prevForward = LOW;
bool prevBackward = LOW;
bool prevRight = LOW;
bool prevLeft = LOW;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println();
  Serial.println("==================== NEW RUN ====================");
  Serial.println();

  pinMode(BUTTON_MIDDLE, INPUT);
  pinMode(BUTTON_FORWARD, INPUT);
  pinMode(BUTTON_BACKWARD, INPUT);
  pinMode(BUTTON_RIGHT, INPUT);
  pinMode(BUTTON_LEFT, INPUT);

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
  Serial.println("USB filesystem mounted. Ready - press a stance button to start.");
}

void loop() {
  checkButton(BUTTON_MIDDLE, prevMiddle, "Middle");
  checkButton(BUTTON_FORWARD, prevForward, "Forward");
  checkButton(BUTTON_BACKWARD, prevBackward, "Backward");
  checkButton(BUTTON_RIGHT, prevRight, "Right");
  checkButton(BUTTON_LEFT, prevLeft, "Left");

  if (isRecording && millis() - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = millis();
    writeSample();
  }
}

void checkButton(int pin, bool &prevState, const String &label) {
  bool current = digitalRead(pin);

  if (current == HIGH && prevState == LOW) {
    if (millis() - lastButtonPressTime > DEBOUNCE_MS) {
      lastButtonPressTime = millis();
      handleButtonPress(label);
    }
  }
  prevState = current;
}

void handleButtonPress(const String &label) {
  if (!isRecording) {
    startRecording(label);
  } else if (currentLabel == label) {
    stopRecording();
  }
}

void startRecording(const String &label) {
  dataFile = fopen("/usb/data.txt", "a");
  if (dataFile == nullptr) {
    Serial.println("Failed to open data.txt for writing.");
    return;
  }
  isRecording = true;
  currentLabel = label;
  Serial.print("Started recording - label: ");
  Serial.println(label);
}

void stopRecording() {
  if (dataFile != nullptr) {
    fclose(dataFile);
    dataFile = nullptr;
  }
  Serial.print("Stopped recording - label: ");
  Serial.println(currentLabel);
  isRecording = false;
  currentLabel = "";
}

void writeSample() {
  if (dataFile == nullptr) return;

  uint16_t rightHeel   = analogRead(A0);
  uint16_t middleJoint = analogRead(A1);
  uint16_t leftJoint   = analogRead(A2);
  uint16_t rightJoint  = analogRead(A3);
  uint16_t bigToe      = analogRead(A4);
  uint16_t index       = analogRead(A5);
  uint16_t middleToe   = analogRead(A6);
  uint16_t ring        = analogRead(A7);
  uint16_t leftHeel    = analogRead(FSR_A8_PIN);

  fprintf(dataFile, "%u,%u,%u,%u,%u,%u,%u,%u,%u,%s\n",
          rightHeel, middleJoint, leftJoint, rightJoint,
          bigToe, index, middleToe, ring, leftHeel,
          currentLabel.c_str());
  fflush(dataFile); // force write to actually hit the drive, not just sit buffered
}
