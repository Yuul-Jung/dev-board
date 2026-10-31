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

**보안 한계(설계안, 프로젝트 문서 §6-11)**: 태그 UID 만으로 출입 권한을 판단하는 것은 강한 인증이 아니며(복제·재생 가능) 학습용 예제로만 씁니다. 인증·복제 방지·로그 무결성은 별도 과제입니다. 이 저장소에는 RC522 전용 드라이버가 없습니다(문서만).

## W5500 (유선 이더넷, TCP/IP 하드와이어) — D1(요약 경유) + D3
근거: WIZnet W5500 Datasheet v1.0.5 (NOV 2014). **이 환경에서는 PDF 를 직접 내려받지 못했고**, 웹 가져오기(WebFetch)가 돌려준 본문 요약(사본 2개: docs.wiznet.io 한글판, pdf.direnc.net 영문판)에서 값을 옮겼습니다. 두 사본이 겹치는 항목은 같았고, 독립 소스로 Apache NuttX `drivers/net/w5500.c`(D3)와 대조했습니다. 요약은 모델이 가공한 텍스트이므로 원문 PDF 와 한 글자씩 같다는 보장은 없습니다.

| 항목 | 값 |
|---|---|
| SPI | 모드 0, 3 · 설계 속도 80 MHz, 보장 최소 33.3 MHz |
| 프레임 | 주소 16비트(상위, 하위) + 제어 8비트 + 데이터. 제어 = BSB[7:3] \| RWB[2] \| OM[1:0], **RWB: 0 읽기, 1 쓰기**(p.16) |
| OM | 00 가변 길이(SCSn 이 길이 결정), 01/10/11 고정 1/2/4바이트. 드라이버는 가변 길이만 씀 |
| BSB | 0 = 공통 레지스터, 소켓 n: 4n+1 레지스터, 4n+2 TX 버퍼, 4n+3 RX 버퍼 (제어 바이트는 소켓 0 TX 쓰기 0x14, RX 읽기 0x18 처럼 계산, 시험이 DS·NuttX 두 식으로 대조) |
| 소켓 | 8개 · 버퍼 32 KB(TX 16 KB, RX 16 KB), 소켓별 크기 0/1/2/4/8/16 KB 합계 ≤ 16 KB, 초기 소켓당 2 KB |
| 프로토콜 | TCP, UDP, ICMP, IPv4, ARP, IGMP, PPPoE (IP 단편화 미지원) |
| PHY | 10BaseT/100BaseTX 내장, 자동 협상 |
| 전원 | 3.3 V (2.97~3.63 V), 5 V 입력 허용 |
| 레지스터 | MR 0x0000 (리셋 0x00), VERSIONR 0x0039 = 0x04, 나머지는 `ref/w5500_facts.txt` |

연결 테스트: VERSIONR(0x0039)을 읽어 0x04 확인. SoC: 빠른 버스(SPI1) + CS + INT/RST GPIO. 이더넷 경로로는 AXI Ethernet Lite 와 대안 관계 (IP 문서는 [`../ethernet/`](../ethernet/README.md)).

