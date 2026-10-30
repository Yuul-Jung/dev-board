# sw

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [spi](../README.md)

드라이버 C 소스

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [axi_qspi.c](axi_qspi.c) |  |
| [axi_qspi.h](axi_qspi.h) | AXI Quad SPI (PG153 v3.2) 마스터 드라이버, 표준 SPI 모드. |
| [ili9341.c](ili9341.c) |  |
| [ili9341.h](ili9341.h) | ILI9341 드라이버 (ili9341_axil 전용 RTL 위). 레지스터 맵은 rtl/README.md 와 같다. |
| [max7219.c](max7219.c) |  |
| [max7219.h](max7219.h) | MAX7219 8자리 7세그/8×8 LED 드라이버 (체인 최대 8개). axi_qspi 위에서 동작. LOAD 를 CS 로 쓴다(MAX7221 도 같은 방식). |
| [mcp2515.c](mcp2515.c) | 표본 점 = (1 + PRSEG + PHSEG1) / n 을 87.5% 에 가깝게. S = PRSEG + PHSEG1 |
| [mcp2515.h](mcp2515.h) | MCP2515 독립형 CAN 컨트롤러 (SPI). axi_qspi 위에서 동작. 모드 0,0 / 1,1, SCK ≤ 10 MHz. |
| [nrf24l01.c](nrf24l01.c) |  |
| [nrf24l01.h](nrf24l01.h) | nRF24L01+ 2.4 GHz 송수신기 (SPI + CE + IRQ). axi_qspi 위에서 동작. SPI 모드 0,0 (그림 26~28 에서 읽음, 글로 명 |
| [pmod_cls.c](pmod_cls.c) | Pmod CLS 드라이버 구현. |
| [pmod_cls.h](pmod_cls.h) | Pmod CLS — SPI 문자 LCD 16×2 드라이버. |
| [spi_devices.c](spi_devices.c) |  |
| [spi_devices.h](spi_devices.h) | SPI 소자 도우미. 사양은 devices/spi 의 md 문서 (데이터시트 확인분)에서만 가져왔다. |
| [spi_sensors.c](spi_sensors.c) | 3바이트 프레임(24클럭): 앞의 0 일곱 개 + 시작 비트, SGL/D2/D1/D0, 나머지는 결과를 받기 위한 클럭 |
| [spi_sensors.h](spi_sensors.h) | SPI 센서: MCP3008(10비트 ADC), ADXL345(가속도). axi_qspi 위에서 동작. |
| [w5500.c](w5500.c) | 프레임 하나: 주소 2바이트(MSB 먼저) + 제어 1바이트 + 데이터. SCSn 은 프레임 내내 Low (p.14, p.19, p.22) |
| [w5500.h](w5500.h) | W5500 유선 이더넷(TCP/IP 하드와이어) 드라이버. axi_qspi 위에서 동작. SPI 모드 0 또는 3, 설계 속도 80 MHz / 보장 최소 33.3 |

<!-- index:end -->
