#include <stdio.h>
#include "../sw/axi_timer_tach.h"

static uint32_t tcsr0, tlr0; static int cap_pending;
uint32_t mock_t_rd(uint32_t off) { return off == TMR_TCSR0 ? (tcsr0 | (cap_pending ? TCSR_TINT : 0)) : off == TMR_TLR0 ? tlr0 : 0; }
void mock_t_wr(uint32_t off, uint32_t v)
{
    if (off == TMR_TCSR0) { if (v & TCSR_TINT) cap_pending = 0; tcsr0 = v & ~TCSR_TINT; }
    else if (off == TMR_TLR0) tlr0 = v;
}
static void capture(uint32_t v) { tlr0 = v; cap_pending = 1; }
static int fails;
#define CHK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
int main(void)
{
    tach_t t; tach_init(&t, 0);
    CHK((tcsr0 & (TCSR_MDT | TCSR_CAPT | TCSR_ARHT | TCSR_ENT)) == (TCSR_MDT | TCSR_CAPT | TCSR_ARHT | TCSR_ENT) && !(tcsr0 & TCSR_LOAD));
    CHK(tach_poll(&t) == 0);
    capture(1000); CHK(tach_poll(&t) == 0 && !cap_pending);       /* 기준점 */
    /* 2000 RPM, 회전당 2펄스 → 펄스 주파수 66.67 Hz → 100 MHz 에서 주기 1,500,000 클럭 */
    capture(1000 + 1500000); CHK(tach_poll(&t) == 1 && t.period == 1500000);
    CHK(tach_rpm(&t, 100000000, 2) == 2000);
    capture(100); CHK(tach_poll(&t) == 1);                         /* 카운터 래핑: 100 - (2^32 - ...) mod 2^32 */
    CHK(t.period == (uint32_t)(100u - 1501000u));
    CHK(tach_rpm(&t, 100000000, 0) == 0);
    printf(fails ? "FAILED %d\n" : "ALL PASS\n", fails);
    return fails != 0;
}