### 드라이버 `sw/w5500.h/.c` (axi_qspi 위, 호스트 모델 시험만)
- 사용 범위: 초기화(RSTn 핀 콜백이 있으면 핀 리셋, 없으면 MR.RST 소프트 리셋 후 자동 해제를 폴링 → VERSIONR 확인 → MAC/IP/마스크/게이트웨이·버퍼 크기를 쓰고 읽어 되돌려 확인), TCP 클라이언트·서버(`w5500_tcp_connect`, `w5500_tcp_listen`), 송신(`w5500_send`, `w5500_udp_sendto`), 수신(`w5500_rx_avail`, `w5500_recv`(TCP 원시 바이트), `w5500_udp_recvfrom`(UDP 8바이트 헤더 해석, 형식과 근거는 [소켓 위 응용](../ethernet/socket-apps.md)의 1절)), 닫기·연결 끊기.
- 설계 결정(확인된 규칙에서 도출): 소켓 명령은 `Sn_CR` 이 0 으로 돌아온 것을 확인하고서야 다음 동작으로 간다. `TX_FSR`/`RX_RSR` 은 값이 같을 때까지 반복해서 읽는다. 버퍼 접근은 블록 경계에서 나눠 쓰고 포인터는 16비트 원값으로 기록한다(올림 버림). 버퍼 크기는 줄이는 쪽을 먼저 써서 합계가 항상 16 KB 이내가 되게 한다. `w5500_recv` 는 `Sn_IR.RECV` 를 건드리지 않는다(호출자가 `w5500_clear_ir`).
- 오류 코드: VERSION -60, VERIFY -61, TIMEOUT -62, STATE -63, NOSPACE -64, PEER -65, NOPKT -66(UDP 완전한 패킷 없음), FRAME -67(UDP 헤더 길이가 버퍼보다 큼). SPI 계층 오류는 `axi_qspi` 값 그대로 전달.
- 시험 `tb/test_w5500_model.c`(장치 모델은 `tb/w5500_model.h` 로 분리해 NTP·HTTP 시험과 공유): 데이터시트 규칙을 구현한 W5500 모델(SPI 프레임 규칙, 읽기 전용 레지스터 쓰기 금지, 버퍼 합계, 리셋 타이밍, 소켓 명령 전제, 블록 경계 위반 등을 어기면 기록)에 대해 초기화·TCP·UDP·전체 버퍼·타임아웃·쓰기 누락 주입을 실행. ASan/UBSan 통과, 의도적으로 틀리게 만든 사본 86개 + UDP 수신 23개가 모두 실패로 잡힘. 모델은 내가 읽은 요약대로 만든 것이라 요약이 틀렸으면 같이 틀립니다.
- 교차검증: `tools/crosscheck_pdf.py` 가 헤더 상수를 `ref/w5500_facts.txt` 와 대조(NuttX 값이 있는 항목은 DS·NuttX 일치도 확인). 알려진 불일치 두 가지는 실패로 세지 않고 기록: ① MACRAW 의 `Sn_SR` 값(영문 사본 p.47 은 0x02, p.51 은 0x42) → 드라이버가 MACRAW 를 쓰지 않음, ② `Sn_RXBUF_SIZE` 8 KB 의 값(DS 는 8, NuttX 는 5) → NuttX 쪽 오기로 보고 DS 를 따름.
- **미확인(TODO)**: UDP 수신 헤더의 **W5500 에서의 바이트 순서**(형식 8바이트 = IP 4 + 포트 2 + 길이 2 는 WIZnet 문서 D1(요약 경유) + W5100 라이브러리 D3 로 확인, 순서는 W5100 코드 근거만)와 `Sn_RX_RD` 전진량(문서 본문은 길이 + 8, 같은 페이지의 코드는 길이만 — 본문을 따름), 소프트 리셋 후 대기 시간, `w5500_send` 가 TIMEOUT 으로 끝났을 때 TX 포인터 상태, `Sn_TX_RD`/`Sn_RX_WR` 가 읽기 전용인지(요약에서 확인 못 하고 모델에서 읽기 전용으로 둠), MACRAW, 실제 모듈의 PHY/자석/RJ45 구성, 보드에서의 실행.

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

## nRF24L01+ (2.4 GHz 무선) — D1
출처: Nordic nRF24L01+ Product Specification v1.0 (78쪽 PDF 를 직접 읽음). 6.1절 모드·타이밍, 8.3절 SPI, 9.1절 레지스터 맵(Table 28), 부록 A 송수신 절차.

| 항목 | 값 |
|---|---|
| 전송 속도 | 250 kbps / 1 Mbps / 2 Mbps (RF_DR_LOW, RF_DR_HIGH) |
| 전원 | 1.9~3.6 V, 입력 5 V 허용 (3.6 V 초과 입력이면 VDD 2.7~3.3 V) |
| SPI | 4선, SCK ≤ 10 MHz (Cload 5 pF) / 8 MHz (10 pF). CSN 비활성 ≥ 50 ns, CSN→SCK·SCK→CSN 2 ns. **모드 0,0** — 본문에 모드 이름은 없고 그림 26~28(SCK 가 Low 로 대기, 첫 상승 에지에서 데이터 표본)에서 읽은 값 |
| 바이트 순서 | 명령은 MSBit 먼저. 다중 바이트 레지스터·페이로드는 **LSByte 먼저**(각 바이트는 MSBit 먼저). 주소 쓰기는 중간에 끊어도 됨(나머지 바이트 유지) |
| STATUS | 모든 명령에서 첫 바이트가 MISO 로 나온다. NOP 로 읽을 수 있다. 새 명령은 CSN High→Low 에서 시작 |
| 페이로드·FIFO | 1~32바이트, TX/RX 각 3단계 FIFO(RX 는 전 파이프 공유) |
| 핀 | CE(RX/TX 활성), IRQ(active-low: RX_DR, TX_DS, MAX_RT) |

