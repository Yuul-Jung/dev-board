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

## 필수 안전 확인
- 비동기 외부 입력은 동기화 후 사용. 동기화 플립플롭 사이의 타이밍 예외는 실제 제약에 근거할 때만 적용.
- 외부 5V 출력은 Basys3 GPIO로 직접 연결하지 말고 레벨 시프터 또는 분압 검토.
- Basys3 보드 LED/버튼의 실제 XDC 핀명은 Digilent 보드 마스터 XDC 및 실제 사용 포트와 일치시켜야 한다. 이 폴더의 01과제는 보드 핀 선택 이전의 범용 RTL이다.
