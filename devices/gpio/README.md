# GPIO 소자와 PWM 부하

<!-- nav -->[저장소 홈](../../README.md) › [devices](../README.md)

단순 입력·출력은 AXI GPIO(PG144) 경로, 독자 프로토콜·타이밍이 필요한 소자는 전용 RTL, PWM·캡처는 AXI Timer(PG079) 경로입니다. 구분 표기는 [../README.md](../README.md).

| 소자 | 문서 | 구현 |
|---|---|---|
| HC-SR04 초음파 | [hc-sr04.md](hc-sr04.md) | 전용 RTL ([rtl/README.md](rtl/README.md)), [sw/ultrasonic.c](sw/ultrasonic.c) |
| DHT22 | [dht22.md](dht22.md) | 전용 RTL ([rtl/README.md](rtl/README.md)), [sw/dht22.c](sw/dht22.c) |
| HC-SR501 PIR | [hc-sr501.md](hc-sr501.md) | AXI GPIO 입력 |
| 릴레이 SRD-05VDC | [relay-srd05.md](relay-srd05.md) | AXI GPIO 출력 |
| 로터리 엔코더 | [rotary-encoder.md](rotary-encoder.md) | 전용 RTL `quad_*.v`, [sw/quad.h](sw/quad.h) |
| TTP223 터치 | [ttp223.md](ttp223.md) | AXI GPIO 입력 |
| 4선 PWM 팬 | [fan-pwm.md](fan-pwm.md) | AXI Timer PWM [sw/axi_timer_pwm.c](sw/axi_timer_pwm.c) + 캡처 [sw/axi_timer_tach.c](sw/axi_timer_tach.c) |
| 부저 | [buzzer.md](buzzer.md) | AXI Timer PWM (사양 미확인) |
| RTTTL 플레이어 | [rtttl.md](rtttl.md) | 부저 PWM 위 해석기+비차단 재생 (명세 2차 자료, 충돌 3건) |
| LED·버튼·RGB·비차단 타이머 | [basic-io.md](basic-io.md) | AXI GPIO [sw/axi_gpio.c](sw/axi_gpio.c), [sw/basic_io.c](sw/basic_io.c) (모델 시험) |
| LM35·CdS·가변저항·조이스틱 | [analog-sensors.md](analog-sensors.md) | 변환 함수 [sw/analog_sensors.c](sw/analog_sensors.c) (ADC 는 I2C/SPI 쪽 드라이버) |

모터·서보·스테퍼는 [../motor/](../motor/README.md). 호스트 테스트: [tb/](tb/) (`../run_host_tests.sh`).

<!-- nav-foot -->[devices 목차](../README.md) · [1-wire](../1-wire/README.md) →

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 하위 폴더

| 폴더 | 내용 |
|---|---|
| [ref/](ref/README.md) | 근거 자료와 사실표(교차 검증 대상) |
| [rtl/](rtl/README.md) | Verilog RTL 과 RTL 테스트벤치(시뮬레이터 없음, 미실행) |
| [sw/](sw/README.md) | 드라이버 C 소스 |
| [tb/](tb/README.md) | 호스트 테스트와 레지스터/장치 모델(mock) |

### 파일

| 파일 | 설명 |
|---|---|
| [analog-sensors.md](analog-sensors.md) | 아날로그 센서 변환: LM35, CdS, 가변저항, 조이스틱 |
| [basic-io.md](basic-io.md) | 기본 입출력: LED, 버튼, RGB, 비차단 타이머 |
| [buzzer.md](buzzer.md) | 부저 (액티브 / 패시브) |
| [dht22.md](dht22.md) | DHT22 (AM2302) 온습도 센서 |
| [fan-pwm.md](fan-pwm.md) | 4선 PWM 팬 (Intel 4-Wire PWM Controlled Fans 규격) |
| [hc-sr04.md](hc-sr04.md) | HC-SR04 초음파 거리 센서 |
| [hc-sr501.md](hc-sr501.md) | HC-SR501 PIR 모션 모듈 |
| [relay-srd05.md](relay-srd05.md) | 릴레이 (Songle SRD-05VDC-SL-C 기준) |
| [rotary-encoder.md](rotary-encoder.md) | 로터리 엔코더 (A/B 직교 + 푸시 스위치) |
| [rtttl.md](rtttl.md) | RTTTL 플레이어 (책 17장) |
| [ttp223.md](ttp223.md) | TTP223 정전식 터치 센서 (1채널) |

<!-- index:end -->
