# CLAUDE → ChatGPT 검증 인계 (CLAUDE_HANDOFF)

작성: 2026-10-31 (KST 19시대). 저장소 `Yuul-Jung/dev-board`, 브랜치 `main`. 요약 색인은 [RESULTS.md](RESULTS.md), 구조는 [README.md](README.md).
**검증 완료 보고와 실제 검증 완료는 다르다.** 아래 PASS 는 표의 런·커밋에 한정되며, 실행 로그 원문은 열람이 막혀 있어 **실행 단계 성공 여부와 `::error/::notice` 주석 본문**으로 판정했다(로그 파일·artifact zip 은 내려받지 못함).

## A. 검증 요약

- 검증 대상 SHA: `d6bd7e8`(세션 시작 시 main 헤드, 문서만 변경됨) 및 그 위에 Claude 가 올린 `3d259fa`, `7979c6b`.
- 검증 완료 SHA: RTL·변형은 `3d259fa`(런 41191680387 / 41191680348). UVM 은 `841a3a6`(런 41190695277)이며 이후 `rtl03_uart_led/`·`uvm/`·`rtl-uvm.yml` 무변경(`git diff` 빈 결과)이라 `3d259fa` 에도 유효하다고 **판단**(재실행은 안 함).
- 도구/버전: Icarus Verilog = Ubuntu `apt install iverilog`(버전 **미기록**), Verilator 5.052 + accellera-official/uvm-core(커밋 **미기록**), Python 3.
- RTL 검증 결과(런 41191680387, `3d259fa`): 32/32 PASS. 개수 검사 `total==32`, 폭 검사(lint_widths) 통과. 과제별 PASS 행은 `results/rtl-summary.tsv` artifact 에 있으나 **내려받지 못해** 개별 확인은 못 함(런 성공 = 스크립트가 32건 모두 PASS 로 끝났다는 간접 증거).
- UVM 검증 결과: RTL03 `uart_led_controller`, 테스트 `led_test`, PASS(런 41190695277). UVM_ERROR/FATAL 개수와 `UVM_SCOREBOARD_PASS` 는 `run.sh` 가 판정하며 성공 종료. 수치 원문은 미열람.
- Mutation 검증 결과(런 41191680348, `3d259fa`): 변형 222(컴파일 불가 0), 검출 127, 생존 95, 검출률 **57.2 %**. 동치 변형 수동 판정은 **미실시**.
- Lint: 비트 폭/리터럴 잘림만 검사(`lint_widths.py`). **Latch·Multiple Driver·조합 루프 검사는 하지 않았다.**
- 미검증: Vivado 합성/Implementation/Timing/XDC/Basys3 실물 전부, Latch·다중구동 정적 검사, RTL03 외 UVM, 생존 변형 동치 판정.

### 실행 이력(커밋별로 구분)
| 커밋 | 워크플로 | 런 | 결과 |
|---|---|---|---|
| adb50e7 | RTL directed | 41189056034 | **FAIL**(RTL08·09 타임아웃) |
| 248f08e | RTL directed | 41189209256 | PASS (RTL08·09 폭 수정 후) |
| a5535b7 | UVM | 41189224402 | **FAIL**(컴파일: `env.agent.scoreboard`) |
| b6b6111 | UVM | 41189772574 | PASS |
| 841a3a6 | UVM | 41190695277 | PASS |
| 3050551 | mutation | 41190240306 | 51.4 %(114/222) |
| 8c13ce3 | mutation | 41191096201 | 54.5 %(121/222), 5개 TB 보강 후 |
| d6bd7e8 | RTL directed | 41191564680 | PASS |
| 3d259fa | RTL directed | 41191680387 | PASS |
| 3d259fa | mutation | 41191680348 | 57.2 %(127/222) |
URL 형식: `https://github.com/Yuul-Jung/dev-board/actions/runs/<런 번호>`.

## B. 발견 오류

