# HC-SR04 초음파 거리 센서

<!-- nav -->[저장소 홈](../../README.md) › [devices](../README.md) › [gpio](README.md)
- 분류: 거리 · 인터페이스: GPIO(Trig 입력, Echo 출력) · 근거: D1 (제조사 데이터시트 사본)

## 전기 사양 (확인)
| 항목 | 값 |
|---|---|
| 동작 전압 | DC 5 V |
| 동작 전류 | 15 mA |
| 측정 범위 | 최소 2 cm, 최대 4 m |
| 측정 각도 | 15도 |
| Trig 입력 | 10 µs TTL 펄스 |
| Echo 출력 | TTL 신호, High 시간이 거리에 비례 |
| 거리 식 | 거리[cm] = High 시간[µs] / 58 |
| 측정 주기 | 60 ms 초과 권장 |

사본에 동작 주파수가 "40Hz"로 적혀 있으나 같은 문서가 40 kHz 초음파를 언급하므로 오기로 보임. 이 값은 쓰지 않음.

## 실제 모듈 변형 대조 (2026-11-02, 제조사·공식 제품 문서 확인)

**같은 '거리 센서'여도 부품·전원·프로토콜은 동일하지 않습니다.** 기존 `ultrasonic_distance_meter.v`는 **TRIG 출력 1선 + ECHO 입력 1선** 방식만 대상으로 합니다.

