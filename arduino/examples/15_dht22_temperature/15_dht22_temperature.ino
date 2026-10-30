#include <DHT.h>
const byte DHT_PIN=2;
DHT dht(DHT_PIN,DHT22);
void setup() {
  Serial.begin(115200);
  dht.begin();
}
void loop() {
  delay(2000); // DHT22 갱신 간격 확보
  float h=dht.readHumidity();
  float t=dht.readTemperature();
  if (isnan(h)||isnan(t)) {
    Serial.println("DHT22 READ ERROR");
    return;
  }
  Serial.print("T(C): "); Serial.print(t,1);
  Serial.print(" RH(%): "); Serial.println(h,1);
}
