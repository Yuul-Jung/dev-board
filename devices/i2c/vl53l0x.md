# VL53L0X ToF 거리 센서 — 기판별 전기적 연결 조사

<!-- nav -->[devices](../README.md) › [I2C](README.md)

**상태:** 조사 문서만. 전용 RTL·C 드라이버·테스트벤치·Basys3 실물 검증 없음. 기존 I12(VL53L0X/L1X) 후보 중 VL53L0X만 조사; VL53L1X의 레지스터·API 호환성은 미검증.

## IC와 브레이크아웃의 차이

| 구분 | IC/기판 | 전원·신호 | Basys3 연결 판단 |
|---|---|---|---|
| ST VL53L0X IC | 2.8V 계열 센서, I²C, 기본 7비트 주소 0x29 | 외부 전원·로직 정격은 ST 데이터시트로 설계 검증 필요 | 칩을 3.3V GPIO에 직접 연결 승인하지 않음 |
| Pololu #2490 | 2.8V 레귤레이터와 I²C 양방향 레벨 시프터 | VIN 2.6~5.5V; SDA/SCL HIGH는 VIN 레일 | **VIN=3.3V** 조건으로 설계 검토 가능. VIN=5V이면 FPGA I/O 직접 연결 금지 |
| Adafruit VL53L0X breakout | 레귤레이터·레벨 변환, 헤더형/STEMMA QT 변형 | VIN 3~5V; GPIO 인터럽트는 레벨 변환되지 않음 | 3.3V 전원/풀업 조건과 보드 리비전 확인 후 연결 |
| 무명 AliExpress/범용 보드 | PCB·레귤레이터·풀업·레벨 변환 불명 | 판매명만으로 Pololu/Adafruit 회로 동일성 입증 불가 | **실물 PCB 회로/풀업 레일 측정 전 직결 미승인** |

Pololu #2490 표기 핀: **VDD(2.8V 레귤레이터 출력), VIN, SDA, SCL, XSHUT, GPIO1, GND**. 핀의 물리적 순서는 실물 실크와 해당 리비전 회로도로 확인한다. XSHUT·GPIO1은 I²C 레벨 시프터 경유 신호가 아니다. XSHUT 구동은 저전압 오픈드레인/하이임피던스 제어 등 제조사 지침을 확인한다.

## 인터페이스·설계 주의

- I²C 기본 7비트 주소 **0x29**. 다중 센서는 XSHUT 제어로 한 개씩 초기화하고 주소를 소프트웨어로 재설정해야 한다. 전원 재투입 뒤 주소 유지 여부를 가정하지 않는다.
- SDA/SCL은 오픈드레인 방식. **FPGA 3.3V I/O에 5V 풀업 금지**. 여러 기판 병렬 연결 시 합성 풀업 저항과 레일을 확인한다.
- **XADC 0~1V 아날로그 입력과 무관**: 거리 값은 I²C 디지털 데이터로 읽는다. HC-SR04의 5V ECHO 펄스 측정 RTL도 재사용하지 않는다.
- VL53L0X 전용 초기화/API 및 거리 데이터 해석이 필요하다. 임의의 I²C 레지스터 주소, 타이밍, 비트폭을 만들지 않는다.
- RTL/AXI/C 드라이버는 **현재 없음**. 따라서 FSM, ACK/NACK, clock stretching, AW/W 독립 처리, WSTRB, 테스트 커버리지는 모두 **미검증**이다.

## 원본 출처·재배포

- [Pololu #2490 제품 및 핀 설명](https://www.pololu.com/product/2490): 제조사 제품 페이지, 조회 2026-11-03. [회로도·데이터시트 링크 목록](https://www.pololu.com/product/2490/resources). 웹 페이지에 고정 Git SHA 없음, 재배포 라이선스 미확인.
- [Adafruit VL53L0X 핀 설명](https://learn.adafruit.com/adafruit-vl53l0x-micro-lidar-distance-sensor-breakout/pinouts): 제조사 교육 문서, 조회 2026-11-03. 고정 Git SHA·재배포 라이선스 미확인.
- [ST VL53L0X 원본 자료](https://www.st.com/en/imaging-and-photonics-solutions/vl53l0x.html): 데이터시트·API의 최종 버전/개정번호는 별도 확인 필요.
- Keyestudio/Freenove/Seeed 및 국내 판매처의 **동일한 VL53L0X 기판별 회로·핀맵은 이번 회차 미확인**. 해당 업체의 다른 ToF 센서 자료를 VL53L0X 근거로 대체하지 않는다.
- 원본 그림·코드·회로도 복제 없음. 위 링크는 인용·추적 목적이며 재배포 허가를 뜻하지 않는다.

## 검증 게이트

**P0:** 실물 보드의 VIN, SDA/SCL 풀업 레일, GPIO1/XSHUT 레벨과 핀 배열 확인 전 Basys3 직결 불승인. **P1:** 주소 중복, 초기화/거리 측정 API, RTL/C 및 테스트 미구현. 보드 측정·Vivado 합성·시뮬레이션 결과 없음.

<!-- nav-foot -->← [devices](../README.md)
