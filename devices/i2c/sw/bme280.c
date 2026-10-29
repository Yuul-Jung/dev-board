#include "bme280.h"

static int16_t s16(uint8_t lo, uint8_t hi) { return (int16_t)((uint16_t)hi << 8 | lo); }
static uint16_t u16(uint8_t lo, uint8_t hi) { return (uint16_t)((uint16_t)hi << 8 | lo); }

void bme280_parse_calib(const uint8_t a[26], const uint8_t b[7], bme280_calib_t *c)
{
    c->T1 = u16(a[0], a[1]);  c->T2 = s16(a[2], a[3]);  c->T3 = s16(a[4], a[5]);
    c->P1 = u16(a[6], a[7]);  c->P2 = s16(a[8], a[9]);  c->P3 = s16(a[10], a[11]);
    c->P4 = s16(a[12], a[13]); c->P5 = s16(a[14], a[15]); c->P6 = s16(a[16], a[17]);
    c->P7 = s16(a[18], a[19]); c->P8 = s16(a[20], a[21]); c->P9 = s16(a[22], a[23]);
    c->H1 = a[25];                                   /* 0xA1 (a[24] 는 0xA0 예약) */
    c->H2 = s16(b[0], b[1]); c->H3 = b[2];
    /* H4: 0xE4 → [11:4], 0xE5 하위 니블 → [3:0]. H5: 0xE5 상위 니블 → [3:0], 0xE6 → [11:4] */
    c->H4 = (int16_t)(((int16_t)(int8_t)b[3] << 4) | (b[4] & 0x0F));
    c->H5 = (int16_t)(((int16_t)(int8_t)b[5] << 4) | (b[4] >> 4));
    c->H6 = (int8_t)b[6];
}

int32_t bme280_comp_T(const bme280_calib_t *c, int32_t adc_T, int32_t *t_fine)
{
    int32_t v1 = (((adc_T >> 3) - ((int32_t)c->T1 << 1)) * (int32_t)c->T2) >> 11;
    int32_t d = (adc_T >> 4) - (int32_t)c->T1;
    int32_t v2 = (((d * d) >> 12) * (int32_t)c->T3) >> 14;
    *t_fine = v1 + v2;
    return (*t_fine * 5 + 128) >> 8;
}

uint32_t bme280_comp_P(const bme280_calib_t *c, int32_t adc_P, int32_t t_fine)
{
    int64_t v1 = (int64_t)t_fine - 128000;
    int64_t v2 = v1 * v1 * (int64_t)c->P6;
    v2 += (v1 * (int64_t)c->P5) << 17;
    v2 += (int64_t)c->P4 << 35;
    v1 = ((v1 * v1 * (int64_t)c->P3) >> 8) + ((v1 * (int64_t)c->P2) << 12);
    v1 = (((int64_t)1 << 47) + v1) * (int64_t)c->P1 >> 33;
    if (v1 == 0) return 0;
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - v2) * 3125) / v1;
    v1 = ((int64_t)c->P9 * (p >> 13) * (p >> 13)) >> 25;
    v2 = ((int64_t)c->P8 * p) >> 19;
    p = ((p + v1 + v2) >> 8) + ((int64_t)c->P7 << 4);
    return (uint32_t)p;
}

uint32_t bme280_comp_H(const bme280_calib_t *c, int32_t adc_H, int32_t t_fine)
{
    int32_t x = t_fine - 76800;
    x = (((((adc_H << 14) - ((int32_t)c->H4 << 20) - ((int32_t)c->H5 * x)) + 16384) >> 15) *
         (((((((x * (int32_t)c->H6) >> 10) * (((x * (int32_t)c->H3) >> 11) + 32768)) >> 10) + 2097152) *
           (int32_t)c->H2 + 8192) >> 14));
    x = x - (((((x >> 15) * (x >> 15)) >> 7) * (int32_t)c->H1) >> 4);
    x = x < 0 ? 0 : x;
    x = x > 419430400 ? 419430400 : x;
    return (uint32_t)(x >> 12);
}

void bme280_split(const uint8_t d[8], int32_t *P, int32_t *T, int32_t *H)
{
    *P = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);
    *T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);
    *H = ((int32_t)d[6] << 8) | d[7];
}

static int rd(axi_iic_t *d, uint8_t a, uint8_t reg, uint8_t *b, unsigned n)
{ return axi_iic_write_read(d, a, &reg, 1, b, n); }
static int wr(axi_iic_t *d, uint8_t a, uint8_t reg, uint8_t v)
{ uint8_t t[2] = { reg, v }; return axi_iic_write(d, a, t, 2); }

int bme280_init(bme280_t *s, axi_iic_t *d, uint8_t addr)
{
    uint8_t id, a[26], b[7]; int r;
    s->addr = addr;
    if ((r = rd(d, addr, 0xD0, &id, 1)) != 0) return r;
    if (id != BME280_CHIP_ID) return -10;               /* 칩 ID 불일치 */
    if ((r = rd(d, addr, 0x88, a, 26)) != 0) return r;
    if ((r = rd(d, addr, 0xE1, b, 7)) != 0) return r;
    bme280_parse_calib(a, b, &s->c);
    return 0;
}

int bme280_measure(bme280_t *s, axi_iic_t *d, int32_t *t, uint32_t *p, uint32_t *h)
{
    uint8_t raw[8], st; int r; int32_t aP, aT, aH, tf;
    if ((r = wr(d, s->addr, 0xF2, 0x01)) != 0) return r;      /* osrs_h ×1, ctrl_meas 쓰기 전에 */
    if ((r = wr(d, s->addr, 0xF5, 0x00)) != 0) return r;      /* 필터 끔 */
    if ((r = wr(d, s->addr, 0xF4, (1u << 5) | (1u << 2) | 0x01)) != 0) return r; /* T×1 P×1 강제 */
    for (int i = 0; i < 50; i++) {                             /* measuring(bit3) 해제 대기, 최대 약 50 ms */
        iic_delay_ms(1);
        if ((r = rd(d, s->addr, 0xF3, &st, 1)) != 0) return r;
        if (!(st & 0x08)) break;
        if (i == 49) return -11;
    }
    if ((r = rd(d, s->addr, 0xF7, raw, 8)) != 0) return r;     /* 한 번에 burst 읽기 */
    bme280_split(raw, &aP, &aT, &aH);
    *t = bme280_comp_T(&s->c, aT, &tf);
    *p = bme280_comp_P(&s->c, aP, tf);
    *h = bme280_comp_H(&s->c, aH, tf);
    return 0;
}
