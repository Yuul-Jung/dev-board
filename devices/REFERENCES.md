# GitHub 참고 자료 대장

<!-- nav -->[저장소 홈](../README.md) › [devices](README.md)

프로젝트 문서(`arduino_io_module_tech_tree_20261030_github_updated.md`) §5·§9 의 GitHub 참고 자료를 **차시와 연결한 대장**입니다.

**근거 수준(중요)**: 이 표의 저장소 이름·역할·라이선스는 모두 프로젝트 문서에서 옮긴 것입니다(D3, 문서 경유). **이 저장소 작업에서 각 GitHub 원문을 직접 열어 확인하지 않았습니다 → 미확인.** 문서가 라이선스를 적은 3곳만 적었고 나머지는 「미기록」입니다. `commit`(재현 기준 커밋) 열이 아직 없는 것은 원문을 열지 않아 고정할 수 없기 때문입니다. 사용 전에 각 저장소의 해당 버전 라이선스와 커밋을 확인해 이 표에 적어야 합니다.

GitHub 의 Arduino C/C++ 라이브러리는 FPGA 에 합성되는 RTL 이 아닙니다. 통신 순서와 레지스터 동작을 참고하는 자료이며, 목록에 있다는 것이 Basys3·Vivado 2024.2 에서 동작한다는 뜻은 아닙니다. 등급 A/B 는 열람 우선순위일 뿐 검증 완료가 아닙니다.

| 구분 | 저장소 | 역할 | 연결 차시 | 라이선스 | commit |
|---|---|---|---|---|---|
| A-01 | [Digilent/Basys-3](https://github.com/Digilent/Basys-3) | 보드 공식 데모(GPIO·XADC) | 01–06, 12 | 미기록 | 미기록 |
| A-02 | [Digilent/vivado-library](https://github.com/Digilent/vivado-library) | Vivado Pmod IP | 19–23, 29–32 | 미기록 | 미기록 |
| A-03 | [suoglu/HC-SR04](https://github.com/suoglu/HC-SR04) | HC-SR04 Basys3 RTL | 08 | 미기록 | 미기록 |
| A-04 | [ZipCPU/wbuart32](https://github.com/ZipCPU/wbuart32) | UART RTL·검증(비교용) | 14–16, 30 | 미기록 | 미기록 |
| A-05 | [nandland/spi-master](https://github.com/nandland/spi-master) | SPI Master RTL | 24–28 | MIT(문서 기재) | 미기록 |
| A-06 | [adafruit/Adafruit_BusIO](https://github.com/adafruit/Adafruit_BusIO) | I²C·SPI 버스 추상화 | 19–28 | MIT(문서 기재) | 미기록 |
| A-07 | [adafruit/Adafruit_Sensor](https://github.com/adafruit/Adafruit_Sensor) | 센서 공통 데이터형 | 20–23, 32 | 미기록 | 미기록 |
| A-08 | [Xilinx/embeddedsw](https://github.com/Xilinx/embeddedsw) | AMD 공식 C 드라이버(gpio·uartlite·iic·spi) | 29–32 | 미기록 | 미기록 |
| B-09 | [sparkfun/SparkFun_BME280_Arduino_Library](https://github.com/sparkfun/SparkFun_BME280_Arduino_Library) | I²C 센서 드라이버 | 20–23 | 미기록 | 미기록 |
| B-10 | [adafruit/Adafruit_CircuitPython_Bundle](https://github.com/adafruit/Adafruit_CircuitPython_Bundle) | 센서 드라이버 색인 | 카탈로그 보강 | 미기록 | 미기록 |
| B-11 | [fpganinja/taxi](https://github.com/fpganinja/taxi) | SystemVerilog IP(I²C·UART·AXI-Lite) | 19, 30–32 | CERN-OHL-S-2.0 또는 상용(문서 기재) | 미기록 |
| B-12 | [sparkfun/Qwiic_Scale](https://github.com/sparkfun/Qwiic_Scale) | 모듈 회로도(Eagle) | 20–23 | 미기록 | 미기록 |
| 9.3 | [Digilent/Basys-3-HW](https://github.com/Digilent/Basys-3-HW) | Basys3 하드웨어 참고 | 01–06 | 미기록 | 미기록 |
| 9.3 | [Digilent/vivado-boards](https://github.com/Digilent/vivado-boards) | Basys3 보드 정의 | 환경 | 미기록 | 미기록 |
| 9.3 | [janschiefer/verilog_spi](https://github.com/janschiefer/verilog_spi) | SPI 4모드 Master/Slave | 24–26 | 미기록 | 미기록 |
| 9.3 | [hdl-util/i2c](https://github.com/hdl-util/i2c) | I²C Master/Slave RTL | 19 | 미기록 | 미기록 |
| 9.3 | [alexforencich/cocotbext-i2c](https://github.com/alexforencich/cocotbext-i2c) | I²C 검증 모델(BFM) | 19–20 | 미기록 | 미기록 |
| 9.3 | [alexforencich/verilog-i2c](https://github.com/alexforencich/verilog-i2c) | 구형 I²C RTL(유지보수 종료 명시) | 참고만 | 미기록 | 미기록 |
| 9.3 | [pluttan/ws2812b-fpga](https://github.com/pluttan/ws2812b-fpga) | WS2812 타이밍 RTL | C03 | 미기록 | 미기록 |
| 9.3 | [sparkfun/SparkFun_INA2XX_Arduino_Library](https://github.com/sparkfun/SparkFun_INA2XX_Arduino_Library) | INA228/237(INA219 아님) | I11 참고 | 미기록 | 미기록 |
| 9.3 | [miguelbalboa/rfid](https://github.com/miguelbalboa/rfid) | RC522 라이브러리(유지보수 동결에 가까움) | 25 | 미기록 | 미기록 |
| 9.3 | [nRF24/RF24](https://github.com/nRF24/RF24) | nRF24L01 라이브러리 | 27 | 미기록 | 미기록 |
| 9.3 | [arduino-libraries/Ethernet](https://github.com/arduino-libraries/Ethernet) | W5x00 소켓 API | 28 | 미기록 | 미기록 |
| 5 | adafruit/RTClib, DHT-sensor-library, Adafruit_NeoPixel, Adafruit_BME280_Library, Adafruit_MPU6050, Adafruit_SSD1306, Adafruit-PWM-Servo-Driver-Library, Adafruit_ADS1X15, Adafruit_MCP3008 | §5 의 장치별 참고 라이브러리 | 21–23, 26 등 | 미기록 | 미기록 |

## 사용 주의 (프로젝트 문서 §12 기재)

- `suoglu/HC-SR04`: README 에 measure 신호를 계속 High 로 두면 문제가 있다고 명시. 이 저장소 `gpio/rtl/` 의 HC-SR04 래퍼와 비교할 때 1클럭 시작 펄스로 검증.
- `alexforencich/verilog-i2c`: 유지보수 종료·Taxi 이관 명시 → 신규 표준 IP 로 쓰지 않음.
- `hdl-util/i2c`: README 의 TODO·미시험 항목 확인 필요.
- `Digilent/vivado-library`: 과거 Pmod IP 가 MicroBlaze V + 2024.2 에서 재현된다는 뜻이 아님.
- `Xilinx/embeddedsw`: `master` 는 2024.2 이후 수정을 포함할 수 있어 설치된 2024.2 와 호환되는 기준으로 판단.

<!-- nav-foot -->← [devices 목차](README.md)
