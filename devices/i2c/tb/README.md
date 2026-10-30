# tb

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [i2c](../README.md)

호스트 테스트와 레지스터/장치 모델(mock)

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [fuzz_bme280.c](fuzz_bme280.c) | 적대적 시험: 현실적인 범위의 임의 보정 계수·원시값으로 UBSan 아래에서 보정식을 돌려 오버플로·미정의 동작을 찾는다. |
| [mock_iic.h](mock_iic.h) | 호스트 모의 테스트용: 레지스터 접근을 모의 함수로 바꾼다. gcc -include tb/mock_iic.h 로 사용 |
| [test_ads1115.c](test_ads1115.c) | ADS1115 드라이버 호스트 시험: I2C 층(axi_iic_write/write_read)을 ADS1115 동작 모델로 대체한다. |
| [test_axi_iic_mock.c](test_axi_iic_mock.c) | AXI IIC 드라이버 호스트 테스트. PG090 의 예제 워드열과 비교한다. |
| [test_bme280.c](test_bme280.c) | 계수: 널리 쓰이는 BMP280 데이터시트 예제 값(T,P). 습도 계수는 임의의 현실적 값 — 습도는 범위·단조성만 검사 |
| [test_hd44780.c](test_hd44780.c) | HD44780 드라이버 시험 — 데이터시트 요약(i2c/ref/hd44780_facts.txt)을 옮겨 만든 모델(자작)에 대해서만 확인한다. 실제 LCD 모듈이 |
| [test_io_expander.c](test_io_expander.c) | PCF8574 / MCP23017 호스트 시험: I2C 층을 장치 모델로 대체. 모델은 확인한 규칙만 구현 |
| [test_oled_gfx.c](test_oled_gfx.c) | SSD1306 그래픽 확장(선·원·삼각형·비트맵·5x7 글꼴) 호스트 시험 (교재 23장). |
| [test_rtc_eeprom.c](test_rtc_eeprom.c) | DS1307 / 24xx256 호스트 시험. I2C 층을 장치 모델로 대체. |
| [test_ssd1306.c](test_ssd1306.c) | SSD1306 드라이버 호스트 시험: axi_iic_write 를 가짜로 바꿔 전송 바이트열을 기록하고 검사한다. |

<!-- index:end -->
