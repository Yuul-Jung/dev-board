# 교재 목차 대응표 (아두이노 우노 R4 교재 ↔ 이 저장소)

<!-- nav -->[저장소 홈](../README.md) › [devices](README.md)

사용자가 보여 준 「상상을 스케치하는 아두이노 우노 R4」 목차(7부 31장)를 학습 순서의 기준선으로 삼아, 같은 주제가 이 저장소에서 어디까지 채워졌는지 맞춰 본 표입니다.

**한계**: 책 본문은 보지 못했고 목차 제목만으로 대응시켰습니다. 책은 아두이노 API(`digitalWrite`, `Wire`, `SPI` 등)를 쓰지만 이 저장소는 MicroBlaze V + AXI IP/전용 RTL 위에서 같은 일을 하는 것이라, "같은 주제"는 하드웨어 소자·프로토콜 기준입니다.

**상태 표기**: 완비 = 문서 + 드라이버 + 호스트 테스트 / 문서 = 소자 문서만 / 일부 = 한쪽만 있거나 범위가 좁음 / 없음 / 해당 없음 = 아두이노 보드 고유 내용. 근거 수준(D1·D3·설계안)은 각 소자 문서를 따릅니다.

## 1~3부 기초 (1~17장)
| 책 | 주제 | 이 저장소 | 상태 |
|---|---|---|---|
| 1~5장 | 아두이노 보드·IDE·Serial 클래스 | 보드·IDE 는 해당 없음. Serial 에 해당하는 것은 [UART Lite 드라이버](uart/sw/uartlite.c) | 해당 없음 / 일부 |
| 6장 | 디지털 출력(LED, 깜박임, 제어) | [AXI GPIO 드라이버와 기본 입출력](gpio/basic-io.md): 핀 출력·토글·마스크 쓰기(모델 시험). 릴레이([relay-srd05.md](gpio/relay-srd05.md))도 같은 경로 | 완비(모델 시험, 보드 미실행) |
| 7장 | 디지털 입력(풀업/풀다운, 버튼, 누른 횟수) | [버튼 디바운스·누른 횟수·길게 누름](gpio/basic-io.md)(모델 시험). [TTP223](gpio/ttp223.md)·[PIR](gpio/hc-sr501.md)은 같은 AXI GPIO 입력 | 완비(모델 시험, 보드 미실행) |
| 8장 | 아날로그 입력(ADC, 가변저항, 조이스틱) | 외부 ADC: [MCP3008, ADS1115](spi/sensor-adc-dac.md). 가변저항 %·map·조이스틱 변환은 [analog-sensors.md](gpio/analog-sensors.md)(설계안, 테스트). XADC 는 분류표에서 언급만 | 일부 |
| 9장 | 아날로그 출력(PWM, LED 밝기, RGB LED) | [AXI Timer PWM 드라이버](gpio/sw/axi_timer_pwm.c), RGB LED·HSV([기본 입출력](gpio/basic-io.md), 모델 시험), [MCP4921 DAC](spi/sensor-adc-dac.md) | 완비(모델 시험, 보드 미실행) |
| 10장 | delay·millis | AXI Timer 자유 실행 시계 `mclock_*` 와 비차단 `ticker_*`([기본 입출력](gpio/basic-io.md), PG079 확인, 모델 시험) | 완비(모델 시험, 보드 미실행) |
| 11장 | 7세그먼트, 74595 | [74HC595](spi/comm-storage-io.md) 완비. 7세그 자체(보드 내장)는 [display.md](spi/display.md) 언급 수준 | 일부 |
| 12~13장 | 4자리 7세그·LED 매트릭스(MAX7219) | [MAX7219 문서와 드라이버](spi/display.md): 체인 8개, 숫자·문자·소수점·8×8(모델 시험, 요약 경유 D1) | 완비(모델 시험, 보드 미실행) |
| 14장 | 내장 LED 매트릭스 | 우노 R4 고유 | 해당 없음 |
| 15장 | 텍스트 LCD | [HD44780 드라이버](i2c/lcd1602.md): 16×2·20×4, 사용자 글자, 백라이트(모델 시험, 요약 경유 D1). 백팩 핀 배치는 모듈 관례(미확인) | 완비(모델 시험, 보드 미실행) |
| 16장 | 센서(LM35, CdS, PIR, 초음파, 온습도) | [PIR](gpio/hc-sr501.md), [HC-SR04](gpio/hc-sr04.md), [DHT22](gpio/dht22.md), LM35·CdS 변환([analog-sensors.md](gpio/analog-sensors.md), LM35 는 요약 경유 D1, CdS 조도 환산은 셀 자료 필요) | 일부(CdS lux 미확인) |
| 17장 | 소리(tone, RTTTL, 버저) | [부저](gpio/buzzer.md)(사양 미확인), PWM 드라이버. RTTTL 없음. I2S 오디오 [INMP441·MAX98357A](i2s/devices.md)는 책 밖 추가 | 일부 |

