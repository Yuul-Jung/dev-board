# src

<!-- nav -->[저장소 홈](../../../README.md) › [rtl](../../README.md) › [rtl07_buzzer_melody](../README.md)



## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [basys3_buzzer_top.v](basys3_buzzer_top.v) | Shell/Wrapper: 외부 스위치 동기화 및 재사용 모듈 간 연결만 수행. |
| [melody_sequencer.v](melody_sequencer.v) | 버튼 enable이 1인 동안 A4(440)→C5(523)→E5(659) 순환. |
| [tone_generator.v](tone_generator.v) | tone_enable 동안 half_period_clks 클럭마다 buzzer 출력 반전. |

<!-- index:end -->
