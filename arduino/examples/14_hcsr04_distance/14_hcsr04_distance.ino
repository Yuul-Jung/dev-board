// HC-SR04: cm = 에코 폭(µs)/58 근처(데이터시트 식은 devices/gpio/hc-sr04.md). 규격 범위 2~400 cm 밖은 신뢰하지 않는다.
const byte TRIG=7,ECHO=8;
void setup() {
  pinMode(TRIG,OUTPUT); pinMode(ECHO,INPUT);
  digitalWrite(TRIG,LOW);
  Serial.begin(115200);
}
void loop() {
  digitalWrite(TRIG,LOW); delayMicroseconds(2);
  digitalWrite(TRIG,HIGH); delayMicroseconds(10);
  digitalWrite(TRIG,LOW);
  unsigned long us=pulseIn(ECHO,HIGH,30000UL);
  if (us==0) Serial.println("TIMEOUT");
  else {
    float cm=(us*0.0343f)/2.0f;
    if (cm<2.0f || cm>400.0f) Serial.println("OUT OF RANGE");
    else { Serial.print("Distance cm: "); Serial.println(cm,1); }
  }
  delay(100); // 측정 주기 60 ms 초과 권장
}
