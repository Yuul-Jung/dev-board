#include <SPI.h>
const byte CS_PIN=10;
void writeReg(byte addr,byte value) {
  digitalWrite(CS_PIN,LOW);
  SPI.transfer(addr); SPI.transfer(value);
  digitalWrite(CS_PIN,HIGH);
}
void setup() {
  pinMode(CS_PIN,OUTPUT); digitalWrite(CS_PIN,HIGH);
  SPI.begin();
  writeReg(0x0F,0); // display test off
  writeReg(0x0C,1); // shutdown exit
  writeReg(0x09,0); // no BCD decode
  writeReg(0x0B,7); // 8 rows
  writeReg(0x0A,3); // intensity
}
void loop() {
  for (byte row=1;row<=8;row++)
    writeReg(row,(byte)(1u<<(row-1)));
  delay(500);
  for (byte row=1;row<=8;row++)
    writeReg(row,(byte)(0x80u>>(row-1)));
  delay(500);
}
