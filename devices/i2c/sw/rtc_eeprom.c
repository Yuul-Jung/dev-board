#include "rtc_eeprom.h"
#include <string.h>

static uint8_t b2bcd(unsigned v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }
static int bcd_ok(uint8_t b) { return (b & 0x0F) <= 9 && (b >> 4) <= 9; }

/* ---------------- DS1307 ---------------- */
int ds1307_read(axi_iic_t *d, ds1307_time_t *t, int *halted)
{
    uint8_t r[7], reg = 0;
    if (!t) return IIC_ERR_ARGS;
    int rc = axi_iic_write_read(d, DS1307_ADDR, &reg, 1, r, 7);   /* 한 번에 7바이트: START 시점 복사본을 읽음 */
    if (rc) return rc;
    if (halted) *halted = (r[0] >> 7) & 1;
    uint8_t sec = r[0] & 0x7F, min = r[1], hr = r[2], wd = r[3], dt = r[4], mo = r[5], yr = r[6];
    if (!bcd_ok(sec) || !bcd_ok(min) || !bcd_ok(wd) || !bcd_ok(dt) || !bcd_ok(mo) || !bcd_ok(yr)) return DS_ERR_RANGE;
    unsigned hour;
    if (hr & 0x40) {                         /* 12시간제: bit5 = PM, bit4 = 10시 */
        uint8_t h = hr & 0x1F;
        if (!bcd_ok(h)) return DS_ERR_RANGE;
        unsigned v = (h >> 4) * 10 + (h & 0xF);
        if (v < 1 || v > 12) return DS_ERR_RANGE;
        hour = (v % 12) + ((hr & 0x20) ? 12 : 0);
    } else {                                 /* 24시간제: bit5 = 20시 */
        uint8_t h = hr & 0x3F;
        if (!bcd_ok(h)) return DS_ERR_RANGE;
        hour = (h >> 4) * 10 + (h & 0xF);
    }
    t->sec = (uint8_t)((sec >> 4) * 10 + (sec & 0xF));
    t->min = (uint8_t)((min >> 4) * 10 + (min & 0xF));
    t->wday = wd & 0xF; t->date = (uint8_t)((dt >> 4) * 10 + (dt & 0xF));
    t->month = (uint8_t)((mo >> 4) * 10 + (mo & 0xF)); t->year = (uint8_t)((yr >> 4) * 10 + (yr & 0xF));
    t->hour = (uint8_t)hour;
    if (t->sec > 59 || t->min > 59 || hour > 23 || t->wday < 1 || t->wday > 7 || t->date < 1 || t->date > 31 ||
        t->month < 1 || t->month > 12) return DS_ERR_RANGE;
    return IIC_OK;
}

int ds1307_set(axi_iic_t *d, const ds1307_time_t *t)
{
    if (!t) return IIC_ERR_ARGS;
    if (t->sec > 59 || t->min > 59 || t->hour > 23 || t->wday < 1 || t->wday > 7 || t->date < 1 || t->date > 31 ||
        t->month < 1 || t->month > 12 || t->year > 99) return DS_ERR_RANGE;
    uint8_t b[8] = { 0, b2bcd(t->sec) /* CH=0 */, b2bcd(t->min), b2bcd(t->hour) /* bit6=0: 24시간제 */,
                     t->wday, b2bcd(t->date), b2bcd(t->month), b2bcd(t->year) };
    return axi_iic_write(d, DS1307_ADDR, b, sizeof b);
}

int ds1307_set_sqw(axi_iic_t *d, int mode)
{
    uint8_t c;
    switch (mode) {
    case DS1307_SQW_1HZ: case DS1307_SQW_4KHZ: case DS1307_SQW_8KHZ: case DS1307_SQW_32KHZ:
        c = (uint8_t)(0x10 | mode); break;                    /* SQWE=1, RS1:RS0 */
    case DS1307_SQW_OFF_LOW:  c = 0x00; break;
    case DS1307_SQW_OFF_HIGH: c = 0x80; break;                /* OUT=1 */
    default: return IIC_ERR_ARGS;
    }
    uint8_t b[2] = { 0x07, c };
    return axi_iic_write(d, DS1307_ADDR, b, 2);
}

