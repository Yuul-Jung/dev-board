// 10차시: 수동 피에조 부저에서 버튼을 누르는 동안 3음 멜로디 반복
// 버튼 D2-GND, 수동 피에조 부저 D9-GND (구동 전류 확인)
// 능동 부저는 자체 발진하므로 이 멜로디 실습에 사용하지 않는다.
const byte BTN=2, BUZZER=9;
const unsigned int notes[]={440, 523, 659}; // A4, C5, E5
const unsigned long NOTE_MS=250;
bool playing=false;
byte noteIndex=0;
unsigned long noteAt=0;

void setup() {
  pinMode(BTN, INPUT_PULLUP);
}

void loop() {
  const unsigned long now=millis();
  const bool pressed=(digitalRead(BTN)==LOW);
  if (!pressed) {
    if (playing) noTone(BUZZER);
    playing=false;
    noteIndex=0;
    return;
  }
  if (!playing) {
    playing=true;
    noteAt=now;
    noteIndex=0;
    tone(BUZZER, notes[noteIndex]);
  } else if (now-noteAt>=NOTE_MS) {
    noteAt=now;
    noteIndex=(noteIndex+1)%3;
    tone(BUZZER, notes[noteIndex]);
  }
}
