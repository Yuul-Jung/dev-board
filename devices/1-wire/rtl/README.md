# 1-Wire 마스터 (전용 RTL)

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [1-wire](../README.md)

AMD 쪽에서 열람한 1-Wire IP가 없어서 직접 설계합니다. AXI4-Lite 슬레이브로 MicroBlaze V에 붙고, 소프트웨어가 비트 단위로 명령을 내립니다.

## 모듈
| 파일 | 역할 |
|---|---|
| `onewire_axil_top.v` | 배선만 하는 상위 모듈 |
| `onewire_axil_regs.v` | AXI4-Lite 레지스터, 인터럽트 |
| `onewire_bit_engine.v` | 리셋·쓰기·읽기 슬롯 타이밍 |
| `onewire_iobuf.v` | 오픈 드레인 핀 (`SIMULATION` 정의 시 동작 모델) |
| `../tb/tb_onewire_engine.v` | 엔진 단독 테스트벤치 |
| `../tb/tb_onewire_axil_regs.v` | AW/W 독립 수락, WSTRB, CMD·IRQ, B/R 백프레셔 회귀시험 |
| `../sw/ds18b20.c` | MicroBlaze V 쪽 드라이버 (비트 단위 모의 소자 시험 있음, `../tb/test_ds18b20.c`) |

## 레지스터
| 오프셋 | 이름 | 비트 |
|---|---|---|
| 0x00 | CMD (쓰기) | [1:0] 0=리셋 1=쓰기 2=읽기, [2] 쓸 비트. 쓰면 바로 시작 |
| 0x04 | STATUS (읽기) | [0] busy, [1] done, [2] presence, [3] rbit |
| 0x08 | IRQ_EN | [0] done 인터럽트 허용 |

## AXI4-Lite 인터페이스 기준 (2026-11-03 수정)

- `onewire_axil_top.v`: `s_axi_wstrb[3:0]` 포트 추가. 기존 Top은 기능 로직 없이 연결만 유지합니다.
- `onewire_axil_regs.v`: 주소(AW)와 데이터(W)가 **서로 다른 클록**에 도착해도 각각 수락하고, 두 채널이 모두 수락되면 한 번만 쓰기 응답을 생성합니다.
- `WSTRB[0]`이 유효한 경우에만 `CMD` 시작 또는 `IRQ_EN` 비트 변경을 허용합니다. 상위 바이트·0 스트로브 쓰기는 정상 응답하되 부작용이 없습니다.
- `CMD` 쓰기는 `busy` 중이면 기존 정책대로 **응답은 OKAY, 시작은 무시**합니다. 응용 소프트웨어는 `STATUS.busy`를 확인해야 합니다.
- `BVALID`과 `RVALID`은 상대방 `READY`가 도착할 때까지 유지합니다. AXI VIP 정식 전수 프로토콜 검증과는 구별합니다.

## 슬롯 타이밍 (DS18B20 데이터시트 AC 표 범위 안에서 선택)
| 동작 | 데이터시트 값 | 사용한 값 |
|---|---|---|
| 리셋 Low | tRSTL ≥ 480 µs | 500 µs |
| 리셋 High | tRSTH ≥ 480 µs | 500 µs |
| Presence 샘플 | tPDHIGH 15–60, tPDLOW 60–240 µs | 해제 후 70 µs (Low 구간이 항상 60–75 µs를 덮음: 계산) |
| 쓰기 0 Low | tLOW0 60–120 µs | 65 µs |
| 쓰기 1 Low | tLOW1 1–15 µs | 6 µs |
| 슬롯 길이 | tSLOT 60–120 µs, tREC ≥ 1 µs | 80 µs |
| 읽기 | 시작 Low > 1 µs, tINIT+tRC+tSAMPLE < 15 µs | Low 2 µs, 시작 후 12 µs에 샘플, 슬롯 75 µs |

기생 전원에서는 tRSTL이 960 µs를 넘으면 전원 리셋이 걸릴 수 있다고 문서에 있음. 500 µs는 이보다 낮음.

## 상태
- **AXI 레지스터 지향 테스트벤치 CI 통과(2026-11-03):** [Actions #41499100802](https://github.com/Yuul-Jung/dev-board/actions/runs/41499100802)에서 `onewire_axil_top` 컴파일과 `PASS tb_onewire_axil_regs`를 확인했습니다. 이는 AW/W·WSTRB·B/R·CMD·IRQ에 대한 지정 시나리오의 시뮬레이션이며, AXI VIP 전수 검증·비트 엔진 실물 타이밍·Vivado 합성·보드 실행은 여전히 미검증입니다.
- 읽기 샘플 시점은 풀업으로 올라오는 상승 시간을 고려하지 않았습니다. 실제 배선에서 파형 확인이 필요합니다.
- 바이트 단위 명령, CRC 검사, 기생 전원용 강한 풀업 제어는 없습니다.
- 핀 제약(XDC)은 연결할 Pmod 핀을 정한 뒤 작성합니다.

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [onewire_axil_regs.v](onewire_axil_regs.v) | AXI4-Lite 슬레이브 레지스터. 로직만 있고 버스 핀은 갖지 않는다. |
| [onewire_axil_top.v](onewire_axil_top.v) | 배선만 한다. 로직은 하위 모듈에 있다. |
| [onewire_bit_engine.v](onewire_bit_engine.v) | 1-Wire 비트 엔진: 리셋/쓰기/읽기 한 동작을 µs 단위로 실행한다. |
| [onewire_iobuf.v](onewire_iobuf.v) | 오픈 드레인 버스 핀. low=1이면 0을 출력하고, 아니면 Hi-Z로 풀업에 맡긴다. |

<!-- index:end -->
