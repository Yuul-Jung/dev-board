# DC 모터 구동 (전용 RTL)

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [motor](../README.md)

L298과 TB6612FNG는 둘 다 `PWM + 방향 2핀`으로 구동되므로 같은 RTL로 맞춥니다.

| 파일 | 역할 |
|---|---|
| `dcmotor_axil_top.v` | 배선만 하는 상위 모듈 |
| `dcmotor_axil_regs.v` | AXI4-Lite 레지스터 |
| `dcmotor_engine.v` | PWM, 방향, 방향 전환 보호 |
| `../tb/tb_dcmotor_engine.v` | 엔진 테스트벤치(정회전, 듀티, 방향 전환 코스트, 브레이크, 비활성) |
| `../sw/dcmotor.c` | MicroBlaze V 드라이버 예시 |

## 레지스터
| 오프셋 | 이름 | 내용 |
|---|---|---|
| 0x00 | CTRL (RW) | [0] enable, [2:1] mode: 0 정지(코스트), 1 정회전, 2 역회전, 3 브레이크 |
| 0x04 | DUTY (RW) | 켜짐 클럭 수 (32비트; 예전 RTL 은 16비트였고 기본 주기 100000 에서 66 % 넘는 듀티가 잘려 나갔다 — VERIFICATION.md 행 40) |
| 0x08 | PERIOD (RW) | PWM 주기 클럭 수 (기본 100,000 = 100 MHz에서 1 kHz) |
| 0x0C | STATUS (R) | [1:0] 적용된 mode, [2] 방향 전환 대기 |

## 드라이버별 동작
- **L298**: ENA ← `motor_pwm`, IN1/IN2 ← `motor_in1/in2`. 정회전은 C=H, D=L, 역회전은 C=L, D=H, 브레이크는 ENA High에서 C=D(진리표 확인). 코스트는 PWM 0으로 Ven Low.
- **TB6612FNG**: PWM ← `motor_pwm`, IN1/IN2 동일. 브레이크는 IN1=IN2=High. STBY는 별도 GPIO로 High 유지.
- **방향 전환 보호**: 정↔역 전환 시 `DEAD_US`(기본 1 ms) 동안 코스트를 거칩니다. 이 값은 설계 선택이며 드라이버 문서의 요구값이 아닙니다.
- 리셋 후 enable = 0이라 코스트 상태로 시작합니다.

## 확인 필요
- **시뮬레이션과 합성은 하지 않았습니다.**
- 사용자 노션의 `l298_distance_motor_controller`(CdS + 초음파 + 거리 비례 속도)는 응용 계층입니다. 이 레지스터 맵 위에서 소프트웨어로 만들 수도 있고, 노션 RTL을 그대로 쓸 수도 있습니다.
- 전원·GND 분리(포토커플러), 모터 전원 용량은 하드웨어 설계 몫입니다. 일반 L298N 모듈은 환류 다이오드 8개가 이미 있고, **ENA 점퍼를 꽂은 채 FPGA 가 ENA 를 구동하면 5 V 레일과 맞붙으므로 점퍼를 빼야 합니다**([l298.md](../l298.md)). SENSE 핀은 모듈에서 GND 에 직결이라 전류 감지는 못 씁니다.

## 서보 PWM (SG90 등)
`servo_engine.v`, `servo_axil_regs.v`, `servo_axil_top.v` — 주기·폭을 µs 단위로 받는 PWM. 폭은 MIN/MAX 레지스터(기본 1000/2000 µs)로 제한하며, SG90 자료마다 펄스 범위가 달라서(500–2400 µs vs 1–2 ms) 소프트웨어가 안전한 범위를 정하도록 했습니다.
레지스터: 0x00 CTRL(enable) · 0x04 WIDTH · 0x08 PERIOD(기본 20000) · 0x0C MIN · 0x10 MAX · 0x14 APPLIED(읽기). 테스트벤치 `tb/tb_servo_engine.v`. 시뮬레이션 전. C 드라이버 `../sw/servo.[ch]`, 호스트 시험 `../tb/test_servo.c`.

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [dcmotor_axil_regs.v](dcmotor_axil_regs.v) | DC 모터용 AXI4-Lite 레지스터. 버스 로직만 있다. |
| [dcmotor_axil_top.v](dcmotor_axil_top.v) | 배선만 한다. 로직은 하위 모듈에 있다. |
| [dcmotor_engine.v](dcmotor_engine.v) | DC 모터 구동 엔진: PWM 1개 + 방향 2개. L298(ENA, IN1, IN2)과 TB6612FNG(PWM, IN1, IN2) 둘 다 이 출력으로 구동한다 |
| [servo_axil_regs.v](servo_axil_regs.v) | 서보 PWM용 AXI4-Lite 레지스터. 버스 로직만 있다. |
| [servo_axil_top.v](servo_axil_top.v) | 배선만 한다. 로직은 하위 모듈에 있다. |
| [servo_engine.v](servo_engine.v) | 서보 PWM 엔진: 주기 period_us, 펄스 폭 width_us (마이크로초 단위). |

<!-- index:end -->
