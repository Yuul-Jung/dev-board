# 변형 검사 생존 변형 전수 분류 (2차 적대적 검증)

작성: 2026-10-31. 도구: `rtl/verification/rtl/mutate.py` v2, Icarus Verilog 12.0. 1차 목록은 런 41194273037(커밋 e8630da) 의 주석에서 확보한 **121건 전수**(집합 A 95 + 집합 B 26)이며,
이전에 인계한 「88/95건」의 누락(RTL31·32)은 주석 본문이 약 4 KB 에서 잘리던 것이 원인이었다(덩어리를 3.2 KB 로 나눠 해결).

## 1. 결과 요약
- 1차(e8630da): 집합 A 127/222 = 57.2 %, 집합 B 46/72 = 63.9 %. 생존 A 95 + B 26 = 121.
- 최종(4ae45f8, 런 41195505539): 집합 A 213/222 = **95.9 %**, 집합 B 61/72 = **84.7 %**. 생존 A 9 + B 11 = 20.
- 최종 생존 20건은 모두 아래 근거로 **동치 판정**(`equivalent.txt`). 동치 제외 실질 검출률은 A·B 모두 100 % (런 41195651507, 커밋 aef57f4).
- 주의: 100 % 는 「이 연산자 집합이 만든 변형 294개 중 동치를 뺀 나머지를 모두 잡았다」는 뜻일 뿐이다. 연산자 집합이 좁고(문장 삭제·포트 오연결·상태 인코딩 변형 없음) 변형 수가 0~2개인 과제(RTL11·15·20·25 등)도 있다. 동치 판정은 사람의 논리 추론이며 형식 증명이 아니다.

## 2. 과제별 해소 현황
| 과제 | 1차 생존 | 최종 생존 | 해소 방법(TB 보강) |
|---|---|---|---|
| rtl01_button_led | 11 | 2 | btn_ev_chk 8개(ACTIVE_LOW 0/1, TICKS 1·2·3·5·6·8)+led_chk, 매 클럭 모델 비교 |
| rtl02_rgb_buttons | 14 | 2 | lvl_chk 8개(TICKS 1~8)+진리표 4조합 |
| rtl04_pwm_led | 24 | 5 | pwm_chk CLK_DIV 1~5 모델 비교+샘플 경계 -1/0/+1+주기 High 수+switch_duty_controller+top |
| rtl06_timer_led | 5 | 1 | tick_chk N=1~8, sync_chk(switch_sync Reset·지연) |
| rtl07_buzzer_melody | 18 | 4 | melody_chk 7구성+tone_chk 단독+top 반주기 간격 |
| rtl08_hc595_7seg | 2 | 2 | (3d259fa) 데이터·래치·주기·2회 전송 |
| rtl09_max7219 | 2 | 2 | (3d259fa) 데이터·cs_n·주기·2회 전송 |
| rtl12_dht22_alarm | 2 | 0 | 경계 299/300/301, 799/800/801, 음수, 파라미터 2종, 모델 |
| rtl14_i2c_scan | 7 | 0 | 스코어보드 12스캔(ack 규칙 5종, 무작위 지연, start 반복, 중간 Reset) |
| rtl16_lcd1602 | 3 | 0 | 모델(핸드셰이크, start 무시, Reset) |
| rtl18_spi_request | 3 | 0 | 모델 |
| rtl19_sd_logger | 1 | 0 | 모델(overflow, 완료+캡처 동시) |
| rtl21_imu_level | 2 | 0 | ±5000 경계+LIMIT 0/1/32767 인스턴스 |
| rtl22_dc_motor | 1 | 0 | 모델(충돌 정지, pwm_gate) |
| rtl23_servo_pulse | 5 | 2 | 각도 0..255 전수 |
| rtl26_bt_command | 3 | 0 | 명령 바이트와 이웃 바이트 |
| rtl27_rs485_packet | 3 | 0 | 모델(DE 시점, 순서) |
| rtl29_integration | 4 | 0 | 480/20 경계, valid 독립 |
| rtl30_event_fifo | 2 | 0 | 2칸 큐 모델(full/empty, 동시 push/pop, Reset) |
| rtl31_register_map | 2 | 0 | 쓰기 주소 0..3, 읽기 주소 0..3, Reset |
| rtl32_capstone_control | 7 | 0 | 20/480 경계, valid 유지, log_event |

