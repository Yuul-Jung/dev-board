# 1-Wire 마스터 (전용 RTL)

AMD 쪽에서 열람한 1-Wire IP가 없어서 직접 설계합니다. AXI4-Lite 슬레이브로 MicroBlaze V에 붙고, 소프트웨어가 비트 단위로 명령을 내립니다.

## 모듈
| 파일 | 역할 |
|---|---|
| `onewire_axil_top.v` | 배선만 하는 상위 모듈 |
| `onewire_axil_regs.v` | AXI4-Lite 레지스터, 인터럽트 |
| `onewire_bit_engine.v` | 리셋·쓰기·읽기 슬롯 타이밍 |
| `onewire_iobuf.v` | 오픈 드레인 핀 (`SIMULATION` 정의 시 동작 모델) |
| `../tb/tb_onewire_engine.v` | 엔진 단독 테스트벤치 |
| `../sw/ds18b20.c` | MicroBlaze V 쪽 드라이버 예시 |

## 레지스터
| 오프셋 | 이름 | 비트 |
|---|---|---|
| 0x00 | CMD (쓰기) | [1:0] 0=리셋 1=쓰기 2=읽기, [2] 쓸 비트. 쓰면 바로 시작 |
| 0x04 | STATUS (읽기) | [0] busy, [1] done, [2] presence, [3] rbit |
| 0x08 | IRQ_EN | [0] done 인터럽트 허용 |

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
- **시뮬레이션과 합성은 아직 하지 않았습니다.** 이 환경에는 시뮬레이터가 없어 코드를 눈으로만 검토했습니다. Vivado에서 `SIMULATION`을 정의하고 테스트벤치를 돌려 보세요.
- 읽기 샘플 시점은 풀업으로 올라오는 상승 시간을 고려하지 않았습니다. 실제 배선에서 파형 확인이 필요합니다.
- 바이트 단위 명령, CRC 검사, 기생 전원용 강한 풀업 제어는 없습니다.
- 핀 제약(XDC)은 연결할 Pmod 핀을 정한 뒤 작성합니다.
