# UVM(SystemVerilog) 독립 검증

## 환경
- Verilator 5.052 이상과 Accellera UVM reference implementation.
- Verilator의 2026-09 버전부터 UVM 지원이 추가되었지만, 제한 사항이 있으므로 **UVM 지원 = 모든 UVM 프로젝트 PASS**가 아니다.
- 현재 첫 번째 UVM DUT는 RTL03 `uart_led_controller`. 나머지 과제는 UVM 검증을 **수행하지 않았다**.

## 구성
- `uart_led_if.sv`: DUT 인터페이스.
- `uart_led_uvm_pkg.sv`: uvm_sequence_item, uvm_driver, uvm_monitor, uvm_scoreboard, uvm_agent, uvm_env, uvm_test.
- `tb_top.sv`: RTL03 모듈 인스턴스 및 `run_test`.
- `run.sh`: Verilator/UVM 컴파일 및 실행, `results/` 로그 저장.

## 판정 기준
UVM_ERROR/UVM_FATAL 발생 또는 Simulation 비정상 종료는 FAIL. 정상 종료 후 scoreboard에 명시적인 `UVM_SCOREBOARD_PASS`가 있어야 PASS.

검증 자료와 실행 로그를 RTL Directed Test 결과와 혼합하지 않는다.

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [run.sh](run.sh) | 실패하면 로그 앞·뒤를 ::error 주석으로 남긴다(로그 파일을 내려받지 못하는 환경에서도 원인 확인). |
| [tb_top.sv](tb_top.sv) |  |
| [uart_led_if.sv](uart_led_if.sv) |  |
| [uart_led_uvm_pkg.sv](uart_led_uvm_pkg.sv) |  |

<!-- index:end -->
