/* DHT22 드라이버 호스트 시험. 모의 소자는 dht22_axil_regs.v 의 레지스터 동작(머리말 주석)을 흉내 낸다 — 비트 타이밍(RTL 엔진)은 시험하지 않는다.
 * 한계: 모델은 내가 RTL 주석을 읽고 만든 것이라 RTL 과 어긋나도 모른다. 부호 표현·체크섬은 RIOT-OS 코드 기준(D3). */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "dht22.h"

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

static struct {
    uint8_t frame[5];                 /* 센서가 보낸 5바이트(상위 바이트 먼저) */
    int err, force_csum_bad, dead, no_done;
    uint32_t busy_polls, busy_left, nstarts, violations, stat_reads;
    int done, started_ok;
    uint32_t cmd_log[8]; int ncmd;
} M;

static void set_frame(unsigned rh, unsigned t)
{
    M.frame[0] = (uint8_t)(rh >> 8); M.frame[1] = (uint8_t)rh; M.frame[2] = (uint8_t)(t >> 8); M.frame[3] = (uint8_t)t;
    M.frame[4] = (uint8_t)(M.frame[0] + M.frame[1] + M.frame[2] + M.frame[3]);
}
static void fresh(void) { memset(&M, 0, sizeof M); M.busy_polls = 3; set_frame(652, 351); }

uint32_t mock_dht_rd(uint32_t off)
{
    switch (off) {
    case 0x04: {
        M.stat_reads++;
        if (M.dead) return 0x1;
        if (M.busy_left) { M.busy_left--; return 0x1; }
        if (M.no_done) return 0x0;
        uint32_t s = M.done ? 0x2u : 0u;
        if (M.done && M.err) s |= 0x4;
        int csum_ok = (uint8_t)(M.frame[0] + M.frame[1] + M.frame[2] + M.frame[3]) == M.frame[4] && !M.force_csum_bad;
        if (M.done && !M.err && csum_ok) s |= 0x8;
        return s;
    }
    case 0x08: return (uint32_t)(M.frame[0] << 8 | M.frame[1]);
    case 0x0C: return (uint32_t)(M.frame[2] << 8 | M.frame[3]);
    case 0x10: return M.frame[4];
    default: return 0;
    }
}
void mock_dht_wr(uint32_t off, uint32_t v)
{
    if (off != 0x00) return;
    if (M.busy_left) { M.violations++; return; }                   /* busy 중 명령은 무시(RTL) — 드라이버가 이러면 안 된다 */
    if (v & 1) { M.nstarts++; M.done = 1; M.busy_left = M.busy_polls; }
    /* RTL: done 은 새 명령 때 지워졌다가 끝나면 다시 켜진다. 모델은 busy 가 끝난 뒤에만 done 으로 보이게 한다 */
}

#define BASE ((uintptr_t)0x44A20000u)

