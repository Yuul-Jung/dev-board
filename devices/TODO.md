# 다음 후보 (데이터시트 확인 전, 사양 미기록)
- GPIO: PIR 모션, 릴레이, 서보(PWM)
- 1-Wire: DS2438 등
- I2C: AXI IIC(PG090) 레지스터·절차, SSD1306 I2C 주소·명령 형식, SHT3x 응답 바이트 배치, DS3231 비트 배치, MPU-6050 온도 변환식, ADS1115, PCF8574, MCP23017, EEPROM
- SPI: microSD 사양 재확인, MCP4921 정식 데이터시트, ILI9341 타이밍·초기화 순서(데이터시트 본문 필요), SSD1306 SPI 타이밍, MCP2515 명령 코드, nRF24L01+·MAX31855 SPI 모드, PG153 전송 절차(`spi/README.md` 7절)
- UART: AXI UART Lite 보레이트 설정 방식(PG142 본문), HC-05 KEY 핀/AT 모드 진입, NEO-6M UBX 설정 명령, ESP, LoRa, NMEA 파서 검증
- I2S: PG308 본문(Artix-7 지원, 라이선스, 포트, 클럭), 마스터 코덱 소자, Vivado IP 카탈로그 확인 후 필요하면 전용 수신 RTL
- CAN: 트랜시버 모듈
- Bluetooth / Ethernet: 모듈 선정부터 필요
각 항목은 제조사 문서를 열어 값을 확인한 뒤 해당 폴더에 추가.
