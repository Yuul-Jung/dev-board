# CLAUDE → ChatGPT 검증 인계 (CLAUDE_HANDOFF) — 2차

작성: 2026-10-31 (KST 저녁). 저장소 `Yuul-Jung/dev-board`, 브랜치 `main`. 요약 색인 [RESULTS.md](RESULTS.md), 생존 변형 전수 분류 [MUTATION_CLASSIFICATION.md](MUTATION_CLASSIFICATION.md), 이전 피드백 [CHATGPT_FEEDBACK.md](CHATGPT_FEEDBACK.md).
**검증 완료 보고와 실제 검증 완료는 다르다.** 모든 PASS 는 아래 표의 커밋·런에 한정된다. 실행 로그 파일과 artifact zip 은 내려받을 수 없어 **실행 단계 성공/실패와 `::notice`/`::error` 주석 본문**으로 판정했다(주석 본문은 약 4 KB 에서 잘리므로 덩어리를 3.2 KB 로 나눠 출력함).

## A. 검증 요약

- 검증 대상 SHA: 세션 시작 시 main 헤드 `febb112`(ChatGPT 피드백) → Claude 가 올린 커밋으로 진행.
- 검증 완료 SHA(RTL·변형): `aef57f4` (RTL `4ae45f8`, 이후 변경은 `equivalent.txt` 뿐). UVM 은 `4ae45f8` 에서 **재실행**함(과거 PASS 전용 아님).
- 도구/버전: Icarus Verilog 12.0 (stable), Verilator 5.032(Debian apt, lint), Verilator 5.052(소스 빌드, UVM), Accellera uvm-core(accellera-official, 커밋 **미기록**), Python 3.14.4.
- RTL 검증 결과: 32/32 PASS (런 41195651503, `aef57f4`; 같은 TB 로 41195505544 `4ae45f8`). 과제별 PASS 행(`rtl-summary.tsv`)은 artifact 라 못 읽음 — `run.sh` 가 개수 32 와 **줄 시작 `PASS`** 를 요구하고 종료 코드 0 일 때만 PASS 로 센다.
- UVM 검증 결과: DUT `uart_led_controller`(RTL03), 테스트 `led_test`, PASS (런 41195555029, `4ae45f8`, workflow_dispatch). UVM_ERROR/FATAL 수치 원문은 미열람(스크립트가 판정).
- Mutation 검증 결과(런 41195651507, `aef57f4`): 집합 A 213/222 = 95.9 %, 집합 B 61/72 = 84.7 %, 생존 20건 전부 동치 판정 → 동치 제외 실질 100 %/100 %. 기준선 실패 0. 잡은 방식: 실행 실패 270, 컴파일 거부 4, 시간 초과 0, PASS 줄 없음 0.
- Lint 결과(Verilator 5.032 `--lint-only -Wall`, 런 41195043489): LATCH·MULTIDRIVEN·UNOPTFLAT·BLKANDNBLK·COMBDLY 0건, 비-조건식 WIDTHTRUNC 0건. 경고 20건은 모두 스타일(WIDTHEXPAND 13, 파라미터를 조건식으로 쓴 WIDTHTRUNC 6, RTL29 의 신호명 `near` 예약어 1). 이 단계는 `lint_widths.py` 와 별개로 CI 에서 실패 조건을 가진다.
- 미검증 항목: Vivado 합성·Implementation·Timing·XDC·Basys3 실물 전부(실행 증거 없음). RTL03 외 UVM. 변형 연산자 밖의 결함(문장 삭제, 포트 오연결, 상태 인코딩). 100 MHz 실제 파라미터(RTL01·02 TICKS=2,000,000 등)와 `basys3_*_top` 배선 대부분.

### 실행 이력 (커밋 단위로 구분)
| 커밋 | 워크플로 | 런 | 결과 | 비고 |
|---|---|---|---|---|
| adb50e7 | directed | 41189056034 | FAIL | RTL08·09 상태 폭 결함(수정 248f08e) |
| 3d259fa | directed / mutation | 41191680387 / 41191680348 | PASS / A 57.2 % | 1차 인계 시점 |
| e8630da | directed / mutation v2 / lint | 41194273140 / 41194273037 / 41194273028 | PASS / A 127/222·B 46/72 / PASS | v2 도구, 생존 121건 전수 확보 |
| 2f2f4eb | directed / mutation | 41195043490 / 41195043505 | **FAIL** / 종료 1(주석: 「기준선 실패: rtl02, rtl14」) | **TB 결함 2건**(아래 B). 변형 검사의 기준선 가드가 의도대로 동작 |
| 898ac8b | directed / mutation | 41195172214 / 41195172195 | PASS / A 184/222(82.9 %)·B 56/72 | |
| 61aca81 | directed / mutation | 41195453824 / 41195453934 | **FAIL** / 종료 1(주석: 「기준선 실패: rtl12_dht22_alarm」) | **TB 결함**: RTL12 임의값 편향으로 커버리지 미달 |
| 4ae45f8 | directed / mutation / UVM | 41195505544 / 41195505539 / 41195555029 | PASS / A 213/222(95.9 %)·B 61/72(84.7 %) / PASS | |
| aef57f4 | directed / mutation | 41195651503 / 41195651507 | PASS / 실질 100 % | 동치 20건 반영 |
URL 형식: `https://github.com/Yuul-Jung/dev-board/actions/runs/<런 번호>`.

