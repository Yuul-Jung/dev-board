/* 적대적 시험: 현실적인 범위의 임의 보정 계수·원시값으로 UBSan 아래에서 보정식을 돌려 오버플로·미정의 동작을 찾는다. */
#include <stdio.h>
#include <stdlib.h>
#include "../sw/bme280.h"
/* Bosch 원문 32비트 습도식(참조). 넘침이 없는 입력에서만 비교한다 */
static uint32_t ref_H(const bme280_calib_t *c, int32_t adc_H, int32_t t_fine, int *overflow)
{
    int64_t x = (int64_t)t_fine - 76800, a;
    a = ((int64_t)adc_H * 16384) - ((int64_t)c->H4 * 1048576) - ((int64_t)c->H5 * x) + 16384;
    int64_t u = (a >> 15);
    int64_t v1 = ((x * (int64_t)c->H6) >> 10), v2 = (((x * (int64_t)c->H3) >> 11) + 32768), v3 = ((v1 * v2) >> 10) + 2097152;
    int64_t w = (v3 * c->H2 + 8192) >> 14, y = u * w;
    int64_t z = ((((y >> 15) * (y >> 15)) >> 7) * c->H1) >> 4;
    int64_t lim = 2147483647LL;
    if (a > lim || a < -lim - 1 || v1 > lim || v1 < -lim - 1 || v2 > lim || v3 > lim || v3 < -lim - 1 || v3 * c->H2 > lim || v3 * c->H2 < -lim - 1 ||
        y > lim || y < -lim - 1 || (y >> 15) * (y >> 15) > lim || (((y >> 15) * (y >> 15)) >> 7) * c->H1 > lim) *overflow = 1;
    y -= z; if (y < 0) y = 0; if (y > 419430400) y = 419430400;
    return (uint32_t)(y >> 12);
}
static uint32_t rs = 12345;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs >> 8; }
static int32_t around(int32_t v, int pct) { int32_t d = (int32_t)((int64_t)v * pct / 100); if (d < 0) d = -d; if (!d) d = 1; return v + (int32_t)(rnd() % (2 * d + 1)) - d; }
int main(void)
{
    long bad = 0, compared = 0;
    for (long it = 0; it < 2000000; it++) {
        bme280_calib_t c = { around(27504, 15), around(26435, 15), around(-1000, 50), around(36477, 15), around(-10685, 15), around(3024, 50),
                             around(2855, 50), around(140, 50), around(-7, 100), around(15500, 30), around(-14600, 30), around(6000, 30),
                             around(75, 30), around(362, 30), around(0, 1), around(317, 30), around(50, 30), around(30, 100) };
        c.T1 = (uint16_t)c.T1; c.H1 = (uint8_t)c.H1; c.H3 = (uint8_t)c.H3; c.H6 = (int8_t)c.H6;
        int32_t aT = 400000 + (int32_t)(rnd() % 250000);          /* 약 -10~60 °C 에 해당하는 대역 */
        int32_t aP = 300000 + (int32_t)(rnd() % 250000);
        int32_t aH = (int32_t)(rnd() % 65536);
        int32_t tf; int32_t T = bme280_comp_T(&c, aT, &tf);
        uint32_t P = bme280_comp_P(&c, aP, tf), H = bme280_comp_H(&c, aH, tf);
        int ovf = 0; uint32_t R = ref_H(&c, aH, tf, &ovf);
        if (!ovf) compared++;
        if (!ovf && R != H) bad++;                                 /* 넘침 없는 입력에서 int64 구현과 32비트 기준식이 달라지면 안 됨 */
        if (H > 102400) bad++;                                    /* 습도 상한(100 %RH = 102400)을 넘으면 안 됨 */
        (void)T; (void)P;
    }
    printf("비교한 입력 %ld, 불일치/상한 초과 %ld\n%s\n", compared, bad, bad ? "FAILED" : "ALL PASS");
    return bad != 0;
}
