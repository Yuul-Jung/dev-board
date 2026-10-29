# 교재 목차 대응표 (아두이노 우노 R4 교재 ↔ 이 저장소)

<!-- nav -->[저장소 홈](../README.md) › [devices](README.md)

사용자가 보여 준 「상상을 스케치하는 아두이노 우노 R4」 목차(7부 31장)를 학습 순서의 기준선으로 삼아, 같은 주제가 이 저장소에서 어디까지 채워졌는지 맞춰 본 표입니다.

**한계**: 책 본문은 보지 못했고 목차 제목만으로 대응시켰습니다. 책은 아두이노 API(`digitalWrite`, `Wire`, `SPI` 등)를 쓰지만 이 저장소는 MicroBlaze V + AXI IP/전용 RTL 위에서 같은 일을 하는 것이라, "같은 주제"는 하드웨어 소자·프로토콜 기준입니다.

**상태 표기**: 완비 = 문서 + 드라이버 + 호스트 테스트 / 문서 = 소자 문서만 / 일부 = 한쪽만 있거나 범위가 좁음 / 없음 / 해당 없음 = 아두이노 보드 고유 내용. 근거 수준(D1·D3·설계안)은 각 소자 문서를 따릅니다.

## 1~3부 기초 (1~17장)
| 책 | 주제 | 이 저장소 | 상태 |
|---|---|---|---|
| 1~5장 | 아두이노 보드·IDE·Serial 클래스 | 보드·IDE 는 해당 없음. Serial 에 해당하는 것은 [UART Lite 드라이버](uart/sw/uartlite.c) | 해당 없음 / 일부 |
| 6장 | 디지털 출력(LED, 깜박임, 제어) | AXI GPIO 출력 경로는 [릴레이](gpio/relay-srd05.md)가 같은 방식. LED 전용 문서·예제 없음 | 없음 |
| 7장 | 디지털 입력(풀업/풀다운, 버튼, 누른 횟수) | [TTP223](gpio/ttp223.md)·[PIR](gpio/hc-sr501.md)이 AXI GPIO 입력. 버튼 디바운스·횟수 세기 없음 | 없음 |
| 8장 | 아날로그 입력(ADC, 가변저항, 조이스틱) | 외부 ADC: [MCP3008, ADS1115](spi/sensor-adc-dac.md). 가변저항·조이스틱 예제 없음. XADC 는 분류표에서 언급만 | 일부 |
| 9장 | 아날로그 출력(PWM, LED 밝기, RGB LED) | [AXI Timer PWM 드라이버](gpio/sw/axi_timer_pwm.c), [MCP4921 DAC](spi/sensor-adc-dac.md). RGB LED 없음 | 일부 |
| 10장 | delay·millis | AXI Timer(PG079 확인). millis 식 비차단 패턴 예제 없음 | 일부 |
| 11장 | 7세그먼트, 74595 | [74HC595](spi/comm-storage-io.md) 완비. 7세그 자체(보드 내장)는 [display.md](spi/display.md) 언급 수준 | 일부 |
| 12~13장 | 4자리 7세그·LED 매트릭스(MAX7219) | [MAX7219 문서](spi/display.md)만 있고 드라이버 없음 | 문서 |
| 14장 | 내장 LED 매트릭스 | 우노 R4 고유 | 해당 없음 |
| 15장 | 텍스트 LCD | [PCF8574](i2c/io-expander.md)(I2C 백팩 경로)만 있음. LCD 컨트롤러 문서 없음 | 없음 |
| 16장 | 센서(LM35, CdS, PIR, 초음파, 온습도) | [PIR](gpio/hc-sr501.md), [HC-SR04](gpio/hc-sr04.md), [DHT22](gpio/dht22.md) 있음. LM35·CdS 없음([BH1750](i2c/sensors.md)이 조도 대체) | 일부 |
| 17장 | 소리(tone, RTTTL, 버저) | [부저](gpio/buzzer.md)(사양 미확인), PWM 드라이버. RTTTL 없음. I2S 오디오 [INMP441·MAX98357A](i2s/devices.md)는 책 밖 추가 | 일부 |

