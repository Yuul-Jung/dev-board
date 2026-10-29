# SPI 통신·저장·I/O 확장

## MCP2515 (CAN 컨트롤러) — D1 일부
| 항목 | 값 |
|---|---|
| CAN | V2.0B, 최대 1 Mb/s |
| SPI | 10 MHz, 모드 0,0 / 1,1 |
| 전원 | 2.7~5.5 V |
| 버퍼 | 수신 2개(RXB0, RXB1), 송신 3개(우선순위·중단 기능) |
| 핀 | INT 1개 (PDIP/SOIC 12번) |
| 동작 모드 | 설정(Configuration) 모드에서만 마스크·필터·비트 타이밍 변경, Sleep 대기 전류 1 µA(전형) |
| SPI 명령 옵코드, CANCTRL 주소 | 열어본 발췌에 없음 — 미확인 |

CAN 버스에는 트랜시버가 별도로 필요합니다 (`../CATEGORIES.md`). SoC: 센서 버스 + CS + INT GPIO.

## MFRC522 (13.56 MHz RFID) — D1
| 항목 | 값 |
|---|---|
| 전원 | AVDD/DVDD/TVDD 2.5~3.6 V, PVDD/SVDD 1.6~3.6 V |
| 규격 | ISO/IEC 14443 A / MIFARE, NTAG |
| 호스트 | SPI 10 Mbit/s, I2C 400 kBd(Fast)/3400 kBd(High-speed), UART 1228.8 kBd |
| SPI 주소 바이트 | 비트 7 = 읽기(1)/쓰기(0), 비트 6~1 주소, LSB = 0 |
| SPI 타이밍 | MSB 먼저, SCK 상승 에지에서 데이터 안정, 하강 에지에서 변경 (모드 이름은 문서에 없음) |
| FIFO | 64바이트 |
| IRQ | 23번 핀, CMOS/오픈 드레인은 DivIEnReg 의 IRQPushPull |
| NRSTPD | 6번 핀, Low = 파워다운, 상승 에지로 리셋, 최소 Low 100 ns |
| VersionReg 0x37 값 | 미확인 (레지스터 이름만 확인) |

## W5500 (유선 이더넷, TCP/IP 하드와이어) — D1
| 항목 | 값 |
|---|---|
| SPI | 모드 0, 3 · 설계 속도 80 MHz, 보장 최소 33.3 MHz |
| 프레임 | 16비트 오프셋 주소 + 8비트 제어(BSB[4:0], RWB, OM[1:0]) + 데이터 |
| OM | 00 가변 길이(CS 가 길이 결정), 01/10/11 고정 1/2/4바이트 |
| 소켓 | 8개 |
| 버퍼 | 32 KB (TX 16 KB, RX 16 KB), 초기 소켓당 2 KB |
| 프로토콜 | TCP, UDP, ICMP, IPv4, ARP, IGMP, PPPoE (IP 단편화 미지원) |
| PHY | 10BaseT/100BaseTX 내장, 자동 협상 |
| 전원 | 3.3 V (2.97~3.63 V), 5 V 입력 허용 |
| 레지스터 | MR 0x0000 (리셋 0x00), VERSIONR 0x0039 = 0x04 |

연결 테스트: VERSIONR(0x0039)을 읽어 0x04 확인. SoC: 빠른 버스(SPI1) + CS + INT/RST GPIO. 이더넷 경로로는 AXI Ethernet Lite 와 대안 관계 (IP 문서는 `../ethernet/` 에서 다룰 예정).

## W25Q64JV (SPI NOR 플래시 64 Mbit) — D1 일부
| 항목 | 값 |
|---|---|
| 용량·전원 | 64 Mbit(8 MB), 2.7~3.6 V |
| SPI | 모드 0, 3 · 표준 SPI 최대 133 MHz |
| 페이지 | 256바이트, 32,768 페이지 |
| 지우기 | 4 KB 섹터 20h · 32 KB 블록 52h · 64 KB 블록 D8h · 칩 C7h 또는 60h |
| 명령 | 06h Write Enable · 03h Read · 0Bh Fast Read · 02h Page Program · 05h Read Status-1 · B9h Power-down · ABh Release/Device ID · 9Fh JEDEC ID |
| JEDEC ID | EFh 40h 17h (ID15-0 = 4017h). 장치 ID 표에는 16h 도 나옴 |
| BUSY | Status-1 비트 S0(읽기 전용), 쓰기·지우기 중 1 |
| 프로그램/지우기 시간 | AC 특성 절을 못 열어 값 없음 — 미확인 |

연결 테스트: 9Fh → EF 40 17. 쓰기 순서(설계안): Write Enable → Page Program → BUSY 폴링.
참고: Basys3 는 Artix-7 구성용 QSPI 플래시가 따로 있으나(이 문서에서 확인하지 않음, 미확인) 이 칩은 외부 Pmod 용 별도 소자입니다.

## nRF24L01+ (2.4 GHz 무선) — D1 일부
| 항목 | 값 |
|---|---|
| 전송 속도 | 250 kbps / 1 Mbps / 2 Mbps |
| 전원 | 1.9~3.6 V, 입력 5 V 허용 (3.6 V 초과 입력이면 VDD 2.7~3.3 V) |
| SPI | 4선, 최대 10 Mbps. CPOL/CPHA 는 문서에 없음 (미확인) |
| 페이로드·FIFO | 1~32바이트, TX/RX 각 3단계 FIFO |
| 핀 | CE(RX/TX 활성), IRQ(active-low: TX_DS, RX_DR, MAX_RT) |
| 명령 | R_REGISTER 000A AAAA · W_REGISTER 001A AAAA · R_RX_PAYLOAD 61h · W_TX_PAYLOAD A0h · FLUSH_TX E1h · FLUSH_RX E2h · NOP FFh |
| 규칙 | 새 명령은 CSN 의 High→Low 에서 시작 |

## 74HC595 (직렬→병렬 시프트 레지스터) — D1
| 항목 | 값 |
|---|---|
| 핀 | DS(14) 데이터 · SHCP(11) 시프트 클럭 · STCP(12) 저장 클럭 · OE(13, active-low) · MR(10, active-low) · Q7S(9) 직렬 출력 |
| 전원 | 권장 2.0~6.0 V |
| 최대 클럭 (-40~85 °C 최소) | 2 V 4.8 MHz · 4.5 V 24 MHz · 6 V 28 MHz |
| 출력 전류 | Q7S ±25 mA, Qn ±35 mA (한계값) |

3.3 V 에서의 최대 클럭은 표에 없어 미확인(2 V~4.5 V 사이). SoC: AXI Quad SPI 또는 GPIO 비트뱅잉, STCP 를 CS 처럼 사용(설계안).

## microSD (SPI 모드) — 미확인
이번 작업에서 SD 사양서를 열었으나 SPI 모드 초기화 절차·클럭 한계를 확인한 기록이 남지 않아 값을 적지 않습니다. 필요하면 SD Physical Layer Simplified Specification 의 SPI 모드 절을 다시 열어 확인합니다.
