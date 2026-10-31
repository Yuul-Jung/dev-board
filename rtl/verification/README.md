# RTL/UVM 독립 검증

[RTL 검증](rtl/README.md) · [SystemVerilog UVM 검증](uvm/README.md) · [검증 결과](RESULTS.md) · [Claude 인계](CLAUDE_HANDOFF.md)

**원칙:** 같은 RTL 소스를 수정 없이 검증하며, 서로 다른 시뮬레이터·검증 방법의 결과를 분리한다.

| 검증 방법 | 구현 | 범위 | 결과 |
|---|---|---|---|
| RTL Directed Test | Icarus Verilog | 기존 32개 과제의 tb 자동 검색 및 각 DUT 실행 | 32/32 PASS (CI 런 41189772658) |
| SystemVerilog UVM | Verilator 5.052 + Accellera UVM | RTL03 UART LED Controller에 대한 UVM Test/Driver/Monitor/Scoreboard | PASS (CI 런 41189772574) |
| Synthesis/Timing | Vivado | 별도 하드웨어 검증 | 미실시 |

자동 생성된 로그는 GitHub Actions Artifacts에서 확인한다. CI 실행 기록이 없는 경우 PASS로 표기하지 않는다.

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 하위 폴더

| 폴더 | 내용 |
|---|---|
| [rtl/](rtl/README.md) | Verilog RTL 과 RTL 테스트벤치(시뮬레이터 없음, 미실행) |
| [uvm/](uvm/README.md) | UVM(SystemVerilog) 독립 검증 |

### 파일

| 파일 | 설명 |
|---|---|
| [CLAUDE_HANDOFF.md](CLAUDE_HANDOFF.md) | CLAUDE → ChatGPT 검증 인계 (CLAUDE_HANDOFF) |
| [RESULTS.md](RESULTS.md) | 검증 방법별 결과 기록 |
| [lint_widths.py](lint_widths.py) | 간이 폭 검사(시뮬레이터 없이 소스만 읽음). 사용: python3 rtl/verification/lint_widths.py [루트] |

<!-- index:end -->


## Claude 및 외부 검토자를 위한 검증 자료 색인

검증 시 반드시 **현재 main의 SHA**를 먼저 기록하고, 해당 SHA와 일치하는 GitHub Actions 실행 로그를 확인한다. 과거 성공 실행을 최신 커밋의 성공으로 전용하지 않는다.

| 검증 영역 | 결과 기록 | 실행 설정 | 원본 증거 |
|---|---|---|---|
| RTL Directed Test(32개) | [RESULTS.md](RESULTS.md) | [rtl-tests.yml](../../.github/workflows/rtl-tests.yml) | [GitHub Actions 실행 목록](https://github.com/Yuul-Jung/dev-board/actions/workflows/rtl-tests.yml), 각 실행의 Artifacts |
| UVM(현재 RTL03) | [RESULTS.md](RESULTS.md) | [rtl-uvm.yml](../../.github/workflows/rtl-uvm.yml) | [UVM 실행 목록](https://github.com/Yuul-Jung/dev-board/actions/workflows/rtl-uvm.yml), 각 실행의 Artifacts |
| Mutation Test(변형 검증) | [RESULTS.md](RESULTS.md) | [rtl-mutation.yml](../../.github/workflows/rtl-mutation.yml) | [Mutation 실행 목록](https://github.com/Yuul-Jung/dev-board/actions/workflows/rtl-mutation.yml), mutation.tsv 및 survivors |
| 폭 정적 검사 | [lint_widths.py](lint_widths.py) | [rtl-tests.yml](../../.github/workflows/rtl-tests.yml) | 해당 실행의 Step 로그 |
| Vivado/Timing/실물 | [VERIFICATION_STATUS.md](../VERIFICATION_STATUS.md) | 미구성 | 미검증 |
| 오류 이력 및 수정 근거 | [ADVERSARIAL_REVIEW_20261031.md](../ADVERSARIAL_REVIEW_20261031.md) | 별도 보고서 | Git 커밋 및 변경분 |

### 이미 기록된 과거 실행 (현재 코드의 최신 PASS를 보장하지 않음)
- RTL: [41189772658](https://github.com/Yuul-Jung/dev-board/actions/runs/41189772658) — 당시 32/32 PASS 기록.
- UVM: [41189772574](https://github.com/Yuul-Jung/dev-board/actions/runs/41189772574) — 당시 RTL03 PASS 기록.
- Mutation: [41190240306](https://github.com/Yuul-Jung/dev-board/actions/runs/41190240306) — 222개 중 114개 검출, 108개 생존(51.4%) 기록.
- 이후 RTL10·11·13·15·25 테스트벤치를 수정했으므로 Mutation 검출률은 **재실행 전 미확정**.

### 검증 결과 기재 규칙
1. 날짜, 대상 커밋 SHA, workflow run ID/URL, 검증 도구·버전 기록.
2. RTL: 실제 테스트 수, PASS/FAIL, 실패 과제와 원본 로그 경로 기록.
3. UVM: DUT명, 테스트명, UVM_ERROR/UVM_FATAL 개수 및 Scoreboard 결과 기록.
4. Mutation: 생성·무효·유효 변형, 검출·생존 수, 동치 변형 수동 판정 결과 분리.
5. Vivado: Synthesis·Implementation·Timing·XDC·보드 검증을 독립 판정.
6. 결과 파일은 검증 실행 단위별로 별도 보관하고, `RESULTS.md`는 **요약 색인**으로만 사용.

