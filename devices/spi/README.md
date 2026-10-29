# SPI 소자와 SoC 연결

<!-- nav -->[저장소 홈](../../README.md) › [devices](../README.md)

Basys3(XC7A35T) + MicroBlaze V 에서 SPI 소자를 붙이는 방법을 정리합니다.
구분: **확인** = 이번 작업에서 데이터시트/제품 가이드를 열어 확인, **계산** = 확인 값으로 계산, **설계안** = 제안, **미확인** = 문서에서 못 찾음.

## 1. AMD IP: AXI Quad SPI (PG153) — 확인
| 항목 | 값 |
|---|---|
| 슬레이브 선택 수 | 1~32 (SPISSR 크기 한계) |
| FIFO 깊이 (표준 모드) | 0(없음), 16, 256 |
| SCK 비율 | PG153 v3.2 본문이 둘로 갈림: 커스터마이즈 절 "2에서 2,048까지 2의 거듭제곱" / 설계 노트 "2, 4, 16, 32 이후 16의 배수 2048까지". SCK = ext_spi_clk / 비율. 런타임 변경 불가(파라미터, 확인). Vivado 창에서 실제 선택지 확인 필요 |
| 전송 폭 | Transaction Width 8/16/32 (파라미터). 드라이버는 8 가정 |
| 모드 | 마스터/슬레이브, CPOL/CPHA, LSB first, 루프백 |

레지스터 오프셋 (확인)

| 레지스터 | 오프셋 | 리셋값 |
|---|---|---|
| SRR | 0x40 | 쓰기 전용(0x0000_000A) |
| SPICR | 0x60 | 0x180 |
| SPISR | 0x64 | 0x0A5 |
| SPI DTR | 0x68 | 0x0 |
| SPI DRR | 0x6C | - |
| SPISSR | 0x70 | 0xFFFF (선택 없음) |
| Tx FIFO 점유 | 0x74 | 0x0 (FIFO 16/256일 때만) |
| Rx FIFO 점유 | 0x78 | 0x0 (FIFO 16/256일 때만) |
| DGIER / IPISR / IPIER | 0x1C / 0x20 / 0x28 | 0x0 |

SPICR 비트 (확인): 9 LSB First, 8 Master Transaction Inhibit(리셋 1), 7 Manual Slave Select Assertion Enable(리셋 1), 6 Rx FIFO Reset, 5 Tx FIFO Reset, 4 CPHA, 3 CPOL(0=SCK 대기 Low), 2 Master, 1 SPE, 0 LOOP.
표준 슬레이브 모드는 SCK 비율 2에서 지원하지 않습니다 (확인).

추가 확인 (표 열람): SRR 은 0x0000000A 쓰기만 허용. SPISR 비트: 0 Rx_Empty(리셋 1), 1 Rx_Full, 2 Tx_Empty(리셋 1), 3 Tx_Full, 4 MODF, 5 Slave_Mode_Select(리셋 1), 6 CPOL_CPHA_Error(CPOL/CPHA 가 01 또는 10 일 때), 7 Slave Mode Error, 8 MSB Error, 9 Loopback Error, 10 Command Error. IPISR 비트 이름 14개(Command Error ~ MODF) 중 DRR_Not_Empty 는 비트 8. SPIDTR 는 채우기 전에 리셋 상태여야 함. 마스터 트랜잭션 inhibit 설정은 전송 중에 하면 결과 불명.
확인(PG153 PDF 열람): SCK 비율은 런타임 변경 불가, 전송 순서는 6절. 미확인: 인터럽트 조건 상세, 「Example Programming Sequence」(p.105) 본문.

## 2. 경로 결정
사용자 규칙: AMD IP가 있으면 IP, 없을 때만 전용 RTL.
SPI 마스터는 AXI Quad SPI 가 있으므로 **소자 접근은 IP**로 합니다. 전용 RTL 후보는 IP로 부족한 부분뿐입니다.

| 후보 | 이유 | 상태 |
|---|---|---|
| ILI9341 바이트·fill 엔진 | D/CX 제어와 프레임 1,228,800 비트(계산) 채우기를 CPU 없이 처리 | `rtl/` 에 작성, 미시뮬레이션 (`rtl/README.md`). 화면 DMA/메모리 스트리밍은 미작성 |
| 74HC595 / MAX7219 | 단순 시프트 출력이라 IP로 충분 | 전용 RTL 불필요 |

