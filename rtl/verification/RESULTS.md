# 검증 방법별 결과 기록

기준일: 2026-10-31. 대상: `dev-board/rtl` 32개 과제.

| 방법 | RTL 범위 | 실행 여부 | 실제 결과 증거 | 판정 |
|---|---|---|---|---|
| Icarus RTL Directed Test | RTL01~RTL32 | 이 대화에서 실행 로그 확보 못함 | `rtl/verification/rtl/results/*.log` 생성 대기 | **미검증** |
| SystemVerilog UVM | RTL03 단일 과제 | 이 대화에서 실행 로그 확보 못함 | `rtl/verification/uvm/results/rtl03-uvm.log` 생성 대기 | **미검증** |
| SystemVerilog UVM | RTL01~02, RTL04~32 | UVM 환경 미작성 | 해당 없음 | **미착수** |
| Vivado Synthesis/Timing | RTL01~RTL32 | 미실행 | 보고서 없음 | **미검증** |

**실행한 적 없는 검증을 PASS로 표시하지 않는다.** GitHub Actions 런이 성공해도 아티팩트 로그와 요약 파일을 확인해 변경 후 갱신한다.

### 온라인 실행
- [GitHub Actions](https://github.com/Yuul-Jung/dev-board/actions)
- [Icarus RTL 테스트 파이프라인](../../.github/workflows/rtl-tests.yml)
- [UVM 검증 파이프라인](../../.github/workflows/rtl-uvm.yml)

UVM은 Accellera 표준 라이브러리 기반의 **SystemVerilog UVM**이며, 일반적인 Verilog 테스트벤치를 UVM이라고 부르지 않는다.
