# rtl

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [ps2](../README.md)

Verilog RTL 과 RTL 테스트벤치(시뮬레이터 없음, 미실행)

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [ps2_axil_regs.v](ps2_axil_regs.v) | PS/2 수신용 AXI4-Lite 레지스터 + 16바이트 FIFO. |
| [ps2_axil_top.v](ps2_axil_top.v) | 배선만 한다. 로직은 하위 모듈에 있다. PS/2 두 선은 입력 전용(수신만). |
| [ps2_rx_engine.v](ps2_rx_engine.v) | PS/2 수신 엔진 (장치 -> 호스트 전용). 송신(호스트 -> 장치)은 만들지 않았다. |

<!-- index:end -->
