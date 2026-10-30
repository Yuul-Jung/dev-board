// 밝을수록 A0 증가하는 분압 배치: 출력은 어두울수록 증가
const byte PWM_LED=9;
void setup() { pinMode(PWM_LED,OUTPUT); analogReadResolution(10); analogWriteResolution(8); }
void loop() {
  int light=analogRead(A0);
  int duty=map(constrain(light,0,1023),0,1023,255,0);
  analogWrite(PWM_LED,duty);
  delay(30);
}
