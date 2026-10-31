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

| 온도 | DHT22·DS18B20·LM35 모듈(책 16장) | [Pmod TMP2](https://digilent.com/reference/pmod/pmod/tmp2/ref_manual) (ADT7420, I2C, D3) | LM35·DHT22·DS18B20 드라이버([gpio/](gpio/)). TMP2 드라이버 없음 |
| 습도 | DHT22·BME280 모듈(책 16장) | [Pmod HYGRO](https://digilent.com/reference/pmod/pmodhygro/reference-manual) (HDC1080, I2C 0x40, D1 요약 경유) | DHT22([gpio/dht22.md](gpio/dht22.md)), BME280([i2c/sensors.md](i2c/sensors.md)). HYGRO 드라이버 없음 |
| 거리 | HC-SR04 모듈(책 16장) | 해당 없음(디질런트에 초음파 Pmod 없음) | [gpio/hc-sr04.md](gpio/hc-sr04.md) 완비 |
| 조도 | CdS 분압(책 16장), BH1750 I2C 모듈 | [Pmod ALS](https://reference.digilentinc.com/pmod:pmod:als:ref_manual) (TEMT6000X01+ADC081S021, SPI, D1 요약 경유) | CdS 변환([analog-sensors.md](gpio/analog-sensors.md)), BH1750([i2c/sensors.md](i2c/sensors.md)). ALS 드라이버 없음 |
| 모션(가속도·자이로) | MPU-6050·ADXL345 모듈(책 24장) | [Pmod ACL2](https://reference.digilentinc.com/pmod:pmod:acl2:ref_manual) (ADXL362, SPI, D1 요약 경유) / [Pmod GYRO](https://digilent.com/reference/pmod/pmod/gyro/ref_manual) (L3G4200D, SPI, D3) | MPU-6050·ADXL345([i2c/sensors.md](i2c/sensors.md)) 완비. Pmod 드라이버 없음 |
| 서보 | SG90(책 26장) | 해당 없음(디질런트에 서보 전용 Pmod 없음) | [motor/sg90.md](motor/sg90.md), 전용 RTL. RTL 미시뮬레이션 |
| 스테퍼 | 28BYJ-48+ULN2003(책 27장) | 해당 없음(디질런트에 스테퍼 전용 Pmod 없음) | [motor/28byj-48.md](motor/28byj-48.md), 드라이버·모델 시험 있음(완비, 실물 미확인) |
| 시계(RTC) | DS3231·DS1307 모듈(I2C, 책 22장) | [Pmod RTCC](https://reference.digilentinc.com/pmod/pmod/rtcc/old_ref_manual) (MCP79410, I2C/SPI, D3) | DS3231([i2c/clock.md](i2c/clock.md)), DS3234([spi/clock.md](spi/clock.md)) 완비. RTCC 드라이버 없음 |
| 저장(SD) | microSD SPI 모듈(책 20장) | [Pmod MicroSD](https://digilent.com/reference/pmod/pmodmicrosd/reference-manual) (microSD, SPI) / [Pmod SD](https://reference.digilentinc.com/pmod/pmod/sd/ref_manual) (full-size SD, SPI) | microSD 미확인([spi/comm-storage-io.md](spi/comm-storage-io.md) 언급만) |
| 저장(직렬 플래시) | W25Q64JV SPI 플래시 모듈 | Pmod MEM (SPI 플래시, D3 — 칩·가용성 미확인) | W25Q64JV([spi/comm-storage-io.md](spi/comm-storage-io.md)) 완비 |
| 오디오 | 수동 부저(책 17장), MAX98357A I2S 앰프 모듈, INMP441 I2S 마이크 모듈 | [Pmod AMP3](https://reference.digilentinc.com/pmod:pmod:amp3:ref_manual) (SSM2518 Class-D, I2S+I2C, D1 요약 경유) / [Pmod I2S2](https://reference.digilentinc.com/pmod/pmodi2s2/reference-manual) (CS4344 DAC+CS5343 ADC, D3) | 부저([gpio/buzzer.md](gpio/buzzer.md)), MAX98357A·INMP441([i2s/devices.md](i2s/devices.md)). Pmod 드라이버 없음 |
| CAN | MCP2515+SN65HVD230 모듈(책 21장) | [Pmod CAN](https://digilent.com/reference/pmod/pmodcan/start) (MCP2515+TJA1050, D3) | MCP2515 드라이버([can/](can/README.md)) 완비 |
| 이더넷 | W5500 모듈(책 30장) | [Pmod NIC100](https://reference.digilentinc.com/pmod/pmodnic100/reference-manual) (ENC28J60, SPI) | W5500([ethernet/](ethernet/README.md)) 완비 |
| GPS | NEO-6M UART 모듈 | [Pmod GPS](https://digilent.com/reference/pmod/pmod/gps/ref_manual) (MediaTek MT3329, UART 9600, D1 요약 경유) | NEO-6M NMEA 파서([uart/](uart/)) 있음 |
| ADC | MCP3008 8ch 10-bit SPI 모듈, ADS1115 4ch 16-bit I2C 모듈 | Pmod AD1 (AD7476A, 12-bit SPI, D3) / Pmod AD2 (AD7991, 4ch 12-bit I2C, D3) | MCP3008([spi/sensor-adc-dac.md](spi/sensor-adc-dac.md)), ADS1115([i2c/ads1115.md](i2c/ads1115.md)) 완비 |
| DAC | MCP4921 12-bit SPI 모듈 | Pmod DA3 (AD5446, 14-bit SPI, D3) / Pmod DA4 (AD5628, 8ch 12-bit SPI, D3) | MCP4921([spi/sensor-adc-dac.md](spi/sensor-adc-dac.md)) 완비 |
| OLED 표시 | SSD1306 I2C/SPI 모듈(책 23장) | [Pmod OLED](https://reference.digilentinc.com/pmod/pmod/oled/ref_manual) (SSD1306, SPI, 128×32 단색) | SSD1306([i2c/ssd1306.md](i2c/ssd1306.md)) 완비. Pmod 쪽은 SPI 모드 |
| TFT 표시 | ILI9341 SPI TFT 모듈 | 해당 없음(디질런트에 동급 Pmod 없음) | ILI9341([spi/display.md](spi/display.md)) RTL·드라이버 있음 |

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

## 새로 조사한 분류 (2026-10-30)

**온도·습도 — 아두이노 모듈이 주류, Pmod HYGRO·TMP2 는 I2C 방식**
- Pmod HYGRO: HDC1080, I2C 주소 0x40, 습도+온도 복합, 2.7~5.5 V(D1 요약 경유). 저장소의 DHT22·BME280 과 측정 항목이 겹쳐 새 드라이버 우선순위는 낮다.
- Pmod TMP2: ADT7420, I2C 16-bit 온도 전용(D3, 상품명 기반). 참조 매뉴얼은 열지 않음.
- 거리 분류: HC-SR04 가 저장소에 완비되어 있고 동급 초음파 Pmod 가 없어 2순위 선택지 없음.

**조도 — Pmod ALS 는 SPI 방식, 저장소 BH1750(I2C)과 인터페이스가 다름**
- TEMT6000X01 광트랜지스터 + ADC081S021 12-bit ADC, SPI Mode 3, SCLK 1~4 MHz(D1 요약 경유). 출력은 원시 ADC 값(0~255)이며 lux 환산식이 없다. 저장소의 CdS 분압 방식과 같은 한계(lux 환산 미확인).

**모션(가속도·자이로) — 저장소 MPU-6050(I2C 복합)과 달리 Pmod 두 개가 각각 SPI**
- Pmod ACL2(ADXL362): 3축 ±2/±4/±8 g, 초저전력, SPI, 3.3 V(D1 요약 경유). 저장소 ADXL345(SPI) 드라이버와 칩이 달라 레지스터 맵이 다름.
- Pmod GYRO(L3G4200D): 3축 250/500/2000 dps, SPI 또는 I2C 선택(D3). 저장소에 자이로 전용 드라이버 없음.

**서보·스테퍼 — 디질런트에 전용 Pmod 없음, GPIO/PWM 직결**
- SG90 서보와 28BYJ-48+ULN2003 스테퍼 모두 저장소에 문서 있음. Pmod 를 쓰는 대신 Pmod GPIO 헤더를 직접 배선하는 방식이 필요.

**시계(RTC) — Pmod RTCC(MCP79410)는 저장소 DS3231·DS1307 과 칩이 다름**
- MCP79410: I2C 와 SPI 겸용, 배터리 백업 핀, 1 KB EEPROM 내장(D3, 상품 설명 기반). 저장소 드라이버를 그대로 쓸 수 없고, 새 드라이버가 필요.

**저장(SD) — Pmod SD(full-size)·Pmod MicroSD 두 종, 공통 SPI 인터페이스**
- 둘 다 SPI 로 SD 카드에 접근. 저장소에 아직 microSD 드라이버가 없어 이 분류의 1·2순위 모두 구현 전이다.

**오디오 — Pmod AMP3·Pmod I2S2 모두 I2S, 저장소 MAX98357A·INMP441 과 인터페이스 동일**
- Pmod AMP3: SSM2518, I2S 입력 + I2C 설정(선택 사항), 2 W Class-D 출력, 2.5~5.5 V, 12핀 Pmod(J1+J2 두 포트 사용)(D1 요약 경유).
- Pmod I2S2: CS4344 DAC + CS5343 ADC, 3.5 mm 잭 제공(D3, 상품명 기반). 저장소 I2S 드라이버를 핀 연결 변경으로 전용할 가능성 있음(설계안).

**CAN — Pmod CAN 도 MCP2515, 트랜시버만 다름(TJA1050 vs SN65HVD230)**
- 저장소 MCP2515 드라이버를 Pmod CAN 에 바로 쓸 수 있을 가능성이 높음(설계안). TJA1050 은 5 V 를 선호하는 설계가 있어 Basys3 3.3 V 전원과의 적합성 확인 필요(미확인).

**이더넷 — W5500(하드웨어 TCP/IP 내장) vs Pmod NIC100 ENC28J60(스택 직접 구현 필요)**
- ENC28J60 은 MAC+PHY 만 제공하며 IP·ARP·TCP·UDP 스택을 SoC 에서 전부 처리해야 함(D3, Microchip 2차 자료). 저장소의 W5500 소켓 드라이버와 구조가 완전히 달라 별도 구현이 필요. 이더넷 응용이 이미 W5500 으로 완비되어 있어 NIC100 추가 우선순위는 낮다.

**GPS — Pmod GPS(MT3329 UART 9600)는 저장소 NEO-6M NMEA 파서와 인터페이스 호환**
- 두 모듈 모두 UART 9600, NMEA 0183 출력(D1 요약 경유). 저장소 NMEA 파서를 핀 연결만 바꿔 그대로 쓸 수 있을 가능성이 높다(설계안).

**ADC·DAC — 저장소에 완비, Pmod 대안도 같은 SPI/I2C 계열**
- Pmod AD1(AD7476A 12-bit SPI), Pmod AD2(AD7991 4ch 12-bit I2C), Pmod Da3(AD5446 14-bit SPI), Pmod DA4(AD5628 8ch 12-bit SPI) — 모두 D3 수준, 참조 매뉴얼 링크만 확인. 저장소에 이미 동급 드라이버가 있어 우선순위 낮음.

**OLED·TFT 표시 — Pmod OLED 는 SSD1306 SPI 128×32, 칩은 같으나 인터페이스가 다름**
- 저장소 SSD1306 드라이버는 I2C 기반. Pmod OLED 에 쓰려면 SPI 초기화 순서와 D/C 핀 제어가 추가됨(설계안). TFT(ILI9341)는 동급 Pmod 없음.

## 한계
- 디바이스마트·에듀이노 페이지는 회로 근거가 되지 못했습니다(상품 설명 수준).
- 디질런트 매뉴얼의 "회로도 링크"는 요약에 URL 이 없어 BT2 하나만 제3자 사본으로 읽었습니다. HB5·BLE·CLS 회로도는 읽지 못했습니다.
- 위 대응표는 사용자가 "확정"했다고 한 실제 모듈의 회로도가 아닙니다. 확정본을 받으면 이 표와 모듈 문서를 그것으로 다시 대조합니다([TODO.md](TODO.md)).
- 2026-10-30 조사에서 참조 매뉴얼을 **열지 않은** 항목(D3 수준): Pmod TMP2, Pmod GYRO, Pmod RTCC, Pmod MEM, Pmod I2S2, Pmod CAN, Pmod AD1, Pmod AD2, Pmod Da3, Pmod DA4. 이 항목들의 핀 배치·타이밍·레지스터 맵 값은 상품명/2차 자료 기반이며, 드라이버 작성 전 매뉴얼 열람이 필요합니다.
- Pmod MEM 의 현재 판매 여부와 SPI 플래시 칩 종류는 미확인입니다.
- Pmod NIC100 ENC28J60 의 최대 SPI 클럭은 미확인입니다([TODO.md](TODO.md) 기존 항목).

<!-- nav-foot -->[devices 목차](README.md)
