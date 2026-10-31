# 기술 트리 32차시 ↔ 저장소 모듈 대응

<!-- nav -->[저장소 홈](../README.md) › [devices](README.md)

프로젝트 문서(`arduino_io_module_tech_tree_20261030_github_updated.md` §4·§10)의 32차시를 이 저장소의 모듈과 연결한 표입니다. 이 파일은 `tools/gen_sessions.py` 가 `data/session-map.csv` 와 `data/module-status.csv` 에서 만듭니다.

- **대응(차시→모듈 ID)은 제목을 보고 맞춘 설계안**입니다. 문서가 모듈 ID 로 차시를 지정한 것이 아닙니다.
- **차시 상태**는 소속 모듈 중 가장 낮은 상태입니다(계산). 「완비」도 호스트 모의 시험까지이고, 보드·RTL 시뮬레이션은 하나도 없습니다([STATUS.md](STATUS.md) §1).
- AMD IP 열은 해당 차시에서 자체 RTL 대신 쓰는 AMD IP입니다(사용자 결정 2026-10-31, [TODO.md](TODO.md)). 14·19 차시는 모듈이 없고 AMD IP 만 씁니다.

| 차시 | 트랙 | 실습 | 모듈 | AMD IP | 상태 | 메모 |
|---|---|---|---|---|---|---|
| 01 | A 기초 GPIO | 버튼과 LED | G01 택트 스위치(완비), G07 LED·RGB LED(완비) | — | 완비 |  |
| 02 | A 기초 GPIO | 적외선 감지 경보 | G02 HC-SR501 PIR(문서만), G08 능동 부저(문서만) | — | 문서만 |  |
| 03 | A 기초 GPIO | 문 열림 모니터 | G04 리드 스위치(없음), G01 택트 스위치(완비), G07 LED·RGB LED(완비), G08 능동 부저(문서만) | — | 없음 | 리드 스위치 전용 문서 없음(G04) |
| 04 | A 기초 GPIO | 조도 센서 단순 디지털 입력 | A01 CdS 조도센서(일부), G07 LED·RGB LED(완비) | — | 일부 | 비교기 DO 입력은 일반 GPIO 입력. 모듈 DO 문서 없음 |
| 05 | A 기초 GPIO | 4×4 키패드 | B02 4x4 키패드(없음) | — | 없음 |  |
| 06 | A 기초 GPIO | 입력 이벤트 레지스터 | G01 택트 스위치(완비), G02 HC-SR501 PIR(문서만), G04 리드 스위치(없음), G05 홀 스위치 A3144(없음), G06 포토 인터럽터(없음) | — | 없음 |  |
| 07 | B 시간·파형·ADC | PWM LED 조광 | P05 MOSFET LED·팬 PWM(일부) | AXI Timer(PG079) PWM | 일부 |  |
| 08 | B 시간·파형·ADC | 초음파 거리 | T01 HC-SR04(완비) | — | 완비 | HC-SR04 RTL 래퍼는 시뮬레이션 전 |
| 09 | B 시간·파형·ADC | 서보 위치 제어 | P01 SG90 서보(일부) | AXI Timer PWM 대안 | 일부 |  |
| 10 | B 시간·파형·ADC | DC 모터 제어 | P02 DC 모터 + L298N(완비), P03 TB6612FNG(문서만) | — | 문서만 | TB6612 는 L298 드라이버 공용(문서만) |
| 11 | B 시간·파형·ADC | 엔코더 회전수 | T02 로터리 엔코더(일부) | — | 일부 | 전용 RTL, 시뮬레이션 전 |
| 12 | B 시간·파형·ADC | CdS 스마트 조명 | A01 CdS 조도센서(일부), P05 MOSFET LED·팬 PWM(일부) | XADC | 일부 | XADC 는 AMD 블록. 0~1.0 V 주의 |
| 13 | B 시간·파형·ADC | 2입력·2출력 환경 제어 | A01 CdS 조도센서(일부), G02 HC-SR501 PIR(문서만), G07 LED·RGB LED(완비), G08 능동 부저(문서만) | — | 문서만 |  |
| 14 | C UART·전용 | UART 센서 명령·응답 | — | AXI UART Lite(PG142) | —(AMD IP만) | 자체 uart_core 대신 AMD IP 사용(사용자 결정) |
| 15 | C UART·전용 | Bluetooth Serial 제어 | U01 HC-05 / HC-06(완비) | AXI UART Lite | 완비 |  |
| 16 | C UART·전용 | GPS 수신 | U02 NEO-6M GPS(완비) | AXI UART Lite | 완비 |  |
| 17 | C UART·전용 | DS18B20 온도 수집 | W01 DS18B20(완비) | — | 완비 | 1-Wire 전용 RTL 시뮬레이션 전 |
| 18 | C UART·전용 | 전용 타이밍 장치(DHT22 또는 HX711) | C01 DHT11 / DHT22(완비), C02 HX711 로드셀(없음) | — | 없음 | HX711 은 없음 |
| 19 | D I²C | I²C 마스터 회로 | — | AXI IIC(PG090) | —(AMD IP만) | 자체 마스터 RTL 대신 AMD IP 사용(사용자 결정) |
| 20 | D I²C | 조도 읽기 | I01 BH1750(완비) | AXI IIC | 완비 |  |
| 21 | D I²C | RTC + OLED 표시 | I05 DS3231 RTC(완비), I06 SSD1306 OLED(완비) | AXI IIC | 완비 |  |
| 22 | D I²C | 동작·자세 측정 | I04 MPU-6050(완비) | AXI IIC | 완비 |  |
| 23 | D I²C | 다채널 PWM 확장 | I09 PCA9685 16채널 PWM(없음), P01 SG90 서보(일부) | AXI IIC | 없음 | PCA9685 드라이버 없음 |
| 24 | E SPI | SPI 마스터·표시 | S03 MAX7219 매트릭스(완비) | AXI Quad SPI(PG153) | 완비 |  |
| 25 | E SPI | 태그 읽기 | S01 MFRC522 RFID(문서만) | AXI Quad SPI | 문서만 | RC522 드라이버 없음 |
| 26 | E SPI | 외부 ADC | S05 MCP3008(완비) | AXI Quad SPI | 완비 |  |
| 27 | E SPI | 무선 패킷 | S08 nRF24L01+(완비) | AXI Quad SPI | 완비 |  |
| 28 | E SPI | 저장·네트워크 확장 | S02 microSD (SPI)(문서만), S09 W5500(완비) | AXI Quad SPI | 문서만 | SD 는 문서만 |
| 29 | F SoC 확장 | AXI GPIO·Timer 보안 입력장치 | G01 택트 스위치(완비), G02 HC-SR501 PIR(문서만), G04 리드 스위치(없음) | AXI GPIO(PG144) + AXI Timer | 없음 |  |
| 30 | F SoC 확장 | AXI UART 센서 게이트웨이 | U01 HC-05 / HC-06(완비), U02 NEO-6M GPS(완비) | AXI UART Lite | 완비 |  |
| 31 | F SoC 확장 | 외부 통신 IP + 센서 드라이버 | I02 BME280(완비), I04 MPU-6050(완비), S05 MCP3008(완비) | AXI IIC / AXI Quad SPI | 완비 |  |
| 32 | F SoC 확장 | Custom AXI IP 주변장치 | W01 DS18B20(완비), T01 HC-SR04(완비), P01 SG90 서보(일부), P02 DC 모터 + L298N(완비) | 자체 RTL(AXI4-Lite 래퍼) | 일부 | RTL 4종 모두 시뮬레이션 전 |

## 요약

| 완비 | 일부 | 문서만 | 없음 | AMD IP만 |
|---|---|---|---|---|
| 13 | 6 | 5 | 6 | 2 |

<!-- nav-foot -->← [devices 목차](README.md)
