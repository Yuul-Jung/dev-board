# dev-board

개발 보드 조사와 Basys3 + MicroBlaze V SoC 학습 자료.

## 폴더
| 폴더 | 내용 |
|---|---|
| [`catalog/`](catalog/README.md) | 보드 50종(티어 1–5) 카탈로그와 주변장치 요약표 |
| [`atlas/`](atlas/README.md) | Basys3 IO 회로도 아틀라스 |
| [`soc-map/`](soc-map/README.md) | Basys3 SoC 주변장치 구현 맵 ([soc_map.html](soc-map/soc_map.html)) |
| [`devices/`](devices/README.md) | 인터페이스별 외부 소자(센서·모듈) 자료, 전용 RTL, 드라이버 |

## 주변장치 자료 바로가기 (`devices/`)
| 인터페이스 | 폴더 |
|---|---|
| GPIO (초음파, DHT22, PIR, 릴레이, 엔코더, 터치, 팬, 부저) | [gpio](devices/gpio/README.md) |
| 1-Wire (DS18B20) | [1-wire](devices/1-wire/README.md) |
| I2C (BME280, MPU-6050, SHT31, BH1750, DS3231) | [i2c](devices/i2c/README.md) |
| SPI (디스플레이, 시계, 센서·ADC·DAC, 통신·저장) | [spi](devices/spi/README.md) |
| UART (NEO-6M, HC-05, MAX485) | [uart](devices/uart/README.md) |
| I2S (INMP441, MAX98357A) | [i2s](devices/i2s/README.md) |
| CAN (AXI CAN, MCP2515) | [can](devices/can/README.md) |
| 이더넷 (W5500, AXI Ethernet Lite) | [ethernet](devices/ethernet/README.md) |
| 블루투스 | [bluetooth](devices/bluetooth/README.md) |
| PS/2 | [ps2](devices/ps2/README.md) |
| 모터·서보·스테퍼 | [motor](devices/motor/README.md) |

범주 전체: [CATEGORIES](devices/CATEGORIES.md) · 남은 일: [TODO](devices/TODO.md) · **검증 결과: [VERIFICATION](devices/VERIFICATION.md)**

## 읽는 법
모든 소자 문서는 값마다 근거 수준(확인 / 계산 / 설계안 / 미확인, D1 = 데이터시트 열람, D3 = 2차 자료)을 적습니다. 코드는 호스트 모의 테스트까지만, Verilog 는 시뮬레이션 전입니다. 자세한 기준은 [devices/README.md](devices/README.md).

내용은 폴더별로 계속 추가·갱신합니다.