| RTL | 문제 | 심각도 | 오류 근거 | 수정 여부 |
|---|---|---|---|---|
| 08, 09 | `reg [1:0] state` 에 상태 4·5 대입 → 0·1 로 잘려 `done` 도달 불가 | 높음(기능 정지) | 런 41189056034 타임아웃, 3비트 수정 뒤 PASS | 수정함(248f08e) |
| 03(UVM) | `led_test` 가 존재하지 않는 `env.agent.scoreboard` 참조 | 중(검증 환경 결함) | 런 41189224402 컴파일 실패 | 수정함(b6b6111) |
| 08, 09 | TB 가 에지 수만 검사 → 데이터 순서·래치·주기 변형 5 중 4 생존 | 중(TB 공백) | 런 41191096201 생존 목록 | 수정함(3d259fa), 1/5→4/5 검출 |
| 전체 | TB 검출력 낮음(57.2 %) | 중 | 런 41191680348 | **미해결** |
| 도구 | `gen_pages.py` 리터럴 `\n`·이중 역슬래시로 실행 불가 | 낮음(문서 생성) | 로컬 실행 SyntaxError/PatternError | 수정함(841a3a6) |
| 전체 | 문서가 "미검증/로그 없음" 로 낡음 | 낮음 | 열람 | 수정함(b0c3382 등) |

## C. 수정 이력

| 파일 | 기존 문제 | 수정 내용 | 재검증 결과 |
|---|---|---|---|
| `rtl/rtl08_hc595_7seg/src/hc595_7seg.v`, `rtl/rtl09_max7219/src/max7219_tx.v` | state 2비트 | 3비트 | directed PASS(41189209256 이후 연속) |
| `rtl/verification/lint_widths.py`, `rtl-tests.yml` | 폭 잘림 정적 검사 없음 | 리터럴≥2^폭 대입 검출 단계 추가 | 변형(강제 잘림)으로 확인, CI 통과 |
| `rtl/verification/uvm/uart_led_uvm_pkg.sv` | 잘못된 계층 경로 | `env.scoreboard` | UVM PASS |
| `rtl/verification/uvm/run.sh`, `rtl/verification/rtl/run.sh` | 실패 원인을 로그 없이 볼 수 없음 | 실패 시 `::error` 주석 출력 | 실제 실패에서 원인 확인에 사용 |
| `rtl/verification/rtl/mutate.py`, `.github/workflows/rtl-mutation.yml` | 변형 검사 없음 | 연산자·상수·부정 변형 CI 도입 | 런 41190240306 등 |
| `rtl/rtl08.../tb/tb_hc595_7seg.v`, `rtl/rtl09.../tb/tb_max7219_tx.v` | 에지 수만 검사 | 데이터(MSB 먼저)·래치/cs_n·주기 3×DIV·2회 전송 | directed PASS, 변형 4/5 |
| `devices/tools/gen_pages.py` | 실행 불가 | 이스케이프 수정 | 생성·`--check` 통과 |

(RTL10·11·13·15·25 TB 보강 `dcc6410` 은 ChatGPT 작업, Claude 는 CI 결과만 확인.)

## D. 남은 문제

- 미해결 오류: 생존 변형 95건의 동치/실결함 미분류. 과제별 생존 수: RTL04 18, RTL07 13, RTL02 12, RTL01 10, RTL32 5, RTL14 5, RTL06 4, RTL29 3, RTL27 3, RTL23 3, RTL18 3, RTL16 3, RTL26 2, RTL21 2, RTL12 2, RTL31 2, RTL08 1, RTL09 1, RTL19 1, RTL22 1, RTL30 1.
- 미검증 기능: RTL05 는 실제 XADC 가 아님, RTL14/18/19/26/27/28 은 PHY·버스 IP 없는 컨트롤러, RTL32 는 장치 전체 통합이 아님(ChatGPT 자체 감사 기준).
- 개선이 필요한 TB: RTL04(PWM 경계 `>`/`>=`, 카운터 상수), RTL07(멜로디 시퀀서·톤 분주), RTL01/02(리셋값·디바운스 경계), RTL14(주소 범위 0x08~0x77 경계), RTL16/18/29(`&&`→`||` 조건 결합).
- 실물 검증 필요: Vivado 합성·타이밍·XDC·Basys3 보드, 5 V/3.3 V 레벨, 실제 센서 타이밍. 현재 **전부 0**.
- 해당 파일 `rtl/**/src/*.v` 에 `timescale` 없음(경고 수준).

