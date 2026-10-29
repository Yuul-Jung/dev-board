/* DS1307 / 24xx256 호스트 시험. I2C 층을 장치 모델로 대체.
 * DS1307 모델: 포인터 자동 증가, 3Fh 다음 00h. 24xx256 모델: 페이지 경계 감김(wrap), 쓰기 사이클 동안 NACK. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../sw/rtc_eeprom.h"

void iic_delay_ms(unsigned ms) { (void)ms; }
static uint8_t rtc[64], rptr; static int nack_all;
static uint8_t mem[32768]; static int busy_left, busy_len = 3, polls, bad_boundary;
static uint16_t eptr; static int chunks;

int axi_iic_write(axi_iic_t *d, uint8_t a, const uint8_t *b, unsigned n)
{
    (void)d; if (nack_all || n == 0) return IIC_ERR_NACK;
    if (a == 0x68) {
        rptr = b[0] & 0x3F;
        for (unsigned i = 1; i < n; i++) { rtc[rptr] = b[i]; rptr = (rptr + 1) & 0x3F; }
        return IIC_OK;
    }
    if (a == 0x50) {
        if (busy_left > 0) { busy_left--; polls++; return IIC_ERR_NACK; }      /* 쓰기 사이클 중 */
        eptr = (uint16_t)(((b[0] << 8) | b[1]) & 0x7FFF);
        if (n > 2) {
            chunks++;
            if (n - 2 > 64) bad_boundary++;
            if (((eptr & 63) + (n - 2)) > 64) bad_boundary++;      /* 드라이버가 경계를 넘겨 보냈는지 */
            uint16_t base = eptr & ~63u, off = eptr & 63;
            for (unsigned i = 2; i < n; i++) { mem[base + off] = b[i]; off = (off + 1) & 63; }   /* 모델은 감김 */
            busy_left = busy_len;
        }
        return IIC_OK;
    }
    return IIC_ERR_NACK;
}
int axi_iic_write_read(axi_iic_t *d, uint8_t a, const uint8_t *w, unsigned wl, uint8_t *r, unsigned rl)
{
    int rc = axi_iic_write(d, a, w, wl); if (rc) return rc;
    if (a == 0x68) { for (unsigned i = 0; i < rl; i++) { r[i] = rtc[rptr]; rptr = (rptr + 1) & 0x3F; } return IIC_OK; }
    for (unsigned i = 0; i < rl; i++) { r[i] = mem[eptr]; eptr = (eptr + 1) & 0x7FFF; }
    return IIC_OK;
}

static int bad;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); bad++; } } while (0)

