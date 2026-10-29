#include "analog_sensors.h"
#ifndef AN_NO_FLOAT
#include <math.h>
#endif

static int64_t rnd64(int64_t num, int64_t den)           /* den > 0, 0 에서 먼 쪽으로 반올림 */
{
    return num >= 0 ? (num + den / 2) / den : -((-num + den / 2) / den);
}
static int32_t sat32(int64_t q) { return q > INT32_MAX ? INT32_MAX : q < INT32_MIN ? INT32_MIN : (int32_t)q; }
static int32_t div_round(int64_t num, int64_t den) { return sat32(rnd64(num, den)); }   /* int32 범위로 포화 */

int32_t lm35_uv_to_mdegc(int32_t uv) { return div_round(uv, AN_LM35_UV_PER_MDEGC); }
int32_t lm35_mv_to_mdegc(int32_t mv) { return div_round((int64_t)mv * 1000, AN_LM35_UV_PER_MDEGC); }

int lm35_in_range(int variant, int32_t m)
{
    switch (variant) {
    case AN_LM35_FULL: return m >= AN_LM35_FULL_MIN_MDEGC && m <= AN_LM35_FULL_MAX_MDEGC;
    case AN_LM35_C:    return m >= AN_LM35_C_MIN_MDEGC && m <= AN_LM35_C_MAX_MDEGC;
    case AN_LM35_D:    return m >= AN_LM35_D_MIN_MDEGC && m <= AN_LM35_D_MAX_MDEGC;
    default: return AN_ERR_ARGS;
    }
}

int lm35_read_uv(int variant, int32_t uv, int32_t *mdegc)
{
    if (!mdegc) return AN_ERR_ARGS;
    int32_t m = lm35_uv_to_mdegc(uv);
    int ok = lm35_in_range(variant, m);
    if (ok < 0) return ok;
    *mdegc = m;
    return ok ? AN_OK : AN_ERR_RANGE;
}

int ldr_ohms(int32_t v, int32_t vcc, uint32_t rf, int ldr_top, uint32_t *out)
{
    if (!out || vcc <= 0 || rf == 0) return AN_ERR_ARGS;
    if (v <= 0 || v >= vcc) return AN_ERR_RANGE;
    uint64_t r = ldr_top ? ((uint64_t)rf * (uint64_t)(vcc - v) + (uint64_t)v / 2) / (uint64_t)v
                         : ((uint64_t)rf * (uint64_t)v + (uint64_t)(vcc - v) / 2) / (uint64_t)(vcc - v);
    if (r > 0xFFFFFFFFu) return AN_ERR_RANGE;
    *out = (uint32_t)r;
    return AN_OK;
}

#ifndef AN_NO_FLOAT
int ldr_lux(uint32_t r, uint32_t r10, double gamma, double *lux)
{
    if (!lux || r == 0 || r10 == 0 || !(gamma > 0.0)) return AN_ERR_ARGS;
    *lux = 10.0 * pow((double)r10 / (double)r, 1.0 / gamma);
    return AN_OK;
}
#endif

int32_t an_map(int32_t x, int32_t in_lo, int32_t in_hi, int32_t out_lo, int32_t out_hi)
{
    if (in_lo == in_hi) return out_lo;
    int64_t num = ((int64_t)x - in_lo) * ((int64_t)out_hi - out_lo), den = (int64_t)in_hi - in_lo;
    if (den < 0) { den = -den; num = -num; }
    return sat32((int64_t)out_lo + rnd64(num, den));          /* 범위 밖 입력의 외삽 결과는 int32 로 포화 */
}

int an_percent(uint32_t raw, uint32_t raw_max)
{
    if (raw_max == 0) return AN_ERR_ARGS;
    if (raw > raw_max) raw = raw_max;
    return (int)(((uint64_t)raw * 100u + raw_max / 2u) / raw_max);
}

int joy_axis(uint32_t raw, uint32_t center, uint32_t dz, uint32_t raw_max)
{
    if (raw_max == 0 || center > raw_max) return AN_ERR_ARGS;
    if (raw > raw_max) raw = raw_max;
    uint64_t hi = (uint64_t)center + dz;                        /* 이 값 넘어서부터 + */
    if (raw > hi) {
        uint64_t span = (uint64_t)raw_max - hi;                 /* raw > hi 이고 raw <= raw_max 이므로 span >= 1 */
        uint64_t v = ((uint64_t)raw - hi) * 100u;
        v = (v + span / 2) / span;
        return (int)(v > 100 ? 100 : v);
    }
    if (center > dz && raw < center - dz) {                     /* center - dz 아래부터 − */
        uint64_t lo = (uint64_t)center - dz;                    /* raw < lo 이므로 lo >= 1 */
        uint64_t v = (lo - raw) * 100u;
        v = (v + lo / 2) / lo;
        return -(int)(v > 100 ? 100 : v);
    }
    return 0;
}