## 3. 버스 계획 (설계안)
확인된 모드 값으로 공용 모드를 찾았습니다.

| 소자 | 모드 | 최대 SCK | 근거 |
|---|---|---|---|
| ADXL345 | 3 (CPOL=1, CPHA=1) | 5 MHz | D1 |
| DS3234 | 1, 3 | 4 MHz (VCC 2.7~5.5 V) | D1 |
| MCP3008 | 0,0 / 1,1 | 5 V: 3.6 MHz, 2.7 V: 1.35 MHz | D1 |
| MCP2515 | 0,0 / 1,1 | 10 MHz | D1 |
| W5500 | 0, 3 | 보장 최소 33.3 MHz (설계 속도 80 MHz) | D1 |
| W25Q64JV | 0, 3 | 133 MHz | D1 |
| MAX31855 | 미확인 | 5 MHz | D1 (모드 미확인) |
| MAX7219 | 미확인 (CLK 상승 에지에서 로드) | 10 MHz | D1 |
| MFRC522 | 이름 없음 (SCK 상승 에지에서 안정, 하강 에지에서 변경) | 10 Mbit/s | D1 |
| nRF24L01+ | 미확인 | 10 Mbps | D1 (모드 미확인) |
| ILI9341 | 노션 문서는 Mode 0 | 미확인 | 사용자 자료 |
| SSD1306 | 미확인 | 미확인 | - |
| MCP4921 | 미확인 | 미확인 | - |

**센서 버스 (SPI0, 설계안)**: 모드 3 하나로 ADXL345, DS3234, MCP3008, MCP2515, W5500, W25Q64JV 를 공유 가능 (위 표의 확인 값 기준).
SCK 비율: MCP3008 의 3.3 V 한계가 문서에서 확인되지 않아(2.7 V 1.35 MHz, 5 V 3.6 MHz) 보수적으로 1.35 MHz 이하로 둡니다.
ext_spi_clk 를 100 MHz 로 가정하면(설계안) 2의 거듭제곱 비율에서 64 → 1.5625 MHz(1.35 MHz 초과), **128 → 781.25 kHz**(계산). 그래서 센서 버스는 비율 128 로 둔다. 이전 판의 "비율 80"은 PG153 본문으로 허용 여부가 불확실해 철회.
느리게 쓰는 소자만 모으고, 빠른 소자(W5500, 플래시, 디스플레이)는 별도 인스턴스(SPI1)로 분리합니다. SCK 비율은 런타임 변경이 불가능(PG153: 파라미터로만 설정)이므로 속도가 다른 소자는 인스턴스를 나눕니다(확인).

**CS 선**: 소자마다 1개. IP의 SPISSR 비트로 구동.
**핀 예산 (설계안)**: 버스 3선(SCK, MOSI, MISO) + CS 개수. Pmod 한 포트는 8신호(Basys3 RM)이므로 JC 한 포트에 버스 3 + CS 5가 들어갑니다. 디스플레이의 D/C, RESET 은 GPIO 별도.
Basys3 Pmod 핀 (RM, 확인): JA J1,L2,J2,G2,H1,K2,H2,G3 · JB A14,A16,B15,B16,A15,A17,C15,C16 · JC K17,M18,N17,P18,L17,M19,P17,R18.

## 4. 전압 주의
Pmod 신호는 3.3 V 로 취급합니다(설계안). 문서 값:
- MAX7219 V+ 4.0~5.5 V — 3.3 V 로 못 씀. 입력 하이 레벨은 미확인이므로 레벨 변환 여부 판단 전 확인 필요.
- MAX31855 3.0~3.6 V, ADXL345 2.0~3.6 V, W5500 3.3 V(입력 5 V 허용), W25Q64JV 2.7~3.6 V, MFRC522 2.5~3.6 V.
- nRF24L01+ 1.9~3.6 V, 입력 5 V 허용(3.6 V 초과 입력이면 VDD 2.7~3.3 V).
- DS3234 2.0~5.5 V, MCP3008 2.7~5.5 V, MCP2515 2.7~5.5 V, 74HC595 2.0~6.0 V.
- SSD1306: 로직 VDD 1.65~3.3 V, 패널 VCC 7~15 V(모듈은 보통 내장 승압 회로를 쓰지만 모듈별 확인 필요).

