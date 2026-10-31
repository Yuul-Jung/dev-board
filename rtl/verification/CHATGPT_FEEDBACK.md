# ChatGPT 독립 적대적 검증 피드백 — 2026-10-31

원본: [CLAUDE_HANDOFF.md](CLAUDE_HANDOFF.md) · [RESULTS.md](RESULTS.md). 검토한 핵심 커밋: [248f08e](https://github.com/Yuul-Jung/dev-board/commit/248f08eadf151fea90805f9f0bac9945e1c2ed30), [3d259fa](https://github.com/Yuul-Jung/dev-board/commit/3d259fad21622581474d32b900e7939e7abd01af), [3853185](https://github.com/Yuul-Jung/dev-board/commit/38531855ba2b49a9a977f17c4381725591fe53f4).

## 교차 검토 판정

| 항목 | 독립 판정 | 근거와 주의 |
|---|---|---|
| RTL08/09 state 폭 | 오류 원인과 3비트 수정 타당 | diff에서 2비트→3비트 확인. 시뮬레이터 로그 원문 직접 조회한 것은 아님 |
| RTL08/09 TB 개선 | 실제 비교 범위 확대 확인 | 데이터, 에지 간격, 래치, 두 번째 전송 추가. 정상적인 구현 변경에도 3×DIV 사이클 규약을 수정해야 할 수 있음 |
| RTL 32/32 PASS | **인계 기록에 근거한 과거 실행 판정** | 런 41191680387의 원본 로그/Artifact는 독립 확보하지 못함. 최신 커밋 자동 적용 금지 |
| UVM RTL03 PASS | **해당 소스 버전에 한정해 유효 추정** | 이후 UVM/DUT/설정이 무변경이라면 회귀 신뢰성은 높으나 새로운 실행 PASS라 기록 금지 |
| Mutation 57.2% | 127/222 계산·자료 일치 | 95 생존 전수 목록 없음(88 확보). 분모는 선택된 변형 연산자에 한정됨 |
| Vivado·Timing·보드 | 미검증 | PASS 증거 없음 |

## 4-a 동치/비동치 선별: 실제 소스 기준

동치는 **정해진 파라미터·Reset 가정·공개 출력·시간 규약**에 관해 모든 입력 시퀀스에서 출력이 동일함을 뜻함. 일반적 코드 형태만으로 일괄 판정하지 않는다.

| 생존 변형 | 판정 | 이유 / 후속 시험 |
|---|---|---|
| RTL01 `CW=(TICKS > 1)?...:1`의 `> → >=` | 동치(정상 양수 TICKS) | TICKS=1인 경우 양쪽 모두 1비트 |
| RTL04 `DIV_W=(CLK_DIV > 1)?...:1`의 `> → >=` | 동치(정상 양수 CLK_DIV) | CLK_DIV=1에서 둘 다 1비트 |
| RTL06 `CW=(CLOCKS_PER_TICK > 1)?...:1`의 `> → >=` | 동치(정상 양수 범위) | 위와 동일 |
| RTL07 `CW=(NOTE_CLOCKS > 1)?...:1`의 `> → >=` | 동치(정상 양수 범위) | 위와 동일 |
| RTL04 `duty_in > 9'd256`을 `>=` | 동치 | duty_in=256에서 양쪽 결과가 256 |
| RTL01·02 reset의 `1'b0↔1'b1` | **대개 비동치 후보** | 초기 동기화 상태·press_pulse와 다음 이벤트가 달라질 수 있음. Reset 해제 직후 관찰 필요 |
| RTL04 `phase=0→1`, `duty_reg=0→1` | **비동치** | 첫 PWM 출력·첫 사이클 위상이 바뀜 |
| RTL04 `sw_sync == 8'hFF`를 `!=` | **비동치** | Duty 매핑이 반대가 됨 |
| RTL04 `8'hFF → 8'h100` | **비동치(폭 잘림)** | 8'h100은 8비트에서 0으로 잘려 phase 경계가 255에서 0으로 변경 |
| RTL04 `9'd256 → 9'd257` | **비동치 후보** | 100% 경계가 바뀌거나 257이 출력 경로에 나타남 |
| RTL07 `2'd1→2'd2` | **비동치** | 멜로디의 음계 분기 선택 변경 |
| RTL08·09 마지막 시프트 채움 `0→1` | **외부 관측 범위에서 동치 후보** | 마지막 비트가 전송된 이후의 내부 상태만 달라질 수 있음. 재전송·Reset·후속 출력까지 확인 후 판정 |

**95건 중 이 표는 대표 유형만 판정한 1차 샘플**이며 전수 판정이 아니다. 현재 88건만 인계되어 RTL31/32 7건은 데이터 부족으로 판정 유보. mutation_survivors.txt Artifact 다운로드 후 전수 테이블을 작성할 것.

## 4-b 표본 수가 작은 100%
RTL10·11·13·15·25의 변형 1~2개에 대한 100% 검출은 **그 변형 집합에 대해 100%**라는 뜻일 뿐 결함 검출 일반 능력을 보장하지 않는다. 따라서 검증 강도가 약하다는 Claude의 지적은 타당하다. mutation 연산자·경계값·상태 전이를 늘릴 것.

## 4-c RTL08·09 타이밍 하드코딩
`3×DIV`, `edge_count == 8*latch_count` 검사는 현재 RTL 구조의 **마이크로아키텍처 계약**을 검증하는 용도로 유효하다. 외부 74HC595/MAX7219 인터페이스 계약만 요구한다면 과도한 구현 의존이다. 최소 High/Low 시간, 데이터 setup/hold, 8/16비트 정확성, CS/latch 순서의 외부 관측 검사와 분리 권장. DIV=1, DIV>2, 전송 중 start·Reset 별도 검증.

## 4-d mutate.py 판정 안전성
현재 판정은 `vvp returncode==0` 및 stdout에 부분 문자열 `PASS` 포함. `$fatal`이 실제로 실행되어 비정상 종료하면 대체로 kill로 분류되지만, 실행 종료 0으로 누락된 assertion, 조기 `$finish`, 단순 문자열 `PASS`를 출력하는 다른 메시지의 **거짓 생존**에 취약하다. 반대로 테스트벤치 구조 때문에 정상 통과인데 PASS를 출력하지 않는 경우 거짓 kill이 가능하다.
- 테스트 종료 전 `TEST_PASS:<testname>` 고유 토큰을 표준화하고, 정확한 줄 단독 매칭 필요.
- baseline TB 자체가 통과하는지 먼저 확인. FAIL baseline인 항목의 mutation 결과는 유효하지 않음.
- timeout, 컴파일 실패, 런타임 실패, 기능 assertion 실패를 **각각 별도 유형**으로 표시.
- mutation 생성기 자체가 상수폭을 깨뜨리는 경우는 별도로 Invalid 범주로 분리; 의미상 동치와 구별.

## 4-e 추가 경계값 우선순위
1. RTL04: 0/1/255/256 및 잘못된 9비트 duty 입력(257~511), PWM 첫 주기 및 변경 경계.
2. RTL01/02: Reset 해제 직후 안정화/바운스 및 유효 TICKS=1.
3. RTL07: NOTE_CLOCKS=1, 마지막 음에서 처음 음으로 래핑, 주파수 분주 경계.
4. RTL08/09: DIV=1, DIV 큰 값, busy 중 start 재요청, 전송 중 Reset.
5. RTL14: 주소 08~77의 양끝과 NACK, 다수 ACK, 고착 ready/reply timeout.
6. RTL23: 0/90/180/181/255°, 1000/1500/2000us 출력.
7. RTL30: empty/full 및 동시 push/pop, wraparound, FIFO 순서 불변식.

## 4-f 합성 관점
- RTL08/09의 `delay_count`/`timer`는 32비트지만 보통 `DIV` 값에서 필요한 폭은 `max(1,$clog2(DIV))`. **선택적 자원 최적화**이며 합성 오류로 취급하지 않는다.
- Clocked `always @(posedge clk)`의 `case`에 `default`가 없으면 자동으로 Latch를 추론하는 것은 **아니다**. 불법/X 상태에서 보존되거나 복구가 안 되는 문제는 별도 확인.
- FPGA 외부 비동기 입력에 2FF/필요시 debounce·CDC, XDC, 모터·센서 전압 규칙을 분리 검사해야 한다.
- 현재 lint_widths.py는 범용 lint를 대신하지 못한다. Verilator lint, Yosys proc/check 등 추가 필요.

## ChatGPT 최종 검토 결과
- **동의:** RTL08/09 FSM 폭과 UVM 경로 오류 수정, 검출률 취약성, 실제 하드웨어 미검증.
- **조건부 동의:** RTL03 UVM 옛 PASS를 변경 없는 소스에 적용할 수 있다는 근거(재실행 PASS가 아님).
- **보완 요청:** 95건 전체 생존 변형의 원문 확보, 동치 정의를 파라미터 범위별로 설정, baseline 유효성·변형 판정기 강화, 최신 RTL/UVM/Mutation 실제 로그 및 버전 보존.
- 본 문서는 독립 **코드 및 커밋 diff 검토** 결과이고, CI 원문·Vivado Timing·Board 측정 결과를 생성하거나 확인한 문서가 아니다.
