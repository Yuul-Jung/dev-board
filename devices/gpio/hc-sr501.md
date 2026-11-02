# HC-SR501 PIR 모션 모듈

<!-- nav -->[저장소 홈](../../README.md) › [devices](../README.md) › [gpio](README.md)

- 분류: PIR(수동 적외선) 움직임 감지 · 신호 인터페이스: **디지털 GPIO 입력 1선(OUT)**
- 자료 대조: 2026-11-02 · 근거: 제품 특정 [Joy-IT SEN-HC-SR501](https://joy-it.net/en/products/SEN-HC-SR501)(공급사 공식 제품 정보), [Espruino HC-SR501](https://www.espruino.com/Pyroelectric)(기술 안내, D3), [HSHL 대학 정리](https://wiki.hshl.de/wiki/index.php/Bewegungsmelder%2C_PIR-Sensor_-_HC-SR501)(D3). **모듈별 사양과 실물 검증은 구분**한다.

## HC-SR501의 확인된 정보와 충돌

| 항목 | 자료별 확인 내용 | 적용 판단 |
|---|---|---|
| 전원 VCC | Espruino·HSHL: **4.5~20 V**(일반 HC-SR501), Joy-IT **SEN-HC-SR501: 5 V DC** | 실습 기본은 해당 기판의 명시된 전원 사용. **3.3 V 전원만으로 정상 동작한다고 가정 금지** |
| OUT 신호 | Joy-IT 제품: 논리 레벨 **3 V**; Espruino 일반형: HIGH **3.3 V**, LOW **0 V** | 흔한 제품의 공칭값이지 모든 호환 보드의 보증값 아님 |
| OUT 변형 | Espruino: 일부는 **오픈드레인**이거나 **전원 전압에 따른 펄스**를 출력할 수 있다고 경고 | 실제 OUT HIGH 및 풀업 전원을 확인하기 전 FPGA 직접 연결 승인 금지 |
| 핀 이름 | **VCC / OUT / GND**(Espruino·HSHL) | 커넥터에서의 물리적 순서는 구매 기판의 실크/회로도 확인 |
| 트리거 점퍼 | H = 재트리거, L = 비재트리거(HSHL) | 점퍼 유무와 극성은 실제 기판 기준 |
| 감지 거리/각도 | Joy-IT: 최대 7 m, 각도 <120° | 주변 온도·감도·기판에 따라 변동 |
| 출력 유지 | Espruino: 대략 2 s~5분 이상(가변), HSHL은 별도 수치 | **0.3 s~5분**처럼 판매처마다 다른 범위는 단일 보증값으로 사용 금지 |
| 워밍업 | HSHL: 전원 후 **30~60 s** 동안 OUT이 여러 차례 HIGH가 될 수 있음 | 소프트웨어에서 약 60 s 입력 무시(설계안) |
| 소비전류 | HSHL: 동작 **65 mA**, 대기 **<50 µA**; Espruino: 동작 **<50 µA** | **서로 상충.** 측정 조건·순간 전류·기판 차이 미확인. 전원 예산의 확정값으로 사용 금지 |
| 동작 온도 | HSHL: -15~+70 °C | 해당 자료 기준(D3) |

**전류 사양 주의:** 이전 문서의 '65 mA(동작), 50 µA(대기)'는 HSHL 표에 실제로 있지만 Espruino 설명과 다르다. 단순히 '65 mA는 오타'라고 단정하지 않고 **상충·미확인**으로 표시한다. 65 mA와 50 µA는 1,300배 차이이므로 배터리·전원 예산에는 실제 모듈 전류를 측정해야 한다.

## 모양·명칭이 비슷해도 다른 모듈

| 제품 | 실제 신호/전원 | HC-SR501과의 차이 |
|---|---|---|
| **HC-SR501** 일반 3핀 모듈 | VCC/OUT/GND, 디지털 출력. 흔히 외부 5 V 전원 | OUT 전압·오픈드레인 여부는 실제 기판 확인 |
| [Seeed Grove PIR Motion Sensor](https://wiki.seeedstudio.com/Grove-PIR_Motion_Sensor/) | **Grove 4선: SIG/NC/VCC/GND**, 제조사 전원 3~5 V, 3 V에서 100 µA·5 V에서 150 µA | 기능은 비슷하지만 **별도 기판**. Seeed의 전류값을 HC-SR501에 적용 금지 |
| [Arduino Modulino Movement](https://docs.arduino.cc/hardware/modulino-movement/) | **LSM6DSOXTR 6축 가속도·자이로**, Qwiic/I²C | 이름의 Movement는 PIR가 아님. 단순 OUT GPIO로 읽을 수 없음 |
| [Keyestudio Project 47](../capstone/README.md) DIY PIR | ESP32용 PIR + LED/부저 튜토리얼 | 원본에 사용된 **DIY PIR 보드**를 HC-SR501과 동일 기판으로 간주 금지 |

## Basys3 + MicroBlaze V 연결 (실물 연결 전 점검)

1. **전원과 신호를 분리**: VCC는 구매한 HC-SR501의 전원 정격에 맞는 외부 전원(일반형은 5 V)에서 공급하고, Basys3와 GND를 공통으로 한다. VCC 5 V가 OUT 5 V라는 뜻도, OUT 3.3 V라는 보장도 아니다.
2. **OUT 실측·회로 확인**: FPGA에서 분리한 상태로 OUT의 LOW/HIGH, 풀업 전원, 출력 방식(푸시풀/오픈드레인)을 확인한다. **3.3 V GPIO 입력 한도를 넘으면 직접 연결 금지**. 오픈드레인 변형의 외부 풀업은 적절한 **3.3 V** 전원으로만 검토한다.
3. **연결 승인 후**: OUT → Basys3 GPIO 입력 1비트. PIR은 비동기 입력이므로 2-FF 동기화 후 상승/하강 이벤트를 검출한다. AXI GPIO 자체의 동기화 보장 여부는 미확인이다.
4. **XADC와 구분**: PIR의 디지털 OUT은 XADC 0~1 V 아날로그 입력이 아닌 **3.3 V 디지털 GPIO**로 연결한다. HC-SR04의 5 V ECHO와도 별개다.
5. **동작 확인**: 워밍업 60 s 동안 경보를 막고, 미감지/감지/유지시간/재트리거/전원 재투입을 시험한다. 점퍼 모드에 따라 감지 출력 유지 시간이 달라질 수 있다.

## 코드·검증 범위

- [RTL10 PIR 이벤트](../../rtl/rtl10_pir_event/README.md): `pir_event.v`(2-FF 동기화·에지 펄스), `tb_pir_event.v`(자체 기대값 비교 테스트벤치). **테스트벤치 존재는 보드 검증을 의미하지 않는다.**
- [캡스톤 01](../capstone/README.md): PIR+HC-SR04 보안 제어 확장 **설계안**. PIR 자체를 위한 전용 C 드라이버는 없음(AXI GPIO 후보).
- Joy-IT 공식 페이지는 **제품 사양/데이터시트 다운로드 링크**를 제공하지만 실제 구매한 저가 호환 보드의 회로를 증명하지 않는다. 원본 회로·사진·코드는 복제하지 않고 링크로만 참조한다.
- **미확인:** 구매 모듈의 기판 버전/IC·실제 OUT 전압·풀업/출력단·소비전류·핀 순서, RTL 시뮬레이션·합성·Timing·실물 보드 동작.

<!-- nav-foot -->[gpio 목차](README.md)
