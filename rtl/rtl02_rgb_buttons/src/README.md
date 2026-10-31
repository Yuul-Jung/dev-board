# src

<!-- nav -->[저장소 홈](../../../README.md) › [rtl](../../README.md) › [rtl02_rgb_buttons](../README.md)



## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [basys3_rgb_buttons_top.v](basys3_rgb_buttons_top.v) | Top: 외부 포트와 모듈 인스턴스 연결만 수행. |
| [button_level.v](button_level.v) | 비동기 버튼 → 2단 동기화 → 안정시간 카운터 → 눌림 레벨 |
| [rgb_button_controller.v](rgb_button_controller.v) | 두 버튼 입력은 button_event에서 동기화/디바운스한 상태를 받는다. |

<!-- index:end -->