## 5. 소자 문서
| 문서 | 소자 |
|---|---|
| [display.md](display.md) | ILI9341, SSD1306, MAX7219 |
| [clock.md](clock.md) | DS3234 (시계), 시계류 인터페이스 정리 |
| [sensor-adc-dac.md](sensor-adc-dac.md) | MAX31855, ADXL345, MCP3008, MCP4921 |
| [comm-storage-io.md](comm-storage-io.md) | MCP2515, MFRC522, W5500, nRF24L01+, W25Q64JV, microSD, 74HC595 |

## 6. 드라이버 (devices/spi/sw, 호스트 모의 테스트만)
`axi_qspi.h/.c`(표준 SPI 마스터), `spi_devices.h/.c`(W25Q64JV JEDEC ID·BUSY, MAX31855 해석).

**전송 절차 (확인, PG153 v3.2 「Standard SPI Mode Transactions」 p.77, 수동 SS 흐름)**
1. SPISSR 을 전부 1 로 → 2. SPICR 에 수동 SS(bit7) + SPE + inhibit(bit8) 을 설정(SCK/IO0 초기화, 전송은 막음) → 3. inhibit 상태에서 DTR 를 채움(SPIDTR 는 리셋 상태에서 채워야 함) → 4. SPISSR 로 SS 선택 → 5. SPICR 에서 inhibit 을 해제(전송 시작) → 6. 인터럽트(IPISR bit4) 또는 폴링으로 완료 대기 → 7. inhibit 설정 → 8. 새 데이터 기록 후 다시 해제. 드라이버는 이 순서를 그대로 따른다(`qspi_xfer` 한 덩어리 = 3·5·6·7).
- 전송 중 inhibit 을 설정하면 결과를 알 수 없다(확인) → 완료 확인 뒤에만 설정.
- SPISSR 은 active-Low one-hot(확인). FIFO 가 있으면 FIFO 안 데이터는 모두 같은 SS 를 쓴다(SS 용 FIFO 없음, 확인).
- SPISR 의미(확인): FIFO 없음 — Tx_Empty 는 전송 완료 때 세트, Rx_Empty 는 수신 레지스터를 읽으면 세트되고 전송 성공 끝에서 지워짐. FIFO 있음 — Tx_Empty 는 FIFO 가 비는 즉시 세트되며 마지막 바이트는 아직 파이프라인에 있을 수 있음. 그래서 완료 판단은 **Rx_Empty 해제**를 사용(두 구성 모두 성립).
- 드라이버는 FIFO 깊이(0/16/256)에 맞춰 한 번에 DTR 로 넣는 양을 제한하고, 같은 양만큼 DRR 를 읽어 비운다(수신 FIFO 넘침 방지, 설계안). 문서는 INCR/버스트를 FIFO 주소에 쓰지 말라고 함 → 단일 AXI4-Lite 쓰기만 사용.
- 버스트(AXI4 성능 모드)·XIP·듀얼/쿼드 모드는 다루지 않음.

테스트(`tb/test_qspi_mock.c`): SPICR 값(모드 3 = 0x19E, 모드 0 = 0x186, 초기엔 inhibit), DTR 쓰기가 항상 inhibit 상태에서 일어나고 끝나면 다시 inhibit, CS 해제, W25Q JEDEC EF 40 17·BUSY, MAX31855 양수·음수·오류 비트, FIFO 16 에서 40바이트가 16+16+8 로 나뉘어 순서대로 돌아옴, 타임아웃 후 inhibit 복구 → ALL PASS. 모의 IP 는 PG153 에 적힌 대로 내가 구현한 것이라 실제 IP 동작의 검증은 아님.

미확인: 비율 선택지(위), 모드 변경 시 SPE 를 내려야 하는지(인스턴스당 모드 고정으로 회피), MAX31855 의 SPI 모드, ADXL345·W5500·MCP3008 도우미(읽기/쓰기 비트 값·프레임 미확인으로 미작성), 인터럽트 구동 방식.

## 7. 한계
- 모든 RTL/드라이버는 시뮬레이션·합성 전. RTL 은 ILI9341 엔진 하나뿐입니다.
- microSD, ILI9341 시리얼 타이밍·리셋 타이밍·초기화 순서(재시도했으나 발췌에 본문 없음), SSD1306 SPI 타이밍, MCP4921 명령어 형식은 이번에 열어본 문서에서 값을 얻지 못해 미확인으로 남겼습니다.

<!-- nav-foot -->← [i2c](../i2c/README.md) · [devices 목차](../README.md) · [uart](../uart/README.md) →
