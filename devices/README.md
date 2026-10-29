# devices — 인터페이스별 주변장치 자료

Basys3 + MicroBlaze V SoC에 붙일 외부 소자(센서·모듈)를 통신 인터페이스별로 정리합니다.
SoC 쪽 구현 경로(AXI IP, 핀 예산)는 `../soc-map/` 과 짝을 이룹니다.

## 폴더
| 폴더 | 인터페이스 | 소자 문서 |
|---|---|---|
| `gpio/` | GPIO (트리거/에코, 단일선 독자 프로토콜 포함) | HC-SR04, DHT22 |
| `1-wire/` | 1-Wire | DS18B20 |
| `i2c/` | I2C | BME280 |
| `spi/` `uart/` `i2s/` `can/` `bluetooth/` `ethernet/` | 각 인터페이스 | 아직 없음 (`TODO.md` 참고) |
| `data/devices.json` | 위 소자의 구조화 데이터 | |

**전용 RTL 방침**: 열람한 AMD IP가 없는 인터페이스는 AXI4-Lite 슬레이브 전용 RTL로 설계합니다. 현재 `1-wire/rtl/` (시뮬레이션 전). 다음 후보: I2S, DHT22 단일선, PS/2, CAN.

## 근거 수준
- **D1 데이터시트 열람**: 제조사 또는 제조사 문서 사본을 열어 값을 확인함. 사본 출처를 소자 문서에 적음.
- **D3 판매처·2차 자료만**: 값이 판매처 페이지 등에서만 나옴.
- **설계안**: 일반 지식이나 계산으로 세운 구현 제안. 실제 보드에서 검증 전.
소자 문서의 "확인" 항목은 모두 D1이며, 계산은 계산식을 함께 적었습니다.

## 소자 문서 형식
`TEMPLATE.md` 참고. 항목: 개요 / 전기 사양 / 인터페이스 / SoC 연결(Basys3) / 근거와 한계.

## 인터페이스별 SoC 구현 요약 (AMD 문서 열람분만)
| 인터페이스 | AMD IP | 확인한 내용 |
|---|---|---|
| GPIO | AXI GPIO (PG144) | 1–32비트, 이중 채널, 비트별 입출력(TRI), 선택적 인터럽트 |
| UART | AXI UART Lite (PG142) | 16문자 FIFO, 5–8비트, 패리티, 상승 에지 인터럽트 |
| I2C | AXI IIC (PG090) | 100k/400k/1M, 마스터·슬레이브, 7/10비트, FIFO 16바이트 |
| SPI | AXI Quad SPI (PG153) | 기능 요약만 확인 |
| 타이머·캡처 | AXI Timer (PG079) | Generate / Capture / PWM / Cascade 모드 (오프셋 미확인) |
| 1-Wire, I2S, CAN, Bluetooth, Ethernet | — | 열람한 AMD IP 없음 → 전용 RTL로 설계 (1-Wire 먼저 작성) |
