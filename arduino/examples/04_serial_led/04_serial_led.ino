// Serial Monitor: 115200 baud, 명령 '1' 또는 '0'
void setup() {
  pinMode(LED_BUILTIN,OUTPUT);
  Serial.begin(115200);
  Serial.println("Send 1=ON or 0=OFF");
}
void loop() {
  if (!Serial.available()) return;
  char c=Serial.read();
  if (c=='1') { digitalWrite(LED_BUILTIN,HIGH); Serial.println("ON"); }
  if (c=='0') { digitalWrite(LED_BUILTIN,LOW); Serial.println("OFF"); }
}
