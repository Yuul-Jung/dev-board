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
    Serial.print("Distance cm: "); Serial.println(cm,1);
  }
  delay(100);
}
