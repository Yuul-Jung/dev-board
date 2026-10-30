# tb

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [motor](../README.md)

호스트 테스트와 레지스터/장치 모델(mock)

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [mock_motor.h](mock_motor.h) | 모의 레지스터. 읽기/쓰기를 함수로 받아 DUTY 폭(mock_duty_mask)을 흉내 내고 쓰기 순서를 기록한다. |
| [mock_servo.h](mock_servo.h) | endif |
| [tb_dcmotor_engine.v](tb_dcmotor_engine.v) | 엔진 단독 테스트. 1 MHz 클럭, 주기 100, 듀티 25로 줄여 돌린다. |
| [tb_servo_engine.v](tb_servo_engine.v) | 자체 점검 테스트벤치 (미실행: 시뮬레이터 없음). |
| [test_dcmotor.c](test_dcmotor.c) | motor/tb/test_dcmotor.c — dcmotor.c 호스트 모의 시험. |
| [test_hb5.c](test_hb5.c) | motor/tb/test_hb5.c — Pmod HB5 드라이버 호스트 모의 시험. |
| [test_servo.c](test_servo.c) | 서보 드라이버 호스트 시험. 모의 레지스터는 servo_axil_regs.v 머리말(리터럴 오프셋)과 servo_engine.v 의 제한 동작(MIN>MAX 면  |
| [test_stepper.c](test_stepper.c) | 28BYJ-48 스테퍼 드라이버 호스트 시험 (교재 27장). |

<!-- index:end -->
