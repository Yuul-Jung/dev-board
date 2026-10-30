# CAN 소자와 SoC 연결

<!-- nav -->[저장소 홈](../../README.md) › [devices](../README.md)

구분: **확인** = 문서를 열어 확인, **계산**, **설계안**, **미확인**.

## 경로 A: AMD CAN (PG096 v5.1, 2023-05-16) — 확인 (문서 본문 열람)
이전의 "열람한 AMD IP 없음"은 틀렸습니다. 이번에 PG096 본문으로 확인했습니다.

| 항목 | 값 |
|---|---|
| 규격 | CAN 2.0A/2.0B, ISO 11898-1(제품 페이지), 표준 11비트·확장 29비트 |
| 비트율 | 최대 1 Mbps |
| FIFO | 송신 최대 64, 수신 최대 64(설정), 고우선순위 송신 버퍼 1, 자동 재전송 |
| 필터 | 설정 가능한 수의 수락 필터, 최대 4개 |
| 기능 | 슬립(자동 깨움), 루프백, 마스크 가능한 오류·상태 인터럽트, 오류 카운터 |
| 디바이스 | 지원 계열에 7 series 포함 (Artix-7 최대 주파수 표는 확인 안 함) |
| 인터페이스 | AXI4-Lite |
| 포트 | can_clk(입력, 문서 표에 "24 MHz oscillator clock input"), can_phy_tx, can_phy_rx, p2bus_intrevent(레벨 인터럽트) |
| 라이선스 | Core License Agreement. 포함 여부는 Vivado IP 카탈로그의 License 열("Included"/"Purchase")로 확인하라고 안내 → 이 환경에서는 미확인 |

레지스터 (확인): 0x000 SRR · 0x004 MSR · 0x008 BRPR · 0x00C BTR · 0x010 ECR · 0x014 ESR · 0x018 SR · 0x01C ISR · 0x020 IER · 0x024 ICR · 0x030~0x03C TXFIFO(ID, DLC, DW1, DW2) · 0x040~0x04C TXHPB · 0x050~0x05C RXFIFO · 0x060 AFR · 0x064~0x080 AFMR1/AFIR1 ~ AFMR4/AFIR4.
SRR 비트: 0 SRST, 1 CEN. 전원 인가 시 CEN=0, SR의 CONFIG=1.

**초기화 절차 (확인)**: 1) BRPR, BTR 설정(CEN=0일 때만 쓰기 가능) 2) 루프백은 MSR의 LBACK, 슬립은 SLEEP 설정 3) SRR의 CEN=1. 연속 11개 열성(recessive) 비트 후 SR의 CONFIG가 0으로 바뀜.
**비트 타이밍 (확인)**: tq = tosc × (BRP + 1), BRPR[7:0] (실제값 = 쓴 값 + 1). BTR: SJW[8:7], TS2[6:4], TS1[3:0], 각 실제값 = 쓴 값 + 1. 동기 구간은 항상 1 tq. 비트 시간 = 1 + TS1 + TS2 (tq).
**계산 예**: can_clk 24 MHz, 목표 1 Mbps → BRP+1 = 3 (BRPR=2) 이면 tq = 125 ns, 비트당 8 tq → 1 Mbps. TS1 실제 5(필드 4), TS2 실제 2(필드 1), SJW 실제 1(필드 0) → 샘플 지점 6/8 = 75% (샘플 지점은 설계 선택). Basys3 클럭은 100 MHz(RM)이므로 24 MHz 는 MMCM으로 만들어야 함(계산). can_clk 를 24 MHz 외 값으로 써도 되는지는 문서 표 서술만 있어 미확인.

## 경로 B: MCP2515 (SPI) — `../spi/comm-storage-io.md`
컨트롤러가 칩 안에 있어 FPGA는 SPI 만 쓰면 됩니다(AXI Quad SPI). 확인된 사양: CAN 2.0B 1 Mb/s, SPI 10 MHz, 수신 버퍼 2, 송신 버퍼 3. SPI 명령 코드·모드·비트 타이밍·버퍼 배치는 DS20001801K PDF 로 확인했고 드라이버(`../spi/sw/mcp2515.c`)와 시험이 있다(보드 미실행).