**명령 (Table 20)**: R_REGISTER 000A AAAA · W_REGISTER 001A AAAA(대기/전원차단에서만) · R_RX_PAYLOAD 61h · W_TX_PAYLOAD A0h · FLUSH_TX E1h · FLUSH_RX E2h · REUSE_TX_PL E3h · R_RX_PL_WID 60h · W_ACK_PAYLOAD 1010 1PPP · W_TX_PAYLOAD_NOACK B0h · NOP FFh. 뒤 세 개(R_RX_PL_WID, W_ACK_PAYLOAD, W_TX_PAYLOAD_NOACK)는 FEATURE 비트가 켜져 있어야 한다.

**레지스터 (Table 28)**

| 주소 | 이름 | 리셋 | 내용 |
|---|---|---|---|
| 00 | CONFIG | 08h | 6 MASK_RX_DR · 5 MASK_TX_DS · 4 MASK_MAX_RT · 3 EN_CRC(EN_AA 가 하나라도 켜지면 1 강제) · 2 CRCO(0 = 1바이트, 1 = 2바이트) · 1 PWR_UP · 0 PRIM_RX(1 = PRX). 7 은 0 만 |
| 01 | EN_AA | 3Fh | 파이프별 자동 ACK |
| 02 | EN_RXADDR | 03h | 파이프 0·1 만 기본 켜짐 |
| 03 | SETUP_AW | 03h | 1 = 3바이트, 2 = 4바이트, 3 = 5바이트, 0 불가. 폭보다 짧은 주소는 LSByte 사용 |
| 04 | SETUP_RETR | 03h | ARD[7:4]: (값+1)×250 µs, ARC[3:0]: 재전송 횟수(0 = 끔) |
| 05 | RF_CH | 02h | f = 2400 + RF_CH MHz. 동작 범위 2400~2525 MHz(RF_CH 0~125). 쓰면 PLOS_CNT 초기화 |
| 06 | RF_SETUP | 0Eh | 5 RF_DR_LOW · 3 RF_DR_HIGH · 2:1 RF_PWR(00 −18, 01 −12, 10 −6, 11 0 dBm) · 7 CONT_WAVE. [LOW,HIGH] = 00 1 Mbps · 01 2 Mbps · 10 250 kbps |
| 07 | STATUS | 0Eh | 6 RX_DR · 5 TX_DS · 4 MAX_RT (1 을 쓰면 해제) · 3:1 RX_P_NO(0~5, 7 = 비어 있음) · 0 TX_FULL |
| 08 | OBSERVE_TX | 00h | PLOS_CNT[7:4](15 에서 멈춤), ARC_CNT[3:0] |
| 09 | RPD | 00h | bit0, −64 dBm 초과 신호. 구형 nRF24L01 의 CD 와 이름·문턱이 다름 |
| 0A~0F | RX_ADDR_P0~P5 | E7E7E7E7E7 · C2C2C2C2C2 · C3 · C4 · C5 · C6 | P0·P1 은 5바이트, P2~P5 는 LSByte 만(상위는 P1 과 같음) |
| 10 | TX_ADDR | E7E7E7E7E7 | 자동 ACK 를 받으려면 RX_ADDR_P0 를 같은 값으로 |
| 11~16 | RX_PW_P0~P5 | 0 | 1~32, 0 = 파이프 미사용 |
| 17 | FIFO_STATUS | 11h | 6 TX_REUSE · 5 TX_FULL · 4 TX_EMPTY · 1 RX_FULL · 0 RX_EMPTY |
| 1C | DYNPD | 00h | DPL_P0~P5. EN_DPL 과 해당 ENAA 필요 |
| 1D | FEATURE | 00h | 2 EN_DPL · 1 EN_ACK_PAY · 0 EN_DYN_ACK |

주소 18~1B 는 시험용으로 예약, 건드리면 오동작.

**타이밍 (Table 16)**: 전원차단→대기 Tpd2stby = 1.5 ms(외부 수정, Ls<30 mH) / 3 ms(60 mH) / 4.5 ms(90 mH), 이 시간이 지나기 전에 CE 를 올리면 안 된다. 대기→TX/RX Tstby2a = 130 µs. CE High 최소 10 µs. CE 상승 후 CSN 하강까지 ≥ 4 µs(Tpece2csn). 한 번의 CE 펄스는 한 패킷(TX FIFO 한 단)을 보낸다. TX 모드를 4 ms 넘게 유지하지 말 것(PLL 개루프).