(RTL08·09 의 1차 생존은 3d259fa 보강 뒤의 값이다. 보강 전 값은 CLAUDE_HANDOFF.md 이력 참조.)

## 3. 최종 생존 20건 — 동치 판정과 근거
| 변형(과제 파일:줄:열 원본->변형) | 근거 |
|---|---|
| rtl01_button_led button_event.v:28:26 1'b0->1'b1 | Reset 직후 sync 레지스터는 대기(비눌림) 값이라 첫 클럭에 pressed==stable → count 가 0 으로 덮임. Reset 때 count 값은 포트로 관측 불가 |
| rtl01_button_led button_event.v:19:12 -->+ | 카운터 폭을 +2 로 넓힌 변형: 값이 항상 2^CW 미만이라 상위 비트는 0, 비교·리셋(0 확장) 결과 동일(폭 확장 동치) |
| rtl02_rgb_buttons button_level.v:23:24 1'b0->1'b1 | 위와 동일(sync1/sync2 Reset 값이 대기 상태라 첫 클럭에 value==pressed → cnt=0 으로 덮임) |
| rtl02_rgb_buttons button_level.v:17:12 -->+ | 카운터 폭을 +2 로 넓힌 변형: 값이 항상 2^CW 미만이라 상위 비트는 0, 비교·리셋(0 확장) 결과 동일(폭 확장 동치) |
| rtl04_pwm_led pwm_core.v:29:38 >->>= | duty_in==256 에서 '>=' 는 256 을 고르고 이는 duty_in 과 같다 → 함수 동일 |
| rtl04_pwm_led pwm_core.v:29:40 9'd256->9'd257 | duty_in==257 이 그대로 duty_reg=257 이 되지만 phase 는 최대 255 라 (phase<257)=항상 1 = 256 과 같다. 출력 포트는 pwm_out 뿐(duty_reg 는 내부) |
| rtl04_pwm_led pwm_core.v:29:50 9'd256->9'd257 | 클램프 값 257 도 위와 같은 이유로 pwm_out 동일 |
| rtl04_pwm_led pwm_core.v:12:15 -->+ | 카운터 폭을 +2 로 넓힌 변형: 값이 항상 2^CW 미만이라 상위 비트는 0, 비교·리셋(0 확장) 결과 동일(폭 확장 동치) |
| rtl04_pwm_led pwm_core.v:29:40 9'd256->9'd255 | 비교 상수 255: duty_in>255 면 256 선택 → duty_in==256 은 256, 그 외 동일 → 함수 동일 |
| rtl06_timer_led tick_generator.v:12:12 -->+ | 카운터 폭을 +2 로 넓힌 변형: 값이 항상 2^CW 미만이라 상위 비트는 0, 비교·리셋(0 확장) 결과 동일(폭 확장 동치) |
| rtl07_buzzer_melody melody_sequencer.v:14:41 >->>= | NOTE_CLOCKS=1 일 때 CW=0 → reg [-1:0](2비트, 합법). elapsed 는 항상 0 이고 elapsed==NOTE_CLOCKS-1(=0) 이 매 클럭 참이라 동작 동일. 2 이상에서는 식 자체가 같다 |
| rtl07_buzzer_melody melody_sequencer.v:15:12 -->+ | 카운터 폭을 +2 로 넓힌 변형: 값이 항상 2^CW 미만이라 상위 비트는 0, 비교·리셋(0 확장) 결과 동일(폭 확장 동치) |
| rtl07_buzzer_melody tone_generator.v:11:30 -->+ | 카운터 폭을 +2 로 넓힌 변형: 값이 항상 2^CW 미만이라 상위 비트는 0, 비교·리셋(0 확장) 결과 동일(폭 확장 동치) |
| rtl07_buzzer_melody tone_generator.v:14:23 -->+ | 카운터 폭을 +2 로 넓힌 변형: 값이 항상 2^CW 미만이라 상위 비트는 0, 비교·리셋(0 확장) 결과 동일(폭 확장 동치) |
| rtl08_hc595_7seg hc595_7seg.v:9:79 1'b0->1'b1 | 채워 넣는 비트는 8번 시프트 뒤에야 bits[7] 에 도달하는데 전송은 그 전에 끝나고 다음 전송은 bits 를 재적재 → 포트 불변 |
| rtl08_hc595_7seg hc595_7seg.v:9:89 +->- | n 은 4비트: k번 시프트 뒤 n=16-k, n==8 은 k=8 에서 참 → 종료 시점 동일(모듈로 대칭의 우연; 전송 길이가 바뀌면 동치 아님) |
| rtl09_max7219 max7219_tx.v:9:63 1'b0->1'b1 | 위와 동일(16번 시프트) |
| rtl09_max7219 max7219_tx.v:9:73 +->- | n 은 5비트: n=32-k, n==16 은 k=16 에서 참 → 동일(우연; 전송 길이 변경 시 동치 아님) |
| rtl23_servo_pulse servo_target.v:9:34 >->>= | angle==180 에서 '>=' 는 180 을 고르고 이는 angle 과 같다 → 함수 동일 |
| rtl23_servo_pulse servo_target.v:9:35 8'd180->8'd179 | 비교 상수 179: angle>179 면 180 → angle==180 은 180, 그 외 동일 → 함수 동일 |