static void t_raw(void)
{
    uint16_t rh = 1, t = 2; uint8_t chk = 9;
    fresh(); set_frame(652, 351);                                   /* 65.2 %RH, 35.1 °C */
    CHECK(dht22_read_raw(BASE, &rh, &t, &chk) == DHT_OK && rh == 652 && t == 351 && chk == (uint8_t)(2 + 140 + 1 + 95));
    CHECK(M.nstarts == 1 && M.violations == 0);
    CHECK(dht22_read_raw(BASE, &rh, &t, 0) == DHT_OK);               /* chk 생략 가능 */
    CHECK(M.nstarts == 2 && M.violations == 0);
    fresh(); M.err = 1; rh = 7; t = 8;
    CHECK(dht22_read_raw(BASE, &rh, &t, 0) == DHT_ERR_NORESP && rh == 7 && t == 8);
    fresh(); M.force_csum_bad = 1; rh = 7; t = 8;
    CHECK(dht22_read_raw(BASE, &rh, &t, 0) == DHT_ERR_CSUM && rh == 7 && t == 8);
    fresh(); M.frame[4] ^= 0x01; CHECK(dht22_read_raw(BASE, &rh, &t, 0) == DHT_ERR_CSUM);       /* 체크섬 바이트 한 비트 */
    fresh(); M.frame[1] ^= 0x10; CHECK(dht22_read_raw(BASE, &rh, &t, 0) == DHT_ERR_CSUM);       /* 데이터 한 비트 */
    fresh(); M.err = 1; M.force_csum_bad = 1; CHECK(dht22_read_raw(BASE, &rh, &t, 0) == DHT_ERR_NORESP);   /* err 가 csum 보다 먼저 */
    CHECK(dht22_read_raw(BASE, 0, &t, 0) == DHT_ERR_ARGS && dht22_read_raw(BASE, &rh, 0, 0) == DHT_ERR_ARGS);
    fresh(); M.dead = 1; M.stat_reads = 0;
    CHECK(dht22_read_raw(BASE, &rh, &t, 0) == DHT_ERR_BUS && M.nstarts == 0 && M.stat_reads == 1000001);
    fresh(); M.no_done = 1; M.stat_reads = 0;
    CHECK(dht22_read_raw(BASE, &rh, &t, 0) == DHT_ERR_BUS && M.nstarts == 1 && M.stat_reads == 1 + 1000001);
    fresh(); M.busy_polls = 0; CHECK(dht22_read_raw(BASE, &rh, &t, 0) == DHT_OK && M.violations == 0);
    fresh(); M.busy_polls = 50; CHECK(dht22_read_raw(BASE, &rh, &t, 0) == DHT_OK && M.violations == 0);
    /* 이미 busy 인 채로 시작하면 끝나기를 기다린 뒤 명령 */
    fresh(); M.busy_left = 9; CHECK(dht22_read_raw(BASE, &rh, &t, 0) == DHT_OK && M.violations == 0 && M.nstarts == 1);
}

static void t_decode(void)
{
    CHECK(dht22_temp_x10(0) == 0 && dht22_temp_x10(351) == 351 && dht22_temp_x10(0x7FFF) == 32767);
    CHECK(dht22_temp_x10(0x8000) == 0);                              /* 음의 0 */
    CHECK(dht22_temp_x10(0x8001) == -1 && dht22_temp_x10(0x8065) == -101 && dht22_temp_x10(0x8190) == -400);
    CHECK(dht22_temp_x10(0xFFFF) == -32767);
    CHECK(dht22_temp_x10(0x0320) == 800);
    /* 2의 보수로 잘못 읽으면 0x8001 이 -32767 이 된다 → 부호-크기 표현임을 가르는 값 */
    CHECK(dht22_temp_x10(0x8001) != (int16_t)0x8001);
}

static void t_read(void)
{
    int32_t rh = -1, t = -1;
    fresh(); set_frame(652, 351); CHECK(dht22_read(BASE, &rh, &t) == DHT_OK && rh == 652 && t == 351);
    fresh(); set_frame(1000, 0x8000 | 400); CHECK(dht22_read(BASE, &rh, &t) == DHT_OK && rh == 1000 && t == -400);    /* 범위 끝 */
    fresh(); set_frame(0, 800); CHECK(dht22_read(BASE, &rh, &t) == DHT_OK && rh == 0 && t == 800);
    rh = 5; t = 6;
    fresh(); set_frame(1001, 100); CHECK(dht22_read(BASE, &rh, &t) == DHT_ERR_RANGE && rh == 5 && t == 6);
    fresh(); set_frame(500, 801);  CHECK(dht22_read(BASE, &rh, &t) == DHT_ERR_RANGE && rh == 5 && t == 6);
    fresh(); set_frame(500, 0x8000 | 401); CHECK(dht22_read(BASE, &rh, &t) == DHT_ERR_RANGE && rh == 5 && t == 6);
    fresh(); M.err = 1; CHECK(dht22_read(BASE, &rh, &t) == DHT_ERR_NORESP);
    fresh(); M.force_csum_bad = 1; CHECK(dht22_read(BASE, &rh, &t) == DHT_ERR_CSUM);
    CHECK(dht22_read(BASE, 0, &t) == DHT_ERR_ARGS && dht22_read(BASE, &rh, 0) == DHT_ERR_ARGS);
    CHECK(DHT_OK == 0 && DHT_ERR_NORESP == -1 && DHT_ERR_CSUM == -2 && DHT_ERR_ARGS == -3 && DHT_ERR_BUS == -4 && DHT_ERR_RANGE == -5 && DHT_ERR_TOOSOON == -6);
}

