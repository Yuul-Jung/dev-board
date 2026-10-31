// UNO R4 10-bit ADC / 8-bit PWM, 공통 캐소드 RGB LED
const byte R=3,G=5,B=6;
void setup() {
  analogReadResolution(10); analogWriteResolution(8);
  pinMode(R,OUTPUT); pinMode(G,OUTPUT); pinMode(B,OUTPUT);
}
void loop() {
  int x=analogRead(A0), y=analogRead(A1);
  int red=map(x,0,1023,0,255);
  int green=map(y,0,1023,0,255);
  int distance=max(abs(x-512),abs(y-512));
  int blue=map(constrain(distance,0,512),0,512,255,0);
  analogWrite(R,red); analogWrite(G,green); analogWrite(B,blue);
  delay(20);
}
