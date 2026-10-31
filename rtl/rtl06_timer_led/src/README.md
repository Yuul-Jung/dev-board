# src

<!-- nav -->[저장소 홈](../../../README.md) › [rtl](../../README.md) › [rtl06_timer_led](../README.md)



## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [basys3_timer_led_top.v](basys3_timer_led_top.v) | Top Shell/Wrapper: module 인스턴스와 신호 연결만. |
| [switch_sync.v](switch_sync.v) | 비동기 1비트 스위치를 clk로 동기화하는 2단 FF. |
| [tick_generator.v](tick_generator.v) | 지정된 CLOCKS_PER_TICK 클럭마다 정확히 1클럭 tick 출력. |
| [timer_led_controller.v](timer_led_controller.v) | tick이 High인 posedge에서만 LED 상태를 반전. |

<!-- index:end -->
