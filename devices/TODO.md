# 남은 일 (데이터시트·시뮬레이션 확인 전)

<!-- nav -->[저장소 홈](../README.md) › [devices](README.md)
## 문서를 열어야 값을 확인할 수 있는 것
- SPI: ILI9341 데이터시트 본문(타이밍·초기화·COLMOD/MADCTL — 막힘, PDF 필요), microSD 사양, MCP4921 정식 데이터시트, SSD1306 SPI 타이밍·차지 펌프, MCP2515 명령 코드, nRF24L01+·MAX31855 SPI 모드, ADXL345 읽기 비트·W5500 RWB·MCP3008 프레임(도우미 미작성), PG153 SCK 비율 선택지(본문 상충)와 p.105 예제
- GPIO: AXI Timer PWM 0 %/100 %·ARHT 필요 여부(PG079 에 없음), 타코 캡처 후 카운터 동작, 엔코더 위상·클릭당 카운트, TTP223 원문, 부저 사양, 서보 3.3 V 신호 레벨
- I2C: SSD1306 데이터시트 원문 대조(현재 2차 자료 기반 드라이버 작성됨, COM 핀 0xDA 인자·최대 SCL 미확인), SHT3x 응답 바이트 배치, DS3231 비트 마스크, MPU-6050 온도 변환식, ADS1115(비교기·ALERT·고속 모드·일반 호출 리셋 미작성), PCF8574A 주소·MCP23017 인터럽트·BANK=1 지도(미작성), EEPROM, BME280 데이터시트 예제 계수로 재검증, 습도식 괄호 원문 대조
- UART: HC-05 KEY 핀/AT 모드 진입·AT 모드 보레이트, NEO-6M UBX 설정, GGA 필드 순서와 `$`·`*` 의 XOR 범위, ESP, LoRa
- CAN: RXNEMP 재세트 동작, MCP2515 SPI 명령, 트랜시버 핀·종단, 24 MHz can_clk 생성(MMCM), 라이선스 포함 여부
- PS/2: 송신 절차, 마우스 F4/응답, Basys3 핀, 스캔 코드 표
- Ethernet: Pmod NIC100 최대 SPI 클럭, W5500 모듈 사양, MicroBlaze V 에서 emaclite 동작
- I2S: 12.288 MHz 를 MMCM 으로 정확히 만들 수 있는지, 마스터 코덱 소자
- 기타: 1-Wire DS2438 등, BLE 모듈 선정
## 사용자 쪽 실행이 필요한 것
- Vivado 에서 모든 `tb/` Verilog 테스트벤치 실행(이 환경에는 시뮬레이터 없음), 합성·타이밍 확인
- 드라이버를 보드에서 실행(현재는 호스트 모의 테스트만)
각 항목은 제조사 문서를 열어 값을 확인한 뒤 해당 폴더에 추가.
