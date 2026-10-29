/* SPI 소자 도우미. 사양은 devices/spi 의 md 문서 (데이터시트 확인분)에서만 가져왔다. */
#ifndef SPI_DEVICES_H
#define SPI_DEVICES_H
#include "axi_qspi.h"

/* W25Q64JV (모드 0/3): 9Fh JEDEC ID → EFh 40h 17h, 05h Status-1 의 S0 = BUSY */
int w25q_read_jedec(qspi_t *s, unsigned cs, uint8_t id[3]);
int w25q_is_busy(qspi_t *s, unsigned cs, int *busy);

/* MAX31855: 32비트 읽기 전용(모드는 데이터시트 발췌에서 미확인, SCK ≤ 5 MHz, 변환 ≥ 100 ms 간격) */
typedef struct { int32_t tc_mc; int32_t cj_mc; int fault, scv, scg, oc; } max31855_t;  /* m°C */
void max31855_decode(uint32_t raw, max31855_t *o);
int  max31855_read(qspi_t *s, unsigned cs, max31855_t *o);

/* 74HC595 직렬→병렬 시프트 레지스터 (Nexperia 74HC/HCT595 Rev.12, D1).
 * SHCP(=SCK) 상승 에지에서 DS 를 시프트, STCP 상승 에지에서 저장 레지스터로 전송 → SPI 모드 0, MSB 먼저,
 * STCP 를 SPI 의 CS 에 연결(전송 중 Low, 끝나면 상승 = 래치).
 * 첫 번째로 보낸 비트가 8클럭 뒤 Q7 에 이르므로 바이트 하나는 bit7→Q7 … bit0→Q0.
 * 여러 개를 Q7S→DS 로 직렬 연결하면 첫 번째로 보낸 바이트가 가장 먼 칩에 도착한다.
 * OE(Low = 출력 활성)와 MR(Low = 시프트 레지스터만 지움; 저장 레지스터는 그대로)은 SPI 가 아니라 별도 핀/GPIO. */
int hc595_write(qspi_t *s, unsigned cs, const uint8_t *data, unsigned n);   /* n = 칩 수(1~16) */
#endif
