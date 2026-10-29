# SPI 통신·저장·I/O 확장

<!-- nav -->[저장소 홈](../../README.md) › [devices](../README.md) › [spi](README.md)

## MCP2515 (CAN 컨트롤러) — D1 일부
| 항목 | 값 |
|---|---|
| CAN | V2.0B, 최대 1 Mb/s |
| SPI | 10 MHz, 모드 0,0 / 1,1 |
| 전원 | 2.7~5.5 V |
| 버퍼 | 수신 2개(RXB0, RXB1), 송신 3개(우선순위·중단 기능) |
| 핀 | INT 1개 (PDIP/SOIC 12번) |
| 동작 모드 | 설정(Configuration) 모드에서만 마스크·필터·비트 타이밍 변경, Sleep 대기 전류 1 µA(전형) |
| SPI 명령 옵코드, CANCTRL 주소 | 데이터시트 PDF 텍스트가 87,709자에서 끝나 12장(SPI 명령 표)과 제어 레지스터 표가 통째로 빠져 있음. 일부는 2차 자료로 확인(아래) |

**근거를 세 구역으로 나눈 상세** (드라이버 [`sw/mcp2515.h`](sw/mcp2515.h) 머리말과 동일)
| 구역 | 내용 | 근거 |
|---|---|---|
| A 확인 | 비트 타이밍: CNF1 = SJW[7:6] BRP[5:0], CNF2 = BTLMODE[7] SAM[6] PHSEG1[5:3] PRSEG[2:0], CNF3 = SOF[7] WAKFIL[6] PHSEG2[2:0]; Tq = 2×(BRP+1)/Fosc, 구간 = (필드+1)×Tq, PHSEG2 최소 2 Tq (5.3절의 "2×BRP×TOSC" 와 불일치 — 레지스터 설명을 따름) | D1 |
| A 확인 | 주소: CNF3 28h, CNF2 29h, CNF1 2Ah, TEC 1Ch, REC 1Dh, EFLG 2Dh, TXBnCTRL 30/40/50h, TXBnSIDH 31/41/51h … DLC 35/45/55h, 데이터 36~3Dh …, RXB0CTRL 60h, RXB1CTRL 70h, RXBnSIDH 61/71h, RXBnDLC 65/75h, 데이터 66~6Dh / 76~7Dh, BFPCTRL 0Ch, TXRTSCTRL 0Dh | D1 |
| A 확인 | 필드: TXREQ = TXBnCTRL bit3, TXP[1:0] bit1:0; TXBnSIDL = SID[2:0] bit7:5, EXIDE bit3, EID[17:16] bit1:0; TXBnDLC = RTR bit6, DLC bit3:0 (8 초과도 8바이트만 전송); RXB0CTRL RXM[1:0] = bit6:5, RXM=11 → 필터 끔; CANINTF RX0IF bit0, RX1IF bit1; 수신 플래그는 MCU 가 지워야 다음 메시지가 들어옴 | D1 |
| B 2차 자료 | SPI 옵코드 READ 03h, BIT MODIFY 05h, LOAD TX BUFFER 40h, RTS 80h(\|nnn: TXB0/1/2), READ STATUS A0h, RESET C0h | D3 (Zephyr mcp2515 드라이버 커밋) |
| C 미확인 | WRITE, READ RX BUFFER 옵코드; CANCTRL/CANSTAT/CANINTE/CANINTF 주소(CANINTE/F 는 CNF1 2Ah 와 EFLG 2Dh 사이 두 칸이라는 계산만 가능); REQOP/OPMOD 모드 코드; TX0IF 비트; RXBnSIDL 의 IDE/SRR 비트; LOAD TX 의 abc 해석; 리셋 후 발진기 안정 시간 | 없음 — **DS21801 PDF 필요** |

