# sw

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [motor](../README.md)

드라이버 C 소스

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [dcmotor.c](dcmotor.c) |  |
| [dcmotor.h](dcmotor.h) | DC 모터 드라이버 (L298N / TB6612FNG / Pmod HB5). AXI4-Lite 커스텀 RTL 경유. |
| [hb5.c](hb5.c) | Pmod HB5 드라이버 구현. |
| [hb5.h](hb5.h) | Pmod HB5 — 1채널 H 브리지 드라이버 (양방향 DC 모터, 최대 2 A). |
| [servo.c](servo.c) |  |
| [servo.h](servo.h) | 서보(SG90 등) 드라이버 — `rtl/servo_axil_*` 레지스터 위에서 동작. 교재 26장. |
| [stepper.c](stepper.c) | 8단계 코일 표. 비트 0 = IN1 ... 비트 3 = IN4. A..D 는 IN1..IN4 (대응은 설계안, stepper.h 참고) |
| [stepper.h](stepper.h) | 28BYJ-48 스테퍼 + ULN2003 보드 드라이버 (교재 27장 대응). AXI GPIO(PG144) 4핀으로 코일 4개를 직접 구동한다. |

<!-- index:end -->
