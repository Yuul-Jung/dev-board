const byte DATA_PIN=2, CLOCK_PIN=3, LATCH_PIN=4;
// Q0..Q6 -> a..g, Q7 -> dp, 공통 캐소드 기준
const byte digits[10]={0x3F,0x06,0x5B,0x4F,0x66,
                       0x6D,0x7D,0x07,0x7F,0x6F};
byte n=0;
unsigned long last=0;
void showDigit(byte v) {
  digitalWrite(LATCH_PIN,LOW);
  shiftOut(DATA_PIN,CLOCK_PIN,MSBFIRST,v);
  digitalWrite(LATCH_PIN,HIGH);
}
void setup() {
  pinMode(DATA_PIN,OUTPUT); pinMode(CLOCK_PIN,OUTPUT);
  pinMode(LATCH_PIN,OUTPUT); showDigit(digits[0]);
}
void loop() {
  if (millis()-last>=1000) {
    last=millis();
    n=(n+1)%10;
    showDigit(digits[n]);
  }
}
