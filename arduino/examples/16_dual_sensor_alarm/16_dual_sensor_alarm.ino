#include <OneWire.h>
#include <DallasTemperature.h>
const byte ONE_WIRE_PIN=2,TRIG=7,ECHO=8,BUZZER=9;
OneWire bus(ONE_WIRE_PIN);
DallasTemperature temp(&bus);
// 변환 대기를 loop 를 막지 않고 millis 로 처리한다(거리 경고가 온도 변환 때문에 늦어지지 않게).
// 12비트 변환 최대 750 ms(devices/1-wire/ds18b20.md). 더 낮은 해상도에서도 이 값이면 충분하다.
const unsigned long CONV_MS=750;
unsigned long convStart=0, lastPrint=0;
bool converting=false, tempValid=false;
float c=DEVICE_DISCONNECTED_C;
void setup() {
  pinMode(TRIG,OUTPUT); pinMode(ECHO,INPUT);
  pinMode(LED_BUILTIN,OUTPUT);
  Serial.begin(115200);
  temp.begin();
  temp.setWaitForConversion(false);
}
void loop() {
  unsigned long now=millis();
  if (!converting) { temp.requestTemperatures(); convStart=now; converting=true; }
  else if (now-convStart>=CONV_MS) { c=temp.getTempCByIndex(0); tempValid=true; converting=false; }
  digitalWrite(TRIG,LOW); delayMicroseconds(2);
  digitalWrite(TRIG,HIGH); delayMicroseconds(10);
  digitalWrite(TRIG,LOW);
  unsigned long us=pulseIn(ECHO,HIGH,30000UL);
  float cm=us?us*0.0343f/2.0f:-1.0f;
  bool inRange=(cm>=2.0f && cm<=400.0f); // 규격 범위 밖은 경고 판단에 쓰지 않는다
  bool alarm=inRange && cm<20.0f;
  digitalWrite(LED_BUILTIN,alarm?HIGH:LOW);
  if(alarm) tone(BUZZER,800); else noTone(BUZZER);
  if (now-lastPrint>=1000) {
    lastPrint=now;
    if(!tempValid) Serial.print("Temp WAIT");
    else if(c==DEVICE_DISCONNECTED_C) Serial.print("Temp ERROR");
    else { Serial.print("Temp C: ");Serial.print(c,1); }
    Serial.print("  Distance cm: ");
    if(inRange) Serial.println(cm,1); else Serial.println("none");
  }
  delay(60); // HC-SR04 측정 주기 60 ms 초과 권장
}
