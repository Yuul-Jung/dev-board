const byte BTN=2, BUZZER=9;
bool sounding=false;
void setup() { pinMode(BTN,INPUT_PULLUP); }
void loop() {
  bool pressed=(digitalRead(BTN)==LOW);
  if (pressed && !sounding) { tone(BUZZER,440); sounding=true; }
  if (!pressed && sounding) { noTone(BUZZER); sounding=false; }
}
