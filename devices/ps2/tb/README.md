# tb

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [ps2](../README.md)

호스트 테스트와 레지스터/장치 모델(mock)

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [model_ps2_rx.py](model_ps2_rx.py) | rtl/ps2_rx_engine.v 의 알고리즘을 클럭 단위로 옮긴 파이썬 모델. Verilog 자체를 검증하지는 않는다(시뮬레이터 없음). |
| [tb_ps2_rx_engine.v](tb_ps2_rx_engine.v) | 자체 검사 테스트벤치 (시뮬레이터 없어 실행하지 못함). 클럭 주기 40 us (Tck 30~50 us 범위). |
| [test_ps2_sw.c](test_ps2_sw.c) |  |

<!-- index:end -->
