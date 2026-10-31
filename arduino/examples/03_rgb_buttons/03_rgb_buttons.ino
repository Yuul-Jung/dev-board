// 공통 캐소드 RGB LED. 각 색상 핀에 개별 전류 제한 저항 필수.
const byte B1=2, B2=4, R=3, G=5, B=6;
void setup() {
  pinMode(B1,INPUT_PULLUP); pinMode(B2,INPUT_PULLUP);
  pinMode(R,OUTPUT); pinMode(G,OUTPUT); pinMode(B,OUTPUT);
}
void loop() {
  bool a=digitalRead(B1)==LOW, b=digitalRead(B2)==LOW;
  digitalWrite(R,a&&!b); digitalWrite(G,b&&!a);
  digitalWrite(B,a&&b);
}
