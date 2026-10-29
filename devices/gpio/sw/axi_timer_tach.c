#include "axi_timer_tach.h"

void tach_init(tach_t *t, uintptr_t b)
{
    t->base = b; t->prev = 0; t->period = 0; t->have_prev = 0;
    uint32_t cfg = TCSR_MDT | TCSR_CAPT | TCSR_ARHT;             /* 캡처, 외부 트리거 사용, 덮어쓰기 */
    TMR_WR(b, TMR_TCSR0, 0);                                      /* 정지 */
    TMR_WR(b, TMR_TLR0, 0);
    TMR_WR(b, TMR_TCSR0, cfg | TCSR_LOAD);                        /* 카운터를 TLR0(0)로 */
    TMR_WR(b, TMR_TCSR0, cfg | TCSR_ENT);                         /* LOAD 를 지우면서 ENT 설정 */
}

int tach_poll(tach_t *t)
{
    uint32_t s = TMR_RD(t->base, TMR_TCSR0);
    if (!(s & TCSR_TINT)) return 0;
    uint32_t cap = TMR_RD(t->base, TMR_TLR0);
    TMR_WR(t->base, TMR_TCSR0, s);                                /* TINT 에 1 을 써서 지움 (나머지 비트는 그대로) */
    if (!t->have_prev) { t->prev = cap; t->have_prev = 1; return 0; }
    t->period = cap - t->prev; t->prev = cap;
    return 1;
}

uint32_t tach_rpm(const tach_t *t, uint32_t clk_hz, unsigned ppr)
{
    if (t->period == 0 || ppr == 0) return 0;
    return (uint32_t)((60ull * clk_hz) / ((uint64_t)t->period * ppr));
}
