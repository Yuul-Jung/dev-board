# HC-SR04 거리계 (AXI4-Lite 래퍼)

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [gpio](../README.md)

거리 측정 로직은 사용자 노션의 `ultrasonic_distance_meter`를 **수정 없이** 가져왔고, 그 둘레에 AXI4-Lite 레지스터를 새로 붙였습니다.

| 파일 | 역할 |
|---|---|
| `ultrasonic_distance_meter.v` | 노션 원본. 트리거 10 µs, 측정 간격 60 ms, 에코 타임아웃 30 ms, 거리 = µs / 58 (최대 999 cm) |
| `us_axil_regs.v` | AXI4-Lite 레지스터와 인터럽트 (새로 작성) |
| `us_axil_top.v` | 배선만 하는 상위 모듈 (새로 작성) |
| `../tb/tb_ultrasonic.v` | 코어 테스트벤치 (10 cm, 100 cm, 타임아웃) |
| `../sw/ultrasonic.c` | MicroBlaze V 드라이버 (호스트 모의 레지스터 시험 있음, `../tb/test_ultrasonic.c`) |

## 레지스터
| 오프셋 | 이름 | 비트 |
|---|---|---|
| 0x00 | CTRL (RW) | [0] enable |
| 0x04 | STATUS | [0] busy, [1] valid, [2] timeout(1을 쓰면 지움), [3] done(1을 쓰면 지움) |
| 0x08 | DIST (R) | [9:0] 거리 cm |
| 0x0C | BCD (R) | [11:0] BCD 3자리 |
| 0x10 | IRQ_EN (RW) | [0] 측정 완료, [1] 타임아웃 |

## AXI4-Lite 버스 정정 (2026-11-03)

