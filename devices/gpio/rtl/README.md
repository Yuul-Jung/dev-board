# HC-SR04 거리계 (AXI4-Lite 래퍼)

거리 측정 로직은 사용자 노션의 `ultrasonic_distance_meter`를 **수정 없이** 가져왔고, 그 둘레에 AXI4-Lite 레지스터를 새로 붙였습니다.

| 파일 | 역할 |
|---|---|
| `ultrasonic_distance_meter.v` | 노션 원본. 트리거 10 µs, 측정 간격 60 ms, 에코 타임아웃 30 ms, 거리 = µs / 58 (최대 999 cm) |
| `us_axil_regs.v` | AXI4-Lite 레지스터와 인터럽트 (새로 작성) |
| `us_axil_top.v` | 배선만 하는 상위 모듈 (새로 작성) |
| `../tb/tb_ultrasonic.v` | 코어 테스트벤치 (10 cm, 100 cm, 타임아웃) |
| `../sw/ultrasonic.c` | MicroBlaze V 드라이버 예시 |

## 레지스터
| 오프셋 | 이름 | 비트 |
|---|---|---|
| 0x00 | CTRL (RW) | [0] enable |
| 0x04 | STATUS | [0] busy, [1] valid, [2] timeout(1을 쓰면 지움), [3] done(1을 쓰면 지움) |
| 0x08 | DIST (R) | [9:0] 거리 cm |
| 0x0C | BCD (R) | [11:0] BCD 3자리 |
| 0x10 | IRQ_EN (RW) | [0] 측정 완료, [1] 타임아웃 |

## 확인 필요
- **시뮬레이션과 합성은 하지 않았습니다.** 이 환경에 시뮬레이터가 없습니다. AXI 래퍼는 눈으로만 검토했습니다.
- 에코 입력은 5 V일 수 있으므로 분압 또는 레벨 시프터 뒤에서 연결해야 합니다.
- 코어는 active-high 리셋이라 래퍼에서 `rstn`을 반전해 넘깁니다.
- 코어의 `distance_valid`는 다음 측정 전까지 1로 유지됩니다. 새 값 알림은 `done` 플래그를 쓰세요.
- 에코 타임아웃 30 ms는 4 m 최대 에코 약 23.2 ms보다 깁니다(계산).
