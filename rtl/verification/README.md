# RTL/UVM 독립 검증

[RTL 검증](rtl/README.md) · [SystemVerilog UVM 검증](uvm/README.md) · [검증 결과](RESULTS.md)

**원칙:** 같은 RTL 소스를 수정 없이 검증하며, 서로 다른 시뮬레이터·검증 방법의 결과를 분리한다.

| 검증 방법 | 구현 | 범위 | 결과 |
|---|---|---|---|
| RTL Directed Test | Icarus Verilog | 기존 32개 과제의 tb 자동 검색 및 각 DUT 실행 | 32/32 PASS (CI 런 41189772658) |
| SystemVerilog UVM | Verilator 5.052 + Accellera UVM | RTL03 UART LED Controller에 대한 UVM Test/Driver/Monitor/Scoreboard | PASS (CI 런 41189772574) |
| Synthesis/Timing | Vivado | 별도 하드웨어 검증 | 미실시 |

자동 생성된 로그는 GitHub Actions Artifacts에서 확인한다. CI 실행 기록이 없는 경우 PASS로 표기하지 않는다.
