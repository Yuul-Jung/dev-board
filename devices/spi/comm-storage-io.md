# SPI 통신·저장·I/O 확장

<!-- nav -->[저장소 홈](../../README.md) › [devices](../README.md) › [spi](README.md)

## MCP2515 (CAN 컨트롤러) — D1
| 항목 | 값 |
|---|---|
| CAN | V2.0B, 최대 1 Mb/s |
| SPI | 10 MHz, 모드 0,0 / 1,1 |
| 전원 | 2.7~5.5 V |
| 버퍼 | 수신 2개(RXB0, RXB1), 송신 3개(우선순위·중단 기능) |
| 핀 | INT 1개 (PDIP/SOIC 12번) |
| 동작 모드 | 설정(Configuration) 모드에서만 마스크·필터·비트 타이밍 변경, Sleep 대기 전류 1 µA(전형) |
| 근거 | Microchip DS20001801K **PDF 본문을 직접 읽음**(D1). 이전 요약에서 "열어본 발췌에 없음"이던 SPI 옵코드·모드 레지스터 표는 PDF 12장·10장·11장에서 확인 |

**확인된 값 (DS20001801K)**
| 항목 | 값 |
|---|---|
| SPI 명령(Table 12-1) | RESET C0h, READ 03h, READ RX BUFFER 1001 0nm0 (nm 00→RXB0SIDH 61h, 01→RXB0D0 66h, 10→RXB1SIDH 71h, 11→RXB1D0 76h; **CS 가 올라가면 해당 RXnIF 자동 해제**), WRITE 02h, LOAD TX BUFFER 0100 0abc (000→TXB0SIDH 31h, 001→TXB0D0 36h, 010→TXB1SIDH 41h, 011→TXB1D0 46h, 100→TXB2SIDH 51h, 101→TXB2D0 56h), RTS 1000 0nnn (bit0 TXB0, bit1 TXB1, bit2 TXB2), READ STATUS A0h, RX STATUS B0h, BIT MODIFY 05h(+주소·마스크·값; 지원하지 않는 레지스터에서는 마스크가 FFh 로 강제되어 바이트 쓰기가 됨) |
| 명령 규칙 | 모드 0,0 / 1,1. 명령 하나마다 CS 를 내렸다 올려야 함. WRITE/READ 는 CS Low 동안 순차 접근. RESET 은 구성 모드로 만들고 전원 투입 때 보내기를 강하게 권장 |
| 모드(Register 10-1/10-2) | CANCTRL[7:5] REQOP: 000 정상, 001 슬립, 010 루프백, 011 수신 전용, 100 구성(전원 투입 시 100, CANCTRL 리셋값 87h). CANSTAT[7:5] OPMOD 가 현재 모드. CANCTRL 주소는 xFh, CANSTAT 은 xEh (어느 상위 니블이든 동일 레지스터) |
| 인터럽트(Register 7-1/7-2) | CANINTE 2Bh, CANINTF 2Ch: bit7 MERR, 6 WAK, 5 ERR, 4 TX2, 3 TX1, 2 TX0, 1 RX1, 0 RX0. 플래그는 MCU 가 지워야 함(READ RX BUFFER 의 RXnIF 는 예외) |
| READ STATUS(Figure 12-8) | bit0 RX0IF, 1 RX1IF, 2 TXB0 TXREQ, 3 TX0IF, 4 TXB1 TXREQ, 5 TX1IF, 6 TXB2 TXREQ, 7 TX2IF |
| 수신 버퍼(Register 4-4~4-8) | RXBnSIDL: SID[2:0] bit7:5, SRR bit4(표준 원격), IDE bit3, EID[17:16] bit1:0. RXBnDLC: RTR bit6(확장 프레임의 원격), RB1/RB0, DLC bit3:0. 송신 쪽은 TXBnSIDL 의 EXIDE bit3 |
| 비트 타이밍(5장) | CNF1 = SJW[7:6] BRP[5:0], CNF2 = BTLMODE[7] SAM[6] PHSEG1[5:3] PRSEG[2:0], CNF3 = SOF[7] WAKFIL[6] PHSEG2[2:0]. TQ = 2×(BRP+1)/Fosc (Register 5-1; 5.3절 식 5-2 의 "2·BRP·TOSC" 는 표기가 달라 레지스터 설명을 따름). 구간 범위: PropSeg 1~8, PS1 1~8, PS2 2~8(IPT = 2 Tq), SJW 1~4. 규칙(5.5절): PropSeg+PS1 ≥ PS2, PropSeg+PS1 ≥ TDELAY(보통 1~2 Tq), PS2 ≥ SJW. CNF 는 구성 모드에서만 변경 가능 |
| 발진기 시작(8.1절) | OST 가 전원 투입·SPI RESET·RESET 핀·슬립 복귀 후 **128 OSC1 클럭** 동안 리셋 유지. 그동안 SPI 접근 금지 |

**여전히 미확인**: 필터·마스크 설정 절차의 예제, 슬립/웨이크업 절차, 오류 카운터 처리, BFPCTRL/TXRTSCTRL 핀 기능 사용, 한 번 송신 모드(OSM), 클럭 출력 설정(드라이버는 건드리지 않음). 드라이버는 필터·마스크를 쓰지 않고 `RXM=11`(모두 수신)로 둠.

**드라이버**: [`sw/mcp2515.h`](sw/mcp2515.h), [`sw/mcp2515.c`](sw/mcp2515.c) — `calc_timing`, `init`(구성 모드 확인 → CNF 쓰기 → 필터 끔 → 인터럽트 끔 → 요청 모드), `set_mode`(BIT MODIFY 후 CANSTAT 확인), `send`(TXREQ 0 인 TXB 를 찾아 LOAD TX → RTS), `recv`(READ STATUS → READ RX BUFFER 13바이트). 시험: [`tb/test_mcp2515.c`](tb/test_mcp2515.c)(옵코드·타이밍·버퍼 배치를 표의 값과 직접 비교, 비트 타이밍은 독립 전수 탐색과 해 존재 여부 일치), [`tb/test_mcp2515_model.c`](tb/test_mcp2515_model.c)(위 규칙을 구현한 SPI 레벨 모델 위에서 루프백 왕복 20,000 프레임, 구성 모드 전용 CNF 쓰기, RXnIF 자동 해제, RXB0 우선, TXB 가득 참, 모드 전환 실패 감지, 모드 중 재초기화). 모델은 내가 읽은 규칙의 해석이므로 해석이 틀리면 같이 틀린다. 보드 미실행. 레지스터·비트·옵코드 33개는 [`tools/crosscheck_pdf.py`](../tools/crosscheck_pdf.py) 로 PDF 텍스트와 자동 대조(전부 일치, 변조 사본은 실패로 잡힘).

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
