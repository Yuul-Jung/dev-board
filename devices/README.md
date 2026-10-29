# devices — 인터페이스별 주변장치 자료

<!-- nav -->[저장소 홈](../README.md)

Basys3 + MicroBlaze V SoC에 붙일 외부 소자(센서·모듈)를 통신 인터페이스별로 정리합니다.
SoC 쪽 구현 경로(AXI IP, 핀 예산)는 `../soc-map/` 과 짝을 이룹니다.

## 폴더
| 폴더 | 인터페이스 | 소자 문서 |
|---|---|---|
| [`gpio/`](gpio/README.md) | GPIO (트리거/에코, 단일선, 엔코더, PWM 부하 포함) | HC-SR04, DHT22, HC-SR501, 릴레이, 로터리 엔코더, TTP223, 팬, 부저 |
| [`1-wire/`](1-wire/README.md) | 1-Wire | DS18B20 |
| [`i2c/`](i2c/README.md) | I2C | BME280, MPU-6050, SHT31, BH1750, DS3231 (AXI IIC 본문 확인, 드라이버 `sw/`) |
| [`motor/`](motor/README.md) | DC 모터, 서보, 스테퍼 구동 | L298, TB6612FNG, SG90, 28BYJ-48 |
| [`spi/`](spi/README.md) | SPI 소자 14종 + AXI Quad SPI 버스 계획 | 작성됨 (PG153 본문 확인, QSPI·ILI9341 드라이버, ILI9341 전용 RTL; 일부 미확인) |
| [`uart/`](uart/README.md) | UART 모듈 | NEO-6M, HC-05, MAX485 (UART Lite 본문 확인, 드라이버·NMEA RMC) |
| [`i2s/`](i2s/README.md) | I2S 오디오 | INMP441, MAX98357A, AMD I2S IP(PG308) 일부 확인 |
| [`can/`](can/README.md) | CAN 컨트롤러·트랜시버 | AXI CAN(PG096 본문 확인, 드라이버), MCP2515(데이터시트 PDF 로 명령·레지스터 확인, 드라이버는 spi/sw), SN65HVD230 |
| [`ps2/`](ps2/README.md) | PS/2 키보드·마우스 | 수신 전용 RTL, 프로토콜은 2차 자료 |
| [`ethernet/`](ethernet/README.md) | 이더넷 3경로 비교 | W5500, Pmod NIC100, AXI Ethernet Lite (일부 미확인) |
| [`bluetooth/`](bluetooth/README.md) | Bluetooth | HC-05 는 `uart/` 에 있음 |
| [`data/`](data/README.md) | 위 소자의 구조화 데이터 ([devices.json](data/devices.json)) | |

**전용 RTL 방침**: 열람한 AMD IP가 없는 인터페이스는 AXI4-Lite 슬레이브 전용 RTL로 설계합니다. 현재 `1-wire/rtl/`, `gpio/rtl/`(HC-SR04 래퍼, DHT22), `motor/rtl/`(DC 모터, 서보 PWM) — 모두 시뮬레이션 전. `ps2/rtl/`(수신 전용). I2S, CAN 은 AMD IP 확인으로 대상에서 제외.

전체 범주 목록과 상태는 [CATEGORIES.md](CATEGORIES.md), 남은 일은 [TODO.md](TODO.md), 문서 형식은 [TEMPLATE.md](TEMPLATE.md).

## 검증 자료
- [VERIFICATION.md](VERIFICATION.md): 적대적 검증 결과와 재현 방법 (원문 대조, 새니타이저, 퍼징, 링크 검사).
- `./run_host_tests.sh`: 호스트 테스트 일괄 실행(`SAN=1` 이면 ASan/UBSan, `FUZZ=1` 이면 퍼저 포함). `python3 tools/check_links.py`: 문서 링크 검사. `tools/crosscheck_pdf.py`: 코드 값과 AMD 문서 대조.
- 저장소 홈: [../README.md](../README.md)

## 근거 수준
- **D1 데이터시트 열람**: 제조사 또는 제조사 문서 사본을 열어 값을 확인함. 사본 출처를 소자 문서에 적음.
- **D3 판매처·2차 자료만**: 값이 판매처 페이지 등에서만 나옴.
- **설계안**: 일반 지식이나 계산으로 세운 구현 제안. 실제 보드에서 검증 전.
소자 문서의 "확인" 항목은 각 문서에 적힌 근거 수준을 따르며(D3 가 섞인 문서가 있음: HC-05, HC-SR501, 릴레이, PS/2, TTP223, 엔코더 등), 계산은 계산식을 함께 적었습니다.

## 소자 문서 형식
`TEMPLATE.md` 참고. 항목: 개요 / 전기 사양 / 인터페이스 / SoC 연결(Basys3) / 근거와 한계.

## 인터페이스별 SoC 구현 요약 (AMD 문서 열람분만)
| 인터페이스 | AMD IP | 확인한 내용 |
|---|---|---|
| GPIO | AXI GPIO (PG144) | 1–32비트, 이중 채널, 비트별 입출력(TRI), 선택적 인터럽트 |
| UART | AXI UART Lite (PG142 v2.0) | 본문 확인: 16 FIFO, 5–8비트, 패리티, 보레이트는 설계 시점 파라미터 |
| I2C | AXI IIC (PG090 v2.1) | 본문 확인: 100k/400k/1M, 레지스터, THIGH/TLOW 식, 동적 절차 |
| SPI | AXI Quad SPI (PG153 v3.2) | 본문 확인: 레지스터·SPICR/SPISR 비트·수동 SS 전송 절차, 비율 런타임 변경 불가. 비율 선택지는 본문이 상충 |
| 타이머·캡처 | AXI Timer (PG079 v2.0) | 본문 확인: 오프셋, TCSR 비트, PWM 공식, 캡처. 0 %/100 % PWM 처리는 문서에 없음 |
| I2S | I2S Transmitter/Receiver (PG308) | 본문 확인: 7 series 지원, 무료, 마스터 전용, 24/16비트 |
| CAN | CAN (PG096 v5.1) | 본문 확인: 레지스터·초기화·비트 타이밍, 7 series, 라이선스는 카탈로그 확인 필요 |
| Ethernet | AXI Ethernet Lite (PG135 v3.0) | 본문 확인: MII 18신호, 2 KB 버퍼, 무료, 레지스터 맵 |
| PS/2, 1-Wire, Bluetooth | — | 열람한 AMD IP 없음 → 전용 RTL로 설계 (1-Wire 먼저 작성) |