| 비교 | 경로 A (AXI CAN) | 경로 B (MCP2515) |
|---|---|---|
| 필요 핀 | TX, RX 2핀 + 트랜시버 | SPI 4~5핀 + INT + 트랜시버 |
| FPGA 자원 | IP 사용 | Quad SPI 재사용 가능 |
| 성능 | 송수신 FIFO 각 64 | 버퍼 수신 2/송신 3 |
| 문서 상태 | 레지스터·초기화 절차 확인 | 명령 코드·레지스터 확인(PDF), 필터·슬립 등 일부 미작성 |
| 라이선스 | Vivado 카탈로그 License 열에서 확인 필요 | 없음 |

## 트랜시버: SN65HVD230 (TI) — D1 일부 (제품 페이지)
| 항목 | 값 |
|---|---|
| 전원 | 3.3 V 단일 (VCC 3~3.6 V) |
| 속도 | 최대 1 Mbps |
| RS 핀(8번) | GND = 고속, 저항으로 슬로프 제어, 저전력 대기(청취 전용, 전형 370 µA) |
| 규격 | ISO 11898-2 호환 |
| 버스 | 버스 고장 전압 -4~16 V, 공통 모드 범위 -2~7 V, 버스 핀 ESD 16 kV HBM 초과, 최대 120 노드 |
| 핀 이름 매핑, 종단저항 값 | 제품 페이지에 없음 — 미확인 |

3.3 V 소자라 FPGA 와 직결 가능(설계안). 종단 저항은 버스 양 끝에 두는 것이 일반적이나 값은 이번에 확인하지 못했습니다.

## 핀 예산 (설계안)
경로 A: TX, RX 2핀. 경로 B: SPI 버스 공유 + CS + INT.

## 드라이버 (devices/can/sw, 호스트 모의 테스트만)
`axi_can.h/.c`: 비트 타이밍 계산, 초기화, TX FIFO/HPB 송신, 수신. PG096 표에서 확인한 항목:
- SRR: bit0 SRST, bit1 CEN. MSR: bit0 SLEEP, bit1 LBACK. BRPR/BTR 은 CEN=0 일 때만 쓸 수 있음. 실제값 = 기록값 + 1.
- BTR: SJW[8:7], TS2[6:4], TS1[3:0]. tq = tosc × (BRP+1).
- 메시지 4워드(ID, DLC, DW1, DW2)를 모두 써야 함. ID: [31:21] ID[28:18], [20] SRR/RTR, [19] IDE, [18:1] 확장 ID 하위 18비트, [0] RTR. DLC 는 [31:28]. DB0 = DW1[31:24].
- SR: bit0 CONFIG, 1 LBACK, 3 NORMAL, 9 TXBFLL, 10 TXFLL. ISR 표: bit7 RXNEMP, bit8 ERROR, bit4 RXOK (본문 목록은 RXNEMP 가 빠져 있어 표 기준).
- 24 MHz, 1 Mbps → 계산상 BRPR=2, 8 tq (PG096 예제와 일치).

미확인/설계안: RXNEMP 가 ICR 로 지운 뒤 FIFO 에 메시지가 남아 있으면 다시 세트되는지(테스트 모의는 그렇다고 가정). 샘플 포인트 87.5% 목표는 PG096 이 아닌 일반 관례. CONFIG 해제 확인 대기 로직은 호출자 몫. 인터럽트, 수락 필터, 오류/버스오프 복구 코드는 미작성. can_clk 24 MHz 는 문서 기재 값이며 Basys3 에서 만드는 방법(MMCM)은 미확인.
테스트: `cd devices && gcc -Wall -include can/tb/mock_can.h -I can/sw can/tb/test_can_mock.c can/sw/axi_can.c -o t && ./t` → ALL PASS.

<!-- nav-foot -->← [i2s](../i2s/README.md) · [devices 목차](../README.md) · [ethernet](../ethernet/README.md) →

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 하위 폴더

| 폴더 | 내용 |
|---|---|
| [sw/](sw/README.md) | 드라이버 C 소스 |
| [tb/](tb/README.md) | 호스트 테스트와 레지스터/장치 모델(mock) |

<!-- index:end -->