### 생존 변형 목록(런 41191680348, `파일:줄 원본->변형`, 주석 길이 제한으로 **88/95건만 확보**; RTL31·RTL32 분 7건 누락)
```
rtl01_button_led button_event.v:14 >->>=
rtl01_button_led button_event.v:25 1'b1->1'b0
rtl01_button_led button_event.v:25 1'b0->1'b1
rtl01_button_led button_event.v:26 1'b1->1'b0
rtl01_button_led button_event.v:26 1'b0->1'b1
rtl01_button_led button_event.v:28 1'b0->1'b1
rtl01_button_led button_event.v:35 1'b0->1'b1
rtl01_button_led button_event.v:36 ==->!=
rtl01_button_led button_event.v:37 1'b0->1'b1
rtl01_button_led button_event.v:41 + 1'b1->- 1'b1
rtl02_rgb_buttons button_level.v:14 >->>=
rtl02_rgb_buttons button_level.v:18 ~->(제거)
rtl02_rgb_buttons button_level.v:21 1'b1->1'b0
rtl02_rgb_buttons button_level.v:21 1'b0->1'b1
rtl02_rgb_buttons button_level.v:22 1'b1->1'b0
rtl02_rgb_buttons button_level.v:22 1'b0->1'b1
rtl02_rgb_buttons button_level.v:23 1'b0->1'b1
rtl02_rgb_buttons button_level.v:24 1'b0->1'b1
rtl02_rgb_buttons button_level.v:28 1'b0->1'b1
rtl02_rgb_buttons button_level.v:29 ==->!=
rtl02_rgb_buttons button_level.v:30 1'b0->1'b1
rtl02_rgb_buttons button_level.v:32 +1'b1->- 1'b1
rtl04_pwm_led pwm_core.v:11 >->>=
rtl04_pwm_led pwm_core.v:21 1'b0->1'b1
rtl04_pwm_led pwm_core.v:22 8'd0->8'd1
rtl04_pwm_led pwm_core.v:23 9'd0->9'd1
rtl04_pwm_led pwm_core.v:25 1'b0->1'b1
rtl04_pwm_led pwm_core.v:26 + 1'b1->- 1'b1
rtl04_pwm_led pwm_core.v:27 ==->!=
rtl04_pwm_led pwm_core.v:27 8'hFF->8'h100
rtl04_pwm_led pwm_core.v:29 >->>=
rtl04_pwm_led pwm_core.v:29 9'd256->9'd257
rtl04_pwm_led pwm_core.v:29 9'd256->9'd257
rtl04_pwm_led pwm_core.v:32 + 1'b1->- 1'b1
rtl04_pwm_led switch_duty_controller.v:13 8'd0->8'd1
rtl04_pwm_led switch_duty_controller.v:14 8'd0->8'd1
rtl04_pwm_led switch_duty_controller.v:20 ==->!=
rtl04_pwm_led switch_duty_controller.v:20 8'hFF->8'h100
rtl04_pwm_led switch_duty_controller.v:20 9'd256->9'd257
rtl04_pwm_led switch_duty_controller.v:20 1'b0->1'b1
rtl06_timer_led switch_sync.v:12 1'b0->1'b1
rtl06_timer_led switch_sync.v:13 1'b0->1'b1
rtl06_timer_led tick_generator.v:10 >->>=
rtl06_timer_led tick_generator.v:15 1'b0->1'b1
rtl07_buzzer_melody melody_sequencer.v:14 >->>=
rtl07_buzzer_melody melody_sequencer.v:20 2'd1->2'd2
rtl07_buzzer_melody melody_sequencer.v:25 ||->&&
rtl07_buzzer_melody melody_sequencer.v:31 ==->!=
rtl07_buzzer_melody melody_sequencer.v:31 2'd2->2'd3
rtl07_buzzer_melody melody_sequencer.v:31 2'd0->2'd1
rtl07_buzzer_melody melody_sequencer.v:31 + 1'b1->- 1'b1
rtl07_buzzer_melody melody_sequencer.v:31 1'b1->1'b0
rtl07_buzzer_melody tone_generator.v:16 ||->&&
rtl07_buzzer_melody tone_generator.v:16 ||->&&
rtl07_buzzer_melody tone_generator.v:19 ==->!=
rtl07_buzzer_melody tone_generator.v:19 - 1'b1->+ 1'b1
rtl07_buzzer_melody tone_generator.v:19 1'b1->1'b0
rtl08_hc595_7seg hc595_7seg.v:9 1'b0->1'b1
rtl09_max7219 max7219_tx.v:9 1'b0->1'b1
rtl12_dht22_alarm dht22_limit_controller.v:4 >=->>
rtl12_dht22_alarm dht22_limit_controller.v:4 >=->>
rtl14_i2c_scan i2c_scan_controller.v:4 7'h08->7'h9
rtl14_i2c_scan i2c_scan_controller.v:6 &&->||
rtl14_i2c_scan i2c_scan_controller.v:8 &&->||
rtl14_i2c_scan i2c_scan_controller.v:9 &&->||
rtl14_i2c_scan i2c_scan_controller.v:11 7'h77->7'h78
rtl16_lcd1602 lcd_message_tx.v:6 &&->||
rtl16_lcd1602 lcd_message_tx.v:7 &&->||
rtl16_lcd1602 lcd_message_tx.v:7 &&->||
rtl18_spi_request spi_request_controller.v:6 &&->||
rtl18_spi_request spi_request_controller.v:8 &&->||
rtl18_spi_request spi_request_controller.v:9 &&->||
rtl19_sd_logger sample_logger.v:5 &&->||
rtl21_imu_level imu_tilt_alert.v:4 >->>=
rtl21_imu_level imu_tilt_alert.v:4 <-><=
rtl22_dc_motor motor_dir_controller.v:18 &&->||
rtl23_servo_pulse servo_target.v:9 >->>=
rtl23_servo_pulse servo_target.v:9 8'd180->8'd181
rtl23_servo_pulse servo_target.v:9 8'd180->8'd181
rtl26_bt_command bt_command_controller.v:5 &&->||
rtl26_bt_command bt_command_controller.v:8 8'h73->8'h74
rtl27_rs485_packet rs485_tx_direction.v:6 &&->||
rtl27_rs485_packet rs485_tx_direction.v:8 &&->||
rtl27_rs485_packet rs485_tx_direction.v:9 &&->||
rtl29_integration integrated_alarm.v:4 >->>=
rtl29_integration integrated_alarm.v:4 <-><=
rtl29_integration integrated_alarm.v:4 16'd20->16'd21
rtl30_event_fifo event_fifo2.v:13 -1'
```

