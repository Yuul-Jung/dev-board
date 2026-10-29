# I2C 소자와 SoC 연결

구분: **확인** = 이번 작업에서 데이터시트를 열어 확인, **계산**, **설계안**, **미확인**. 소자 문서: [sensors.md](sensors.md), [clock.md](clock.md), 기존 [bme280.md](bme280.md).

## AXI IIC (PG090 v2.1, 2024-12-18) — 확인 (문서 본문 열람)
| 항목 | 값 |
|---|---|
| 속도 | 표준 100 kHz, Fast 400 kHz, Fast-Mode Plus 1 MHz |
| 기능 | 마스터/슬레이브/멀티 마스터, 7/10비트 주소, 동적 시작·정지 생성, GPO 포트(1~8비트) |
| 디바이스 | 지원 계열에 7 series 포함, Artix-7 최대 주파수 표 있음 |
| 인터페이스 | AXI4-Lite |
| 3상태 포트 | sda_i, sda_o, sda_t, scl_i, scl_o, scl_t (t = 3상태 출력 활성). IOBUF는 상위에서 연결 (IP Integrator에서는 자동 삽입되는 `iic` 인터페이스 사용) |
| 파라미터 | SCL Clock Frequency(IIC_FREQ_KHZ, 기본 100), Address Mode(7/10), SCL/SDA Inertial delay(0~255 AXI clk), AXI Clock Frequency(25~300 MHz), GPO 폭·기본값 |

**SCL 타이밍 (확인)**: THIGH/TLOW 레지스터 기본값은 IP가 설정한 SCL 주파수와 AXI 클럭으로 자동 계산됩니다. 식은 `값 = (AXI 클럭 Hz / (2 × IIC Hz)) − 7 − SCL_INERTIAL_DELAY`. 문서 예시: 25 MHz / 100 kHz / 지연 0 → 118.
계산: AXI 100 MHz, 400 kHz, 지연 0 → 100,000,000 / 800,000 − 7 = 118. 맞지 않으면 레지스터를 읽어 조정(문서 설명). TSUSTA 등은 I2C 규격 시간 + Tr 을 클럭 수로 환산(예: 4.7 µs + 1 µs, 100 MHz → 570).

레지스터 (문서 표): 0x01C GIE · 0x020 ISR · 0x028 IER · 0x040 SOFTR · 0x100 CR · 0x104 SR · 0x108 TX_FIFO · 0x10C RX_FIFO · 0x110 ADR · 0x114 TX_FIFO_OCY · 0x118 RX_FIFO_OCY · 0x11C TEN_ADR · 0x120 RX_FIFO_PIRQ · 0x124 GPO · 0x128 TSUSTA … 0x13C THIGH · 0x140 TLOW · 0x144 THDDAT (타이밍 8개, 0x128 시작). 비트는 Zephyr 드라이버와 일치(CR: EN, TX_FIFO_RST, MSMS, TX, TXAK, RSTA, GC_EN / SR: ABGC, AAS, BB, SRW, TX_FIFO_FULL, RX_FIFO_FULL, RX_FIFO_EMPTY, TX_FIFO_EMPTY).

**동적 컨트롤러 절차 (확인)**
- 초기화: RX_FIFO_PIRQ=0x0F → TX_FIFO 리셋 → CR로 EN=1, TX_FIFO 리셋 해제, 일반 호출 끔. (FIFO 둘 다 비어 있어야 함)
- TX_FIFO 워드: bit8 = START, bit9 = STOP, 하위 8비트 = 주소+R/W 또는 데이터. 읽기는 주소 워드 뒤에 수신 바이트 수를 한 워드로 쓰고 STOP 은 그 워드에 설정.
- 4바이트 읽기 예(주소 0x34): 0x135 쓰기(START+주소+읽기) → 0x204 쓰기(STOP+4바이트) → RX_FIFO 비지 않을 때까지 대기하며 4바이트 읽기.
- 쓰기 예(EEPROM 주소 0x34, 내부 0x33): 0x134, 0x33, 0x89, 0xAB, 0xCD, 0x2EF(STOP+마지막 바이트).
- 전송 전에 SR로 FIFO 비어 있음과 버스 유휴 확인, TX_FIFO 가득 여부 확인 권장.
- 동적 모드는 7비트 주소 마스터만 지원.

## 소자 요약 (확인)
| 소자 | 7비트 주소 | 최대 SCL | 전원 | 문서 |
|---|---|---|---|---|
| MPU-6050 | 0x68 (AD0=Low), 0x69 (AD0=High) | 400 kHz (표준 100 kHz) | VDD 2.375~3.46 V | sensors.md |
| SHT3x (SHT31) | 0x44 (ADDR=Low), 0x45 (ADDR=High) | 1000 kHz | 2.15~5.5 V | sensors.md |
| BH1750FVI | 0x23 (ADDR=Low), 0x5C (ADDR=High) | 400 kHz | 2.4~3.6 V (절대 최대 4.5 V) | sensors.md |
| DS3231 | 0x68 (고정) | 400 kHz | VCC 2.3~5.5 V, VBAT 2.3~5.5 V | clock.md |
| BME280 | 0x76, 0x77 | (bme280.md) | 1.71~3.6 V | bme280.md |

## 버스 계획 (설계안)
- **주소 충돌**: MPU-6050(AD0=Low)과 DS3231 이 둘 다 0x68. MPU-6050 의 AD0 를 High 로 올려 0x69 를 쓰거나 버스를 나눕니다. (확인된 주소 값에서 도출)
- **속도**: 가장 느린 소자 한계가 400 kHz(MPU-6050, BH1750, DS3231)이므로 버스 하나를 400 kHz 로 통일. SHT3x 는 그 이하에서 문제 없음.
- **전원**: 위 소자는 모두 3.3 V 로 문서 범위 안(MPU-6050 3.46 V 상한, BH1750 3.6 V 상한은 여유가 작으므로 3.3 V 레일 사용).
- **풀업**: I2C 는 오픈 드레인이라 외부 풀업 필요(값은 보드/버스 용량에 따라; 이번에 계산하지 않음).
- **핀**: SCL, SDA 2핀. 한 버스에 위 5개 소자 동시 연결 가능(주소 충돌 해소 후).
- **인터럽트**: MPU-6050 INT, DS3231 INT/SQW(active-low), 별도 GPIO.

## 한계
RTL 없음(AMD IP 사용). PG090 본문으로 레지스터·타이밍·절차를 확인했으나 RTL/드라이버는 아직 작성하지 않았습니다(필요 시 위 절차대로 작성). 공통 연결 확인 읽기값: MPU-6050 WHO_AM_I(0x75)=0x68, BME280 chip id 0x60(bme280.md).
