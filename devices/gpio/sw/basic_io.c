#include "basic_io.h"

int mclock_start(mclock_t *m, uintptr_t base, uint32_t clk_hz)
{
    if (!m || clk_hz < 1000) return BIO_ERR_ARGS;
    m->base = base; m->clk_hz = clk_hz; m->cycles = 0;
    TMR_WR(base, TMR_TLR0, 0);
    TMR_WR(base, TMR_TCSR0, TCSR_ARHT | TCSR_LOAD);          /* TCR0 <- TLR0 */
    TMR_WR(base, TMR_TCSR0, TCSR_ARHT | TCSR_ENT);           /* 올림 카운트, 생성 모드, 자동 재적재, LOAD 해제 */
    m->last = TMR_RD(base, TMR_TCR0);
    return BIO_OK;
}
uint64_t mclock_cycles(mclock_t *m)
{
    uint32_t now = TMR_RD(m->base, TMR_TCR0);
    m->cycles += (uint32_t)(now - m->last);                  /* 부호 없는 뺄셈이 넘침을 처리 */
    m->last = now;
    return m->cycles;
}
uint32_t mclock_millis(mclock_t *m)
{
    uint64_t c = mclock_cycles(m);
    uint64_t sec = c / m->clk_hz, rem = c % m->clk_hz;
    return (uint32_t)(sec * 1000u + rem * 1000u / m->clk_hz);
}
uint32_t mclock_micros(mclock_t *m)
{
    uint64_t c = mclock_cycles(m);
    uint64_t sec = c / m->clk_hz, rem = c % m->clk_hz;
    return (uint32_t)(sec * 1000000u + rem * 1000000u / m->clk_hz);
}

int ticker_init(ticker_t *t, uint32_t period, uint32_t now)
{
    if (!t || period == 0) return BIO_ERR_ARGS;
    t->period = period; t->last = now;
    return BIO_OK;
}
int ticker_due(ticker_t *t, uint32_t now)
{
    if ((uint32_t)(now - t->last) < t->period) return 0;
    t->last += t->period;
    if ((uint32_t)(now - t->last) >= t->period) t->last = now;
    return 1;
}

int button_init(button_t *b, uint32_t debounce_ms, int active_low)
{
    if (!b) return BIO_ERR_ARGS;
    b->debounce_ms = debounce_ms; b->active_low = active_low ? 1 : 0;
    b->primed = 0; b->cand = b->stable = 0; b->t_cand = b->t_stable = 0; b->presses = b->releases = 0;
    return BIO_OK;
}
int button_update(button_t *b, int raw, uint32_t now)
{
    uint8_t v = (uint8_t)(b->active_low ? !raw : !!raw);
    if (!b->primed) { b->primed = 1; b->cand = b->stable = v; b->t_cand = b->t_stable = now; return BTN_EV_NONE; }
    if (v != b->cand) { b->cand = v; b->t_cand = now; }
    if (b->cand != b->stable && (uint32_t)(now - b->t_cand) >= b->debounce_ms) {
        b->stable = b->cand; b->t_stable = now;
        if (b->stable) { b->presses++; return BTN_EV_PRESS; }
        b->releases++; return BTN_EV_RELEASE;
    }
    return BTN_EV_NONE;
}
int button_down(const button_t *b) { return b->stable; }
uint32_t button_held_ms(const button_t *b, uint32_t now) { return b->stable ? (uint32_t)(now - b->t_stable) : 0; }

int rgb_init(rgb_t *r, uintptr_t br, uintptr_t bg, uintptr_t bb, uint32_t clk_hz, uint32_t freq_hz, int ca)
{
    if (!r || freq_hz == 0 || clk_hz / freq_hz < 256) return BIO_ERR_ARGS;     /* 8비트 해상도를 위해 주기 256 클럭 이상 */
    r->base[0] = br; r->base[1] = bg; r->base[2] = bb; r->clk_hz = clk_hz; r->freq_hz = freq_hz; r->common_anode = ca ? 1 : 0;
    return BIO_OK;
}
static int one(const rgb_t *r, unsigned i, uint8_t v)
{
    uint32_t period = r->clk_hz / r->freq_hz;
    uint32_t high = (uint32_t)(((uint64_t)period * v + 127) / 255);
    if (r->common_anode) high = period - high;                /* 핀이 낮을 때 켜짐 */
    if (high > 0 && high < 2) high = 2;                       /* tpwm_set_cycles 는 일반 PWM 에 하이 시간 2 이상 필요 */
    if (high < period && period - high < 2) high = period - 2;   /* 로우 시간 쪽도 같은 한계로 보수적으로 (문서에 없는 가정) */
    return tpwm_set_cycles(r->base[i], period, high);
}
int rgb_set(rgb_t *r, uint8_t red, uint8_t green, uint8_t blue)
{
    if (!r) return BIO_ERR_ARGS;
    int e;
    if ((e = one(r, 0, red))) return e;
    if ((e = one(r, 1, green))) return e;
    return one(r, 2, blue);
}

void rgb_from_hsv(unsigned h, uint8_t s, uint8_t v, uint8_t out[3])
{
    h %= 360;
    unsigned region = h / 60, rem = (h % 60) * 255 / 60;
    unsigned p = (unsigned)v * (255 - s) / 255;
    unsigned q = (unsigned)v * (255 - (unsigned)s * rem / 255) / 255;
    unsigned t = (unsigned)v * (255 - (unsigned)s * (255 - rem) / 255) / 255;
    unsigned r, g, b;
    switch (region) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
    }
    out[0] = (uint8_t)r; out[1] = (uint8_t)g; out[2] = (uint8_t)b;
}
