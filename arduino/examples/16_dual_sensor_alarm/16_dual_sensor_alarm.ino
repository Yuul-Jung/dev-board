#include <OneWire.h>
#include <DallasTemperature.h>
const byte ONE_WIRE_PIN=2,TRIG=7,ECHO=8,BUZZER=9;
OneWire bus(ONE_WIRE_PIN);
DallasTemperature temp(&bus);
void setup() {
  pinMode(TRIG,OUTPUT); pinMode(ECHO,INPUT);
  pinMode(LED_BUILTIN,OUTPUT);
  Serial.begin(115200);
  temp.begin();
}
void loop() {
  temp.requestTemperatures();
  float c=temp.getTempCByIndex(0);
  digitalWrite(TRIG,LOW); delayMicroseconds(2);
  digitalWrite(TRIG,HIGH); delayMicroseconds(10);
  digitalWrite(TRIG,LOW);
  unsigned long us=pulseIn(ECHO,HIGH,30000UL);
  float cm=us?us*0.0343f/2.0f:-1.0f;
  bool alarm=us>0 && cm<20.0f;
  digitalWrite(LED_BUILTIN,alarm?HIGH:LOW);
  if(alarm) tone(BUZZER,800); else noTone(BUZZER);
  if(c==DEVICE_DISCONNECTED_C) Serial.print("Temp ERROR");
  else { Serial.print("Temp C: ");Serial.print(c,1); }
  Serial.print("  Distance cm: ");Serial.println(cm,1);
  delay(1000);
}
