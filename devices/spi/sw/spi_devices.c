#include "spi_devices.h"

int w25q_read_jedec(qspi_t *s, unsigned cs, uint8_t id[3])
{
    uint8_t tx[4] = { 0x9F, 0, 0, 0 }, rx[4];
    qspi_cs(s, cs, 1);
    int r = qspi_xfer(s, tx, rx, 4);
    qspi_cs(s, cs, 0);
    if (r) return r;
    id[0] = rx[1]; id[1] = rx[2]; id[2] = rx[3];
    return QSPI_OK;
}

int w25q_is_busy(qspi_t *s, unsigned cs, int *busy)
{
    uint8_t tx[2] = { 0x05, 0 }, rx[2];
    qspi_cs(s, cs, 1);
    int r = qspi_xfer(s, tx, rx, 2);
    qspi_cs(s, cs, 0);
    if (r) return r;
    *busy = rx[1] & 1;
    return QSPI_OK;
}

void max31855_decode(uint32_t raw, max31855_t *o)
{
    int32_t tc = (int32_t)raw >> 18;                       /* D31..D18, 14비트 부호, LSB 0.25 °C */
    int32_t cj = ((int32_t)(raw << 16)) >> 20;             /* D15..D4, 12비트 부호, LSB 0.0625 °C */
    o->tc_mc = tc * 250;
    o->cj_mc = (cj * 625) / 10;
    o->fault = (raw >> 16) & 1; o->scv = (raw >> 2) & 1; o->scg = (raw >> 1) & 1; o->oc = raw & 1;
}

int max31855_read(qspi_t *s, unsigned cs, max31855_t *o)
{
    uint8_t rx[4];
    qspi_cs(s, cs, 1);
    int r = qspi_xfer(s, 0, rx, 4);
    qspi_cs(s, cs, 0);
    if (r) return r;
    max31855_decode(((uint32_t)rx[0] << 24) | ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | rx[3], o);
    return QSPI_OK;
}

int hc595_write(qspi_t *s, unsigned cs, const uint8_t *data, unsigned n)
{
    if (!s || !data || n == 0 || n > 16) return QSPI_ERR_ARGS;
    qspi_cs(s, cs, 1);
    int r = qspi_xfer(s, data, 0, n);
    qspi_cs(s, cs, 0);                       /* CS(=STCP) 상승 → 저장 레지스터로 전송 */
    return r;
}
