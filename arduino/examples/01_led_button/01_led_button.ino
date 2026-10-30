// 01차시: 버튼을 누르는 동안 내장 LED를 켭니다.
// 대상: Arduino UNO R4 계열. 별도 버튼을 D2와 GND 사이에 연결합니다.
// INPUT_PULLUP 사용: 평상시 HIGH, 버튼 누르면 LOW.
const int BUTTON_PIN = 2;
const int LED_PIN = LED_BUILTIN;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  const bool pressed = (digitalRead(BUTTON_PIN) == LOW);
  digitalWrite(LED_PIN, pressed ? HIGH : LOW);
}
