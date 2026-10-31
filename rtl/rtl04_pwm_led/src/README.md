# src

<!-- nav -->[저장소 홈](../../../README.md) › [rtl](../../README.md) › [rtl04_pwm_led](../README.md)



## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [basys3_pwm_led_top.v](basys3_pwm_led_top.v) | Shell/Wrapper: 보드 핀, wire, 모듈 연결만 담당한다. |
| [pwm_core.v](pwm_core.v) | 8-bit PWM phase + 9-bit duty(0..256), 0%/100% 명시 지원. |
| [switch_duty_controller.v](switch_duty_controller.v) | Basys3 스위치 입력은 비동기: 비트마다 2FF 동기화. |

<!-- index:end -->
