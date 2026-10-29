# I2C 소자와 SoC 연결

구분: **확인** = 이번 작업에서 데이터시트를 열어 확인, **계산**, **설계안**, **미확인**. 소자 문서: [sensors.md](sensors.md), [clock.md](clock.md), 기존 [bme280.md](bme280.md).

## AXI IIC (PG090)
기능 요약(Xilinx 위키): 100k/400k/1M, 마스터·슬레이브·멀티 마스터, 7/10비트 주소, 송수신 FIFO 각 16바이트.
레지스터는 PG090 본문을 열지 못해 Zephyr 드라이버 헤더(`i2c_xilinx_axi.h`, Apache-2.0)에서 확인했습니다. **D3 (2차 자료, 드라이버 구현 기준)**.

| 오프셋 | 이름 | 오프셋 | 이름 |
|---|---|---|---|
| 0x01C | GIE (bit31 = 활성) | 0x11C | TEN_ADR (10비트 주소) |
| 0x020 | ISR | 0x120 | RX_FIFO_PIRQ |
| 0x028 | IER | 0x124 | GPO |
| 0x040 | SOFTR (0xA 쓰기) | 0x128~0x144 | 타이밍 레지스터 8개 (TSUSTA, TSUSTO, THDSTA, TSUDAT, TBUF, THIGH, TLOW, THDDAT) |
| 0x100 | CR | 0x108 | TX_FIFO (bit8 START, bit9 STOP) |
| 0x104 | SR | 0x10C | RX_FIFO (하위 8비트 데이터) |
| 0x110 | ADR | 0x114 / 0x118 | TX / RX FIFO 점유 (하위 4비트) |

- CR 비트: 0 EN · 1 TX_FIFO_RST · 2 MSMS(컨트롤러/타깃) · 3 TX(송신/수신) · 4 TXAK · 5 RSTA(반복 시작) · 6 GC_EN
- SR 비트: 0 ABGC · 1 AAS · 2 BB(버스 사용 중) · 3 SRW · 4 TX_FIFO_FULL · 5 RX_FIFO_FULL · 6 RX_FIFO_EMPTY · 7 TX_FIFO_EMPTY
- ISR/IER 비트: 0 중재 손실 · 1 송신 오류/타깃 송신 완료 · 2 TX FIFO 빔 · 3 RX FIFO 가득 · 4 버스 유휴 · 5 타깃으로 호출됨 · 6 타깃 호출 아님 · 7 TX FIFO 절반 빔
미확인(PG090 본문): 타이밍 레지스터에 넣을 값 계산식과 SCL 400 kHz 설정 방법, SDA/SCL 3상태 포트 이름(IOBUF 구성), 동적 컨트롤러 모드 절차.

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
RTL 없음(AMD IP 사용). AXI IIC 레지스터는 D3 수준으로만 확인했고 SCL 타이밍 계산은 미확인이라 드라이버를 쓰지 않았습니다. 공통 연결 확인 읽기값: MPU-6050 WHO_AM_I(0x75)=0x68, BME280 chip id 0x60(bme280.md).
