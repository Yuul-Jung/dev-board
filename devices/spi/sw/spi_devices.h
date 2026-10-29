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
#endif
