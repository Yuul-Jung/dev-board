# Basys3 순수 RTL 학습 트랙

[저장소 홈](../README.md) · [Arduino 대응](../arduino/ROADMAP.md) · [RTL 학습 순서](ROADMAP.md)

## 원칙
- Arduino 실습을 **그대로 C→Verilog 문법 변환하지 않는다**. 같은 입출력 동작을 조합논리·레지스터·클럭·FSM 관점에서 새로 설계한다.
- Basys3의 100 MHz 클럭(10 ns)을 기준으로 하되 Clock/Reset/Pin/XDC 설정은 실제 프로젝트에서 확인한다.
- Top은 Shell/Wrapper: 핀·wire·인스턴스 연결만. 기능 제어는 Controller, 타이밍은 재사용 IP로 분리한다.
- 기존 [devices](../devices/README.md)의 AXI4-Lite RTL 및 [SoC 맵](../soc-map/README.md)을 복사하지 않는다. UART/I2C/SPI는 순수 RTL 학습과 AMD AXI IP 응용을 구분한다.
- 소스 작성, RTL 시뮬레이션, 합성, Timing, Basys3 실물 검증은 각각 다른 완료 상태다.

## 파일 구조
| 경로 | 역할 | 상태 |
|---|---|---|
| [ROADMAP.md](ROADMAP.md) | Arduino 32차시 ↔ RTL 구현 대응 | 설계안 |
| [rtl01_button_led/](rtl01_button_led/README.md) | 01~02차시 대응: 동기화·디바운스·버튼 토글 | RTL 및 Testbench 작성, 실행 전 |
| [rtl02_rgb_buttons/](rtl02_rgb_buttons/README.md) | Arduino 03차시 대응: 두 버튼과 RGB LED 조합회로 | RTL 및 Testbench 작성, 실행 전 |
| [rtl03_uart_led/](rtl03_uart_led/README.md) | Arduino 04차시 대응: UART Byte Stream 명령 LED | Controller·Testbench 작성, UART 코어 연동 전 |
| [rtl04_pwm_led/](rtl04_pwm_led/README.md) | Arduino 06차시 대응: 스위치 기반 PWM LED | RTL·Testbench 작성, 시뮬레이션 전 |
| [rtl05_xadc_pwm/](rtl05_xadc_pwm/README.md) | ADC 샘플을 기존 PWM에 연결 | Controller·Testbench 작성, XADC 실제 IP 연결 전 |
| [rtl06_timer_led/](rtl06_timer_led/README.md) | Arduino 09차시 대응: 0.5초 Tick과 LED 점멸 | RTL·Testbench 작성, 시뮬레이션 전 |
| [rtl07_buzzer_melody/](rtl07_buzzer_melody/README.md) | Arduino 10차시 대응: 3음 멜로디와 주파수 발생기 | RTL·Testbench 작성, 시뮬레이션 전 |


## RTL08~RTL32 추가 개발 현황

| RTL | 주제 | 상태 |
|---|---|---|
| [RTL08](rtl08_hc595_7seg/README.md) | 74HC595 직렬 세그먼트 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL09](rtl09_max7219/README.md) | MAX7219 전송 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL10](rtl10_pir_event/README.md) | PIR 이벤트 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL11](rtl11_ultrasonic_event/README.md) | 초음파 거리 경보 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL12](rtl12_dht22_alarm/README.md) | DHT22 데이터 경보 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL13](rtl13_ds18b20_alarm/README.md) | 온도·거리 경보 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL14](rtl14_i2c_scan/README.md) | I2C 스캐너 요청 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL15](rtl15_bh1750/README.md) | BH1750 조도 경보 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL16](rtl16_lcd1602/README.md) | LCD 문자열 전송 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL17](rtl17_oled_page/README.md) | OLED 페이지 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL18](rtl18_spi_request/README.md) | SPI 트랜잭션 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL19](rtl19_sd_logger/README.md) | microSD 로깅 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL20](rtl20_rtc_timestamp/README.md) | RTC 시간 스냅샷 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL21](rtl21_imu_level/README.md) | IMU 기울기 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL22](rtl22_dc_motor/README.md) | DC 모터 제어 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL23](rtl23_servo_pulse/README.md) | 서보 목표 펄스 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL24](rtl24_stepper_phase/README.md) | 스테퍼 패턴 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL25](rtl25_distance_motor/README.md) | 거리 기반 모터 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL26](rtl26_bt_command/README.md) | Bluetooth UART | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL27](rtl27_rs485_packet/README.md) | RS485 방향 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL28](rtl28_network_sample/README.md) | 네트워크 데이터 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL29](rtl29_integration/README.md) | 센서 통합 경보 | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL30](rtl30_event_fifo/README.md) | 이벤트 FIFO | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL31](rtl31_register_map/README.md) | Register Map | 기능 RTL 초안, 테스트벤치 강화·실행 전 |
| [RTL32](rtl32_capstone_control/README.md) | 캡스톤 Controller | 기능 RTL 초안, 테스트벤치 강화·실행 전 |

## 적대적 검증
- [2026-10-31 적대적 검증 보고서](ADVERSARIAL_REVIEW_20261031.md): 우선순위별 오류·수정·통합 미완료 항목.
- [검증 상태 대장](VERIFICATION_STATUS.md): 32개 소스 등록과 실제 시뮬레이션/하드웨어 검증을 구분.

