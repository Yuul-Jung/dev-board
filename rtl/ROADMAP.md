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
| 10 부저 | 주파수 Divider + 음계 Sequencer | PWM/Timer 재사용 |
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
