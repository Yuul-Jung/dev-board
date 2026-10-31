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

## 필수 안전 확인
- 비동기 외부 입력은 동기화 후 사용. 동기화 플립플롭 사이의 타이밍 예외는 실제 제약에 근거할 때만 적용.
- 외부 5V 출력은 Basys3 GPIO로 직접 연결하지 말고 레벨 시프터 또는 분압 검토.
- Basys3 보드 LED/버튼의 실제 XDC 핀명은 Digilent 보드 마스터 XDC 및 실제 사용 포트와 일치시켜야 한다. 이 폴더의 01과제는 보드 핀 선택 이전의 범용 RTL이다.
