#include <stdio.h>
#include <string.h>
#include "../sw/uartlite.h"
#include "../sw/nmea.h"
#include "../sw/hc05.h"

/* 모의 UART Lite: rx 큐, tx 로그, 오류 비트는 STAT 읽기 시 지워짐 */
static uint8_t rxq[512]; static unsigned rh, rt;
static char txlog[256]; static unsigned txn; static unsigned txcap = 16;
static uint32_t pend_err; static int slverr;
static void feed(const char *s) { while (*s) rxq[rt++ % 512] = (uint8_t)*s++; }
uint32_t mock_ul_rd(uint32_t off)
{
    if (off == UL_STAT) {
        uint32_t s = pend_err | 4; pend_err = 0;
        if (rh != rt) s |= 1;
        if (txn >= txcap) s |= 8;
        return s;
    }
    if (off == UL_RX) { if (rh == rt) { slverr++; return 0; } return rxq[rh++ % 512]; }
    return 0;
}
void mock_ul_wr(uint32_t off, uint32_t v)
{
    if (off == UL_TX) txlog[txn++ % 256] = (char)v;
    if (off == UL_CTRL && (v & 3)) { rh = rt = 0; }
}

static int fails;
#define CHK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static int cnt; static int poll_ok(void *c) { (void)c; return ++cnt < 1000; }

int main(void)
{
    uartlite_t u; uartlite_init(&u, 0);
    CHK(uartlite_getc(&u) == -1 && slverr == 0);       /* 빈 FIFO 는 읽지 않음 */
    feed("A"); CHK(uartlite_getc(&u) == 'A');
    pend_err = UL_ST_FRAME; (void)uartlite_getc(&u);
    CHK(uartlite_take_errors(&u) == UL_ST_FRAME);      /* 읽으면 지워지는 비트를 누적 */
    CHK(uartlite_take_errors(&u) == 0);

    /* 체크섬: 직접 계산한 문장 */
    const char *body = "GPRMC,123519.00,A,4807.038,N,01131.000,E,022.4,084.4,230394,,,A";
    uint8_t x = 0; for (const char *p = body; *p; p++) x ^= (uint8_t)*p;
    char sent[160]; snprintf(sent, sizeof sent, "$%s*%02X\r\n", body, x);
    nmea_asm_t a; nmea_asm_init(&a);
    int done = 0; for (const char *p = sent; *p; p++) if (nmea_feed(&a, *p) == 1) done++;
    CHK(done == 1 && nmea_check(a.buf));
    nmea_rmc_t r; CHK(nmea_parse_rmc(a.buf, &r) == 1);
    CHK(r.valid && r.hh == 12 && r.mm == 35 && r.ss == 19);
    CHK(r.day == 23 && r.mon == 3 && r.year2 == 94);
    CHK(r.lat_e7 == 481173000 || r.lat_e7 == 481173333);   /* 48 + 7.038/60 = 48.1173 */
    CHK(r.lon_e7 > 115166000 && r.lon_e7 < 115167000);     /* 11 + 31.0/60 = 11.51667 */
    CHK(r.speed_cent_kn == 2240);

    /* 잘못된 체크섬, 다른 문장, 남/서, dddmm 위도 */
    char bad[160]; snprintf(bad, sizeof bad, "%s", a.buf); bad[strlen(bad) - 1] ^= 1;
    CHK(nmea_parse_rmc(bad, &r) == -1);
    CHK(nmea_parse_rmc("GPGGA,1,2*00", &r) == 0);
    const char *b2 = "GPRMC,000000.00,V,3351.000,S,15112.000,W,0.0,0.0,010120,,,N";
    x = 0; for (const char *p = b2; *p; p++) x ^= (uint8_t)*p;
    char s2[160]; snprintf(s2, sizeof s2, "%s*%02X", b2, x);
    CHK(nmea_parse_rmc(s2, &r) == 1 && !r.valid && r.lat_e7 < 0 && r.lon_e7 < 0);
    /* 넘침 */
    nmea_asm_init(&a); nmea_feed(&a, '$'); int ov = 0;
    for (int i = 0; i < 200; i++) if (nmea_feed(&a, 'X') == -1) ov = 1;
    CHK(ov);

    /* HC-05 */
    char c[48];
    CHK(hc05_cmd_uart(c, sizeof c, 9600, 1, 0) > 0 && strcmp(c, "AT+UART=9600,1,0\r\n") == 0);
    CHK(hc05_cmd_name(c, 8, "veryverylong") == -1);
    txn = 0; cnt = 0; feed("OK\r\n");
    CHK(hc05_send(&u, "AT\r\n", poll_ok, 0) == 0 && strncmp(txlog, "AT\r\n", 4) == 0);
    feed("ERROR:(0)\r\n"); cnt = 0; txn = 0;
    CHK(hc05_send(&u, "AT+X\r\n", poll_ok, 0) == -2);
    cnt = 0; CHK(hc05_send(&u, "AT\r\n", poll_ok, 0) == -1);
    CHK(slverr == 0);

    printf(fails ? "FAILED %d\n" : "ALL PASS\n", fails);
    return fails != 0;
}