## 필수 안전 확인
- 비동기 외부 입력은 동기화 후 사용. 동기화 플립플롭 사이의 타이밍 예외는 실제 제약에 근거할 때만 적용.
- 외부 5V 출력은 Basys3 GPIO로 직접 연결하지 말고 레벨 시프터 또는 분압 검토.
- Basys3 보드 LED/버튼의 실제 XDC 핀명은 Digilent 보드 마스터 XDC 및 실제 사용 포트와 일치시켜야 한다. 이 폴더의 01과제는 보드 핀 선택 이전의 범용 RTL이다.

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 하위 폴더

| 폴더 | 내용 |
|---|---|
| [rtl01_button_led/](rtl01_button_led/README.md) | RTL01 — 버튼 입력·디바운스·LED 토글 |
| [rtl02_rgb_buttons/](rtl02_rgb_buttons/README.md) | RTL02 — 버튼 2개로 RGB LED 제어 |
| [rtl03_uart_led/](rtl03_uart_led/README.md) | RTL03 — UART 바이트 명령으로 LED 제어 |
| [rtl04_pwm_led/](rtl04_pwm_led/README.md) | RTL04 — PWM Counter·Comparator·Duty Register |
| [rtl05_xadc_pwm/](rtl05_xadc_pwm/README.md) | RTL05 — XADC 샘플 → PWM 듀티 Controller |
| [rtl06_timer_led/](rtl06_timer_led/README.md) | RTL06 — Timer Tick 기반 LED 점멸 |
| [rtl07_buzzer_melody/](rtl07_buzzer_melody/README.md) | RTL07 — 부저 주파수 발생기 + 3음 멜로디 |
| [rtl08_hc595_7seg/](rtl08_hc595_7seg/README.md) | RTL08 — 74HC595 7세그 직렬 출력 |
| [rtl09_max7219/](rtl09_max7219/README.md) | RTL09 — MAX7219 16비트 워드 전송 |
| [rtl10_pir_event/](rtl10_pir_event/README.md) | RTL10 — PIR 입력 이벤트 검출 |
| [rtl11_ultrasonic_event/](rtl11_ultrasonic_event/README.md) | RTL11 — 초음파 거리 valid 기반 경보 |
| [rtl12_dht22_alarm/](rtl12_dht22_alarm/README.md) | RTL12 — DHT22 온습도 경고 |
| [rtl13_ds18b20_alarm/](rtl13_ds18b20_alarm/README.md) | RTL13 — 온도·거리 복합 경보 |
| [rtl14_i2c_scan/](rtl14_i2c_scan/README.md) | RTL14 — I2C 주소 탐색 요청 컨트롤러 |
| [rtl15_bh1750/](rtl15_bh1750/README.md) | RTL15 — BH1750 lux 비교 제어 |
| [rtl16_lcd1602/](rtl16_lcd1602/README.md) | RTL16 — LCD 문자 전송용 바이트 스트림 |
| [rtl17_oled_page/](rtl17_oled_page/README.md) | RTL17 — OLED 페이지 선택 |
| [rtl18_spi_request/](rtl18_spi_request/README.md) | RTL18 — SPI 트랜잭션 요청 |
| [rtl19_sd_logger/](rtl19_sd_logger/README.md) | RTL19 — microSD 샘플 기록 요청 |
| [rtl20_rtc_timestamp/](rtl20_rtc_timestamp/README.md) | RTL20 — RTC 시각 스냅샷 |
| [rtl21_imu_level/](rtl21_imu_level/README.md) | RTL21 — MPU6050 기울기 경고 |
| [rtl22_dc_motor/](rtl22_dc_motor/README.md) | RTL22 — DC 모터 방향·PWM 구동 |
| [rtl23_servo_pulse/](rtl23_servo_pulse/README.md) | RTL23 — 서보 PWM 목표각 |
| [rtl24_stepper_phase/](rtl24_stepper_phase/README.md) | RTL24 — 스테퍼 하프스텝 패턴 |
| [rtl25_distance_motor/](rtl25_distance_motor/README.md) | RTL25 — 거리 기반 모터 안전 정지 |
| [rtl26_bt_command/](rtl26_bt_command/README.md) | RTL26 — Bluetooth UART 바이트 명령 |
| [rtl27_rs485_packet/](rtl27_rs485_packet/README.md) | RTL27 — RS485 송신 방향 제어 |
| [rtl28_network_sample/](rtl28_network_sample/README.md) | RTL28 — 네트워크 샘플 큐 전단 |
| [rtl29_integration/](rtl29_integration/README.md) | RTL29 — 복합 센서 경보 상태 |
| [rtl30_event_fifo/](rtl30_event_fifo/README.md) | RTL30 — 이벤트 FIFO 버퍼 |
| [rtl31_register_map/](rtl31_register_map/README.md) | RTL31 — 간단한 Register Map |
| [rtl32_capstone_control/](rtl32_capstone_control/README.md) | RTL32 — 캡스톤 모듈 통합 제어 |
| [verification/](verification/README.md) | RTL/UVM 독립 검증 |

### 파일

| 파일 | 설명 |
|---|---|
| [ADVERSARIAL_REVIEW_20261031.md](ADVERSARIAL_REVIEW_20261031.md) | RTL 32개 과제 적대적 검증 — 2026-10-31 |
| [ROADMAP.md](ROADMAP.md) | Arduino 실습을 RTL로 재설계하는 32차시 대응 계획 |
| [VERIFICATION_STATUS.md](VERIFICATION_STATUS.md) | RTL 32개 과제 검증 상태 |
| [run_rtl_tests.sh](run_rtl_tests.sh) | Reused RTL components are external to the current lesson. |

<!-- index:end -->