## 4. ChatGPT 4-a 표와의 대조 (독립 재판정)
| ChatGPT 판정 | 이번 판정 | 근거 |
|---|---|---|
| RTL01·04·06·07 `CW=(X > 1)?…:1` 의 `>`→`>=` 는 동치(「X=1 에서 둘 다 1비트」) | **RTL01·02·04·06 은 동치 아님**, RTL07 만 동치 | `>=` 이면 X=1 에서 `$clog2(1)=0` → CW=0 → `{CW{1'b0}}` 가 길이 0 복제라 컴파일 오류. X=1 인스턴스를 만들자 4개 모두 「컴파일 거부」로 검출(런 41195172195 이후). RTL07 은 `reg [-1:0]` 가 합법이고 NOTE_CLOCKS=1 에서 elapsed==0 이 항상 참이라 동작이 같다 |
| RTL04 `duty_in > 9'd256` 의 `>`→`>=` 동치 | 동의 | duty_in==256 이면 256 선택 = 256 |
| RTL04 `9'd256→9'd257` 는 「비동치 후보」 | **동치**(pwm_out 기준) | phase 최대 255 라 (phase<257)·(phase<256) 모두 항상 참; duty_reg 는 내부 신호 |
| RTL04 `8'hFF→8'h100` 비동치(폭 잘림) | 동의, 검출됨 | 8비트 phase 와 비교해 영원히 거짓 → duty 갱신 안 됨 |
| RTL01·02 reset 의 `1'b0↔1'b1` 대개 비동치 후보 | **혼합**: sync 레지스터 Reset 값 변형은 비동치(검출됨), **count/cnt Reset 값 변형은 동치** | Reset 직후 sync 가 대기 상태라 첫 클럭에 count 가 0 으로 덮임 |
| RTL08·09 마지막 시프트 채움 `0→1` 동치 후보 | 동의 + 추가 발견: `n+1→n-1` 도 동치 | n 이 4/5비트라 16-k / 32-k 로 내려가 k=8/16 에서 종료 조건이 같아짐(우연) |
| RTL07 `2'd1→2'd2` 비동치 | 동의, 검출됨 | 음 순서 변경 |
| (미기재) RTL23 `angle>8'd180` 의 `>=`, 비교 상수 179 | 동치 | angle==180 에서 결과 180 |
