# tb

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [spi](../README.md)

호스트 테스트와 레지스터/장치 모델(mock)

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [mock_lcd.h](mock_lcd.h) | endif |
| [mock_qspi.h](mock_qspi.h) | endif |
| [tb_ili9341_engine.v](tb_ili9341_engine.v) | 자체 점검 테스트벤치 (미실행: 이 환경에 시뮬레이터 없음). |
| [test_hc595.c](test_hc595.c) | 74HC595 호스트 시험: SPI 층을 비트 단위 모델로 대체. 모델 규칙(데이터시트): 클럭 상승마다 시프트(DS → Q0 쪽에서 들어와 Q7S 로 나감), |
| [test_ili9341_sw.c](test_ili9341_sw.c) | 모의 레지스터: TX 쓰기를 {dc,byte} 로 기록, FILL 기록, delay 호출 기록 |
| [test_max7219_model.c](test_max7219_model.c) | MAX7219 체인 장치 모델 시험. 모델은 MAX7219/MAX7221 데이터시트(웹 가져오기 요약 경유)의 규칙을 구현하고 어기면 viol 에 기록한다. |
| [test_mcp2515.c](test_mcp2515.c) | MCP2515 호스트 시험. 두 층으로 나뉜다. |
| [test_mcp2515_model.c](test_mcp2515_model.c) | MCP2515 장치 모델 시험 (DS20001801K 규칙 구현). SPI 레벨: CS 하강 → 명령 바이트 → 데이터, CS 상승에서 마무리. |
| [test_nrf24_model.c](test_nrf24_model.c) | nRF24L01+ 장치 모델 시험. 모델은 Nordic Product Specification v1.0 의 규칙을 구현하고, 규칙을 어기면 viol 에 기록한다. |
| [test_pmod_cls.c](test_pmod_cls.c) | spi/tb/test_pmod_cls.c — Pmod CLS SPI LCD 드라이버 호스트 모의 시험. |
| [test_qspi_mock.c](test_qspi_mock.c) | 모의 IP + 소자: CS(SSR 비트 0)가 눌린 동안 바이트마다 소자 응답. 응답은 명령 바이트 뒤에 오는 스크립트 |
| [test_spi_sensors.c](test_spi_sensors.c) | MCP3008 / ADXL345 호스트 시험. SPI 층(qspi_cs/qspi_xfer)을 비트 단위 장치 모델로 대체. |
| [test_w5500_model.c](test_w5500_model.c) | W5500 드라이버 시험. 장치 모델은 w5500_model.h. 규칙·한계는 그 파일 머리말 참고. |
| [w5500_model.h](w5500_model.h) | W5500 장치 모델 시험. 모델은 W5500 Datasheet v1.0.5(웹 가져오기 요약 경유) 의 규칙을 구현하고, 어기면 viol 에 기록한다. |

<!-- index:end -->
