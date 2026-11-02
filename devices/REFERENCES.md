# GitHub 참고 자료 대장

<!-- nav -->[저장소 홈](../README.md) › [devices](README.md)

프로젝트 문서(`arduino_io_module_tech_tree_20261030_github_updated.md`) §5·§9 의 GitHub 참고 자료를 **차시와 연결한 대장**입니다.

**근거 수준(중요)**: **아래 기존 A/B/9.3 목록**의 저장소 이름·역할·라이선스는 프로젝트 문서에서 옮긴 것입니다(D3, 문서 경유). 그 목록은 각 GitHub 원문을 전수 확인한 결과가 아니므로 **미확인**입니다. 별도 날짜가 있는 아래 공식 자료·라이선스 확인 표는 예외입니다. 문서가 라이선스를 적은 3곳만 적었고 나머지는 「미기록」입니다. `commit`(재현 기준 커밋) 열이 아직 없는 것은 원문을 열지 않아 고정할 수 없기 때문입니다. 사용 전에 각 저장소의 해당 버전 라이선스와 커밋을 확인해 이 표에 적어야 합니다.

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

## 2026-11-02 추가 원문 대조 — 초음파 모듈·교재 라이선스

| 문서·저장소 | 원본 링크 | 확인한 범위 | 재사용/검증 상태 |
|---|---|---|---|
| Arduino UNO R4 Minima 공식 Hardware Docs | [보드 사양·핀맵·회로도](https://docs.arduino.cc/hardware/uno-r4-minima) | 보드 5 V I/O, Renesas RA4M1, 다운로드 자료 제공 | 공개 사실 인용(D1); Basys3 핀과 혼용 금지 |
| Arduino Modulino Distance 공식 Hardware Docs | [ToF 거리 모듈](https://docs.arduino.cc/hardware/modulino-distance) | ToF + I²C/Qwiic 방식. HC-SR04 TRIG/ECHO와 다른 인터페이스 | 공개 사실 인용(D1); 기존 초음파 RTL 재사용 불가 |
| SparkFun 일반 HC-SR04 | [SEN-15569](https://www.sparkfun.com/ultrasonic-distance-sensor-hc-sr04.html) | 제품 동작 전원 5 V, 4핀 | D1, 제품 특정 |
| SparkFun HC-SR04-33 | [SEN-24049](https://www.sparkfun.com/ultrasonic-distance-sensor-3-3v-hc-sr04.html) | 제품 동작 전원 3.3~5 V, 4핀 | D1, 별도 기판/변형; ECHO HIGH는 실물 확인 |
| Adafruit HC-SR04 | [핀 설명](https://learn.adafruit.com/ultrasonic-sonar-distance-sensors/pinouts) | 일반 HC-SR04 ECHO 5 V, 10 kΩ+10 kΩ 분압 예 | D1, 연결 계산은 설계안 |
| Seeed Grove Ultrasonic Ranger | [제조사 Wiki](https://wiki.seeedstudio.com/Grove-Ultrasonic_Ranger/) | V2의 3.3 V 시스템 호환, 3.2~5.2 V, **SIG 1선 양방향** | D1, HC-SR04 RTL 직접 대체 불가 |
| 아이씨팩토리 HC-SR04+ ICF1588 | [국내 상품 상세](https://icfactory.cafe24.com/product/hc-sr04%ED%94%8C%EB%9F%AC%EC%8A%A4-33v5v-%EC%B4%88%EC%9D%8C%ED%8C%8C-%EA%B1%B0%EB%A6%AC-%EC%B8%A1%EC%A0%95-%EA%B0%90%EC%A7%80-%EC%84%BC%EC%84%9C-%EB%AA%A8%EB%93%88-icf1588/827/category/73/display/1/) | HC-SR04+ / SGM324 / 3~5.5 V **판매처 주장** | D3, ECHO HIGH·기판 회로 미확인. 다른 HC-SR04 제품과 혼용 금지 |
| EKU HC-SR04 3~5.5 V 호환 | [국내 상품 상세](https://eku.co.kr/product/hc-sr04-%EC%B4%88%EC%9D%8C%ED%8C%8C-%EA%B1%B0%EB%A6%AC-%EC%84%BC%EC%84%9C-%EB%AA%A8%EB%93%88-3v-55v-%ED%98%B8%ED%99%98/149/) | 제목은 3~5.5 V, 상세에는 DC5V/HIGH5V **내부 모순** | D3, 3.3 V FPGA 직결 승인 불가 |
| Freenove Ultimate Starter Kit | [원본 저장소](https://github.com/Freenove/Freenove_Ultimate_Starter_Kit) · [LICENSE.txt](https://github.com/Freenove/Freenove_Ultimate_Starter_Kit/blob/master/LICENSE.txt) | README와 LICENSE를 직접 열람. **CC BY-NC-SA 3.0** (저작자표시·비영리·동일조건변경허락) | 기준 `5315f61497eea49553aa1b70721751fb16a497e5`; 교육용 참조 링크, 원문 재배포/상업 이용은 조건 확인 |
| Keyestudio KS5003/KS5004 | [원본 저장소](https://github.com/keyestudio/KS5003-KS5004-Keyestudio-ESP32-42-in-1-Sensor-Kit) | Project 47 튜토리얼 열람. 저장소 루트에 LICENSE 파일 없음, API license 필드 미기재 | 기준 `4adb6ce17ee454d10230b03030d5818e27cc0278`; **재배포 권한 미확인**. 사진·코드 복제 금지 |

위 확인은 **2026-11-02 기준** 문서/저장소의 직접 열람 결과입니다. Freenove의 CC BY-NC-SA 3.0은 모든 사용을 허락하는 무조건적 퍼블릭 도메인이 아니며, Keyestudio의 LICENSE 부재도 재배포 허용을 뜻하지 않습니다. 회로/배선 그림과 데이터시트의 **링크 제공**과 **원문 파일의 재배포**를 구분합니다.

## 2026-11-02 PIR 보드 변형 대조 — HC-SR501/Seeed/Modulino

| 출처 URL | 모델·회로·핀·전원 대조 | 원본 버전 / 라이선스·재배포 |
|---|---|---|
| [Joy-IT SEN-HC-SR501 공식 제품](https://joy-it.net/en/products/SEN-HC-SR501) | 제품명 `SEN-HC-SR501`, **VCC 5 V**, 논리 출력 **3 V**, 데이터시트·설명서 다운로드 링크 존재 | 일반 웹 제품 페이지(커밋 SHA 해당 없음), 2026-11-02 열람. 저작권 © Joy-IT, 재배포 허가 **미확인** |
| [Espruino HC-SR501 기술 안내](https://www.espruino.com/Pyroelectric) | 일반형 VCC **4.5~20 V**, OUT **3.3 V**(공칭). **오픈드레인/전원레벨 출력 변형** 경고, 소비전류 **<50 µA** 주장 | 웹 안내(고정 SHA 미기록), 2026-11-02 열람. 원문 재배포 권한 **미확인** |
| [HSHL 대학 HC-SR501 사양표](https://wiki.hshl.de/wiki/index.php/Bewegungsmelder%2C_PIR-Sensor_-_HC-SR501) | VCC **4.5~20 V**, OUT 3.3 V, **65 mA 동작/<50 µA 대기**(Espruino와 상충), VCC/OUT/GND 핀 이름 | 교육용 2차 자료(D3), 웹 페이지(고정 SHA 없음), 라이선스 미확인 |
| [Seeed Grove PIR Motion Sensor 공식 Wiki](https://wiki.seeedstudio.com/Grove-PIR_Motion_Sensor/) | **Grove 4선(SIG/NC/VCC/GND)**, **3~5 V**, **100 µA@3 V / 150 µA@5 V**. 회로도/Eagle 다운로드 제공. **HC-SR501과 다른 기판** | 공식 웹 기술문서(D1, 제품 특정), 2026-11-02 열람. 회로도/원문 재배포 라이선스 미확인, 웹 문서 SHA 해당 없음 |
| [Arduino Modulino Movement 공식 Hardware Docs](https://docs.arduino.cc/hardware/modulino-movement/) | **LSM6DSOXTR 가속도/자이로 6축 + Qwiic/I²C**, PIR GPIO OUT 제품이 아님 | 공식 웹 기술문서(D1), 2026-11-02 열람. 재배포 라이선스 미확인, SHA 해당 없음 |
| [Keyestudio Project 47](https://github.com/keyestudio/KS5003-KS5004-Keyestudio-ESP32-42-in-1-Sensor-Kit) | ESP32 키트의 **DIY PIR 모듈**; 실제 HC-SR501과 동일 PCB/전기 사양이라고 확인되지 않음 | 기준 SHA `4adb6ce17ee454d10230b03030d5818e27cc0278`(기존 대장), 라이선스 미확인 |
| [Freenove Ultimate Starter Kit](https://github.com/Freenove/Freenove_Ultimate_Starter_Kit) | 범용 키트 튜토리얼 참고. **PIR의 전기 사양은 제품별 검증 없이 전용 보드에 적용하지 않음** | 기준 SHA `5315f61497eea49553aa1b70721751fb16a497e5`(기존 대장), **CC BY-NC-SA 3.0** |

**전원/레벨 검증 게이트:** HC-SR501의 `out_high_v=3.3`은 자료에 나온 **공칭값**이며 실제 저가 호환 보드의 FPGA 직결 승인을 뜻하지 않는다. OUT가 VCC 레벨인 변형은 5 V가 될 수 있다. [모듈 문서](gpio/hc-sr501.md)의 전압 측정·출력 구조 확인 후 연결한다. XADC 0~1 V 아날로그 입력과 PIR 디지털 GPIO를 혼동하지 않는다.

## Arduino 공식 자료·캡스톤 출처 (2026-11-02 추가)

| 출처 | 원본 자료와 활용 | 근거 수준·한계 |
|---|---|---|
| [Arduino Hardware Documentation](https://docs.arduino.cc/hardware/) | 보드/Modulino 사양·회로도·핀맵·튜토리얼의 **공식 출발점** | 개별 모델 자료와 범용 호환 모듈은 구별해야 함 |
| [Keyestudio KS5003/KS5004 ESP32 42-in-1](https://github.com/keyestudio/KS5003-KS5004-Keyestudio-ESP32-42-in-1-Sensor-Kit) | ESP32 센서 키트의 Python 및 Arduino C 튜토리얼·media 자료 | 저장소 확인; 재배포 라이선스 미확인 |
| [Project 47: Intrusion Detection](https://github.com/keyestudio/KS5003-KS5004-Keyestudio-ESP32-42-in-1-Sensor-Kit/blob/master/docs/2.Python_Tutorial%28Windows%29/Python_Tutorial%28Windows%29.md) | **PIR → LED·능동 부저** 배선도·코드·결과 | 코드/구성 직접 확인; 자체 RTL 변환·실물 검증은 미실시 |
| [캡스톤 01](capstone/README.md) | 위 공개 사례를 기준으로 HC-SR04와 상태 Register를 추가하는 독립 설계안 | 확장안, 보드 검증 전 |

Keyestudio 기준 조회 커밋: [`4adb6ce17ee454d10230b03030d5818e27cc0278`](https://github.com/keyestudio/KS5003-KS5004-Keyestudio-ESP32-42-in-1-Sensor-Kit/tree/4adb6ce17ee454d10230b03030d5818e27cc0278). 원본 소스·이미지를 복제하여 게시하기 전 라이선스 확인이 필요하다.

## 사용 주의 (프로젝트 문서 §12 기재)

- `suoglu/HC-SR04`: README 에 measure 신호를 계속 High 로 두면 문제가 있다고 명시. 이 저장소 `gpio/rtl/` 의 HC-SR04 래퍼와 비교할 때 1클럭 시작 펄스로 검증.
- `alexforencich/verilog-i2c`: 유지보수 종료·Taxi 이관 명시 → 신규 표준 IP 로 쓰지 않음.
- `hdl-util/i2c`: README 의 TODO·미시험 항목 확인 필요.
- `Digilent/vivado-library`: 과거 Pmod IP 가 MicroBlaze V + 2024.2 에서 재현된다는 뜻이 아님.
- `Xilinx/embeddedsw`: `master` 는 2024.2 이후 수정을 포함할 수 있어 설치된 2024.2 와 호환되는 기준으로 판단.

<!-- nav-foot -->← [devices 목차](README.md)