## 4~5부 통신·고급 입출력 (18~24장)
| 책 | 주제 | 이 저장소 | 상태 |
|---|---|---|---|
| 18장 | UART(SoftwareSerial, 보드 간 통신) | [uart/](uart/README.md) 완비(UART Lite, NMEA, HC-05). SoftwareSerial 은 해당 없음 | 완비 |
| 19장 | I2C(+ I2C 텍스트 LCD) | [i2c/](i2c/README.md) 완비(AXI IIC 드라이버, 센서·EEPROM·확장기, [I2C LCD](i2c/lcd1602.md)) | 완비 |
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
| 28장 | 블루투스 클래식(HC-06) | [HC-06 AT 명령 드라이버](uart/modules.md)(D3, 같은 저자 2건, 제조사 데이터시트 미확인) + [HC-05](uart/modules.md). 연결 후 데이터 송수신은 UART Lite 그대로 | 완비(호스트 시험까지) |
| 29장 | 저전력 블루투스(ESP32-S3) | [HM-10 계열 BLE UART 모듈 드라이버](uart/modules.md)(데이터시트 2건 요약 경유). 책의 ESP32-S3 자체는 모듈 선정 전 | 일부 |
| 30장 | 인터넷·Wi-Fi(웹 클라이언트, 날씨, NTP) | Wi-Fi/ESP 없음. 유선 대안은 [ethernet/](ethernet/README.md)의 W5500([드라이버·모델 시험](spi/comm-storage-io.md), 요약 경유 D1+D3), 소켓 위 응용은 [DNS·NTP 시계 맞추기·HTTP GET](ethernet/socket-apps.md)(호스트 모델 시험까지, HTTPS·날씨 JSON 해석은 없음) | 일부 |
| 31장 | 아두이노 클라우드 | 해당 없음 | 해당 없음 |

## 빈칸을 메운 기록과 남은 것
메운 것(모두 호스트 모델 시험까지, 보드에서 실행하지 않음): 1) MAX7219 드라이버(12~13장), 2) GPIO·타이머 기본 예제(6·7·9·10장), 3) LM35·CdS·가변저항·조이스틱 변환(8·16장), 4) HD44780 텍스트 LCD + PCF8574 백팩(15·19장), 5) W5500 문서·시험 정리(30장의 유선 쪽), 6) W5500 UDP 수신 헤더 해석 + SNTP 클라이언트 + HTTP/1.1 GET 클라이언트 + DNS 이름 풀기 + DHCP + JSON 값 꺼내기(30장의 웹 클라이언트·날씨·NTP, [socket-apps.md](ethernet/socket-apps.md)), 7) HC-06 와 HM-10(BLE) 의 AT 명령 드라이버 + 무음 구간으로 응답 끝을 판단하는 공통 송수신(28·29장, [uart/modules.md](uart/modules.md)). 근거는 각 문서에 적었고 W5500·MAX7219·PG144·LM35·HD44780 은 PDF 원문이 아니라 웹 가져오기 요약에서 옮긴 값입니다([VERIFICATION.md](VERIFICATION.md)).

남은 것:
1. **소켓 위 응용의 나머지**(30장): HTTPS, 실제 날씨 API 응답으로의 확인(JSON 해석 자체는 6절), NTP 왕복 지연 보정, 실제 서버와의 상호 동작 확인. DHCP·DNS·NTP·HTTP·JSON 의 기본 동작과 W5500 UDP 수신 헤더는 [socket-apps.md](ethernet/socket-apps.md).
2. **교재 6~9장 이후의 응용 예제**: 조이스틱·RGB 를 함께 쓰는 데모처럼 장치를 묶는 예제는 만들지 않았다.
3. **자료가 막힌 것**: microSD 규격, HC-06 제조사 데이터시트(2차 자료로 만들었음), 책의 ESP32-S3 BLE 와 Wi-Fi 모듈 선정, 부저 사양, 책의 도형·이미지 수준 OLED, 스텝 모터 속도 제어.

<!-- nav-foot -->[devices 목차](README.md)
