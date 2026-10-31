# src

<!-- nav -->[저장소 홈](../../../README.md) › [rtl](../../README.md) › [rtl01_button_led](../README.md)



## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [basys3_button_led_top.v](basys3_button_led_top.v) | Shell/Wrapper: 보드 핀과 재사용 모듈 연결만 한다. |
| [button_event.v](button_event.v) | 버튼 입력: 2FF 동기화 → 안정시간 필터 → 1클럭 press_pulse |
| [led_toggle_controller.v](led_toggle_controller.v) | 버튼 이벤트마다 1비트 LED 레지스터 상태를 반전한다. |

<!-- index:end -->
