# 주변장치 선정 기준과 분류별 대응

<!-- nav -->[저장소 홈](../README.md) › [devices](README.md)

## 기준 (사용자 지정)
1. **1순위: 아두이노** — 책(상상을 스케치하는 아두이노 우노 R4)이 쓰는 아두이노용 주변 모듈과 아두이노 공식 보드·쉴드.
2. **2순위: 디질런트(Digilent)** — Pmod 모듈. Basys3 의 Pmod 커넥터(3.3 V)에 바로 꽂히고, 참조 매뉴얼·회로도가 공개되어 있다.
3. 그 밖의 소자는 1·2순위에 대응품이 없을 때만 쓴다.

이 기준은 [BOOK-MAP.md](BOOK-MAP.md) 의 순서(책 기준)와 같은 방향입니다. 모듈 보드의 회로(전원·레벨·점퍼)는 칩 데이터시트와 따로 확인합니다([VERIFICATION.md](VERIFICATION.md) 행 27~29).
**근거 표기**: 아래 값은 모두 웹 가져오기 요약을 거친 것입니다. 회로도 PDF 는 열린 것만 "읽음"으로 적고, 열지 못한 것은 링크만 적었습니다.

## 조사한 분류
| 분류 | 1순위 (아두이노 계열) | 2순위 (디질런트 Pmod) | 저장소 상태 |
|---|---|---|---|
| DC 모터 | 일반 L298N 모듈(책 25장 취지), 아두이노 공식 [Motor Shield Rev3](https://store.arduino.cc/collections/core-family/products/arduino-motor-shield-rev3) | [Pmod HB5](https://digilent.com/reference/pmod/pmod/hb5/ref_manual) | L298N 모듈은 [l298.md](motor/l298.md), RTL·드라이버 있음(미시뮬레이션). 나머지는 이 문서에만 |
| 블루투스 클래식 | HC-06(책 28장), HC-05 | [Pmod BT2](https://reference.digilentinc.com/pmod:pmod:bt2:ref_manual) (RN-42, SPP) | HC-05·HC-06 드라이버([uart/](uart/modules.md)). BT2 는 드라이버 없음 |
| 저전력 블루투스 | HM-10 계열(Arduino 주변 모듈), 책 29장은 ESP32-S3 | [Pmod BLE](https://reference.digilentinc.com/pmod/pmodble/reference-manual) (RN4871) | HM-10 드라이버. BLE Pmod 는 드라이버 없음 |
| 문자 LCD | HD44780 16×2 + PCF8574 백팩(책 15·19장) | [Pmod CLS](https://reference.digilentinc.com/pmod/pmodcls/reference-manual) (16×2, 시리얼 명령) | HD44780 드라이버([lcd1602.md](i2c/lcd1602.md)). CLS 는 드라이버 없음 |

미조사: 위 4개 밖의 분류(센서·모션·시계·저장·오디오·CAN·이더넷·GPS 등)는 이 기준으로 대응품을 아직 찾지 않았습니다. [CATEGORIES.md](CATEGORIES.md) 의 소자 목록은 기준 도입 전에 정한 것이라 "1순위"라고 단정할 수 없습니다.

## 조사 결과 (저장소에 미치는 영향)
**DC 모터 — 세 모듈의 구동 방식이 다르다**
| | 일반 L298N 모듈 | 아두이노 Motor Shield Rev3 | Pmod HB5 |
|---|---|---|---|
| 드라이버 칩 | L298(N) | **L298P** | 문서에 칩 이름 없음("full H-bridge") |
| 제어 | EN(PWM) + IN1 + IN2 | 채널당 방향 · PWM · 브레이크 3핀 (A: D12 · D3 · D9, B: D13 · D11 · D8) | **DIR + EN 2핀**, 브레이크 모드 없음 |
| 전류 감지 | SENSE 가 GND 직결 → **불가** | 채널당 A0/A1, **1.65 V/A, 2 A 에서 3.3 V**, 뒷면 점퍼를 자르면 해제 | 없음. 대신 SA/SB 엔코더 피드백 2핀 |
| 모터 전원 | 칩 46 V, 모듈 약 35 V | 5~12 V (Vin 단자 최대 18 V, 권장 7~12 V) | 최대 12 V, 2 A |
| 논리 전원 | 5 V(Pmod 3.3 V 로는 불가) | 아두이노 보드(Vin Connect 점퍼) | Pmod VCC 3.3 V 권장(외부 2.7~5.25 V) |
- Pmod HB5 매뉴얼은 **"Enable 이 High 인 동안 Direction 을 바꾸면 안 된다"** 고 경고합니다. 저장소 `dcmotor_engine.v` 의 방향 전환 보호(코스트를 거쳐 전환, `DEAD_US`)가 같은 이유로 필요하다는 독립 근거가 생겼습니다(경고 문장과 엔진 동작이 방향상 일치한다는 뜻이고, RTL 은 시뮬레이션하지 못했습니다).
- HB5 에는 `mode` 3(브레이크)에 해당하는 동작이 없고 IN1/IN2 2핀이 아니라 DIR/EN 입니다. 같은 RTL 의 `motor_pwm`→EN, `motor_in1`→DIR, `motor_in2`는 미사용으로 연결하면 되는지는 **모터 쪽 회로 확인 전이라 설계안**입니다.
- 아두이노 쉴드의 핀 배치·감지 스케일은 스토어 페이지(요약)에서 읽었고, **회로도 PDF(arduino.cc)는 이 환경에서 열 수 없어** 브레이크·방향이 L298P 의 어느 핀에 어떻게(게이트 경유 여부) 연결되는지, 다이오드·풀다운은 미확인입니다.

**블루투스 — Pmod 쪽이 전원·회로 근거가 낫다**
- [Pmod BT2 회로도](https://www.eetree.cn/wiki/_media/pmodbt2_sch.pdf)(제3자 호스팅 사본, **읽음**): Pmod 6번·12번 핀이 모두 **VCC3V3**, 레귤레이터 없음, UART 신호선마다 **200 Ω 직렬저항**과 버퍼 IC(NL37WZ17) 경유, /RESET 10 kΩ 풀업. → 매뉴얼 핀표의 "3.3V/5V" 표기는 회로와 맞지 않고, 본문의 "외부 전원 3.0~3.6 V"가 회로와 일치합니다. **5 V 를 넣으면 RN-42 가 상할 수 있습니다.**
- Pmod BT2: 기본 **115200 8N1**, 명령 모드는 `$$$`(응답 `CMD`), 나갈 때 `---`+CR(응답 `END`), STATUS 핀은 연결되면 High. Pmod BLE(RN4871): 기본 115200 8N1, `$$$`(응답 `CMD> `), UART 투명 전송 서비스 포함, 전원 1.9~3.6 V.
- 저장소의 HC-06(9600)·HM-10(9600) 드라이버와 달리 **115200** 이라 UART Lite 보레이트를 따로 합성해야 하고([uart/README.md](uart/README.md)), `$$$` 진입 방식이라 `btmod`(무음 구간) 위에 RN-42/RN4871 명령 층이 별도로 필요합니다. 아직 만들지 않았습니다(Roving Networks 명령 레퍼런스를 열지 않음).
- 두 Pmod 모두 매뉴얼 안에서 전원 표기가 서로 어긋납니다(핀표 "3.3V/5V" 대 사양 3.0~3.6 V / 1.9~3.6 V). 회로도로 BT2 는 해소했고, BLE 는 회로도를 열지 않아 미해소입니다.

**문자 LCD — Pmod CLS 는 HD44780 드라이버를 쓸 수 없다**
- Pmod CLS 는 16×2 LCD 앞에 **ATmega48 이 있어** 시리얼(UART 2400/4800/9600, TWI 주소 0x48, SPI)로 `ESC [ …` 이스케이프 명령을 받습니다. HD44780 에 직접 쓰는 `hd44780_*` 와 접근 방식이 다릅니다. 모드는 점퍼 MD0~MD2 로 정하고, Rev E 는 점퍼가 없으면 1, 있으면 0 입니다(매뉴얼). 전원 2.7~5.5 V, VCC 로 급전할 때는 3.3 V 여야 내장 승압 레귤레이터가 동작합니다. 기본 보레이트는 매뉴얼에 없습니다.

## 한계
- 디바이스마트·에듀이노 페이지는 회로 근거가 되지 못했습니다(상품 설명 수준).
- 디질런트 매뉴얼의 "회로도 링크"는 요약에 URL 이 없어 BT2 하나만 제3자 사본으로 읽었습니다. HB5·BLE·CLS 회로도는 읽지 못했습니다.
- 위 대응표는 사용자가 "확정"했다고 한 실제 모듈의 회로도가 아닙니다. 확정본을 받으면 이 표와 모듈 문서를 그것으로 다시 대조합니다([TODO.md](TODO.md)).

<!-- nav-foot -->[devices 목차](README.md)
