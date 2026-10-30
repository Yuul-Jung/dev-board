#include "dht22.h"

int dht22_read_raw(uintptr_t b, uint16_t *rh_raw, uint16_t *temp_raw, uint8_t *chk)
{
    if (!rh_raw || !temp_raw) return DHT_ERR_ARGS;
    for (uint32_t i = 0; DHT_RD(b, DHT_REG_STAT) & DHT_ST_BUSY; i++) if (i >= DHT_SPINS) return DHT_ERR_BUS;
    DHT_WR(b, DHT_REG_CMD, 1);
    uint32_t s;
    for (uint32_t i = 0;; i++) {
        s = DHT_RD(b, DHT_REG_STAT);
        if (s & DHT_ST_DONE) break;
        if (i >= DHT_SPINS) return DHT_ERR_BUS;
    }
    if (s & DHT_ST_ERR) return DHT_ERR_NORESP;
    if (!(s & DHT_ST_CSUM)) return DHT_ERR_CSUM;
    *rh_raw = (uint16_t)DHT_RD(b, DHT_REG_RH);
    *temp_raw = (uint16_t)DHT_RD(b, DHT_REG_TEMP);
    if (chk) *chk = (uint8_t)DHT_RD(b, DHT_REG_CHK);
    return DHT_OK;
}

int32_t dht22_temp_x10(uint16_t raw)
{
    int32_t mag = raw & ~DHT_SIGN_BIT;
    return (raw & DHT_SIGN_BIT) ? -mag : mag;
}

int dht22_read(uintptr_t base, int32_t *rh_x10, int32_t *temp_x10)
{
    if (!rh_x10 || !temp_x10) return DHT_ERR_ARGS;
    uint16_t rh, t;
    int r = dht22_read_raw(base, &rh, &t, 0);
    if (r < 0) return r;
    int32_t tv = dht22_temp_x10(t);
    if (rh > DHT_RH_MAX_X10 || tv > DHT_TEMP_MAX_X10 || tv < DHT_TEMP_MIN_X10) return DHT_ERR_RANGE;
    *rh_x10 = rh; *temp_x10 = tv;
    return DHT_OK;
}

void dht22_init(dht22_t *d, uintptr_t base) { if (!d) return; d->base = base; d->last_ms = 0; d->has_last = 0; }

int dht22_read_paced(dht22_t *d, uint32_t now_ms, int32_t *rh_x10, int32_t *temp_x10)
{
    if (!d || !rh_x10 || !temp_x10) return DHT_ERR_ARGS;
    if (d->has_last && (uint32_t)(now_ms - d->last_ms) <= DHT_MIN_PERIOD_MS) return DHT_ERR_TOOSOON;
    d->last_ms = now_ms; d->has_last = 1;
    return dht22_read(d->base, rh_x10, temp_x10);
}
