// UNO R4: 실습에서는 10-bit(0~1023) 해상도로 명시 설정
void setup() { Serial.begin(115200); analogReadResolution(10); }
void loop() {
  int raw=analogRead(A0);
  float volts=raw*(5.0f/1023.0f); // 5V 기준 가정: 실제 VREF와 오차 가능
  Serial.print("ADC="); Serial.print(raw);
  Serial.print(" approx V="); Serial.println(volts,2);
  delay(200);
}
