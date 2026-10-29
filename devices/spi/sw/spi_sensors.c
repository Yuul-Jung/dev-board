#include "spi_sensors.h"

int mcp3008_read(qspi_t *s, unsigned cs, int sgl, int ch, uint16_t *code)
{
    if (!s || !code || ch < 0 || ch > 7 || (sgl != 0 && sgl != 1)) return QSPI_ERR_ARGS;
    /* 3바이트 프레임(24클럭): 앞의 0 일곱 개 + 시작 비트, SGL/D2/D1/D0, 나머지는 결과를 받기 위한 클럭 */
    uint8_t tx[3] = { 0x01, (uint8_t)((sgl << 7) | (ch << 4)), 0x00 }, rx[3];
    qspi_cs(s, cs, 1);
    int r = qspi_xfer(s, tx, rx, 3);
    qspi_cs(s, cs, 0);                                   /* CS 를 올려 변환을 끝낸다 */
    if (r) return r;
    *code = (uint16_t)(((rx[1] & 0x03) << 8) | rx[2]);   /* 널 비트 뒤 B9 B8 이 rx[1] 하위 2비트, B7..B0 이 rx[2] */
    return QSPI_OK;
}

int adxl345_read_reg(qspi_t *s, unsigned cs, uint8_t reg, uint8_t *v)
{
    if (!s || !v || reg > 0x3F) return QSPI_ERR_ARGS;
    uint8_t tx[2] = { (uint8_t)(0x80 | reg), 0 }, rx[2];
    qspi_cs(s, cs, 1);
    int r = qspi_xfer(s, tx, rx, 2);
    qspi_cs(s, cs, 0);
    if (r) return r;
    *v = rx[1];
    return QSPI_OK;
}

int adxl345_write_reg(qspi_t *s, unsigned cs, uint8_t reg, uint8_t v)
{
    if (!s || reg > 0x3F) return QSPI_ERR_ARGS;
    uint8_t tx[2] = { reg, v };                          /* bit7 = 0 쓰기, bit6 = 0 단일 바이트 */
    qspi_cs(s, cs, 1);
    int r = qspi_xfer(s, tx, 0, 2);
    qspi_cs(s, cs, 0);
    return r;
}

int adxl345_init(qspi_t *s, unsigned cs)
{
    uint8_t id;
    int r = adxl345_read_reg(s, cs, ADXL_REG_DEVID, &id);
    if (r) return r;
    if (id != ADXL_DEVID_VALUE) return ADXL_ERR_DEVID;
    r = adxl345_write_reg(s, cs, ADXL_REG_DATA_FORMAT, 0x0B);
    if (r) return r;
    return adxl345_write_reg(s, cs, ADXL_REG_POWER_CTL, 0x08);
}

int adxl345_read_xyz(qspi_t *s, unsigned cs, int16_t xyz[3])
{
    if (!s || !xyz) return QSPI_ERR_ARGS;
    uint8_t tx[7] = { 0xC0 | ADXL_REG_DATAX0, 0, 0, 0, 0, 0, 0 }, rx[7];   /* 0xF2 */
    qspi_cs(s, cs, 1);
    int r = qspi_xfer(s, tx, rx, 7);
    qspi_cs(s, cs, 0);
    if (r) return r;
    for (int i = 0; i < 3; i++) xyz[i] = (int16_t)(uint16_t)(rx[1 + 2 * i] | (rx[2 + 2 * i] << 8));   /* 하위 바이트 먼저 */
    return QSPI_OK;
}

int mcp4921_write(qspi_t *s, unsigned cs, uint16_t code, int gain2x, int buffered)
{
    if (!s || code > 4095) return QSPI_ERR_ARGS;
    uint16_t w = (uint16_t)(((buffered ? 1u : 0u) << 14) | ((gain2x ? 0u : 1u) << 13) | (1u << 12) | code);
    uint8_t tx[2] = { (uint8_t)(w >> 8), (uint8_t)w };
    qspi_cs(s, cs, 1);
    int r = qspi_xfer(s, tx, 0, 2);
    qspi_cs(s, cs, 0);                         /* CS 상승에서 래치 */
    return r;
}

int mcp4921_shutdown(qspi_t *s, unsigned cs)
{
    if (!s) return QSPI_ERR_ARGS;
    uint8_t tx[2] = { 0x00, 0x00 };            /* SHDN = 0 */
    qspi_cs(s, cs, 1);
    int r = qspi_xfer(s, tx, 0, 2);
    qspi_cs(s, cs, 0);
    return r;
}
