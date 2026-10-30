# Arduino 전용 학습 과정

[저장소 홈](../README.md) · [32차시 테크트리](ROADMAP.md) · [기존 자료 대응표](CROSSWALK.md) · [첫 실습](examples/01_led_button/01_led_button.ino)

## 목적
Arduino UNO R4를 기준으로 GPIO → ADC/PWM → 통신 → 센서 → 구동기 → 통합 프로젝트를 단계적으로 학습한다. Arduino 프로그래밍은 Arduino Sketch(C++ 기반)와 표준 라이브러리 중심으로 구성한다. Basys3 + MicroBlaze V의 RTL/AXI 설계 과정과는 **별개의 구현 트랙**이다.

## 디렉터리 역할
- `arduino/`: Arduino 전용 실습 순서·Sketch·테스트·배선 설명
- [`devices/`](../devices/README.md): 공통 센서·모듈의 기술자료와 근거/한계. 이곳의 MicroBlaze V 드라이버를 Arduino용으로 오인하지 않는다.
- [`soc-map/`](../soc-map/README.md): 동일 부품의 FPGA/SoC 구현 경로

## 01~08차시 실습 바로가기
| 차시 | 실습 | 문서·코드 |
|---|---|---|
| 01 | LED와 버튼 | [열기](examples/01_led_button/README.md) |
| 02 | 버튼 디바운스·토글 | [열기](examples/02_button_toggle/README.md) |
| 03 | 버튼 2개와 RGB LED | [열기](examples/03_rgb_buttons/README.md) |
| 04 | Serial 명령 LED | [열기](examples/04_serial_led/README.md) |
| 05 | 가변저항 ADC | [열기](examples/05_pot_adc/README.md) |
| 06 | 가변저항 PWM | [열기](examples/06_pot_pwm/README.md) |
| 07 | CdS 자동 조명 | [열기](examples/07_cds_auto_light/README.md) |
| 08 | 조이스틱 RGB | [열기](examples/08_joystick_rgb/README.md) |

## 진행 현황
| 산출물 | 상태 |
|---|---|
| 32차시 테크트리 | 교육과정 설계안 |
| 기존 31장 교재/장치 자료 대응표 | 목차 기준 연결 |
| 01~08차시 실습 문서 및 Sketch | 소스 작성, 컴파일·실물 확인 전 |
| 나머지 차시 Sketch | 미작성 |
| Arduino UNO R4 보드 검증 | 미실시 |

## 공통 규칙
1. 1차시는 센서 또는 입력 1개 + 출력 1개부터 시작하고, 이후 입력·출력을 누적한다.
2. 전압·공통 GND·전류·핀 배치는 **실제 보드와 브레이크아웃 회로도** 기준으로 확인한다. FPGA의 3.3V Pmod 규칙을 Arduino에 그대로 전용하지 않는다.
3. 한 차시는 **개요 → 배선 → 전체 코드 → 실행 → 확인 결과 → 다음 확장** 순서로 작성한다.
4. 단계별로 `설계안 / 코드 작성 / 컴파일 확인 / 실물 확인`을 구분한다. 실행하지 않은 코드를 완료로 표시하지 않는다.
5. 기존 데이터시트·드라이버 문서를 복제하지 않고 상대 링크로 연결한다.

> 참고: `devices/BOOK-MAP.md`는 '상상을 스케치하는 아두이노 우노 R4' **31장 목차에 대한 SoC 대응표**이다. 이 디렉터리의 32차시는 별도로 설계한 Arduino 실습 교육과정이며 1:1 교재 복제가 아니다.