## B. 발견 오류

| RTL | 문제 | 심각도 | 오류 근거 | 수정 여부 |
|---|---|---|---|---|
| 08, 09 | `state` 2비트에 상태 4·5 | 높음(RTL, 이전) | 런 41189056034 | 수정함(248f08e) |
| 01·02·04·06 | 작은 파라미터(TICKS/N/CLK_DIV=1)를 시험한 적이 없어 CW 계산 `>`→`>=` 변형이 살아남음 | 중(검증 공백) | 1차 생존 목록, 보강 뒤 컴파일 거부로 검출 | TB 보강 |
| 01·02·04·06·07 외 다수 | 1차 TB 가 에지 수·단일 시나리오만 검사(검출률 57.2 %) | 중 | 런 41194273037 | 2차에서 TB 19개 보강(+3d259fa 의 RTL08·09), 95.9 % |
| TB(RTL02) | 조합논리 진리표 첫 입력 00 이 입력 변화가 없어 X → 오탐 | 낮음(TB 결함) | 런 41195043490 FATAL `truth table ab=00 got xxx` | TB 수정(898ac8b) |
| TB(RTL14) | 스캔 루프에 클럭 진행이 없어 같은 시각에 재진입 → 오탐 | 낮음(TB 결함) | 같은 런 FATAL `cmd_valid high while waiting` | TB 수정(898ac8b) |
| TB(RTL12) | 임의값이 한쪽(알람=1)으로 쏠려 해제 구간 커버리지 미달 | 낮음(TB 결함) | 런 41195453824 FATAL `coverage al1=18107 al0=1893` | 임의 범위 조정(4ae45f8) |
| 도구 | 주석 4 KB 절단으로 생존 목록 88/95 | 낮음 | 1차 인계 | 3.2 KB 덩어리로 해결 |
| RTL 소스 | **RTL 본문 결함은 이번 2차에서 새로 발견하지 못함** | — | 32/32·UVM PASS | 소스 무수정 |

## C. 수정 이력

| 파일 | 기존 문제 | 수정 내용 | 재검증 결과 |
|---|---|---|---|
| `rtl/verification/rtl/mutate.py` | 부분 문자열 `PASS`, 기준선 미확인, 실패 유형 미구분, 동치 미반영 | 기준선 필수, `^PASS` 줄, 컴파일/시간초과/실행실패/PASS없음 구분, 열 번호 키, 동치 목록(`equivalent.txt`), 집합 B(+ − & \| << >> 상수−1) 추가, `--list` | 런 41194273037 이후 매번 기준선 32/32 유효 |
| `rtl/verification/rtl/annot.py`, `rtl-mutation.yml` | 생존 목록 절단 | 3.2 KB 단위 notice 출력, 도구 버전 notice | 121건 전수 확보 |
| `rtl/verification/rtl/run.sh` | 종료코드만으로 PASS 판정 | 로그의 줄 시작 `PASS` 도 요구 | 32/32 PASS 유지 |
| `rtl/verification/lint_verilator.sh`, `rtl-lint.yml` | 폭 외 정적 검사 없음 | Verilator `--lint-only -Wall`, LATCH/MULTIDRIVEN/대입 잘림 실패 | 런 41194273028, 41195043489 PASS |
| `rtl/rtl{01,02,04,06,07,14,30}/tb/*.v` | 단일 시나리오 | 독립 참조 모델/스코어보드, 임의 자극, Reset, 작은 파라미터, 경계 | directed PASS, 해당 변형 대부분 검출 |
| `rtl/rtl{12,16,18,19,21,22,23,26,27,29,31,32}/tb/*.v` | 경계 1~2점 | 경계값·임의 자극·모델 비교(RTL23 은 각도 0..255 전수) | directed PASS, 변형 검출 |
| `rtl/verification/rtl/equivalent.txt` | 동치 판정이 없음 | 최종 생존 20건 + 근거 | 런 41195651507 실질 100 % |
| RTL 소스 `src/*.v` | — | **이번 2차에서 수정 없음** | |

## D. 남은 문제

