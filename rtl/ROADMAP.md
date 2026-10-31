# Arduino 실습을 RTL로 재설계하는 32차시 대응 계획

[RTL 홈](README.md) · [Arduino 원본 테크트리](../arduino/ROADMAP.md) · [공통 장치](../devices/README.md)

| Arduino | RTL 설계 대응 | 구현 전략 |
|---|---|---|
| 01~02 버튼 LED | 2FF 동기화 + 디바운스 Counter + LED 토글 Register | **[rtl01](rtl01_button_led/README.md) 소스 작성** |
| 03 RGB LED | 복수 버튼 입력 필터 + RGB 출력 Controller | **[rtl02](rtl02_rgb_buttons/README.md) 소스 작성** |
| 04 Serial LED | UART RX + 명령 디코더 + 상태 Register | **[rtl03](rtl03_uart_led/README.md) Controller 작성**, UART 코어 연동 전 |
| 05 ADC | XADC 출력값 수신 후 Duty Register/표시 | **[rtl05](rtl05_xadc_pwm/README.md) 샘플 Controller 작성**, AMD XADC IP 연결 전 |
| 06~08 PWM·CdS·조이스틱 | PWM Counter + Duty Register + XADC 샘플 | **[rtl04](rtl04_pwm_led/README.md) PWM + [rtl05](rtl05_xadc_pwm/README.md) 샘플 Controller 작성**, 센서·XADC 연결 전 |
| 09 비차단 타이머 | Counter + Tick pulse + LED Register | **[rtl06](rtl06_timer_led/README.md) RTL·TB 작성**, 시뮬레이션 전 |
| 10 부저 | 주파수 Divider + 음계 Sequencer | **[rtl07](rtl07_buzzer_melody/README.md) RTL·TB 작성**, RTL06 스위치 동기화 재사용 |
| 11~12 7세그·매트릭스 | 시프트 출력·스캔 Counter·Font ROM | 출력 Controller |
| 13 PIR | 입력 동기화 + 이벤트 검출 | GPIO |
| 14 초음파 | Trigger/echo 타이밍 FSM | [기존 코드](../devices/gpio/rtl/README.md) 연결 |
| 15 DHT22 | 단일선 타이밍 FSM | [기존 코드](../devices/gpio/rtl/README.md) 연결 |
| 16 DS18B20 통합 | 1-Wire Controller + 측정 상태 통합 | [기존 1-Wire](../devices/1-wire/README.md) 재사용 |
| 17~20 I2C·LCD·OLED | AXI IIC + Sensor/Display Controller | 별도 독립 I2C 실습 후 통합 |
| 21~24 SPI·SD·RTC·IMU | AXI Quad SPI / IIC + Controller | 인터페이스 IP 재사용 |
| 25~28 Motor | PWM/Direction/Stepper FSM | [기존 motor 자료](../devices/motor/README.md) 재사용 |
| 29~31 통신 | UART/CAN/네트워크 IP + 등록된 상태/응용 Controller | 실제 보드/주변장치 적합성 확인 |
| 32 복합 과제 | 통합 Controller + 표준 데이터 인터페이스 | 기능 모듈 재사용 |

각 RTL 과제는 `src/`·`tb/`·`README.md`로 구성한다. Vivado 프로젝트별로 독립 작성하며 기존 검증된 SoC 프로젝트는 변경하지 않는다.

## RTL 과제 번호 안내

Arduino 32차시와 RTL 32개 과제는 **서로 다른 차시 번호 체계**이다. 위의 대응표는 주제 매핑이며 동일 번호를 의미하지 않는다. RTL08~RTL32은 아래 구현 목록으로 별도 관리한다.

- [RTL08 — 74HC595 직렬 세그먼트](rtl08_hc595_7seg/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL09 — MAX7219 전송](rtl09_max7219/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL10 — PIR 이벤트](rtl10_pir_event/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL11 — 초음파 거리 경보](rtl11_ultrasonic_event/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL12 — DHT22 데이터 경보](rtl12_dht22_alarm/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL13 — 온도·거리 경보](rtl13_ds18b20_alarm/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL14 — I2C 스캐너 요청](rtl14_i2c_scan/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL15 — BH1750 조도 경보](rtl15_bh1750/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL16 — LCD 문자열 전송](rtl16_lcd1602/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL17 — OLED 페이지](rtl17_oled_page/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL18 — SPI 트랜잭션](rtl18_spi_request/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL19 — microSD 로깅](rtl19_sd_logger/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL20 — RTC 시간 스냅샷](rtl20_rtc_timestamp/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL21 — IMU 기울기](rtl21_imu_level/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL22 — DC 모터 제어](rtl22_dc_motor/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL23 — 서보 목표 펄스](rtl23_servo_pulse/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL24 — 스테퍼 패턴](rtl24_stepper_phase/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL25 — 거리 기반 모터](rtl25_distance_motor/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL26 — Bluetooth UART](rtl26_bt_command/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL27 — RS485 방향](rtl27_rs485_packet/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL28 — 네트워크 데이터](rtl28_network_sample/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL29 — 센서 통합 경보](rtl29_integration/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL30 — 이벤트 FIFO](rtl30_event_fifo/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL31 — Register Map](rtl31_register_map/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
- [RTL32 — 캡스톤 Controller](rtl32_capstone_control/README.md): RTL 초안 등록, 기능 Testbench 및 하드웨어 검증 전
