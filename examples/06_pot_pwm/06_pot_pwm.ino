// UNO R4 D9: PWM 지원 핀. 외부 LED에 직렬 저항 사용
const byte PWM_LED=9;
void setup() { pinMode(PWM_LED,OUTPUT); analogReadResolution(10); analogWriteResolution(8); }
void loop() {
  int adc=analogRead(A0);
  int duty=map(adc,0,1023,0,255);
  analogWrite(PWM_LED,constrain(duty,0,255));
  delay(15);
}
