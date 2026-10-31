# 검증 방법별 결과 기록

기준일: 2026-10-31. 대상: `dev-board/rtl` 32개 과제.

| 방법 | RTL 범위 | 실행 여부 | 실제 결과 증거 | 판정 |
|---|---|---|---|---|
| Icarus RTL Directed Test | RTL01~RTL32 | 실행함(GitHub Actions) | 런 41189209256·41189224440·41189772658·41189891691 success(32/32 PASS). 최초 런 41189056034 는 실패였고 RTL08·RTL09 상태폭 결함 때문(수정 248f08e) | **호스트 모의 시험 통과(좁은 지향 시험)** |
| SystemVerilog UVM | RTL03 단일 과제 | 실행함(Verilator 5.052) | 런 41189772574 success (b6b6111). 앞선 a5535b7 은 `env.agent.scoreboard` 경로 오류로 컴파일 실패 | **RTL03 UVM 통과(단일 과제)** |
| SystemVerilog UVM | RTL01~02, RTL04~32 | UVM 환경 미작성 | 해당 없음 | **미착수** |
| RTL 변형 검사(TB 검출력) | RTL01~32 | 실행함(런 41190240306) | 변형 222개 중 114 잡힘·108 살아남음, 검출률 51.4 %. 살아남은 것에는 동치 변형(리셋값 등)이 섞임 → 사람 판독 필요 | **TB 약함: 절반 가까이 못 잡음** |
| Vivado Synthesis/Timing | RTL01~RTL32 | 미실행 | 보고서 없음 | **미검증** |

## 2026-10-31 추가 적대적 검증·보강

- RTL10·11·13·15·25의 기존 간단한 TB를 **DUT 인스턴스 + 참조 모델 + 경계값(19/20/21 등) + 유효신호 유지 + Reset + 반복 입력**으로 개정했다.
- 검증 코드는 각 `rtl/rtlXX_*/tb/`에 저장했다. 실제 RTL 원본은 이 다섯 과제에서 수정하지 않았다.
- 변형 검사 워크플로는 이제 `rtl/rtl*/src/**`, `rtl/rtl*/tb/**` 변경 시에도 자동으로 실행하도록 보완했다.
- **새 커밋의 CI 및 변형 검사 결과는 아직 확인되지 않았다.** 이전 51.4%는 과거 기준값이며 최신 검출률로 재사용하지 않는다.
- 변형 검사 통과 판정은 동치 변형을 수동 분류한 뒤 비교해야 한다. 무조건 100%를 목표로 동치 변형을 억지로 실패시키지 않는다.

**실행한 적 없는 검증을 PASS로 표시하지 않는다.** 위 PASS 는 TB 가 검사하는 좁은 항목에 한한다(변형 검사가 보여 주듯 TB 가 못 잡는 결함이 있다). 합성·타이밍·보드는 여전히 0. GitHub Actions 런이 성공해도 아티팩트 로그와 요약 파일을 확인해 변경 후 갱신한다.

### 온라인 실행
- [GitHub Actions](https://github.com/Yuul-Jung/dev-board/actions)
- [Icarus RTL 테스트 파이프라인](../../.github/workflows/rtl-tests.yml)
- [UVM 검증 파이프라인](../../.github/workflows/rtl-uvm.yml)

UVM은 Accellera 표준 라이브러리 기반의 **SystemVerilog UVM**이며, 일반적인 Verilog 테스트벤치를 UVM이라고 부르지 않는다.
