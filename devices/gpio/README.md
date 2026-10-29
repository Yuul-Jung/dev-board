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

모터·서보·스테퍼는 [../motor/](../motor/README.md). 호스트 테스트: [tb/](tb/) (`../run_host_tests.sh`).

<!-- nav-foot -->[devices 목차](../README.md) · [1-wire](../1-wire/README.md) →
