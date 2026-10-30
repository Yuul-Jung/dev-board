const byte PIR=2;
void setup() {
  pinMode(PIR,INPUT);
  pinMode(LED_BUILTIN,OUTPUT);
  Serial.begin(115200);
}
void loop() {
  bool motion=digitalRead(PIR)==HIGH;
  digitalWrite(LED_BUILTIN,motion?HIGH:LOW);
  static bool prev=false;
  if (motion!=prev) {
    Serial.println(motion?"MOTION":"CLEAR");
    prev=motion;
  }
  delay(30);
}