int main(void)
{
    axi_iic_t iic = {0}; ds1307_time_t t, u; int halted;
    /* ---- DS1307 ---- */
    memset(rtc, 0xFF, sizeof rtc); rtc[0] = 0x80;                     /* 전원 인가 직후: 정의되지 않은 내용 + CH=1 */
    CHECK(ds1307_read(&iic, &t, &halted) == DS_ERR_RANGE && halted == 1);
    ds1307_time_t s = { 56, 34, 21, 3, 29, 10, 26 };
    CHECK(ds1307_set(&iic, &s) == IIC_OK);
    CHECK(rtc[0] == 0x56 && rtc[1] == 0x34 && rtc[2] == 0x21 && rtc[3] == 3 && rtc[4] == 0x29 && rtc[5] == 0x10 && rtc[6] == 0x26);
    CHECK(ds1307_read(&iic, &u, &halted) == IIC_OK && halted == 0 && !memcmp(&s, &u, sizeof s));
    rtc[0] |= 0x80; CHECK(ds1307_read(&iic, &u, &halted) == IIC_OK && halted == 1 && u.sec == 56);   /* CH 가 초 값을 가리지 않음 */
    rtc[0] = 0x00;
    rtc[2] = 0x40 | 0x12; CHECK(ds1307_read(&iic, &u, 0) == IIC_OK && u.hour == 0);           /* 12 AM = 0시 */
    rtc[2] = 0x40 | 0x20 | 0x12; CHECK(ds1307_read(&iic, &u, 0) == IIC_OK && u.hour == 12);   /* 12 PM = 12시 */
    rtc[2] = 0x40 | 0x20 | 0x11; CHECK(ds1307_read(&iic, &u, 0) == IIC_OK && u.hour == 23);   /* 11 PM */
    rtc[2] = 0x40 | 0x01; CHECK(ds1307_read(&iic, &u, 0) == IIC_OK && u.hour == 1);
    rtc[2] = 0x40 | 0x13; CHECK(ds1307_read(&iic, &u, 0) == DS_ERR_RANGE);                     /* 12시간제에 13 */
    rtc[2] = 0x23; CHECK(ds1307_read(&iic, &u, 0) == IIC_OK && u.hour == 23);                   /* 24시간제 20시 비트 */
    rtc[2] = 0x24; CHECK(ds1307_read(&iic, &u, 0) == DS_ERR_RANGE);
    rtc[2] = 0x2A; CHECK(ds1307_read(&iic, &u, 0) == DS_ERR_RANGE);                             /* BCD 아님 */
    rtc[2] = 0x21; rtc[0] = 0x60; CHECK(ds1307_read(&iic, &u, 0) == DS_ERR_RANGE);              /* 초 60 */
    ds1307_time_t bad1 = s; bad1.hour = 24; CHECK(ds1307_set(&iic, &bad1) == DS_ERR_RANGE);
    bad1 = s; bad1.month = 13; CHECK(ds1307_set(&iic, &bad1) == DS_ERR_RANGE);
    bad1 = s; bad1.year = 100; CHECK(ds1307_set(&iic, &bad1) == DS_ERR_RANGE);
    bad1 = s; bad1.wday = 0; CHECK(ds1307_set(&iic, &bad1) == DS_ERR_RANGE);
    CHECK(ds1307_set_sqw(&iic, DS1307_SQW_1HZ) == IIC_OK && rtc[7] == 0x10);
    CHECK(ds1307_set_sqw(&iic, DS1307_SQW_32KHZ) == IIC_OK && rtc[7] == 0x13);
    CHECK(ds1307_set_sqw(&iic, DS1307_SQW_4KHZ) == IIC_OK && rtc[7] == 0x11);
    CHECK(ds1307_set_sqw(&iic, DS1307_SQW_8KHZ) == IIC_OK && rtc[7] == 0x12);
    CHECK(ds1307_set_sqw(&iic, DS1307_SQW_OFF_HIGH) == IIC_OK && rtc[7] == 0x80);
    CHECK(ds1307_set_sqw(&iic, DS1307_SQW_OFF_LOW) == IIC_OK && rtc[7] == 0x00);
    CHECK(ds1307_set_sqw(&iic, 6) == IIC_ERR_ARGS);
    uint8_t ram[56], rb[56]; for (int i = 0; i < 56; i++) ram[i] = (uint8_t)(i * 7 + 1);
    CHECK(ds1307_ram_write(&iic, 0, ram, 56) == IIC_OK && rtc[8] == 1 && rtc[63] == ram[55]);
    CHECK(ds1307_ram_read(&iic, 10, rb, 46) == IIC_OK && !memcmp(rb, ram + 10, 46));
    CHECK(ds1307_ram_write(&iic, 55, ram, 2) == IIC_ERR_ARGS && ds1307_ram_write(&iic, 56, ram, 1) == IIC_ERR_ARGS);
    nack_all = 1; CHECK(ds1307_read(&iic, &u, 0) == IIC_ERR_NACK); nack_all = 0;

    /* ---- 24xx256 ---- */
    eeprom24_t e;
    CHECK(eeprom24_init(&e, &iic, 0x4F, 32768, 64) == IIC_ERR_ARGS && eeprom24_init(&e, &iic, 0x58, 32768, 64) == IIC_ERR_ARGS);
    CHECK(eeprom24_init(&e, &iic, 0x50, 32768, 48) == IIC_ERR_ARGS && eeprom24_init(&e, &iic, 0x50, 32768, 256) == IIC_ERR_ARGS);
    CHECK(eeprom24_init(&e, &iic, 0x50, 32768, 64) == IIC_OK);
    static uint8_t ref[32768], wr[1000], rd[1000];
    memset(mem, 0xFF, sizeof mem); memset(ref, 0xFF, sizeof ref);
    for (int i = 0; i < 1000; i++) wr[i] = (uint8_t)(i * 31 + 5);
    CHECK(eeprom24_write(&e, 60, wr, 5) == IIC_OK);            /* 페이지 경계(64) 걸침 */
    memcpy(ref + 60, wr, 5);
    CHECK(bad_boundary == 0 && chunks == 2 && polls == 2 * busy_len);
    CHECK(eptr == 65);                                         /* 폴링용 주소 설정이 쓴 끝 다음 위치(상위 바이트 먼저)를 가리킴 */
    CHECK(eeprom24_read(&e, 60, rd, 5) == IIC_OK && !memcmp(rd, wr, 5));
    CHECK(eeprom24_write(&e, 100, wr, 1000) == IIC_OK); memcpy(ref + 100, wr, 1000);
    CHECK(bad_boundary == 0 && !memcmp(mem, ref, sizeof mem));
    CHECK(eeprom24_read(&e, 100, rd, 1000) == IIC_OK && !memcmp(rd, wr, 1000));   /* 255 바이트 단위 분할 읽기 */
    CHECK(eeprom24_write(&e, 32767, wr, 1) == IIC_OK && mem[32767] == wr[0]);     /* 마지막 바이트 */
    CHECK(eeprom24_write(&e, 32767, wr, 2) == IIC_ERR_ARGS && eeprom24_write(&e, 32768, wr, 1) == IIC_ERR_ARGS);
    CHECK(eeprom24_read(&e, 32767, rd, 2) == IIC_ERR_ARGS && eeprom24_write(&e, 0, wr, 0) == IIC_ERR_ARGS);
    /* 무작위 시험: 임의 주소·길이 쓰기가 기준 메모리와 일치 */
    srand(7);
    for (int k = 0; k < 300; k++) {
        uint32_t a = (uint32_t)rand() % 32768, l = 1 + (uint32_t)rand() % 300; if (a + l > 32768) l = 32768 - a;
        for (uint32_t i = 0; i < l; i++) wr[i] = (uint8_t)rand();
        CHECK(eeprom24_write(&e, a, wr, l) == IIC_OK); memcpy(ref + a, wr, l);
    }
    CHECK(bad_boundary == 0 && !memcmp(mem, ref, sizeof mem));
    /* 쓰기 사이클이 끝나지 않으면 시간 초과 */
    busy_len = 1000; CHECK(eeprom24_write(&e, 0, wr, 1) == IIC_ERR_TIMEOUT); busy_left = 0; busy_len = 3;
    nack_all = 1; CHECK(eeprom24_write(&e, 0, wr, 1) == IIC_ERR_NACK); nack_all = 0;
    if (!bad) puts("ALL PASS");
    return bad != 0;
}