## E. ChatGPT 검토 요청

- Claude 의 판단 중 재검토: ① UVM 이 `841a3a6` 이후 무변경이라 `3d259fa` 에 유효하다는 판단(재실행 안 함) ② 생존 변형 중 `1'b0↔1'b1` 리셋값·`>`↔`>=` 를 동치로 볼지(미판정) ③ RTL10·11·13·15·25 의 100 % 는 변형 1~2개라 무의미하다는 평가.
- 회귀 위험: RTL08·09 TB 가 `3×DIV` 주기와 `edge_count==8*latch_count` 를 하드코딩했다. DUT 를 정당하게 바꾸면(예: 래치 시점 변경) TB 가 실패한다. 의도된 엄격성이지만 DUT 변경 시 같이 갱신해야 한다.
- 추가 경계조건: DIV=1·DIV 큰 값, 전송 중 `start` 재입력(무시되는가), 전송 중 `rst`, RTL14 주소 0x07/0x08/0x77/0x78, RTL23 서보 펄스 범위 끝값, RTL30 FIFO full/empty 동시 push/pop.
- 기술적 의문: ① 변형 검사 판정이 `vvp 종료코드 0 && 'PASS' 출력` 인데 TB 가 `$fatal` 후에도 PASS 를 찍는 경우는 없는지 ② `mutate.py` 변형은 과제당 상한 60·연산자 위주라 상태 레지스터 폭·case 분기 누락 같은 결함은 못 만든다(폭은 별도 lint) ③ 합성 관점: `delay_count` 32비트 카운터, 상태 case 에 `default` 없음(래치 가능성은 순차 always 라 낮으나 미검사), 비동기 입력 동기화는 RTL별로 확인 필요. ④ Latch·Multiple Driver 검사는 Verilator `--lint-only` 또는 Yosys 로 추가 가능하나 이번에 실행하지 않았다.
