# Arduino ↔ 공통 부품 ↔ Basys3 SoC 대응

[Arduino 목차](README.md) · [32차시](ROADMAP.md) · [기존 SoC 교재 대응](../devices/BOOK-MAP.md)

| 기능 | Arduino에서 사용 | 공통 부품 문서 | Basys3/MicroBlaze V에서 사용 |
|---|---|---|---|
| 버튼·LED | digitalRead/digitalWrite | [GPIO](../devices/gpio/basic-io.md) | AXI GPIO·RTL |
| ADC 입력 | analogRead | [아날로그](../devices/gpio/analog-sensors.md) | XADC 또는 외부 ADC + AXI |
| PWM | PWM 가능한 핀·타이머 지원 확인 | [팬/PWM](../devices/gpio/fan-pwm.md) | AXI Timer 또는 전용 RTL |
| UART | Serial / Serial1 등 보드별 포트 | [UART](../devices/uart/README.md) | AXI UART Lite |
| I2C | Wire | [I2C](../devices/i2c/README.md) | AXI IIC |
| SPI | SPI | [SPI](../devices/spi/README.md) | AXI Quad SPI |
| HC-SR04 | 디지털 타이밍 측정 | [초음파](../devices/gpio/hc-sr04.md) | 타이밍 RTL + 레지스터 |
| DHT22 | 디지털 타이밍/라이브러리 | [DHT22](../devices/gpio/dht22.md) | 전용 RTL·C 드라이버 |
| DS18B20 | 1-Wire 라이브러리 | [1-Wire](../devices/1-wire/README.md) | 전용 RTL·C 드라이버 |
| OLED | 디스플레이 라이브러리 | [OLED](../devices/i2c/ssd1306.md) | I2C + C 드라이버 |
| L298N | GPIO+PWM | [L298](../devices/motor/l298.md) | RTL + AXI 레지스터 |
| W5500 | SPI + 네트워크 라이브러리 | [W5500](../devices/ethernet/README.md) | SPI + C 네트워크 코드 |

## 반드시 구분
- `devices/`의 `sw/` 파일은 Arduino의 `.ino` 예제가 아니라 MicroBlaze V/호스트용 C 드라이버다.
- `devices/`의 '완비'는 실물 검증 완료를 의미하지 않는다.
- Arduino 보드와 Basys3 Pmod의 I/O 전기 사양을 혼동하지 않는다.
- 센서 칩 데이터시트와 구매한 모듈 보드의 전원·레벨시프터·풀업·점퍼를 각각 확인한다.
- Arduino 측의 코드/실물 상태는 `arduino/`에서 독립 관리한다.
