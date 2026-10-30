/* 아날로그 센서 변환 시험: 손으로 계산한 값, 경계, 독립 계산(__int128 정수 반올림)과의 대조, 단조성·범위 불변식. 보드 측정이 아니다. */
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include "../sw/analog_sensors.h"

static int fails;
#define CHK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
static long long rnd_away(long long num, long long den) { return num >= 0 ? (num + den / 2) / den : -((-num + den / 2) / den); }

int main(void)
{
    /* ---- LM35 ---- */
    CHK(lm35_uv_to_mdegc(250000) == 25000 && lm35_uv_to_mdegc(0) == 0 && lm35_uv_to_mdegc(1500000) == 150000);
    CHK(lm35_uv_to_mdegc(255) == 26 && lm35_uv_to_mdegc(254) == 25 && lm35_uv_to_mdegc(-255) == -26 && lm35_uv_to_mdegc(-254) == -25);
    CHK(lm35_uv_to_mdegc(-550000) == -55000);
    CHK(lm35_mv_to_mdegc(750) == 75000 && lm35_mv_to_mdegc(-550) == -55000 && lm35_mv_to_mdegc(0) == 0 && lm35_mv_to_mdegc(1) == 100);
    CHK(lm35_mv_to_mdegc(INT32_MAX) == INT32_MAX && lm35_mv_to_mdegc(INT32_MIN) == INT32_MIN);
    for (int32_t uv = -600000; uv <= 1600000; uv += 977) CHK(lm35_uv_to_mdegc(uv) == rnd_away(uv, 10));
    CHK(lm35_in_range(AN_LM35_FULL, -55000) == 1 && lm35_in_range(AN_LM35_FULL, -55001) == 0 && lm35_in_range(AN_LM35_FULL, 150000) == 1 && lm35_in_range(AN_LM35_FULL, 150001) == 0);
    CHK(lm35_in_range(AN_LM35_C, -40000) == 1 && lm35_in_range(AN_LM35_C, -40001) == 0 && lm35_in_range(AN_LM35_C, 110000) == 1 && lm35_in_range(AN_LM35_C, 110001) == 0);
    CHK(lm35_in_range(AN_LM35_D, 0) == 1 && lm35_in_range(AN_LM35_D, -1) == 0 && lm35_in_range(AN_LM35_D, 100000) == 1 && lm35_in_range(AN_LM35_D, 100001) == 0);
    CHK(lm35_in_range(3, 0) == AN_ERR_ARGS && lm35_in_range(-1, 0) == AN_ERR_ARGS);
    int32_t m = 777;
    CHK(lm35_read_uv(AN_LM35_D, 300000, &m) == AN_OK && m == 30000);
    CHK(lm35_read_uv(AN_LM35_D, -100000, &m) == AN_ERR_RANGE && m == -10000);          /* 범위 밖이어도 값은 돌려줌 */
    CHK(lm35_read_uv(AN_LM35_D, 1000000, &m) == AN_OK && m == 100000);                  /* 100 °C 는 범위 안(포함) */
    CHK(lm35_read_uv(AN_LM35_D, 1000010, &m) == AN_ERR_RANGE && m == 100001);
    m = 777; CHK(lm35_read_uv(7, 300000, &m) == AN_ERR_ARGS && m == 777);              /* 오류 때는 건드리지 않음 */
    CHK(lm35_read_uv(AN_LM35_D, 300000, 0) == AN_ERR_ARGS);
    CHK(lm35_read_uv(AN_LM35_FULL, -500000, &m) == AN_OK && m == -50000);
    /* ADS1115 PGA ±4.096 V 에서 1 LSB = 125 µV → 12.5 m°C: 한 코드가 0.0125 °C */
    CHK(lm35_uv_to_mdegc(125) == 13 && lm35_uv_to_mdegc(250) == 25);
    /* ---- LDR ---- */
    uint32_t r;
    CHK(ldr_ohms(1650, 3300, 10000, 0, &r) == AN_OK && r == 10000);
    CHK(ldr_ohms(1100, 3300, 10000, 0, &r) == AN_OK && r == 5000);
    CHK(ldr_ohms(1100, 3300, 10000, 1, &r) == AN_OK && r == 20000);
    CHK(ldr_ohms(2200, 3300, 10000, 0, &r) == AN_OK && r == 20000);
    CHK(ldr_ohms(2200, 3300, 10000, 1, &r) == AN_OK && r == 5000);
    CHK(ldr_ohms(1, 3, 3, 0, &r) == AN_OK && r == 2 /* 3·1/2 = 1.5 → 2 */ && ldr_ohms(1, 3, 3, 1, &r) == AN_OK && r == 6);
    CHK(ldr_ohms(0, 3300, 10000, 0, &r) == AN_ERR_RANGE && ldr_ohms(3300, 3300, 10000, 0, &r) == AN_ERR_RANGE && ldr_ohms(-5, 3300, 10000, 1, &r) == AN_ERR_RANGE && ldr_ohms(3301, 3300, 10000, 1, &r) == AN_ERR_RANGE);
    CHK(ldr_ohms(1, 3300, 10000, 0, 0) == AN_ERR_ARGS && ldr_ohms(1, 0, 10000, 0, &r) == AN_ERR_ARGS && ldr_ohms(1, 3300, 0, 0, &r) == AN_ERR_ARGS && ldr_ohms(1, -3300, 10000, 0, &r) == AN_ERR_ARGS);
    CHK(ldr_ohms(3299, 3300, 4000000000u, 0, &r) == AN_ERR_RANGE);                        /* 32비트를 넘으면 오류 */
    CHK(ldr_ohms(3299, 3300, 4000000u, 0, &r) == AN_ERR_RANGE);                            /* 4e6·3299/1 > 2^32 */
    CHK(ldr_ohms(3299, 3300, 4000000u, 1, &r) == AN_OK && r == 1212);                       /* 4e6·1/3299 = 1212.4 */
    for (int v = 1; v < 3300; v += 37) for (int top = 0; top < 2; top++) {
        unsigned __int128 num = top ? (unsigned __int128)10000 * (3300 - v) : (unsigned __int128)10000 * v; unsigned den = top ? v : 3300 - v;
        CHK(ldr_ohms(v, 3300, 10000, top, &r) == AN_OK && r == (uint32_t)((num + den / 2) / den));
    }
    /* 밝을수록 저항이 작다: LDR 아래쪽이면 전압이 내려가고 위쪽이면 올라간다 */
    uint32_t r1, r2; ldr_ohms(2000, 3300, 10000, 1, &r1); ldr_ohms(2500, 3300, 10000, 1, &r2); CHK(r2 < r1);
    double lux;
    CHK(ldr_lux(10000, 10000, 1.0, &lux) == AN_OK && fabs(lux - 10.0) < 1e-9);
    CHK(ldr_lux(1000, 10000, 1.0, &lux) == AN_OK && fabs(lux - 100.0) < 1e-9);
    CHK(ldr_lux(2500, 10000, 0.5, &lux) == AN_OK && fabs(lux - 160.0) < 1e-9);
    CHK(ldr_lux(0, 10000, 0.5, &lux) == AN_ERR_ARGS && ldr_lux(10, 0, 0.5, &lux) == AN_ERR_ARGS && ldr_lux(10, 10, 0.0, &lux) == AN_ERR_ARGS && ldr_lux(10, 10, -1.0, &lux) == AN_ERR_ARGS && ldr_lux(10, 10, 1.0, 0) == AN_ERR_ARGS);
    /* ---- map / percent ---- */
    CHK(an_map(512, 0, 1023, 0, 255) == 128 && an_map(0, 0, 1023, 0, 255) == 0 && an_map(1023, 0, 1023, 0, 255) == 255);
    CHK(an_map(256, 0, 1024, 100, 0) == 75 && an_map(-5, -10, 10, 0, 100) == 25 && an_map(2048, 0, 1024, 0, 100) == 200);
    CHK(an_map(1, 10, 0, 0, 100) == 90 && an_map(5, 3, 3, 7, 9) == 7);
    CHK(an_map(1, 0, 3, 0, 1) == 0 && an_map(2, 0, 3, 0, 1) == 1 && an_map(-1, 0, 3, 0, 1) == 0 && an_map(-2, 0, 3, 0, 1) == -1);    /* 1/3→0, 2/3→1, −1/3→0, −2/3→−1 */
    CHK(an_map(INT32_MAX, 0, INT32_MAX, 0, 1000) == 1000 && an_map(INT32_MIN, INT32_MIN, INT32_MAX, -1000, 1000) == -1000);
    CHK(an_map(INT32_MAX, INT32_MIN, INT32_MAX, -1000, 1000) == 1000 && an_map(5, 0, 10, INT32_MIN, INT32_MAX) == 0);   /* 중간 계산이 32비트를 넘는 경우 */
    CHK(an_map(INT32_MAX, 0, 1, 0, INT32_MAX) == INT32_MAX && an_map(-INT32_MAX, 0, 1, 0, INT32_MAX) == INT32_MIN);   /* 결과 포화 */
    CHK(an_percent(0, 1023) == 0 && an_percent(1023, 1023) == 100 && an_percent(512, 1023) == 50 && an_percent(511, 1023) == 50 && an_percent(2000, 1023) == 100);
    CHK(an_percent(5, 1000) == 1 && an_percent(4, 1000) == 0 && an_percent(1, 0) == AN_ERR_ARGS && an_percent(0xFFFFFFFFu, 0xFFFFFFFFu) == 100);
    { int prev = -1, ok = 1; for (unsigned x = 0; x <= 1023; x++) { int p = an_percent(x, 1023); if (p < prev || p < 0 || p > 100) ok = 0; prev = p; } CHK(ok); }
    /* ---- 조이스틱 ---- */
    CHK(joy_axis(512, 512, 20, 1023) == 0 && joy_axis(532, 512, 20, 1023) == 0 && joy_axis(492, 512, 20, 1023) == 0 && joy_axis(533, 512, 20, 1023) == 0 && joy_axis(491, 512, 20, 1023) == 0);
    CHK(joy_axis(600, 512, 20, 1023) == 14 && joy_axis(1023, 512, 20, 1023) == 100 && joy_axis(0, 512, 20, 1023) == -100 && joy_axis(5000, 512, 20, 1023) == 100);
    CHK(joy_axis(246, 512, 20, 1023) == -50 && joy_axis(369, 512, 20, 1023) == -25);
    CHK(joy_axis(5, 10, 20, 100) == 0 && joy_axis(0, 10, 20, 100) == 0);                 /* 데드존이 중심보다 크면 아래쪽 반응 없음 */
    CHK(joy_axis(30, 10, 20, 100) == 0 && joy_axis(31, 10, 20, 100) == 1 && joy_axis(100, 10, 20, 100) == 100);
    CHK(joy_axis(0, 100, 0, 100) == -100 && joy_axis(100, 100, 0, 100) == 0 && joy_axis(99, 100, 0, 100) == -1);
    CHK(joy_axis(1, 0, 0, 100) == 1 && joy_axis(0, 0, 0, 100) == 0);
    CHK(joy_axis(100, 50, 0xFFFFFFFFu, 100) == 0);                                        /* dz 가 매우 커도 더하기가 넘치지 않음 */
    CHK(joy_axis(1, 0, 0, 0) == AN_ERR_ARGS && joy_axis(400, 512, 20, 1023) == -19);
    CHK(joy_axis(1, 2, 0, 0) == AN_ERR_ARGS && joy_axis(1, 101, 0, 100) == AN_ERR_ARGS);
    /* 불변식: 값 범위, 단조 증가, 데드존 안 0, 끝 값 */
    static const unsigned cd[][2] = { {512, 20}, {512, 0}, {300, 40}, {800, 5}, {0, 0}, {1023, 0}, {10, 30}, {1000, 100} };
    for (unsigned k = 0; k < sizeof cd / sizeof *cd; k++) {
        int prev = -101, ok = 1; unsigned c = cd[k][0], dz = cd[k][1];
        for (unsigned x = 0; x <= 1023; x++) {
            int v = joy_axis(x, c, dz, 1023);
            if (v < -100 || v > 100 || v < prev) ok = 0;
            if (x >= (c > dz ? c - dz : 0) && x <= c + dz && v != 0) ok = 0;
            prev = v;
        }
        CHK(ok);
        if (c + dz < 1023) CHK(joy_axis(1023, c, dz, 1023) == 100);
        if (c > dz) CHK(joy_axis(0, c, dz, 1023) == -100);
    }
    /* ---- CdS 상대 밝기 ---- */
    CHK(cds_brightness_pct(0, 1023, 1) == 0 && cds_brightness_pct(1023, 1023, 1) == 100 && cds_brightness_pct(512, 1023, 1) == 50);
    CHK(cds_brightness_pct(0, 1023, 0) == 100 && cds_brightness_pct(1023, 1023, 0) == 0 && cds_brightness_pct(512, 1023, 0) == 50);
    CHK(cds_brightness_pct(2000, 1023, 1) == 100 && cds_brightness_pct(2000, 1023, 0) == 0);        /* 초과는 raw_max 로 */
    CHK(cds_brightness_pct(1, 3, 1) == 33 && cds_brightness_pct(2, 3, 1) == 67 && cds_brightness_pct(1, 3, 0) == 67);
    CHK(cds_brightness_pct(5, 0, 1) == AN_ERR_ARGS && cds_brightness_pct(0, 0, 0) == AN_ERR_ARGS);
    for (uint32_t r = 0; r <= 1023; r++) {                                                           /* 위쪽/아래쪽 배선은 서로 거울상, 단조 */
        int a = cds_brightness_pct(r, 1023, 1), b = cds_brightness_pct(r, 1023, 0);
        CHK(a + b == 100 && a >= 0 && a <= 100);
        if (r) CHK(a >= cds_brightness_pct(r - 1, 1023, 1) && b <= cds_brightness_pct(r - 1, 1023, 0));
    }
    cds_switch_t sw;
    CHK(cds_switch_init(&sw, 30, 60) == AN_OK && sw.bright == 0);
    CHK(cds_switch_update(&sw, 45) == 0 && cds_switch_update(&sw, 59) == 0 && cds_switch_update(&sw, 60) == 1);     /* 60 이상이면 밝음 */
    CHK(cds_switch_update(&sw, 45) == 1 && cds_switch_update(&sw, 31) == 1 && cds_switch_update(&sw, 30) == 0);     /* 30 이하이면 어두움 */
    CHK(cds_switch_update(&sw, 59) == 0 && cds_switch_update(&sw, 100) == 1 && cds_switch_update(&sw, 0) == 0);
    CHK(cds_switch_init(&sw, 50, 50) == AN_ERR_ARGS && cds_switch_init(&sw, 60, 30) == AN_ERR_ARGS && cds_switch_init(&sw, 0, 101) == AN_ERR_ARGS && cds_switch_init(0, 10, 20) == AN_ERR_ARGS);
    CHK(cds_switch_init(&sw, 0, 100) == AN_OK && cds_switch_update(&sw, 0) == 0 && cds_switch_update(&sw, 100) == 1 && cds_switch_update(0, 5) == AN_ERR_ARGS);
    printf(fails ? "FAILED %d\n" : "ALL PASS (analog sensors)\n", fails);
    return fails != 0;
}
