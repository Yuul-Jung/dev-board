# 다음 후보 (데이터시트 확인 전, 사양 미기록)
- GPIO: PIR 모션, 릴레이, 서보(PWM)
- 1-Wire: DS2438 등
- I2C: MPU-6050, SHT31, BH1750, OLED(SSD1306)
- SPI: microSD 사양 재확인, MCP4921 정식 데이터시트, ILI9341 타이밍·초기화 순서, SSD1306 SPI 타이밍, MCP2515 명령 코드, nRF24L01+·MAX31855 SPI 모드, PG153 전송 절차(`spi/README.md` 7절)
- UART: GPS 모듈, 블루투스 SPP 모듈
- I2S: MEMS 마이크, 오디오 코덱
- CAN: 트랜시버 모듈
- Bluetooth / Ethernet: 모듈 선정부터 필요
각 항목은 제조사 문서를 열어 값을 확인한 뒤 해당 폴더에 추가.
