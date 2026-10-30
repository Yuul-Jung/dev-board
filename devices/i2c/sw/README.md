# sw

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [i2c](../README.md)

드라이버 C 소스

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [ads1115.c](ads1115.c) |  |
| [ads1115.h](ads1115.h) | ADS1115 16비트 I2C ADC. axi_iic 드라이버 위에서 동작. |
| [axi_iic.c](axi_iic.c) | 송신 오류 복구: ISR 비트 토글 지우기 + TX FIFO 리셋 (PG090: 송신 오류 시 TX FIFO 리셋 필요) |
| [axi_iic.h](axi_iic.h) | AXI IIC (PG090 v2.1) 동적 컨트롤러 드라이버. 마스터, 7비트 주소. |
| [bme280.c](bme280.c) | 음수 값의 좌측 시프트는 C 에서 미정의 동작이라 곱셈으로 쓴다(우측 시프트는 gcc 에서 산술 시프트로 동작, Bosch 식이 이에 의존). |
| [bme280.h](bme280.h) | BME280 드라이버. 레지스터 맵·보정식은 Bosch 데이터시트(BST-BME280-DS002) 4.2.2~4.2.3, 3.5.1 (PDF 추출, D1). |
| [hd44780.c](hd44780.c) |  |
| [hd44780.h](hd44780.h) | HD44780 문자 LCD(16×2, 20×4 등) 4비트 드라이버 — 버스 방식에 의존하지 않는다: 포트 한 바이트를 쓰는 함수와 마이크로초 지연 함수만 받는다 |
| [hd44780_pcf8574.c](hd44780_pcf8574.c) |  |
| [hd44780_pcf8574.h](hd44780_pcf8574.h) | HD44780 를 PCF8574 I2C 백팩 위에서 쓰기 위한 접착 코드. PCF8574 드라이버(io_expander.h)로 포트 바이트를 쓰고, 마이크로초 지 |
| [i2c_sensors.c](i2c_sensors.c) |  |
| [i2c_sensors.h](i2c_sensors.h) | I2C 소자 코드: MPU-6050, SHT31, BH1750, DS3231. |
| [io_expander.c](io_expander.c) |  |
| [io_expander.h](io_expander.h) | I2C I/O 확장기: PCF8574(8비트 준양방향), MCP23017(16비트). |
| [rtc_eeprom.c](rtc_eeprom.c) |  |
| [rtc_eeprom.h](rtc_eeprom.h) | DS1307 실시간 시계 + 24xx256 계열 I2C EEPROM(2바이트 주소). axi_iic 위에서 동작. |
| [ssd1306.c](ssd1306.c) |  |
| [ssd1306.h](ssd1306.h) | SSD1306 128x64 OLED, I2C. axi_iic 드라이버 위에서 동작. 교재 23장(도형·이미지) 대응으로 선·원·삼각형·비트맵·5x7 글꼴을 추가했 |

<!-- index:end -->
