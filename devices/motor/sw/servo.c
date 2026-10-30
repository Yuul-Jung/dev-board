#include "servo.h"

uint32_t servo_angle_to_us(int deg, uint32_t min_us, uint32_t max_us)
{
    if (deg < 0) deg = 0;
    if (deg > SRV_ANGLE_MAX) deg = SRV_ANGLE_MAX;
    if (max_us < min_us) { uint32_t t = min_us; min_us = max_us; max_us = t; }
    uint64_t span = (uint64_t)(max_us - min_us) * (uint64_t)deg;
    return min_us + (uint32_t)((span + SRV_ANGLE_MAX / 2) / SRV_ANGLE_MAX);
}

static int put(uintptr_t b, uint32_t off, uint32_t v)
{
    SRV_WR(b, off, v);
    return (SRV_RD(b, off) & 0xFFFFu) == v ? SRV_OK : SRV_ERR_REG;
}

int servo_init(uintptr_t b, uint32_t period_us, uint32_t min_us, uint32_t max_us)
{
    if (min_us == 0 || min_us > max_us || max_us >= period_us || period_us > SRV_US_MAX) return SRV_ERR_ARGS;
    SRV_WR(b, SRV_REG_CTRL, 0);                       /* 설정 중에는 끈다 */
    int r;
    if ((r = put(b, SRV_REG_PERIOD, period_us)) < 0) return r;
    if ((r = put(b, SRV_REG_MIN, min_us)) < 0) return r;
    if ((r = put(b, SRV_REG_MAX, max_us)) < 0) return r;
    if ((r = put(b, SRV_REG_WIDTH, (min_us + max_us) / 2)) < 0) return r;
    SRV_WR(b, SRV_REG_CTRL, 1);
    return SRV_OK;
}

int servo_set_width_us(uintptr_t b, uint32_t width_us)
{
    if (width_us > SRV_US_MAX) return SRV_ERR_ARGS;
    return put(b, SRV_REG_WIDTH, width_us);
}

int servo_set_angle(uintptr_t b, int deg)
{
    uint32_t lo = SRV_RD(b, SRV_REG_MIN) & 0xFFFFu, hi = SRV_RD(b, SRV_REG_MAX) & 0xFFFFu;
    return servo_set_width_us(b, servo_angle_to_us(deg, lo, hi));
}

uint32_t servo_applied_us(uintptr_t b) { return SRV_RD(b, SRV_REG_APPLIED) & 0xFFFFu; }
void servo_enable(uintptr_t b, int on) { SRV_WR(b, SRV_REG_CTRL, on ? 1u : 0u); }
