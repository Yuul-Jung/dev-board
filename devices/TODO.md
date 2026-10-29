# 다음 후보 (데이터시트 확인 전, 사양 미기록)
- GPIO: 터치, 로터리 엔코더, 부저, 팬 / 서보 신호 레벨(3.3 V 인식) 확인
- 1-Wire: DS2438 등
- I2C: SSD1306 I2C 주소·명령 형식, SHT3x 응답 바이트 배치, DS3231 비트 배치, MPU-6050 온도 변환식, ADS1115, PCF8574, MCP23017, EEPROM
- SPI: microSD 사양 재확인, MCP4921 정식 데이터시트, ILI9341 타이밍·초기화 순서(데이터시트 본문 필요), SSD1306 SPI 타이밍, MCP2515 명령 코드, nRF24L01+·MAX31855 SPI 모드, PG153 전송 절차(`spi/README.md` 7절)
- UART: HC-05 KEY 핀/AT 모드 진입, NEO-6M UBX 설정 명령, ESP, LoRa, NMEA 파서 검증
- I2S: 마스터 코덱 소자, Vivado IP 카탈로그 확인 후 필요하면 전용 수신 RTL
- CAN: MCP2515 SPI 명령 코드, 트랜시버 핀 매핑·종단
- Ethernet: Pmod NIC100 최대 SPI 클럭, W5500 모듈 사양, MicroBlaze V 에서 emaclite 동작
- Bluetooth: BLE 모듈 선정
각 항목은 제조사 문서를 열어 값을 확인한 뒤 해당 폴더에 추가.
- PS/2: 송신 절차, 마우스 F4/응답, Basys3 핀 확인, Verilog 시뮬레이션
