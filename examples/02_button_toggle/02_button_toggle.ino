// 버튼 디바운스 + 누를 때마다 LED 토글 (millis 비차단)
const byte BTN = 2;
bool ledOn = false, stable = HIGH, previous = HIGH;
unsigned long changedAt = 0;
void setup() { pinMode(BTN, INPUT_PULLUP); pinMode(LED_BUILTIN, OUTPUT); }
void loop() {
  bool reading = digitalRead(BTN);
  if (reading != previous) changedAt = millis();
  if (millis() - changedAt >= 30 && reading != stable) {
    stable = reading;
    if (stable == LOW) {
      ledOn = !ledOn;
      digitalWrite(LED_BUILTIN, ledOn ? HIGH : LOW);
    }
  }
  previous = reading;
}