| 실제 제품·식별 | 전원 / 신호 | 핀·통신 | 기존 2선 RTL 재사용 |
|---|---|---|---|
| [SparkFun SEN-15569](https://www.sparkfun.com/ultrasonic-distance-sensor-hc-sr04.html), 일반 HC-SR04 | **5 V 동작**. [Adafruit 핀 설명](https://learn.adafruit.com/ultrasonic-sonar-distance-sensors/pinouts)에서는 일반형 **ECHO 5 V 로직**, TRIG는 3 V 또는 5 V 입력 가능 | VCC / TRIG / ECHO / GND, 펄스 폭 | **전압 변환 뒤** 재사용 가능(설계안) |
| [SparkFun SEN-24049](https://www.sparkfun.com/ultrasonic-distance-sensor-3-3v-hc-sr04.html), **HC-SR04-33** | 제품 페이지의 동작 전원 **3.3~5 V**. ECHO HIGH 실제 전압은 **사용 전원·해당 기판 회로로 별도 확인** | VCC / TRIG / ECHO / GND | 신호 전압 확인 후 논리 재사용 후보(실물 미검증) |
| [Seeed Grove Ultrasonic Ranger V2](https://wiki.seeedstudio.com/Grove-Ultrasonic_Ranger/) | **3.2~5.2 V**, V2는 3.3 V 시스템 호환으로 명시 | **SIG 1선이 TRIG/ECHO를 시간 분할 공유**, Grove 4핀 커넥터 | **그대로 연결 불가**. 양방향 SIG 제어·방향 전환 로직 별도 필요 |
| [Arduino Modulino Distance](https://docs.arduino.cc/hardware/modulino-distance) ([ABX00102 공식 데이터시트](https://docs.arduino.cc/resources/datasheets/ABX00102-datasheet.pdf)) | **VL53L4CDV0DH/1 ToF IC**, Qwiic **3.3 V 전원**. 센서 권장 공급 전압은 **2.6~3.5 V**(제품 데이터시트) | **I²C SDA/SCL**, Qwiic 4핀. HC-SR04의 TRIG/ECHO 펄스 방식 아님 | **재사용 불가**. 3.3 V I²C 풀업/실제 기판·커넥터 핀 확인 후 별도 컨트롤러·드라이버 필요 |

**Modulino 라이브러리 버전 주의(2026-11-03):** [Arduino_Modulino v0.10.0 소스](https://github.com/arduino-libraries/Arduino_Modulino/blob/24d08a94ac4e8f83aed211df0fa6f38e9994b164/src/Modulino.h)는 거리 센서의 I²C 주소 `0x29`를 확인하고 `VL53L4CD` 초기화 실패 시 `VL53L4ED`를 시도합니다. 이는 **라이브러리 호환 코드**이며, ABX00102 실제 PCB에 두 칩이 모두 실장된다는 뜻이 아닙니다. 실제 하드웨어 식별은 위 Arduino 공식 데이터시트를 기준으로 합니다. 라이브러리의 `MPL-2.0` 라이선스는 원본 [LICENSE](https://github.com/arduino-libraries/Arduino_Modulino/blob/24d08a94ac4e8f83aed211df0fa6f38e9994b164/LICENSE)에서 확인했으며, 코드를 복제하지 않았습니다.

**UNO R4 ↔ Basys3 차이:** [Arduino UNO R4 Minima 공식 문서](https://docs.arduino.cc/hardware/uno-r4-minima)는 보드 I/O 동작 전압이 **5 V**라고 명시합니다. UNO R4에서 직접 연결되던 일반 HC-SR04 ECHO를 Basys3의 3.3 V GPIO에 같은 방식으로 직결하면 안 됩니다.

### 국내 판매처 교차검증 — 제목만으로 3.3 V 호환 판단 금지

- [아이씨팩토리 HC-SR04플러스 ICF1588](https://icfactory.cafe24.com/product/hc-sr04%ED%94%8C%EB%9F%AC%EC%8A%A4-33v5v-%EC%B4%88%EC%9D%8C%ED%8C%8C-%EA%B1%B0%EB%A6%AC-%EC%B8%A1%EC%A0%95-%EA%B0%90%EC%A7%80-%EC%84%BC%EC%84%9C-%EB%AA%A8%EB%93%88-icf1588/827/category/73/display/1/): 상품 페이지에 **HC-SR04+ / SGM324 / 3~5.5 V**라고 기재. **판매처 설명(D3)**이며 실제 ECHO HIGH 전압과 PCB 회로는 미확인.
- [EKU 'HC-SR04 3V~5.5V' 상품](https://eku.co.kr/product/hc-sr04-%EC%B4%88%EC%9D%8C%ED%8C%8C-%EA%B1%B0%EB%A6%AC-%EC%84%BC%EC%84%9C-%EB%AA%A8%EB%93%88-3v-55v-%ED%98%B8%ED%99%98/149/): 제목은 **3~5.5 V 호환**, 같은 상세 설명에는 **DC 5 V 동작 / HIGH 5 V 출력**이라고 쓰여 있어 **페이지 내부 모순(P1)**. 실물 모델 확인 전 3.3 V 직결 후보로 승인하지 않음.
- 위 두 상품은 서로 같은 PCB·회로라는 근거가 없습니다. 국내 판매처 가격·재고·표제는 데이터시트나 보드 레벨 변환 회로의 증거가 아닙니다.

### Basys3 3.3 V GPIO 연결 예 (일반 5 V ECHO형에 한함)

```text
HC-SR04 VCC  ────────────── 외부 5 V (전류 용량 확인)
HC-SR04 GND  ────────────── Basys3 GND (공통 접지)
Basys3 TRIG  ────────────── HC-SR04 TRIG (입력 HIGH 임계 확인)
HC-SR04 ECHO ── Rtop 10kΩ ──┬── Basys3 ECHO 입력
                            │
                         Rbottom 10kΩ
                            │
                           GND
```

- **계산:** 5 V ECHO HIGH → `5 × 10/(10 + 10)`의 결과는 약 **2.5 V**입니다. [Adafruit](https://learn.adafruit.com/ultrasonic-sonar-distance-sensors/pinouts)가 소개한 저항 2개 방식의 **예시**입니다. 2.5 V가 실제 FPGA의 입력 HIGH 임계와 충분한 여유를 갖는지, 저항 오차·노이즈·배선 길이를 확인해야 합니다.
- **P0 금지:** 일반 5 V ECHO → FPGA Pmod 직결 금지. 센서 VCC 5 V를 Basys3 Pmod 3.3 V 공급핀에 연결하지 않습니다. 전원은 외부에서 공급하고 공통 GND를 연결합니다.
- **전원 인가 전:** 제품 모델·PCB 인쇄·실제 핀 순서·ECHO HIGH 전압을 먼저 확인합니다. 판매처가 단순히 'HC-SR04'라고만 표시하면 3.3 V 호환형으로 간주하지 않습니다.
- **XADC 구분:** ECHO는 **디지털 GPIO 입력**이지 XADC 입력이 아닙니다. [AMD UG480](https://docs.amd.com/r/en-US/ug480_7Series_XADC/Instantiating-the-XADC)의 일반 단극성 XADC 측정 범위는 **0~1 V**이므로, 이 분압 예의 2.5 V를 XADC에 연결하면 안 됩니다.

**근거 수준:** 제품별 정격·프로토콜은 위 제조사/공식 문서에서 직접 확인(D1). 저항 분압 값은 계산. Basys3 핀 연결·실물 HIGH 전압·레벨 마진·모듈 변형은 설계안/미검증입니다. 판매 가격·재고는 기준 사양으로 기록하지 않습니다.

## SoC 연결 (Basys3 + MicroBlaze V)
| 항목 | 내용 | 구분 |
|---|---|---|
| 핀 | Trig 출력 1 + Echo 입력 1 = Pmod 2핀 | 확인(신호 구성) |
| 전압 | 센서는 5 V 동작. Echo가 5 V 레벨이면 FPGA 입력 전에 분압 또는 레벨 시프터 필요 | 설계안 (5 V 비허용은 일반 지식) |
| Echo 폭 측정 | AXI Timer 캡처 모드 또는 클럭 카운터 RTL | 모드 존재는 PG079 목차 확인, 구성은 설계안 |
| Trig 생성 | AXI GPIO 출력 + 소프트웨어 지연, 또는 RTL 펄스 생성기 | 설계안 |
| 최대 Echo 폭 | 4 m × 58 µs/cm = 23.2 ms → 100 MHz 클럭 기준 2,320,000 카운트 | 계산 |
| 인터럽트 | Echo 하강 에지 캡처 시 타이머 인터럽트 | 설계안 |

## 전용 RTL
사용자 노션의 `ultrasonic_distance_meter`를 AXI4-Lite로 감싸 `rtl/`에 넣었습니다. 레지스터와 확인 필요 항목은 `rtl/README.md`. 노션 강의자료도 같은 식(µs/58, 100 MHz에서 echo_count/5800)과 5 V 에코 주의를 적고 있습니다(제조사 문서가 아닌 사용자 자료).

## 근거와 한계
드라이버 `sw/ultrasonic.[ch]`는 호스트 모의 레지스터 시험만 통과했습니다(코어의 에코 타이밍·실물 동작은 미검증. RTL 소스에서는 enable을 끄면 측정 상태가 초기화되고 재활성화 시 첫 1 µs tick 이후 새 측정을 시작하도록 작성됨). 2~400 cm 밖의 값은 오류로 돌려줍니다(설계안).

출처: HC-SR04 제조사 데이터시트 사본(미시안티 호스팅). 실물 모듈 변종(3.3 V 호환형 등)은 다루지 않음.
