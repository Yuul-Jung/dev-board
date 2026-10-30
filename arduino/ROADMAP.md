# Arduino UNO R4 전용 32차시 테크트리

[Arduino 목차](README.md) · [기존 부품 대응](CROSSWALK.md)

기준 보드: Arduino UNO R4 계열. Minima/WiFi의 기능 차이가 있는 실습은 별도 보드 확인 후 진행. 각 차시의 예제는 작성·컴파일·실물 검증 단계가 독립적이다.

| 단계 | 차시 | 핵심 실습 | 누적 구성 | 관련 공통 자료 |
|---|---:|---|---|---|
| 1 기초 | 01 | LED와 버튼으로 출력 제어 | 버튼 1 + LED 1 | [기본 IO](../devices/gpio/basic-io.md) |
| 1 기초 | 02 | 버튼 디바운스·상태 전환 | 버튼 1 + LED 1 | [기본 IO](../devices/gpio/basic-io.md) |
| 1 기초 | 03 | 복수 버튼·RGB LED 제어 | 입력 2 + RGB 출력 | [기본 IO](../devices/gpio/basic-io.md) |
| 1 기초 | 04 | Serial 명령으로 LED 제어 | UART 입력 + LED 출력 | [UART](../devices/uart/README.md) |
| 2 아날로그 | 05 | 가변저항 아날로그 읽기 | ADC + Serial | [아날로그](../devices/gpio/analog-sensors.md) |
| 2 아날로그 | 06 | PWM 밝기 제어 | 가변저항 + LED | [PWM](../devices/gpio/README.md) |
| 2 아날로그 | 07 | CdS 조도 자동 조명 | CdS + LED PWM | [아날로그](../devices/gpio/analog-sensors.md) |
| 2 아날로그 | 08 | 조이스틱 + RGB LED | 아날로그 입력 2축 + RGB 출력 | [아날로그](../devices/gpio/analog-sensors.md) |
| 3 시간·표시 | 09 | millis() 기반 비차단 타이머 | 버튼 + LED | [기본 IO](../devices/gpio/basic-io.md) |
| 3 시간·표시 | 10 | 능동·수동 부저 구분 및 멜로디 | 버튼 + 부저 | [부저](../devices/gpio/buzzer.md) |
| 3 시간·표시 | 11 | 7세그먼트·시프트 레지스터 | 버튼 + 숫자 표시 | [SPI 주변](../devices/spi/comm-storage-io.md) |
| 3 시간·표시 | 12 | MAX7219 LED 매트릭스 | 버튼 + 매트릭스 | [표시장치](../devices/spi/display.md) |
| 4 센서 | 13 | PIR 움직임 감지 | PIR + LED | [PIR](../devices/gpio/hc-sr501.md) |
| 4 센서 | 14 | HC-SR04 거리 측정 | 초음파 + Serial | [초음파](../devices/gpio/hc-sr04.md) |
| 4 센서 | 15 | DHT22 온습도 | DHT22 + Serial | [온습도](../devices/gpio/dht22.md) |
| 4 센서 | 16 | DS18B20 온도·거리 경고 통합 | 온도·초음파 + LED/부저 | [1-Wire](../devices/1-wire/README.md) |
| 5 I2C | 17 | I2C 스캐너·주소 확인 | I2C 센서 + Serial | [I2C](../devices/i2c/README.md) |
| 5 I2C | 18 | BH1750 조도 측정 | BH1750 + LED | [I2C 센서](../devices/i2c/sensors.md) |
| 5 I2C | 19 | LCD1602 정보 표시 | 센서 + LCD | [LCD](../devices/i2c/lcd1602.md) |
| 5 I2C | 20 | SSD1306 OLED 대시보드 | 센서 2 + OLED | [OLED](../devices/i2c/ssd1306.md) |
| 6 SPI·저장 | 21 | SPI 장치 선택·기본 전송 | SPI 주변장치 + Serial | [SPI](../devices/spi/README.md) |
| 6 SPI·저장 | 22 | microSD 데이터 기록 | 센서 + 저장 | [SPI 저장](../devices/spi/comm-storage-io.md) |
| 6 SPI·저장 | 23 | RTC 데이터 타임스탬프 | DS3231 + Serial/저장 | [RTC](../devices/i2c/clock.md) |
| 6 SPI·저장 | 24 | MPU-6050 자세값 표시 | IMU + OLED | [I2C 센서](../devices/i2c/sensors.md) |
| 7 구동기 | 25 | DC 모터 + 드라이버 | 버튼 + 모터 | [모터](../devices/motor/l298.md) |
| 7 구동기 | 26 | SG90 서보 위치 제어 | 가변저항 + 서보 | [서보](../devices/motor/sg90.md) |
| 7 구동기 | 27 | 28BYJ-48 스테퍼 | 버튼 + 스테퍼 | [스테퍼](../devices/motor/28byj-48.md) |
| 7 구동기 | 28 | 센서 기반 모터 제어 | 거리 + 모터 + 상태 LED | [모터](../devices/motor/README.md) |
| 8 통신·통합 | 29 | Bluetooth UART 명령 | Bluetooth + LED/서보 | [UART 모듈](../devices/uart/modules.md) |
| 8 통신·통합 | 30 | CAN 또는 RS-485 통신 | 외부 통신 모듈 + Serial | [CAN](../devices/can/README.md) · [UART](../devices/uart/README.md) |
| 8 통신·통합 | 31 | 네트워크 기반 센서 모니터링 | 보드/모듈 선정 후 구현 | [이더넷](../devices/ethernet/README.md) |
| 8 통신·통합 | 32 | 캡스톤: 센서+표시+구동+통신 | 2센서 + 2출력 이상 | [공통 소자](../devices/README.md) |

## 실습 제작 기준
- **1~12차시**: GPIO·ADC·PWM·시간 기반 제어를 먼저 확립한다.
- **13~24차시**: 단일 센서·출력에서 센서 결합·디스플레이·데이터 기록으로 확장한다.
- **25~32차시**: 구동기, 통신, 복합 시스템으로 확장한다.
- 22·30·31차시는 모듈/보드 사양 확인 후 코드와 배선을 확정한다.
- Arduino UNO R4 WiFi의 무선 기능, UNO R4 Minima와의 차이, 외부 모듈의 3.3V/5V 허용 여부를 차시별로 확인한다.
- 기존 [SoC 교재 대응표](../devices/BOOK-MAP.md)의 31개 장은 별도 원전 목차다. 위 32차시와 번호가 같다고 동일 수업은 아니다.