- **P1 수정:** `us_axil_top.v`에 `s_axi_wstrb[3:0]`을 추가하고, `us_axil_regs.v`는 **AW와 W를 별도의 1단 버퍼로 수락**한 뒤 양쪽 수락이 완료되면 한 번만 레지스터를 갱신하고 B 응답을 생성하도록 변경했습니다.
- `WSTRB[0]`이 1일 때만 하위 바이트의 `CTRL[0]`, `IRQ_EN[1:0]`, `STATUS[3:2]` W1C를 갱신합니다. `WSTRB=0` 또는 상위 바이트만 선택한 쓰기는 레지스터 상태를 변경하지 않지만 정상 B 응답은 발생합니다.
- `BVALID` 동안에는 새 AW/W를 받지 않으며, `BREADY`가 늦어져도 응답을 유지합니다. AR/R 채널은 독립 동작하며 `RREADY`가 늦어져도 `RDATA`와 `RVALID`를 유지합니다.
- **회귀시험:** `../tb/tb_us_axil_regs.v`에서 AW 먼저, W 먼저, 동시 수락, WSTRB 마스킹, W1C, IRQ, B/R 백프레셔, 읽기/쓰기 병행을 검사합니다. `.github/workflows/devices-check.yml`에서 Icarus Verilog로 `us_axil_top` 합성 전 **컴파일**과 레지스터 **RTL 시뮬레이션**을 수행합니다.
- **검증 단계:** 테스트벤치/CI의 실제 결과 확인 전에는 통과로 기록하지 않습니다. 본 회차는 AXI VIP·Vivado IP Integrator·합성·Timing·실물 Basys3 시험을 수행하지 않았습니다.
- 공식 규격 대조: [Arm AXI4 소개](https://developer.arm.com/-/media/Arm%20Developer%20Community/PDF/Learn%20the%20Architecture/102202_0100_01_Introduction_to_AMBA_AXI.pdf), [AMD PG138 AXI4-Lite 신호](https://docs.amd.com/r/en-US/pg138-axi-ethernet/AXI4-Lite-Interface), [PG267 프로토콜 검사](https://docs.amd.com/r/en-US/pg267-axi-vip/AXI-Protocol-Checks-and-Descriptions). `../sw/ultrasonic.[ch]` 레지스터 오프셋은 변경하지 않았습니다.

## 확인 필요
- **합성·타이밍·실물 검증은 하지 않았습니다.** 새 AXI 테스트벤치의 시뮬레이션 결과는 GitHub Actions 실행 기록을 별도로 확인합니다.
- 에코 입력은 5 V일 수 있으므로 분압 또는 레벨 시프터 뒤에서 연결해야 합니다.
- 코어는 active-high 리셋이라 래퍼에서 `rstn`을 반전해 넘깁니다.
- 코어의 `distance_valid`는 다음 측정 전까지 1로 유지됩니다. 새 값 알림은 `done` 플래그를 쓰세요.
- 에코 타임아웃 30 ms는 4 m 최대 에코 약 23.2 ms보다 깁니다(계산).

---

# DHT22 읽기 (전용 RTL)

AMD IP로는 맞는 것이 없어 직접 설계했습니다. 구조는 1-Wire RTL과 같습니다(상위는 배선만, 레지스터·엔진·오픈 드레인 핀 분리).

| 파일 | 역할 |
|---|---|
| `dht22_axil_top.v` | 배선만 하는 상위 모듈 |
| `dht22_axil_regs.v` | AXI4-Lite 레지스터, 인터럽트 |
| `dht22_engine.v` | 시작 신호, 응답 확인, 40비트 수신, 체크섬 |
| `dht22_iobuf.v` | 오픈 드레인 핀 (`SIMULATION` 정의 시 동작 모델) |
| `../tb/tb_dht22_engine.v` | 엔진 테스트벤치: 정상, 체크섬 오류, 소자 없음 |
| `../sw/dht22.c` | MicroBlaze V 드라이버 (호스트 모의 레지스터 시험 있음, `../tb/test_dht22.c`) |

## 레지스터
| 오프셋 | 이름 | 내용 |
|---|---|---|
| 0x00 | CMD (W) | [0]=1이면 읽기 시작 |
| 0x04 | STATUS (R) | [0] busy, [1] done, [2] err, [3] csum_ok |
| 0x08 | RH (R) | 습도 원시값 16비트 (0.1 %RH 단위) |
| 0x0C | TEMP (R) | 온도 원시값 16비트 (0.1 °C 단위) |
| 0x10 | CHK (R) | 체크섬 바이트 |
| 0x14 | IRQ_EN (RW) | [0] 종료 인터럽트 |

## 파형 (DHT22 데이터시트 서술, 허용 오차 표는 없음)
| 구간 | 데이터시트 | 사용한 값 |
|---|---|---|
| 호스트 시작 Low | 1 ms 이상 | 1.1 ms |
| 해제 후 응답까지 | 20–40 µs | 최대 100 µs까지 대기 |
| 응답 Low / High | 각 80 µs | 각 최대 150 µs까지 대기 |
| 비트 Low | 50 µs | 최대 120 µs까지 대기 |
| 비트 High | 0은 26–28 µs, 1은 70 µs | High 시작 후 40 µs 시점의 선 레벨로 판정 |
| 읽기 간격 | 2 s 초과 | 소프트웨어 책임 |
| 전원 투입 후 | 1 s 동안 명령 금지 | 소프트웨어 책임 |

판정 시점 40 µs는 28 µs와 70 µs 사이에서 고른 값입니다(양쪽 여유 12 µs, 30 µs: 계산). 마지막 비트도 같은 방식이라 데이터시트에 서술이 없는 종료 신호에 의존하지 않습니다.

## AXI4-Lite 레지스터 정정 (2026-11-03)

- **P1 수정:** `dht22_axil_regs.v`의 AW/W 동시 도착 요구를 제거하고, 각 채널의 주소·데이터를 독립 수락한 뒤 1회만 커밋·B 응답하도록 변경했습니다.
- **P1 수정:** `dht22_axil_top.v`의 `s_axi_wstrb[3:0]`를 추가하고 `WSTRB[0]`이 설정된 쓰기에서만 CMD/IRQ_EN의 하위 비트를 갱신합니다. `WSTRB=0` 쓰기는 부작용 없이 B 응답을 보냅니다.
- **P1 검증 대상:** `../tb/tb_dht22_axil_regs.v`는 AW 먼저·W 먼저·동시 수락, CMD 1회 펄스, busy 중 명령 무시, WSTRB, done/IRQ, R/B 백프레셔와 읽기·쓰기 병행을 확인합니다.
- `dht22_engine.v`와 `../sw/dht22.[ch]`의 기능 코드는 그대로 유지했습니다. RTL 검증 통과 여부는 실제 CI 결과를 확인해 기록합니다.
- **실물 전기 조건:** 단품·Keyestudio KS0430/KS0431·Seeed Grove는 3핀/4핀/RJ11/Grove 및 DATA 풀업이 다를 수 있습니다. 5 V DATA 직접 연결 금지. Arduino Modulino Thermo는 I²C HS3003으로 본 RTL과 직접 호환되지 않습니다.

## 확인 필요
- **DHT22 엔진·AXI 통합의 Vivado 합성·타이밍·실물 검증은 하지 않았습니다.** AXI 레지스터 지정 시나리오 시험은 CI 기록으로 별도 확인합니다.
- **체크섬 규칙**(앞 4바이트 합의 하위 8비트)은 이번에 열람한 데이터시트 서술에 없습니다. 일반적으로 알려진 방식을 썼으므로 실제 센서로 `csum_ok`가 1이 되는지 확인해야 합니다.
- **온도 음수 표현**과 습도·온도 바이트 배치(상위 바이트 먼저)도 일반 지식입니다. 원시값을 그대로 노출하고 변환은 소프트웨어에 맡겼습니다.
- 풀업 저항 값은 데이터시트에 없습니다. 센서 모듈에 풀업이 있는지 확인하세요.
- 핀 제약(XDC)은 연결할 Pmod 핀을 정한 뒤 작성합니다. 센서를 3.3 V로 구동하면 FPGA 입출력과 레벨이 맞습니다.

## 로터리 엔코더 AXI4-Lite 레지스터 정정 (2026-11-03)

- **P1 수정:** `quad_axil_regs.v` AW/W 독립 수락, `quad_axil_top.v` WSTRB 전달, `WSTRB[0]`일 때만 COUNT clear·STATUS W1C·CTRL 갱신.
- **기존 정책 유지:** STATUS의 같은 클록 W1C와 새 press/step/err 이벤트가 충돌하면 새 이벤트가 우선합니다. IRQ는 press 또는 step 이벤트에만 반응합니다.
- **회귀시험 추가:** `../tb/tb_quad_axil_regs.v`에서 독립 AW/W, 32비트 부호 카운트 읽기, clear 1회, WSTRB 무부작용, W1C 이벤트 우선순위, IRQ, 동시 쓰기, B/R 백프레셔를 검사합니다.
- **하드웨어 경고:** KS0013과 Seeed Grove Encoder는 공식 문서에서 5 V 계열이며, 3.3 V Basys3 GPIO 직결을 승인하지 않습니다. Arduino Modulino Knob는 STM32를 경유한 I²C 장치로 별도 제어 경로입니다.
- `quad_engine.v`는 변경하지 않았습니다. CI에서 AXI 레지스터 RTL 시뮬레이션을 수행하더라도 엔진·합성·타이밍·실물 보드 검증과 혼동하지 않습니다.
- **P1 C 드라이버 수정:** `../sw/quad.h`의 `quad_set_invert()`가 `CTRL[1]` IRQ_EN까지 지우던 문제를 읽기-수정-쓰기로 정정하고 `../tb/test_gpio_sw.c` 호스트 회귀시험 2건을 추가했습니다.

## 로터리 엔코더 (quad_*.v)
| 파일 | 역할 |
|---|---|
| quad_engine.v | A/B/SW 동기 + 안정 필터, 직교 전이 표(+1/-1/불가능), 32비트 부호 카운트, 버튼 눌림 펄스 |
| quad_axil_regs.v | 0x00 COUNT(쓰면 0) · 0x04 STATUS([0] 버튼 [1] press* [2] err* [3] step* [4] 방향) · 0x08 CTRL([0] invert [1] irq_en) |
| quad_axil_top.v | 배선만 |
검증: `../tb/model_quad.py`(전이 표·필터 알고리즘의 파이썬 모델)과 새 AXI 레지스터 테스트벤치를 구별합니다. **quad_engine.v 단독 RTL 시뮬레이션·합성·실물 검증은 미실시.** 방향 규약·필터 시간·버튼 극성은 설계안(`../rotary-encoder.md`).

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [dht22_axil_regs.v](dht22_axil_regs.v) | DHT22용 AXI4-Lite 레지스터. 버스 로직만 있다. |
| [dht22_axil_top.v](dht22_axil_top.v) | 배선만 한다. 로직은 하위 모듈에 있다. |
| [dht22_engine.v](dht22_engine.v) | DHT22(AM2302) 단일선 읽기 엔진. |
| [dht22_iobuf.v](dht22_iobuf.v) | 오픈 드레인 핀. low=1이면 0을 출력하고, 아니면 Hi-Z로 외부 풀업에 맡긴다. |
| [quad_axil_regs.v](quad_axil_regs.v) | 엔코더용 AXI4-Lite 레지스터. |
| [quad_axil_top.v](quad_axil_top.v) | 배선만 한다. 로직은 하위 모듈에 있다. |
| [quad_engine.v](quad_engine.v) | 로터리 엔코더(직교 2상 A/B) 디코더 + 버튼. 입력 3개 모두 동기 + 안정 필터. |
| [ultrasonic_distance_meter.v](ultrasonic_distance_meter.v) | 출처: 사용자 노션 'UART응용_05.초음파거리계+CDS+LED디밍 / PE.정병우' 의 ultrasonic_distance_meter. |
| [us_axil_regs.v](us_axil_regs.v) | HC-SR04 거리계용 AXI4-Lite 레지스터. 버스 로직만 있다. |
| [us_axil_top.v](us_axil_top.v) | 배선만 한다. 로직은 하위 모듈에 있다. |

<!-- index:end -->
