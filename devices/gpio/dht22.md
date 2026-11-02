# DHT22 (AM2302) 온습도 센서

<!-- nav -->[저장소 홈](../../README.md) › [devices](../README.md) › [gpio](README.md)
- 분류: 온도·습도 · 인터페이스: 단일선 독자 프로토콜(GPIO 1핀, 1-Wire와 다름) · 근거: D1 (스파크펀 호스팅 데이터시트)

## 2026-11-03 모듈 변형·전압·핀맵 교차 확인

| 제품 | 실제 인터페이스·전원 | 핀/기판 차이 | 근거·판정 |
|---|---|---|---|
| DHT22/AM2302 단품(4핀) | 단일선 양방향 데이터, 일반적으로 3.3~6 V 동작 | 정면 기준 1=VDD, 2=DATA, 3=NC, 4=GND. 실물 패키지·핀1 방향 확인 | [Seeed DHT22](https://wiki.seeedstudio.com/Grove-Temperature_and_Humidity_Sensor_Pro/)·기존 D1; 보드 직결은 풀업 확인 후 |
| Keyestudio KS0430 | 3핀 단일선, 제조사 3.3~5 V | 전용 3핀 기판. 단품의 4핀 배열을 그대로 적용하지 않음 | [KS0430](https://docs.keyestudio.com/projects/KS0430/en/latest/docs/KS0430.html) |
| Keyestudio KS0431 EASY plug | RJ11 6P6C, 3.3~5 V, 내부 신호는 단일선 | 제조사가 타사 RJ11의 내부 선 순서 불일치를 경고. 실물 커넥터 핀배열 확인 전 연결 금지 | [KS0431](https://docs.keyestudio.com/projects/KS0431/en/latest/docs/KS0431.html) |
| Seeed Grove Temperature & Humidity Sensor Pro | 3.3~6 V, **I/O 논리 레벨은 VCC에 따름** | Grove 4핀 케이블에서 신호 1개 사용. Seeed 예제는 5V와 D2를 사용 | [Seeed 공식 문서](https://wiki.seeedstudio.com/Grove-Temperature_and_Humidity_Sensor_Pro/) |
| Arduino Modulino Thermo | **HS3003 + I²C(Qwiic), DHT22 아님** | 3.3V Qwiic 버스. DHT22 RTL·C 드라이버 직접 호환 안 됨 | [Arduino Hardware Docs](https://docs.arduino.cc/hardware/modulino-thermo) |

**P0 전기 안전:** 센서 전원 허용범위와 DATA 핀의 HIGH 전압은 별개입니다. 5 V로 공급하거나 5 V로 DATA를 풀업한 모듈을 Basys3 3.3 V I/O에 직결하지 않습니다. 3.3 V 전원·DATA 3.3 V 풀업을 기본 검토하되 모듈 내장 풀업·레귤레이터·레벨 변환·실제 핀배열을 먼저 확인합니다. DHT22 DATA는 디지털 GPIO이며 XADC(0~1 V 아날로그 입력)가 아닙니다. 센서의 2초 이상 읽기 간격은 C 드라이버 또는 응용 소프트웨어에서 관리하며 RTL 엔진 자체가 자동 보장하지 않습니다.

**출처·저작권:** 공식 제품 문서의 설명만 재구성했고 원본 사진·도면·예제 코드는 복제하지 않았습니다. Seeed 라이브러리는 MIT, Arduino Modulino 라이브러리는 MPL-2.0으로 확인했으며 커밋·라이선스는 [출처 대장](../REFERENCES.md)에 별도 기록했습니다. 판매처마다 PCB 부품과 핀배열이 달라질 수 있으므로 미확인 제품은 직접 연결 승인하지 않습니다.

## 전기 사양 (확인)
| 항목 | 값 |
|---|---|
| 공급 전압 | 3.3–6 V DC (전기 표 기준 5 V 표준) |
| 습도 | 0–100 %RH, 정확도 ±2 %RH(최대 ±5 %RH), 분해능 0.1 %RH |
| 온도 | -40–80 °C, 정확도 <±0.5 °C, 분해능 0.1 °C |
| 샘플링 주기 | 평균 2 s, 수집 주기 2 s 초과 |
| 1회 통신 | 약 5 ms |
| 데이터 프레임 | 40비트 = 습도 정수 8 + 습도 소수 8 + 온도 정수 8 + 온도 소수 8 + 체크섬 8, 상위 비트 먼저 |

풀업 저항 값은 사본에 없음.

## SoC 연결 (Basys3 + MicroBlaze V)
| 항목 | 내용 | 구분 |
|---|---|---|
| 핀 | 데이터 1핀 양방향(Pmod 1핀) | 확인(단일선) |
| 전압 | 3.3 V로 구동하면 FPGA 3.3 V 입출력과 맞출 수 있음 | 계산(3.3 V 하한 충족) |
| 구현 | 비트 타이밍 해석은 RTL 또는 AXI GPIO(TRI로 방향 전환) + 소프트웨어 | 설계안 |
| 읽기 주기 | 2 s마다 1회 이하 | 확인 |

## 단일선 파형 (확인)
| 구간 | 값 |
|---|---|
| 호스트 시작 | Low 1 ms 이상, 그 뒤 해제 후 20–40 µs 안에 응답 |
| 응답 | Low 80 µs, High 80 µs |
| 비트 | Low 50 µs + High (0은 26–28 µs, 1은 70 µs) |
| 전원 투입 후 | 1 s 동안 명령을 보내지 않음 |
| 수집 주기 | 2 s 초과 |
| 전류 | 측정 시 1 mA 이상(전형 1.5 mA), 대기 40–50 µA |
| 필터 | VDD–GND 사이 100 nF 권장 |

데이터시트에 허용 오차 표는 없고, 종료 신호 서술도 없습니다. 풀업 저항 값도 없습니다.

## 전용 RTL
`rtl/`에 AXI4-Lite DHT22 리더를 설계했습니다(`rtl/README.md`). 비트 판정은 High 시작 후 40 µs 시점의 레벨로 합니다.

## 근거와 한계
체크섬 규칙(앞 4바이트 합의 하위 8비트)과 온도 부호 표현(비트 15 = 음수, 크기는 하위 15비트)은 열람한 데이터시트 사본에 없어 RIOT-OS `drivers/dht/dht.c` 코드로 확인했습니다(D3, 웹 가져오기 요약 경유; 데이터시트와 독립인지는 모름). 드라이버 `sw/dht22.[ch]`는 호스트 모의 레지스터 시험만 통과했고, RTL 엔진의 비트 타이밍은 시뮬레이션·합성 전, 보드 실행도 없습니다. DHT11 은 프레임 형식이 달라 지원하지 않습니다.