드라이버는 A·B 구역만 시험하고 C 구역은 `*_UNVERIFIED` 임시값으로 격리해 두었습니다(모드 전환·메시지 수신은 C 구역이 필요하므로 PDF 대조 전에는 완성되지 않음). 비트 타이밍 계산기는 8/16/20 MHz × 10개 속도에서 독립 전수 탐색과 해 존재 여부가 일치하고 비트레이트가 정확합니다([`tb/test_mcp2515.c`](tb/test_mcp2515.c)).

**기존 CAN 과의 차이**: 이 저장소의 [AXI CAN](../can/README.md) 은 FPGA 안의 IP(PG096, 24 MHz 기준 BRPR 방식)이고, MCP2515 는 SPI 로 붙는 외부 컨트롤러(예: 8/16 MHz 수정 발진기, BRP 방식)라 타이밍 식이 다르다. 두 경우 모두 CAN 트랜시버(예: 외부 칩)가 따로 필요하다.

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

연결 테스트: VERSIONR(0x0039)을 읽어 0x04 확인. SoC: 빠른 버스(SPI1) + CS + INT/RST GPIO. 이더넷 경로로는 AXI Ethernet Lite 와 대안 관계 (IP 문서는 [`../ethernet/`](../ethernet/README.md)).

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

3.3 V 에서의 최대 클럭은 표에 없어 미확인(2 V~4.5 V 사이). 2.7~3.6 V 동작은 특징 목록에서 JESD8C 를 인용할 뿐 3.3 V 특성표는 없음(Nexperia Rev.12).

**동작 규칙 (확인, Nexperia 74HC/HCT595 Rev.12)**
- SHCP 상승 에지에서 DS 를 시프트, STCP 상승 에지에서 시프트 레지스터 → 저장 레지스터. 두 클럭은 별개이고, 묶으면 시프트 레지스터가 저장 레지스터보다 한 클럭 앞선다.
- MR(Low)은 시프트 레지스터만 지움(저장 레지스터는 그대로). OE(High)는 Qn 을 고임피던스로 만들 뿐 레지스터 내용은 안 바꿈. Q7S 는 3상태가 아니며 칩을 직렬 연결할 때 다음 DS 로 연결.
- 타이밍 최소(25 °C, 2.0/4.5/6.0 V): DS→SHCP 준비 50/10/9 ns, SHCP→STCP 준비 75/15/13 ns, 유지 3 ns, 펄스 폭 75/15/13 ns. 추출된 표의 열이 섞여 있어 원문 PDF 로 재확인 필요. 위의 fmax 표와 25 °C 최소값(9/30/35 MHz)이 서로 다르므로 어느 온도 조건의 값인지 원문에서 확인할 것.
- 첫 비트가 8클럭 뒤 Q7S 에 이른다는 것은 구조에서 도출한 계산(문서에 직접 서술 없음).

**SoC 연결 (설계안)**: AXI Quad SPI 모드 0, MSB 먼저, STCP 를 SPI CS 에 연결(전송 중 Low → 끝날 때 상승 = 래치). 바이트의 bit7 → Q7 … bit0 → Q0, 여러 개를 직렬 연결하면 먼저 보낸 바이트가 가장 먼 칩에 도착. 드라이버 [`sw/spi_devices.c`](sw/spi_devices.c) `hc595_write`, 시험 [`tb/test_hc595.c`](tb/test_hc595.c). FPGA 3.3 V 출력으로 구동하려면 595 의 VCC 도 3.3 V 근처여야 입력 High 가 인식된다(VCC 5 V 의 74HC 입력 High 최소는 일반적으로 3.5 V 로 알려져 있으나 이 저장소에서는 확인하지 않음).

## microSD (SPI 모드) — 미확인
이번 작업에서 SD 사양서를 열었으나 SPI 모드 초기화 절차·클럭 한계를 확인한 기록이 남지 않아 값을 적지 않습니다. 필요하면 SD Physical Layer Simplified Specification 의 SPI 모드 절을 다시 열어 확인합니다.
