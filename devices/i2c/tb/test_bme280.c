#include <stdio.h>
#include "../sw/bme280.h"
static int fails;
#define CHK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
int main(void)
{
    /* 계수: 널리 쓰이는 BMP280 데이터시트 예제 값(T,P). 습도 계수는 임의의 현실적 값 — 습도는 범위·단조성만 검사 */
    bme280_calib_t c = { 27504, 26435, -1000, 36477, -10685, 3024, 2855, 140, -7, 15500, -14600, 6000,
                         75, 362, 0, 317, 50, 30 };
    int32_t tf; int32_t T = bme280_comp_T(&c, 519888, &tf);
    CHK(T == 2508);                                         /* 25.08 °C (BMP280 예제, 기억 기반) */
    uint32_t P = bme280_comp_P(&c, 415148, tf);
    CHK(P > 25767236 - 8 && P < 25767236 + 8);          /* 약 100653.27 Pa (BMP280 예제 값은 기억 기반이라 ±0.03 Pa 허용) */
    uint32_t h1 = bme280_comp_H(&c, 30000, tf), h2 = bme280_comp_H(&c, 32000, tf);
    CHK(h2 > h1 && h2 < 102400);
    CHK(bme280_comp_H(&c, 65535, tf) == 102400);            /* 상한 클램프 = 100 %RH */
    printf("T=%d P=%u h1=%u h2=%u\n", T, P, h1, h2);

    /* 계수 파싱: H4/H5 니블 */
    uint8_t a[26] = {0}, b[7] = {0x6A, 0x01, 0x00, 0x13, 0x2E, 0x03, 0x1E}; /* 0xE4=0x13 → H4 = 0x13<<4 | 0xE; 0xE5=0x2E → 상위 2, 하위 E; 0xE6=3 */
    a[0] = 0x70; a[1] = 0x6B;                                /* T1 = 0x6B70 = 27504 */
    a[25] = 75;
    bme280_calib_t p; bme280_parse_calib(a, b, &p);
    CHK(p.T1 == 27504 && p.H1 == 75 && p.H2 == 0x016A && p.H4 == ((0x13 << 4) | 0xE) && p.H5 == ((3 << 4) | 2) && p.H6 == 30);

    uint8_t d[8] = { 0x65, 0x5A, 0xC0, 0x7E, 0xED, 0x00, 0x7F, 0x80 };
    int32_t aP, aT, aH; bme280_split(d, &aP, &aT, &aH);
    CHK(aP == 415148 && aT == 519888 && aH == 0x7F80);
    printf(fails ? "FAILED %d\n" : "ALL PASS\n", fails);
    return fails != 0;
}
