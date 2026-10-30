#include "ultrasonic.h"

void us_init(uintptr_t b) { US_WR(b, US_REG_CTRL, 0); US_WR(b, US_REG_STAT, US_ST_TIMEOUT | US_ST_DONE); }
void us_start(uintptr_t b) { US_WR(b, US_REG_STAT, US_ST_TIMEOUT | US_ST_DONE); US_WR(b, US_REG_CTRL, 1); }
void us_stop(uintptr_t b) { US_WR(b, US_REG_CTRL, 0); }

int us_poll(uintptr_t b, int *cm)
{
    if (!cm) return US_ERR_ARGS;
    uint32_t s = US_RD(b, US_REG_STAT);
    if (s & US_ST_TIMEOUT) { US_WR(b, US_REG_STAT, US_ST_TIMEOUT); return US_ERR_TIMEOUT; }
    if (!(s & US_ST_DONE)) return US_PENDING;
    US_WR(b, US_REG_STAT, US_ST_DONE);
    int d = (int)(US_RD(b, US_REG_DIST) & US_DIST_MASK);
    if (d < US_CM_MIN || d > US_CM_MAX) return US_ERR_RANGE;
    *cm = d;
    return US_OK;
}

int us_read_cm(uintptr_t b, int *cm)
{
    if (!cm) return US_ERR_ARGS;
    for (uint32_t i = 0; i < US_SPINS; i++) {
        int r = us_poll(b, cm);
        if (r != US_PENDING) return r;
    }
    return US_ERR_BUS;
}

uint32_t us_echo_to_cm(uint32_t echo_us) { return echo_us / US_US_PER_CM; }