static void t_paced(void)
{
    dht22_t d; int32_t rh = 0, t = 0;
    fresh(); dht22_init(&d, BASE);
    CHECK(dht22_read_paced(&d, 0, &rh, &t) == DHT_OK && M.nstarts == 1);                    /* 처음에는 시각 0 이어도 읽는다 */
    CHECK(dht22_read_paced(&d, 1999, &rh, &t) == DHT_ERR_TOOSOON && M.nstarts == 1);
    CHECK(dht22_read_paced(&d, 2000, &rh, &t) == DHT_ERR_TOOSOON && M.nstarts == 1);        /* "2 s 초과"이므로 정확히 2000 은 아직 */
    CHECK(dht22_read_paced(&d, 2001, &rh, &t) == DHT_OK && M.nstarts == 2);
    CHECK(dht22_read_paced(&d, 4001, &rh, &t) == DHT_ERR_TOOSOON && M.nstarts == 2);       /* 기준은 지난 읽기 시각 */
    CHECK(dht22_read_paced(&d, 4002, &rh, &t) == DHT_OK && M.nstarts == 3);
    M.err = 1;                                                                               /* 실패한 읽기도 간격 기준을 갱신한다 */
    CHECK(dht22_read_paced(&d, 9000, &rh, &t) == DHT_ERR_NORESP);
    M.err = 0;
    CHECK(dht22_read_paced(&d, 10000, &rh, &t) == DHT_ERR_TOOSOON);
    CHECK(dht22_read_paced(&d, 11001, &rh, &t) == DHT_OK);
    /* 32비트 시계가 넘쳐도 간격이 맞음 */
    fresh(); dht22_init(&d, BASE);
    CHECK(dht22_read_paced(&d, 0xFFFFFC00u, &rh, &t) == DHT_OK);
    CHECK(dht22_read_paced(&d, 0x00000000u, &rh, &t) == DHT_ERR_TOOSOON);                   /* 1024 ms 지남 */
    CHECK(dht22_read_paced(&d, 976, &rh, &t) == DHT_ERR_TOOSOON);                           /* 1024+976 = 2000 ms: 초과가 아님 */
    CHECK(dht22_read_paced(&d, 977, &rh, &t) == DHT_OK);                                    /* 2001 ms */
    CHECK(dht22_read_paced(0, 5000, &rh, &t) == DHT_ERR_ARGS && dht22_read_paced(&d, 5000, 0, &t) == DHT_ERR_ARGS && dht22_read_paced(&d, 5000, &rh, 0) == DHT_ERR_ARGS);
    dht22_init(&d, BASE); CHECK(d.base == BASE && d.has_last == 0);
    dht22_init(0, BASE);
    CHECK(DHT_MIN_PERIOD_MS == 2000 && DHT_SIGN_BIT == 0x8000);
}

static void t_property(void)
{
    srand(5);
    for (int n = 0; n < 5000; n++) {
        unsigned rh = (unsigned)(rand() % 1100), tm = (unsigned)(rand() % 900); int neg = rand() & 1;
        unsigned traw = (neg ? 0x8000u : 0) | tm;
        fresh(); set_frame(rh, traw);
        int32_t r, t; int rc = dht22_read(BASE, &r, &t);
        int32_t tv = neg ? -(int32_t)tm : (int32_t)tm;
        int inrange = rh <= 1000 && tv <= 800 && tv >= -400;
        CHECK(inrange ? (rc == DHT_OK && r == (int32_t)rh && t == tv) : rc == DHT_ERR_RANGE);
    }
}

int main(void)
{
    t_raw(); t_decode(); t_read(); t_paced(); t_property();
    printf("dht22: %d 검사, 실패 %d\n", checks, fails);
    if (fails) return 1;
    printf("ALL PASS\n");
    return 0;
}