int ds1307_ram_write(axi_iic_t *d, unsigned off, const uint8_t *buf, unsigned len)
{
    uint8_t b[1 + 56];
    if (!buf || len == 0 || off > 55 || len > 56 - off) return IIC_ERR_ARGS;
    b[0] = (uint8_t)(0x08 + off);
    memcpy(b + 1, buf, len);
    return axi_iic_write(d, DS1307_ADDR, b, len + 1);
}

int ds1307_ram_read(axi_iic_t *d, unsigned off, uint8_t *buf, unsigned len)
{
    uint8_t reg = (uint8_t)(0x08 + off);
    if (!buf || len == 0 || off > 55 || len > 56 - off) return IIC_ERR_ARGS;
    return axi_iic_write_read(d, DS1307_ADDR, &reg, 1, buf, len);
}

/* ---------------- 24xx256 ---------------- */
int eeprom24_init(eeprom24_t *e, axi_iic_t *iic, uint8_t addr7, uint32_t size, uint16_t page)
{
    if (!e || !iic || addr7 < 0x50 || addr7 > 0x57 || size == 0 || size > 65536u ||
        page == 0 || page > 128 || (page & (page - 1))) return IIC_ERR_ARGS;
    e->iic = iic; e->addr7 = addr7; e->size = size; e->page = page;
    return IIC_OK;
}

static int set_addr(eeprom24_t *e, uint32_t a)
{
    uint8_t b[2] = { (uint8_t)(a >> 8), (uint8_t)a };       /* 상위 바이트 먼저 */
    return axi_iic_write(e->iic, e->addr7, b, 2);
}

int eeprom24_write(eeprom24_t *e, uint32_t addr, const uint8_t *buf, uint32_t len)
{
    uint8_t b[2 + 128];
    if (!e || !buf || len == 0 || addr >= e->size || len > e->size - addr) return IIC_ERR_ARGS;
    while (len) {
        uint32_t room = e->page - (addr & (e->page - 1u));    /* 이 페이지에 남은 칸 */
        uint32_t n = len < room ? len : room;
        b[0] = (uint8_t)(addr >> 8); b[1] = (uint8_t)addr;
        memcpy(b + 2, buf, n);
        int rc = axi_iic_write(e->iic, e->addr7, b, n + 2);
        if (rc) return rc;
        /* 쓰기 사이클(최대 5 ms) 동안 ACK 하지 않는다 → 주소 설정 쓰기가 ACK 될 때까지 폴링 */
        int done = 0;
        for (int i = 0; i < 20 && !done; i++) {
            rc = set_addr(e, addr + n < e->size ? addr + n : 0);
            if (rc == IIC_OK) done = 1;
            else if (rc == IIC_ERR_NACK) iic_delay_ms(1);
            else return rc;
        }
        if (!done) return IIC_ERR_TIMEOUT;
        addr += n; buf += n; len -= n;
    }
    return IIC_OK;
}

int eeprom24_read(eeprom24_t *e, uint32_t addr, uint8_t *buf, uint32_t len)
{
    if (!e || !buf || len == 0 || addr >= e->size || len > e->size - addr) return IIC_ERR_ARGS;
    while (len) {
        uint32_t n = len > 255 ? 255 : len;                   /* axi_iic_read 의 길이 한도 */
        uint8_t a[2] = { (uint8_t)(addr >> 8), (uint8_t)addr };
        int rc = axi_iic_write_read(e->iic, e->addr7, a, 2, buf, n);
        if (rc) return rc;
        addr += n; buf += n; len -= n;
    }
    return IIC_OK;
}
