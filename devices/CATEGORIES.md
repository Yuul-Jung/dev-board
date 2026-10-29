# 주변장치 전체 분류 (필수 항목 목록)

<!-- nav -->[저장소 홈](../README.md) › [devices](README.md)

사용자가 직접 든 예(온도, 습도, 초음파)에 한정하지 않고, Basys3 SoC에서 실제로 필요한 외부 장치 범주를 모두 적었습니다.
"문서" 열은 이 저장소에 확인된 자료가 있는지입니다. 소자 이름은 후보이며, 사양은 데이터시트를 열람한 뒤에 적습니다.
"SoC 경로"의 AMD IP는 문서를 열어 확인한 것(GPIO, UART Lite, IIC, Quad SPI, Timer, CAN, I2S, Ethernet Lite, XADC)만 적었습니다.

| 범주 | 소자(후보) | 인터페이스 | SoC 경로 | 문서 |
|---|---|---|---|---|
| 온도 | DS18B20, DHT22, MAX31855(열전대), 서미스터 | 1-Wire, 단일선, SPI, 아날로그 | 전용 RTL, SPI IP, XADC | DS18B20, DHT22, MAX31855 |
| 습도·기압 | BME280, SHT31 | I2C/SPI | AXI IIC, Quad SPI | BME280, SHT31 |
| 거리 | HC-SR04, ToF(VL53L0X 등), 적외선 | GPIO, I2C, 아날로그 | 전용 RTL, IIC | HC-SR04 |
| 조도 | CdS(사용자 노션), BH1750 | 아날로그 또는 I2C | XADC, IIC | BH1750 |
| 모션 | MPU-6050, ADXL345, 자이로 | I2C/SPI | IIC, Quad SPI | MPU-6050, ADXL345 |
| 인체·접촉 | PIR, 터치, 버튼, 로터리 엔코더 | GPIO | AXI GPIO | HC-SR501 (PIR), TTP223, 로터리 엔코더(전용 RTL, 미시뮬레이션) |
| 공기질·가스 | MQ 계열, CO2 모듈, SGP30 | 아날로그, UART, I2C | XADC, UART Lite, IIC | 없음 |
| 전류·전압 | INA219 | I2C | IIC | 없음 |
| 위치 | GPS 모듈(NEO-6M) | UART | UART Lite | NEO-6M |
| 카메라 | OV7670 등 | 병렬 + I2C(SCCB) | 전용 RTL | 없음 |
| **DC 모터** | L298, TB6612FNG | PWM + GPIO | 전용 RTL(`motor/rtl`) | L298, TB6612FNG |
| **서보** | SG90 | PWM | AXI Timer PWM 또는 전용 RTL | SG90 (전용 RTL 작성, 미시뮬레이션; AXI Timer PWM 대안은 PG079 확인) |
| **스테퍼** | 28BYJ-48 + ULN2003 | GPIO 4핀 | AXI GPIO, 시퀀서 RTL | 28BYJ-48 |
| BLDC | ESC | PWM | AXI Timer PWM | 없음 |
| 릴레이·솔레노이드·부저·팬 | 릴레이 모듈 등 | GPIO(+PWM) | AXI GPIO | 릴레이 SRD-05VDC-SL-C, 팬 PWM·타코, 부저(사양 미확인) |
| LED 스트립 | WS2812 | 단일선(타이밍 엄격) | 전용 RTL | 없음 |
| 표시 | 7세그(온보드), LCD1602, OLED SSD1306, TFT ILI9341(사용자 노션), VGA(온보드) | GPIO, I2C, SPI, VGA | AXI GPIO, IIC, Quad SPI, 전용 RTL | ILI9341(전용 RTL+드라이버, 초기화 표는 데이터시트 본문 필요), SSD1306, MAX7219 (일부 미확인) |
| 시계 | DS3234(SPI), DS1302(3선), DS1307, DS3231 | SPI, 3선, I2C | Quad SPI, 전용 RTL, IIC | DS3234, DS3231 |
| 저장 | microSD, SPI 플래시(W25Q64JV), I2C EEPROM | SPI, I2C | Quad SPI, IIC | W25Q64JV (microSD 미확인) |
| ADC·DAC | MCP3008, ADS1115, MCP4921 | SPI, I2C | Quad SPI, IIC | MCP3008, MCP4921(일부) |
| I/O 확장 | PCF8574, MCP23017, 74HC595 | I2C, SPI | IIC, Quad SPI | 74HC595 |
| 오디오 | I2S 마이크(INMP441), I2S 앰프(MAX98357A) | I2S | AMD I2S IP(PG308, 일부 확인) | INMP441, MAX98357A |
| 무선·유선 통신 | HC-05, ESP 모듈, LoRa, W5500, RS-485(MAX485), CAN(AXI CAN 또는 MCP2515, +트랜시버 SN65HVD230), RFID(MFRC522), nRF24L01+, IR 리모컨 | UART, SPI, GPIO | UART Lite, Quad SPI, 전용 RTL | HC-05, MAX485, MCP2515, MFRC522, W5500, nRF24L01+ |
| 입력 장치 | PS/2 키보드·마우스(온보드 USB-HID 변환) | PS/2 | 전용 RTL(수신 전용) | PS/2 README (2차 자료) |
| **절연·레벨·전원** | 포토커플러(PC817, 사용자 노션), 레벨 시프터, 풀업·풀다운 | 하드웨어 | 해당 없음 | L298 문서에 일부 |

## 모든 소자에 공통으로 필요한 점검 항목
- 전압 레벨: 3.3 V FPGA 입력에 5 V 출력이 들어가지 않는가
- 전원 용량과 GND 공통, 모터·릴레이는 전원 분리(사용자 노션: 포토커플러)
- 풀업·풀다운: 리셋 중 입력이 플로팅되어 모터 급발진 같은 동작이 나오지 않는가(사용자 노션)
- 외부 비동기 입력의 2-FF 동기화
- 읽기 주기, 전원 투입 후 대기 시간(소자 문서에 적음)
- 핀 예산: 구현 맵 페이지의 Pmod 24 + 8 핀 계산기

## 진행 순서 제안
1. 센서·모터에서 문서가 있는 소자를 SoC 맵과 맞춰 정리(완료: 온도·습도·거리·모터·서보·스테퍼)
2. 표시(TFT, OLED), 시계(RTC), 모션(IMU) — SPI 소자는 `spi/`, I2C 소자(MPU-6050, SHT31, BH1750, DS3231)는 `i2c/` 에 작성 완료. SSD1306 I2C 드라이버 작성(2차 자료 기반, [i2c/ssd1306.md](i2c/ssd1306.md)). ADS1115 드라이버 작성([i2c/ads1115.md](i2c/ads1115.md)). PCF8574·MCP23017 드라이버 작성([i2c/io-expander.md](i2c/io-expander.md)). DS1307·24xx256 EEPROM 드라이버 작성([i2c/rtc-eeprom.md](i2c/rtc-eeprom.md)).
3. 통신 모듈(UART, SPI)과 저장
4. 오디오(I2S)와 CAN — `i2s/`, `can/` 작성 완료
