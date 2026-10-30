#include "ds18b20.h"

uint8_t ds18b20_crc8(const uint8_t *p, unsigned n)
{
    uint8_t crc = 0;
    while (n--) {
        uint8_t b = *p++;
        for (int i = 0; i < 8; i++) {                 /* 반사형 0x8C = x^8+x^5+x^4+1 */
            uint8_t mix = (uint8_t)((crc ^ b) & 1);
            crc >>= 1;
            if (mix) crc ^= DS_CRC_POLY_REFLECTED;
            b >>= 1;
        }
    }
    return crc;
}

unsigned ds18b20_resolution_bits(uint8_t config) { return 9u + ((config >> DS_CFG_RES_SHIFT) & 3u); }

int32_t ds18b20_raw_to_uc(int16_t raw, unsigned bits)
{
    if (bits < 9 || bits > 12) bits = 12;
    int32_t v = raw;
    int undef = (int)(12 - bits);                       /* 하위 미정의 비트 수 */
    v = (v >> undef) * (1 << undef);                    /* 산술 시프트: 음수도 내림 */
    return v * 62500;                                   /* 1/16 °C = 62500 µ°C: 정수로 정확 */
}

uint32_t ds18b20_tconv_ms(unsigned bits)
{
    static const uint16_t t[4] = { DS_TCONV_MS_9, 188, 375, DS_TCONV_MS_12 };   /* 93.75 / 187.5 / 375 / 750 ms 를 올림 */
    if (bits < 9 || bits > 12) bits = 12;
    return t[bits - 9];
}

static int ow_run(uintptr_t b, uint32_t cmd)
{
    for (uint32_t i = 0; (OW_RD(b, OW_REG_STAT) & OW_ST_BUSY); i++) if (i >= DS_BUS_SPINS) return DS_ERR_BUS;
    OW_WR(b, OW_REG_CMD, cmd);
    for (uint32_t i = 0; !(OW_RD(b, OW_REG_STAT) & OW_ST_DONE); i++) if (i >= DS_BUS_SPINS) return DS_ERR_BUS;
    return DS_OK;
}
static int ow_reset(uintptr_t b)
{
    int r = ow_run(b, OW_OP_RESET);
    if (r < 0) return r;
    return (OW_RD(b, OW_REG_STAT) & OW_ST_PRES) ? DS_OK : DS_ERR_NODEV;
}
static int ow_wbyte(uintptr_t b, uint8_t v)
{
    for (int i = 0; i < 8; i++) { int r = ow_run(b, OW_OP_WRITE | ((uint32_t)((v >> i) & 1) << 2)); if (r < 0) return r; }
    return DS_OK;
}
static int ow_rbit(uintptr_t b)
{
    int r = ow_run(b, OW_OP_READ);
    if (r < 0) return r;
    return (OW_RD(b, OW_REG_STAT) & OW_ST_RBIT) ? 1 : 0;
}
static int ow_rbyte(uintptr_t b, uint8_t *out)
{
    uint8_t v = 0;
    for (int i = 0; i < 8; i++) { int r = ow_rbit(b); if (r < 0) return r; v |= (uint8_t)(r << i); }
    *out = v; return DS_OK;
}

int ds18b20_read_scratchpad(uintptr_t base, uint8_t sp[9])
{
    if (!sp) return DS_ERR_ARGS;
    int r = ow_reset(base); if (r < 0) return r;
    if ((r = ow_wbyte(base, DS_CMD_SKIP_ROM)) < 0) return r;
    if ((r = ow_wbyte(base, DS_CMD_READ_SP)) < 0) return r;
    uint8_t t[9];
    for (int i = 0; i < 9; i++) if ((r = ow_rbyte(base, &t[i])) < 0) return r;
    if (ds18b20_crc8(t, 8) != t[8]) return DS_ERR_CRC;
    for (int i = 0; i < 9; i++) sp[i] = t[i];
    return DS_OK;
}

int ds18b20_set_resolution(uintptr_t base, unsigned bits, int8_t th, int8_t tl)
{
    if (bits < 9 || bits > 12) return DS_ERR_ARGS;
    int r = ow_reset(base); if (r < 0) return r;
    if ((r = ow_wbyte(base, DS_CMD_SKIP_ROM)) < 0) return r;
    if ((r = ow_wbyte(base, DS_CMD_WRITE_SP)) < 0) return r;
    if ((r = ow_wbyte(base, (uint8_t)th)) < 0) return r;
    if ((r = ow_wbyte(base, (uint8_t)tl)) < 0) return r;
    return ow_wbyte(base, (uint8_t)(DS_CFG_BASE | ((bits - 9) << DS_CFG_RES_SHIFT)));
}

int ds18b20_convert(uintptr_t base, uint32_t max_polls)
{
    int r = ow_reset(base); if (r < 0) return r;
    if ((r = ow_wbyte(base, DS_CMD_SKIP_ROM)) < 0) return r;
    if ((r = ow_wbyte(base, DS_CMD_CONVERT)) < 0) return r;
    for (uint32_t i = 0; i < max_polls; i++) {
        r = ow_rbit(base);
        if (r < 0) return r;
        if (r == 1) return DS_OK;
    }
    return DS_ERR_TIMEOUT;
}

int ds18b20_read_uc(uintptr_t base, uint32_t max_polls, int32_t *uc)
{
    if (!uc) return DS_ERR_ARGS;
    int r = ds18b20_convert(base, max_polls); if (r < 0) return r;
    uint8_t sp[9];
    if ((r = ds18b20_read_scratchpad(base, sp)) < 0) return r;
    *uc = ds18b20_raw_to_uc((int16_t)(((unsigned)sp[1] << 8) | sp[0]), ds18b20_resolution_bits(sp[4]));
    return DS_OK;
}
