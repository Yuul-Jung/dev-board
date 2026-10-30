const byte BTN=2;
bool lastRead=HIGH, stable=HIGH, enabled=false, led=false;
unsigned long edgeTime=0, tickTime=0;
void setup() {
  pinMode(BTN,INPUT_PULLUP);
  pinMode(LED_BUILTIN,OUTPUT);
}
void loop() {
  unsigned long now=millis();
  bool raw=digitalRead(BTN);
  if (raw!=lastRead) edgeTime=now;
  if (now-edgeTime>=30 && raw!=stable) {
    stable=raw;
    if (stable==LOW) {
      enabled=!enabled;
      tickTime=now;
      led=false;
      digitalWrite(LED_BUILTIN,LOW);
    }
  }
  lastRead=raw;
  if (enabled && now-tickTime>=500) {
    tickTime=now;
    led=!led;
    digitalWrite(LED_BUILTIN,led?HIGH:LOW);
  }
}
