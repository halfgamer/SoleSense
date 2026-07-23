void setup() {
  Serial.begin(115200);
  while (!Serial);

  analogReadResolution(12); // 0-4095 range

  Serial.println();
  Serial.println("==================== NEW RUN ====================");
  Serial.println();
}

void loop() {
  int a0 = analogRead(A0);
  int a1 = analogRead(A1);
  int a2 = analogRead(A2);
  int a3 = analogRead(A3);
  int a4 = analogRead(A4);
  int a5 = analogRead(A5);

  Serial.print("A0: "); Serial.print(a0);
  Serial.print("  A1: "); Serial.print(a1);
  Serial.print("  A2: "); Serial.print(a2);
  Serial.print("  A3: "); Serial.print(a3);
  Serial.print("  A4: "); Serial.print(a4);
  Serial.print("  A5: "); Serial.println(a5);

  delay(100);
}
