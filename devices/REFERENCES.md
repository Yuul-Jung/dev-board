# GitHub 참고 자료 대장

## 2026-11-03 모터 드라이버·전기 인터페이스 교차 확인

| 원본 URL | 원본 종류·재현 기준 | 검증/라이선스/한계 |
|---|---|---|
| [ST L298](https://www.st.com/en/motor-drivers/l298.html) | 제조사 칩 제품 페이지·데이터시트 (D1) | EN/IN TTL 로직과 VS/VSS 구분. 기판 회로가 아님. 원문·이미지 미복제 |
| [Keyestudio KS0063](https://docs.keyestudio.com/projects/KS0063/en/latest/docs/KS0063.html) | L298N+78M05 기판 공식 제품 문서 (D1) | GPIO/PWM 직접 제어, 5 V 로직. 기판 변형·점퍼는 실물 확인. 원본 라이선스 미확인, 복제 없음 |
| [Seeed Grove I2C Motor Driver v1.2](https://wiki.seeedstudio.com/Grove-I2C_Motor_Driver_V1.2/) | L298N+ATmega8L I²C 모듈 (D1) | I²C 5 V, 2 A 서술 vs 0.5 A 표의 모순 확인. 원본 라이선스 미확인, 복제 없음 |
| [Arduino Motor Shield Rev3](https://docs.arduino.cc/hardware/motor-shield-rev3) | Arduino 공식 L298 Shield (D1) | 일반 빨간 L298N 보드와 다른 회로·핀맵. 공식 문서 원본 미복제 |
| [Arduino Modulino Motors](https://docs.arduino.cc/hardware/modulino-motors) | MAX22211 기반 3.3 V I²C/Qwiic 모듈 (D1) | L298 PWM 직접 제어와 다른 프로토콜. 개별 보드 회로도·라이브러리 호환 미검증 |
| [Arduino_Modulino](https://github.com/arduino-libraries/Arduino_Modulino/tree/24d08a94ac4e8f83aed211df0fa6f38e9994b164) | 라이브러리 기준 커밋 `24d08a94ac4e8f83aed211df0fa6f38e9994b164` | [LICENSE](https://github.com/arduino-libraries/Arduino_Modulino/blob/24d08a94ac4e8f83aed211df0fa6f38e9994b164/LICENSE) MPL-2.0 확인. Modulino Motors 지원 여부는 별도 확인 필요 |
| [Freenove Ultimate Starter Kit](https://github.com/Freenove/Freenove_Ultimate_Starter_Kit/tree/5315f61497eea49553aa1b70721751fb16a497e5) | 보조 키트 튜토리얼, 고정 커밋 `5315f61497eea49553aa1b70721751fb16a497e5` | GitHub 라이선스 메타데이터 NOASSERTION. 원본 자료·그림·코드 재배포 허가 미확인, 복제 없음 |
| [디바이스마트 SZH-EK001](https://www.devicemart.co.kr/goods/view?no=1278835) | 국내 범용 L298N 판매처 (D3) | 판매처 정보는 기판 전기 회로도 근거가 아님. 제품 실물 버전 미확인 |

열람일 2026-11-03. 출처의 개별 커밋을 특정할 수 없는 제조사 웹문서는 **URL + 열람일**로 추적하며 확인하지 않은 SHA를 만들지 않습니다.


## 2026-11-03 DHT22·직교 엔코더 모듈 교차 확인

| 원본 출처 | 확인 사항 | 재현 기준 / 저작권 / 한계 |
|---|---|---|
| [Arduino Modulino Thermo](https://docs.arduino.cc/hardware/modulino-thermo) | HS3003 온습도 센서, Qwiic I²C. DHT22 GPIO 프로토콜과 다름 | 공식 Hardware Docs, 열람 2026-11-03. 그림·코드 미복제 |
| [Arduino Modulino Knob 튜토리얼](https://docs.arduino.cc/tutorials/modulino-knob/how-knob/) | STM32C011F4가 A/B/버튼을 처리, 3.3 V I²C, 7비트 0x3A, PA0/PA1/PA2 직접 연결 핀 별도 | 공식 문서, 열람 2026-11-03. 그림·코드 미복제 |
| [Arduino_Modulino 라이브러리](https://github.com/arduino-libraries/Arduino_Modulino/tree/24d08a94ac4e8f83aed211df0fa6f38e9994b164) | Arduino Modulino의 I²C 인터페이스와 라이브러리 | 고정 커밋 `24d08a94ac4e8f83aed211df0fa6f38e9994b164`, LICENSE 파일 MPL-2.0 직접 확인 |
| [Keyestudio KS0430 DHT22](https://docs.keyestudio.com/projects/KS0430/en/latest/docs/KS0430.html) | 3핀 PCB, 3.3~5 V 단일선 | 공식 제품 문서 열람 2026-11-03, 원본 저작권/재배포 허가 미확인 |
| [Keyestudio KS0431 EASY plug DHT22](https://docs.keyestudio.com/projects/KS0431/en/latest/docs/KS0431.html) | RJ11 6P6C, 타사 RJ11 내부 선 순서 다름 | 공식 제품 문서 열람 2026-11-03, 핀별 선색은 실물 확인 필요 |
| [Seeed Grove DHT22](https://wiki.seeedstudio.com/Grove-Temperature_and_Humidity_Sensor_Pro/) | 3.3~6 V, I/O 레벨 VCC 기준, Grove 신호 1개 | 공식 Wiki 열람 2026-11-03, 보드별 풀업/핀 확인 필요 |
| [Seeed Grove DHT 라이브러리](https://github.com/Seeed-Studio/Grove_Temperature_And_Humidity_Sensor/tree/826ce8f2593f810a6063cb1cbc788fc6519dc25e) | DHT22 Arduino 소프트웨어 참고 | 고정 커밋 `826ce8f2593f810a6063cb1cbc788fc6519dc25e`, LICENSE 파일 MIT 직접 확인 |
| [Keyestudio KS0013 Rotary Encoder](https://docs.keyestudio.com/projects/KS0013/en/latest/docs/KS0013.html) | 5 V 디지털 엔코더, A/B/버튼 | 공식 제품 문서 열람 2026-11-03, 3.3 V 호환성 미확인 |
| [Seeed Grove Encoder](https://wiki.seeedstudio.com/Grove-Encoder/) | 동작 4.5~5.5 V, Grove 커넥터 | 공식 Wiki 열람 2026-11-03, Basys3 3.3 V 직접 연결 미승인 |
| [국내 DHT22 3핀 PCB 판매처](https://eduiot.kr/product/디지털-온습도센서-dht22-고정밀-pcb-모듈-am2302-3pin/) | 3.3~5 V 표기, 저항 내장 기판으로 설명 | 판매처 설명(D3), 내장 저항이 DATA 풀업인지·핀 순서·실물 PCB 미확인 |
| [국내 KY-040 판매처](https://www.11st.co.kr/products/2227891483) | KY-040 저가 판매 예시 | 판매처 설명(D3), 전원·출력 HIGH·풀업 회로 미확인. 가격·재고는 변동 |

**교차 검증 판정:** 동일 센서 기능을 가진 DHT22 단품·Keyestudio·Grove와 Arduino Modulino Thermo는 물리 인터페이스가 같지 않습니다. 마찬가지로 KY-040/KS0013 직접 GPIO와 Modulino Knob I²C는 다른 제어 경로입니다. Freenove 기존 참고 자료는 이들 특정 기판의 데이터시트·핀맵을 대체할 근거로 사용하지 않았습니다. 사진·회로도·라이브러리 원문을 복제하지 않고 출처만 연결했습니다.


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

## 2026-11-02 추가 원문 대조 — ADS1115 ADC·보드 변형

| 구분 | 원본 | 직접 확인한 내용 | 고정 버전·라이선스 / 한계 |
|---|---|---|---|
| TI ADS1115 | [SBAS444E Rev.E](https://www.ti.com/document-viewer/ADS1115/datasheet) §5.1/§5.3/§7.3.3 | **AIN 권장 GND~VDD**, 절대최대 GND−0.3~VDD+0.3, PGA ±6.144 V는 입력 허용 전압이 아님 | D1, TI 공식 웹 데이터시트(2024-12), Git 커밋 해당 없음 |
| Arduino UNO R4 Minima | [Hardware Docs](https://docs.arduino.cc/hardware/uno-r4-minima) | 보드 I/O 5 V, ADC 14비트까지 지원. Arduino 실습 연결과 Basys3 전압을 혼용하지 않음 | D1, Arduino 공식 웹 문서, 웹 페이지 재배포 라이선스 미검토 |
| Seeed Grove 4-channel ADS1115 | [Wiki](https://wiki.seeedstudio.com/Grove-16-bit-ADC-ADS1115/) | 제조사 3.3/5 V, I²C 0x48, Grove A0/A1과 나사 단자 A0/A1 공유 | D1, 보드 풀업 레일·실제 커넥터 핀 순서는 회로도/실물 미확인 |
| DFRobot Gravity DFR0553 | [제조사 상품](https://www.dfrobot.com/product-1730.html) | VCC 3.3~5 V, 제조사 표기 **I²C HIGH 3.3 V**, 2주소 선택 스위치, 4채널 | D1, 제조사 표기를 실물 전압 측정으로 오인하지 않음 |
| 국내 VCTEC DFR0553 | [판매 상품](https://vctec.co.kr/product/16%EB%B9%84%ED%8A%B8-i2c-adc-%EB%AA%A8%EB%93%88-ads1115-gravity-i2c-ads1115-16-bit-adc-module-arduino-/13354/) | DFRobot 제조사 사양을 재기재 | D3, 실제 납품 기판 변형·가격·재고 미검증 |
| Seeed Arduino ADS1115 소스 | [커밋 `00beed48708bae7cce1f78ad05448e1af132e0a3`](https://github.com/Seeed-Studio/Seeed_Arduino_ADS1115/tree/00beed48708bae7cce1f78ad05448e1af132e0a3) · [License.txt](https://github.com/Seeed-Studio/Seeed_Arduino_ADS1115/blob/00beed48708bae7cce1f78ad05448e1af132e0a3/License.txt) | 공식 Arduino 라이브러리와 라이선스 파일 확인 | **MIT**, 저작권 Seeed Technology 2020, 코드 미복제 |
| DFRobot ADS1115 소스 | [커밋 `6d3df78d29693bf9e245a223afe0443406bd2980`](https://github.com/DFRobot/DFRobot_ADS1115/tree/6d3df78d29693bf9e245a223afe0443406bd2980) · [LICENSE](https://github.com/DFRobot/DFRobot_ADS1115/blob/6d3df78d29693bf9e245a223afe0443406bd2980/LICENSE) | 공식 Arduino 라이브러리와 라이선스 파일 확인 | **MIT**, 저작권 DFRobot 2010, 코드 미복제 |

**선별 기준:** Keyestudio/Freenove 기존 튜토리얼은 주변모듈 자료 대장에 유지하지만 **동일 ADS1115 IC·기판에 대한 검증된 원본 사례가 이번 조사에서 확보되지 않아** 다른 센서의 예제를 ADS1115 원본으로 둔갑시키지 않았습니다. 원본 웹 이미지·코드는 복제하지 않았습니다. 자세한 안전 규칙: [ADS1115](i2c/ads1115.md).

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
| [가치창조기술 HC-SR501 국내 판매](https://vctec.co.kr/product/적외선-pir-모션-센서-hc-sr501-infrared-pir-motion-sensor-hc-sr501/10402) | 판매 페이지: VCC 5~20 V, OUT 3.3/0 V, 65 mA 동작. HSHL과 동일 수치이나 측정 조건 미제시 | 국내 판매처(D3), 2026-11-02 열람. 웹 페이지 SHA 없음·재배포 권한 미확인 |
| [에듀이노 HC-SR501 국내 판매](https://m.eduino.kr/product/아두이노-적외선-pir-센서-인체감지-모션센서-hc-sr501-hcsr501/104/) | OEM 모듈 판매. 설명의 '적외선을 내보내고 반사량을 측정'은 **PIR 수동 감지 원리와 불일치** → 사양 근거로 채택하지 않음 | 국내 판매처(D3), 2026-11-02 열람. 웹 페이지 SHA 없음·재배포 권한 미확인 |
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

## 2026-11-03 초음파 AXI4-Lite 통합·모듈 전기 사양 재검증

| 자료·원본 URL | 직접 확인한 사실 | 근거·라이선스·변형 구분 |
|---|---|---|
| [Arm Introduction to AMBA AXI4](https://developer.arm.com/-/media/Arm%20Developer%20Community/PDF/Learn%20the%20Architecture/102202_0100_01_Introduction_to_AMBA_AXI.pdf) | 독립 VALID/READY 채널, 바이트별 WSTRB 정의 | Arm 공식 학습 PDF(D1); © Arm, **원문 재배포하지 않음**. 문서 URL 기준, Git SHA 없음 |
| [AMD PG138 AXI4-Lite Interface](https://docs.amd.com/r/en-US/pg138-axi-ethernet/AXI4-Lite-Interface) | 32비트 데이터의 WSTRB[3:0]과 AW/W/B/AR/R 포트·의미 | AMD 공식 IP 문서(D1); 문서 URL 기준, Git SHA 없음, 재배포 허가 미확인 |
| [AMD PG267 AXI VIP Protocol Checks](https://docs.amd.com/r/en-US/pg267-axi-vip/AXI-Protocol-Checks-and-Descriptions) | WSTRB 유효 바이트, WVALID/WDATA/WSTRB 안정성, BVALID은 주소·데이터 수락 후 발생 | AMD 공식 검증 규칙(D1); 본 저장소 RTL은 AXI VIP로 검증한 것이 아님 |
| [Arduino Modulino Distance](https://docs.arduino.cc/hardware/modulino-distance) · [ABX00102 데이터시트](https://docs.arduino.cc/resources/datasheets/ABX00102-datasheet.pdf) | **VL53L4CDV0DH/1** ToF IC, Qwiic 3.3 V, 센서 공급 권장 2.6~3.5 V | Arduino 공식 하드웨어(D1), 웹/PDF URL 기준, Git SHA 없음. 일반 HC-SR04 TRIG/ECHO와 **IC·핀·프로토콜이 다름**. 회로도 재배포 허가 미확인 |
| [Arduino_Modulino v0.10.0 소스](https://github.com/arduino-libraries/Arduino_Modulino/blob/24d08a94ac4e8f83aed211df0fa6f38e9994b164/src/Modulino.h) · [LICENSE](https://github.com/arduino-libraries/Arduino_Modulino/blob/24d08a94ac4e8f83aed211df0fa6f38e9994b164/LICENSE) | 거리 센서 주소 **0x29**, VL53L4CD 초기화 실패 시 VL53L4ED 폴백. `library.properties` **0.10.0** | 고정 Git SHA `24d08a94ac4e8f83aed211df0fa6f38e9994b164`, **MPL-2.0**. 라이브러리의 호환 로직과 실제 ABX00102 실장 IC는 구별 |
| [SparkFun HC-SR04 SEN-15569](https://www.sparkfun.com/ultrasonic-distance-sensor-hc-sr04.html) · [HC-SR04-33 SEN-24049](https://www.sparkfun.com/ultrasonic-distance-sensor-3-3v-hc-sr04.html) | 5 V 전용 일반형과 3.3~5 V 지원 변형의 제품 식별 분리 | 공식 판매·제품 사양(D1), 기판별 ECHO HIGH 실측 전 Basys3 연결 승인 금지 |
| [Adafruit HC-SR04 핀 설명](https://learn.adafruit.com/ultrasonic-sonar-distance-sensors/pinouts) | 일반형 VCC 5 V / ECHO 5 V / TRIG 3 V 또는 5 V, 분압 예시 | 공식 제품 안내(D1), 설계·실측 검증은 별개 |
| [Seeed Grove Ultrasonic Ranger](https://wiki.seeedstudio.com/Grove-Ultrasonic_Ranger/) | SIG 한 선을 송수신에 사용. 일반형 2선 RTL과 다름 | 공식 제품 Wiki(D1); 보드별 전압·핀맵 재확인, 재배포 허가 미확인 |
| [Keyestudio KS5003/KS5004 Project 47](https://github.com/keyestudio/KS5003-KS5004-Keyestudio-ESP32-42-in-1-Sensor-Kit) · [Freenove Ultimate Starter Kit](https://github.com/Freenove/Freenove_Ultimate_Starter_Kit) | 주변장치 실습 사례 대조용. AXI4-Lite 슬레이브 프로토콜의 근거로 사용하지 않음 | Keyestudio 기준 SHA `4adb6ce17ee454d10230b03030d5818e27cc0278`·라이선스 미확인; Freenove 기준 SHA `5315f61497eea49553aa1b70721751fb16a497e5`·CC BY-NC-SA 3.0 |
| [아이씨팩토리 HC-SR04+](https://icfactory.cafe24.com/product/hc-sr04%ED%94%8C%EB%9F%AC%EC%8A%A4-33v5v-%EC%B4%88%EC%9D%8C%ED%8C%8C-%EA%B1%B0%EB%A6%AC-%EC%B8%A1%EC%A0%95-%EA%B0%90%EC%A7%80-%EC%84%BC%EC%84%9C-%EB%AA%A8%EB%93%88-icf1588/827/category/73/display/1/) · [EKU HC-SR04](https://eku.co.kr/product/hc-sr04-%EC%B4%88%EC%9D%8C%ED%8C%8C-%EA%B1%B0%EB%A6%AC-%EC%84%BC%EC%84%9C-%EB%AA%A8%EB%93%88-3v-55v-%ED%98%B8%ED%99%98/149/) | 국내 저가 호환품의 전원/출력 설명이 모델별로 상이하거나 상충 | 판매처 2차 자료(D3), 회로·실측 미확인. 데이터시트 대신 사용 금지 |

**설계·검증 구분:** 위 공식 자료는 규격 근거이며, `devices/gpio/rtl/us_axil_*.v`의 구현 정확성을 자동으로 보증하지 않는다. 검증은 독립 AW/W 도착, WSTRB=0/상위 바이트, B/R 백프레셔, IRQ/W1C, 리셋과 실물 전기 연결을 분리한다. FPGA 일반 GPIO 3.3 V와 XADC 0~1 V는 별도 핀·입력 범위다. **웹/PDF 원본은 링크만 제공하고 재배포하지 않는다.**

## 2026-11-03 DS18B20 1-Wire 모듈·Arduino·RTL 참고 원문

| 공식 원본 | 확인한 사실 / 칩·기판 차이 | 고정 버전·라이선스·검증 한계 |
|---|---|---|
| [Analog Devices DS18B20 Rev.6 PDF](https://www.analog.com/media/en/technical-documentation/data-sheets/ds18b20.pdf) | 2019-07 Rev.6, 외부 전원 3.0~5.5 V, TO-92 핀 1 GND/2 DQ/3 VDD, DQ 오픈드레인, 4.7 kΩ 풀업 예시, 기생 전원 강한 풀업은 Convert T 이후 10 µs 이내 | 제조사 D1, PDF 문서 버전 기준(저장소 Git SHA 없음). 원문 PDF 재배포하지 않음 |
| [Arduino 공식 1-Wire 설명](https://github.com/arduino/docs-content/blob/475b308dd2c9d7e6783b3dbdc9626b848a454ecd/content/learn/05.communication/06.one-wire/one-wire.md) | 1-Wire 데이터선·기생 전원·OneWire 라이브러리 설명. Arduino 공식 [Hardware Docs](https://docs.arduino.cc/hardware/)는 보드 사양 참조용 | Git SHA `475b308dd2c9d7e6783b3dbdc9626b848a454ecd`; docs 저장소 전체 라이선스 재배포 조건은 별도 확인 필요 |
| [PaulStoffregen/OneWire](https://github.com/PaulStoffregen/OneWire/tree/800f26f3ee6eb446a72e013785cac3700e54cc13) | Arduino 소프트웨어 1-Wire 구현 참고. `OneWire.cpp` 소스 헤더에 저작권·MIT 형식 허가 고지 있음. 일부 CRC 출처가 별도로 표시됨 | Git SHA `800f26f3ee6eb446a72e013785cac3700e54cc13`, 독립 LICENSE 파일 없음. **소스 헤더의 MIT형 고지 확인**, 재사용·재배포 시 개별 출처 조건 추가 확인. 원본 코드 미복제 |
| [Keyestudio KS0329](https://docs.keyestudio.com/projects/KS0329/en/latest/docs/KS0329.html) | DS18B20 방수 프로브 + 별도 4.7 kΩ 저항 동봉, 3.3~5 V, 신호선 노랑 | 제조사 제품 설명 D1, Git SHA/원본 자료 재배포 라이선스 미확인 |
| [Keyestudio KS0124](https://docs.keyestudio.com/projects/KS0124/en/latest/docs/KS0124.html) | DS18B20 EASY plug RJ11 6P6C, 3.3~5 V, 타사 RJ11 내부 핀 순서 비호환 경고 | 제조사 제품 설명 D1, Git SHA/재배포 라이선스 미확인 |
| [Seeed Grove One Wire Temperature Sensor](https://wiki.seeedstudio.com/One-Wire-Temperature-Sensor-DS18B20/) | DS18B20 방수 프로브, Grove 기판에 풀업 저항 실장, 전원 3.0~5.5 V | 공식 Wiki D1, 원본 이미지·문서 재배포 라이선스 미확인 |
| [Freenove Ultimate Starter Kit — thermistor 예제](https://github.com/Freenove/Freenove_Ultimate_Starter_Kit/blob/5315f61497eea49553aa1b70721751fb16a497e5/Sketches/Sketch_12.1.1_Detect_the_temperature/Sketch_12.1.1_Detect_the_temperature.ino) · [DHT11 예제](https://github.com/Freenove/Freenove_Ultimate_Starter_Kit/blob/5315f61497eea49553aa1b70721751fb16a497e5/Sketches/Sketch_22.1.1_Temperature_and_Humidity_Sensor/Sketch_22.1.1_Temperature_and_Humidity_Sensor.ino) | 12장은 **A0 아날로그 서미스터**, 22장은 **DHT11**. DS18B20의 1-Wire 핀맵·타이밍을 입증하는 사례가 아니므로 혼용 금지 | Git SHA `5315f61497eea49553aa1b70721751fb16a497e5`; [LICENSE.txt](https://github.com/Freenove/Freenove_Ultimate_Starter_Kit/blob/5315f61497eea49553aa1b70721751fb16a497e5/LICENSE.txt) **CC BY-NC-SA 3.0** 직접 확인. 코드/그림 미복제 |
| [국내 DS18B20 공급 사례](https://www.bliiot.co.kr/product/programmable-resolution-temperature-sensor-ds18b20.html) | DS18B20 온도센서 소개, 패러사이트/일반 전원 모드. 범용 모듈의 실제 풀업 레일·기판 핀맵 증명 자료는 아님 | 국내 판매처 D3, 실물 기판·정격·호환성 별도 확인 |
| [Arm AMBA AXI4 소개](https://developer.arm.com/-/media/Arm%20Developer%20Community/PDF/Learn%20the%20Architecture/102202_0100_01_Introduction_to_AMBA_AXI.pdf) · [AMD PG267](https://docs.amd.com/r/en-US/pg267-axi-vip/AXI-Protocol-Checks-and-Descriptions) | AW/W 채널 독립 수락·WSTRB·응답 안정성은 DS18B20 데이터시트가 아닌 AXI 규격에서 검증 | 공식 기술문서 D1. 실제 RTL 검증은 별도 테스트·합성·실물 시험 필요 |

**Freenove·Keyestudio 다른 키트의 1-Wire 실습은 제품·회로별 근거가 확인될 때만 DS18B20 자료에 병합**합니다. 기존 [Freenove 자료 대장](REFERENCES.md)의 커밋·라이선스는 유지하고, 이 회차에서 확인하지 않은 특정 핀 번호나 실습 번호를 추정하지 않습니다. Arduino 호환 모듈·국내/해외 판매처의 동일 제품명은 실제 IC/PCB/풀업 레일 일치의 근거가 아닙니다.

## 사용 주의 (프로젝트 문서 §12 기재)

- `suoglu/HC-SR04`: README 에 measure 신호를 계속 High 로 두면 문제가 있다고 명시. 이 저장소 `gpio/rtl/` 의 HC-SR04 래퍼와 비교할 때 1클럭 시작 펄스로 검증.
- `alexforencich/verilog-i2c`: 유지보수 종료·Taxi 이관 명시 → 신규 표준 IP 로 쓰지 않음.
- `hdl-util/i2c`: README 의 TODO·미시험 항목 확인 필요.
- `Digilent/vivado-library`: 과거 Pmod IP 가 MicroBlaze V + 2024.2 에서 재현된다는 뜻이 아님.
- `Xilinx/embeddedsw`: `master` 는 2024.2 이후 수정을 포함할 수 있어 설치된 2024.2 와 호환되는 기준으로 판단.

<!-- nav-foot -->← [devices 목차](README.md)