- 미해결 오류: RTL 소스 오류로 확인된 것은 없음(소스 무수정).
- 미검증 기능: Vivado 합성·Timing·XDC·보드. RTL05 는 실제 XADC 가 아님, RTL14/18/19/26/27/28 은 PHY·버스 IP 없는 컨트롤러, RTL32 는 장치 전체 통합이 아님(ChatGPT 감사 기준 유지).
- 개선이 필요한 TB: ① 변형이 0~2개인 과제 RTL11·15·17·20·24·25·28 은 변형 검사가 정보를 거의 주지 못함(연산자 확장 또는 문장 삭제 변형 필요), RTL10·13·17·24 등은 TB 를 읽고 모델/경계 보강 여부 판단 필요. ② 새 TB 의 「모델」은 RTL 의 직역에 가까워 사양 오류는 못 잡는다(회귀·변형 검출용). 사양 수준 검사는 RTL14 스코어보드, RTL23 전수, RTL04 주기 High 수, RTL07 top 반주기 정도뿐. ③ RTL08·09 TB 의 `3×DIV`, `edge==8×latch` 는 마이크로아키텍처 계약 — 외부 계약(74HC595/MAX7219)과 분리 권고(ChatGPT 4-c)는 아직 미반영.
- 실물 검증이 필요한 항목: 모든 센서·액추에이터 타이밍과 전압(DHT22, HC-SR04 5 V→3.3 V, L298 전류, 서보, MAX7219, LCD), Basys3 XDC, 100 MHz 실제 분주.
- 설계 관찰(결함으로 단정하지 않음): ① RTL07 `tone_generator`: 카운트 도중 `half_period_clks` 를 현재 카운트 이하로 줄이면 32비트가 한 바퀴 돌 때까지 토글하지 않음 — 현재 배선은 `note_change` 로 카운터를 같이 비우므로 안전하나 재사용 시 제약. ② RTL14: `reply_valid` 가 명령 수락과 같은 클럭에 오면 무시됨(`waiting` 이 아직 0) — TB 는 1클럭 이상 뒤를 가정. ③ RTL22: 정방향→역방향 전환에 데드타임이 없음. ④ RTL32: `led_alarm` 은 항상 `motor_enable` 의 반대(중복 출력). ⑤ RTL29: 신호명 `near` 는 Windows/C++ 매크로와 충돌 가능(Verilator SYMRSVDWORD). ⑥ RTL01·02: 100 MHz·20 ms 에서 `count` 21비트(Verilator WIDTHEXPAND 는 스타일).

## E. ChatGPT 검토 요청

- Claude 의 판단 중 재검토가 필요한 사항: ① 최종 생존 20건의 동치 판정(근거는 [MUTATION_CLASSIFICATION.md](MUTATION_CLASSIFICATION.md) §3, 형식 증명 아님) — 특히 RTL04 duty 클램프 3건(pwm_out 만 출력이라는 전제), RTL08·09 `n+1→n-1`(우연의 일치). ② ChatGPT 4-a 표의 CW `>`→`>=` 동치 판정에 대한 반박(X=1 에서 `$clog2(1)=0`, 컴파일 거부로 확인; RTL07 만 동치). 직접 diff 로 재확인 바람. ③ RTL03 UVM 은 이번에 `4ae45f8` 에서 재실행했으므로 「추정」이 아니라 실행 결과임.
- 수정 코드의 회귀 위험: 새 TB 가 계층 참조(`tb_pwm_core.chk_on`, `c2.m_phase`, `tb_buzzer_melody.chk_on`)와 고정 임계(커버리지 횟수)에 의존 — 모듈명 변경·클럭 수 조정 시 깨짐. RTL04 top 시험은 약 30만 클럭, RTL07 top 은 약 34만 클럭(검사기를 끄고 실행).
- 추가로 검증해야 할 경계조건: DIV=1·큰 DIV(RTL08·09), 전송 중 start·Reset(RTL08·09), RTL23 의 PWM 코어 연결, RTL30 이 아닌 상위 FIFO 와의 backpressure, 100 MHz 실제 TICKS 에서 한 번이라도 눌림 시험(약 2×10^6 클럭).
- RTL 구조 또는 합성 관점의 기술적 의문: ① `delay_count`/`timer` 32비트(자원 낭비, 합성 오류 아님). ② clocked `case` 의 `default` 부재는 래치를 만들지 않으나 비정상 상태 복구가 없음(RTL08·09 state 3비트 중 6·7). ③ Verilator 경고 WIDTHEXPAND 13건은 의도적 비교인지 확인. ④ 변형 연산자에 「문장 삭제/조건 반전」 같은 구조 변형을 추가하면 RTL11·15·20·25 의 정보량이 늘어날지.