## 4~5부 통신·고급 입출력 (18~24장)
| 책 | 주제 | 이 저장소 | 상태 |
|---|---|---|---|
| 18장 | UART(SoftwareSerial, 보드 간 통신) | [uart/](uart/README.md) 완비(UART Lite, NMEA, HC-05). SoftwareSerial 은 해당 없음 | 완비 |
| 19장 | I2C(+ I2C 텍스트 LCD) | [i2c/](i2c/README.md) 완비(AXI IIC 드라이버, 센서·EEPROM·확장기). LCD 는 위 15장과 같은 빈칸 | 완비 |
| 20장 | SPI + SD 카드 | [spi/](spi/README.md) 완비(AXI Quad SPI, 14종+). microSD 는 [미확인](spi/comm-storage-io.md) | 일부 |
| 21장 | CAN | [can/](can/README.md): AXI CAN, MCP2515(드라이버·모델 테스트), SN65HVD230 | 완비 |
| 22장 | RTC | [DS3231·DS1307](i2c/clock.md), [DS3234](spi/clock.md). 내장 RTC 는 해당 없음 | 완비 |
| 23장 | OLED(도형·이미지) | [SSD1306 드라이버](i2c/ssd1306.md). 책의 도형·이미지 수준까지 가는지는 확인 안 함 | 일부 |
| 24장 | 가속도·각속도(MPU-6050) | [MPU-6050](i2c/sensors.md) 드라이버. ADXL345 는 책 밖 추가 | 완비 |

## 6~7부 모터·무선 (25~31장)
| 책 | 주제 | 이 저장소 | 상태 |
|---|---|---|---|
| 25장 | DC 모터 | [L298](motor/l298.md), [TB6612FNG](motor/tb6612fng.md), 전용 RTL, [드라이버](motor/sw/dcmotor.c) | 일부(RTL 시뮬레이션 전) |
| 26장 | 서보 | [SG90](motor/sg90.md), 전용 RTL | 일부(RTL 시뮬레이션 전) |
| 27장 | 스텝 모터 | [28BYJ-48 + ULN2003](motor/28byj-48.md). 속도 제어 라이브러리 수준은 없음 | 문서 |
| 28장 | 블루투스 클래식(HC-06) | [HC-05](uart/modules.md)(D3). 책이 쓰는 HC-06 은 없음 | 일부 |
| 29장 | 저전력 블루투스(ESP32-S3) | [bluetooth/](bluetooth/README.md): 모듈 선정 전 | 없음 |
| 30장 | 인터넷·Wi-Fi(웹 클라이언트, 날씨, NTP) | Wi-Fi/ESP 없음. 유선 대안은 [ethernet/](ethernet/README.md)의 W5500(드라이버 작성됨), 소켓 위 응용은 없음 | 일부 |
| 31장 | 아두이노 클라우드 | 해당 없음 | 해당 없음 |

## 빈칸 가운데 지금 바로 가능한 것 (순서 제안)
1. **MAX7219 드라이버**: 데이터시트가 이미 D1 이라 새 자료 없이 12~13장을 메운다.
2. **GPIO·타이머 기본 예제 묶음**(6·7·9·10장): LED·버튼 디바운스·RGB·비차단 타이머. AMD PG144(GPIO)·PG079(Timer) 외에 소자 자료가 필요 없다.
3. **LM35·CdS**: 아날로그 입력(ADS1115/MCP3008) 위의 변환식. LM35 데이터시트가 필요.
4. **텍스트 LCD(HD44780 + PCF8574 백팩)**: 15·19장. HD44780 데이터시트가 필요.
5. **소켓 위 응용**(30장 웹 클라이언트·NTP): W5500 문서·테스트 마무리 뒤에.
6. 자료가 막힌 것: microSD 규격, HC-06, BLE/Wi-Fi 모듈 선정, 부저 사양.

<!-- nav-foot -->[devices 목차](README.md)
