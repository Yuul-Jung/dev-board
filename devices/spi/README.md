# SPI 소자와 SoC 연결

Basys3(XC7A35T) + MicroBlaze V 에서 SPI 소자를 붙이는 방법을 정리합니다.
구분: **확인** = 이번 작업에서 데이터시트/제품 가이드를 열어 확인, **계산** = 확인 값으로 계산, **설계안** = 제안, **미확인** = 문서에서 못 찾음.

## 1. AMD IP: AXI Quad SPI (PG153) — 확인
| 항목 | 값 |
|---|---|
| 슬레이브 선택 수 | 1~32 (SPISSR 크기 한계) |
| FIFO 깊이 (표준 모드) | 0(없음), 16, 256 |
| SCK 비율 (표준 모드) | 2, 4, 8, 16×N (N=1~128) |
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
미확인: 실행 중 SCK 비율 변경 가능 여부, 마스터 전송의 정확한 순서(「Protocol Description」 p.70, 「Example Programming Sequence」 p.105 본문을 열지 못함; 6절은 FreeBSD 드라이버 기준), 인터럽트 조건. 6절 순서는 2차 자료 기준입니다.

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
AXI 클럭을 100 MHz 로 가정하면(설계안) 비율 80 = 5×16 이 표준 모드에서 허용되고 SCK = 1.25 MHz (계산).
느리게 쓰는 소자만 모으고, 빠른 소자(W5500, 플래시, 디스플레이)는 별도 인스턴스(SPI1)로 분리합니다. SCK 비율이 런타임 변경 가능한지 미확인이라 인스턴스 분리가 안전합니다.

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
`axi_qspi.h/.c`(표준 SPI, 바이트 단위), `spi_devices.h/.c`(W25Q64JV JEDEC ID·BUSY, MAX31855 해석).
- 레지스터·비트: PG153 표에서 확인(위 1절). 전송 순서: PG153 본문은 열지 못했고 **FreeBSD `axi_quad_spi.c`(D3)** 를 따름 — SRR=0xA → SPICR 로 FIFO 리셋 → DGIER=0 → SPICR=MASTER|MSS|SPE(inhibit 0), CS 는 SPISSR 비트 0 = 선택(active-low), 바이트마다 DTR 쓰기 → 완료 대기 → DRR 읽기. 그 드라이버는 CPOL/CPHA·클럭 분주·인터럽트·FIFO 를 쓰지 않았음.
- 이 드라이버가 더한 것(설계안): 모드 0~3 비트 설정(CPOL=bit3, CPHA=bit4), 완료 대기를 TX_EMPTY 대신 **Rx_Empty 해제**로 함(마지막 비트가 아직 나가는 중일 가능성 때문), 타임아웃.
- 테스트(`tb/test_qspi_mock.c`): SPICR 값 0x9E(모드 3)/0x86(모드 0), CS 해제, W25Q JEDEC EF 40 17, BUSY 비트, MAX31855 양수·음수·오류 비트, 타임아웃 → ALL PASS. 모의 IP 는 내가 쓴 가정(FreeBSD 순서)대로 동작하므로 실제 IP 동작의 검증이 아님.
- 미확인: SCK 분주비 설정 방법(IP 파라미터로 고정되는지), FIFO 깊이 0 구성에서 SPISR 비트 동작, 모드를 바꿀 때 SPE 를 내려야 하는지(인스턴스당 모드 고정으로 회피), MAX31855 의 SPI 모드, ADXL345·W5500·MCP3008 도우미는 읽기/쓰기 비트 값이나 프레임이 미확인이라 만들지 않음.

## 7. 한계
- 모든 RTL/드라이버는 시뮬레이션·합성 전. RTL 은 ILI9341 엔진 하나뿐입니다.
- microSD, ILI9341 시리얼 타이밍·리셋 타이밍·초기화 순서(재시도했으나 발췌에 본문 없음), SSD1306 SPI 타이밍, MCP4921 명령어 형식은 이번에 열어본 문서에서 값을 얻지 못해 미확인으로 남겼습니다.