**송신 절차(부록 A)**: PRIM_RX = 0 → W_TX_PAYLOAD → CE 10 µs 이상 → TX_DS(ACK 수신 시) 또는 MAX_RT 대기. 자동 ACK 를 쓰면 RX_ADDR_P0 = TX_ADDR. MAX_RT 가 서면 해제해야 다음 송신이 가능하고 페이로드는 FIFO 에 남는다(FLUSH_TX 필요). **수신 절차**: PRIM_RX = 1, 파이프 활성·폭 설정, CE = 1, 130 µs 뒤 수신. 인터럽트 처리 순서(각주 c): 페이로드 읽기 → RX_DR 해제 → FIFO_STATUS 로 더 있는지 확인 → 있으면 반복. DPL 이면 R_RX_PL_WID 로 폭을 읽고 32 초과면 오류 패킷이므로 FLUSH_RX.

**드라이버**: [`sw/nrf24l01.h`](sw/nrf24l01.h), [`sw/nrf24l01.c`](sw/nrf24l01.c) — `init`(옵션 조합 검사 → 레지스터를 쓰고 읽어 검증 → CONFIG 마지막 → Tpd2stby 대기 → FIFO 비움·IRQ 해제), `set_tx_addr`(TX_ADDR 와 RX_ADDR_P0 동시), `set_rx_pipe`, `rx_start/rx_stop`, `send`(PTX 전환 → TX_FULL 확인 → 적재 → CE 15 µs → TX_DS/MAX_RT 폴링, MAX_RT 이면 해제·FLUSH_TX), `recv`(FIFO_STATUS → 폭 → 읽기 → RX_DR 해제), `write_ack_payload`, `read_rpd`. CE 와 마이크로초 지연은 콜백으로 받는다(GPIO·타이머는 이 파일 밖).
옵션 검사는 문서 규칙에서 나온다: 자동 ACK 는 CRC 필요, DPL 은 자동 ACK 필요, ACK 페이로드는 DPL 필요, 250 kbps 는 ARD ≥ 500 µs. "ACK 페이로드를 쓰면 ARD ≥ 500 µs"는 문서의 조건부 규칙(2 Mbps 15바이트 초과·1 Mbps 5바이트 초과)을 길이를 모르는 상태에서 보수적으로 적용한 **설계 선택**이다.
시험: [`tb/test_nrf24_model.c`](tb/test_nrf24_model.c) — 위 규칙을 구현한 모델(예약 비트 쓰기, 읽기 전용, FEATURE 없이 쓴 명령, DPL 선행 조건, 수신 모드 중 STATUS 외 쓰기, Tpd2stby·10 µs·4 µs·130 µs 위반, FIFO 3단, MAX_RT 중 송신 금지)을 상대로 초기화 바이트열을 손으로 계산한 값과 한 바이트씩 비교, RF_SETUP 조합 표, 잘못된 옵션 13종(SPI 트래픽 0), 송수신·MAX_RT·TX_FULL·타임아웃, DPL 1~32바이트, 비정상 폭(0, 33), ACK 페이로드, 칩이 없을 때(읽기 0xFF) 검증 실패. 변이 30종 모두 검출. 레지스터·비트·명령·타이밍 89항목은 `tools/crosscheck_pdf.py` 로 PDF 텍스트와 자동 대조.

**미확인·한계**
- SPI 모드 0,0 은 그림에서 읽은 것이다(글로 된 문장 없음).
- `W_TX_PAYLOAD_NOACK` 패킷에서 TX_DS 가 언제 서는지는 문서에 없다. 모델은 "전송 직후"로 가정했고 실제 칩은 확인하지 못했다. 그래서 `send(noack=1)` 는 실제 칩에서 타임아웃을 낼 수 있다.
- 수신 모드(CE=1)에서 RX_DR 을 해제하려고 STATUS 를 쓴다. 문서는 W_REGISTER 를 "대기·전원차단에서만"이라 쓰면서 각주 c 에서는 이 해제를 안내한다. 모델은 STATUS 쓰기만 허용했다 — 이것이 문서 의도라는 보장은 없다.
- 구형 칩·복제품에 필요한 ACTIVATE(73h) 명령은 이 문서에 없다(자동 대조로 부재 확인). 복제품이면 FEATURE 쓰기가 무시될 수 있다 — 쓰고 읽어 비교하는 init 검증이 이를 잡는다.
- 공중(RF) 구간·주소 매칭·실제 재전송 시간은 모델링하지 않았다. 보드·무선 실행 없음. 모듈 전원은 3.3 V 로 안정적이어야 한다(PA/LNA 모듈은 전류가 커 디커플링 필요 — 이 문서에서 확인한 값은 아님).

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
