# sw

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [gpio](../README.md)

드라이버 C 소스

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [analog_sensors.c](analog_sensors.c) | endif |
| [analog_sensors.h](analog_sensors.h) | 아날로그 센서 변환 모음 (교재 8·16장 대응): LM35 온도, CdS(광저항) 분압, 가변저항·조이스틱. |
| [axi_gpio.c](axi_gpio.c) |  |
| [axi_gpio.h](axi_gpio.h) | AXI GPIO (PG144 v2.0) 드라이버 — 핀 단위 입출력, 최대 2채널(각 최대 32비트), 인터럽트 상태 확인·해제. |
| [axi_timer_pwm.c](axi_timer_pwm.c) |  |
| [axi_timer_pwm.h](axi_timer_pwm.h) | AXI Timer (PG079 v2.0) PWM — 팬(25 kHz)과 패시브 부저(주파수)에 사용. |
| [axi_timer_tach.c](axi_timer_tach.c) |  |
| [axi_timer_tach.h](axi_timer_tach.h) | AXI Timer (PG079 v2.0) 캡처 모드로 팬 타코 주기 측정. 별도 인스턴스의 타이머 0 사용. |
| [basic_io.c](basic_io.c) |  |
| [basic_io.h](basic_io.h) | 기본 입출력 예제 묶음 (교재 6·7·9·10장 대응): 비차단 타이머(millis 식), 버튼 디바운스·누른 횟수, RGB LED(PWM), 자유 실행 시계. |
| [dht22.c](dht22.c) |  |
| [dht22.h](dht22.h) | DHT22(AM2302) 드라이버 — `rtl/dht22_axil_*` 레지스터 위에서 동작. 비트 타이밍은 RTL 이 하고 이 드라이버는 결과 레지스터만 읽는다 |
| [quad.h](quad.h) | quad_axil 엔코더 레지스터 접근 (rtl/quad_axil_regs.v 와 같은 맵). |
| [rtttl.c](rtttl.c) | 옥타브 4 의 C..B, 1/64 Hz 단위 (440·2^((n-69)/12)·64 를 반올림; 계산) |
| [rtttl.h](rtttl.h) | RTTTL (Ring Tone Text Transfer Language) 해석기와 비차단 재생기 (교재 17장 대응). 소리는 AXI Timer PWM 의 buz |
| [ultrasonic.c](ultrasonic.c) |  |
| [ultrasonic.h](ultrasonic.h) | HC-SR04 거리계 드라이버 — `rtl/us_axil_*`(노션 원본 코어를 감싼 AXI4-Lite) 레지스터 위에서 동작. |

<!-- index:end -->
