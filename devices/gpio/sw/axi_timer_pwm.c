#include "axi_timer_pwm.h"

int tpwm_set_cycles(uintptr_t b, uint32_t period, uint32_t high)
{
    if (high > period) return TPWM_ERR_ARGS;
    if (high == 0) {                 /* 상수 비활성 — 정확한 레지스터 값은 Zephyr 요약만 있어 미확인 */
        TMR_WR(b, TMR_TCSR0, TCSR_PWM);
        TMR_WR(b, TMR_TCSR1, TCSR_PWM | TCSR_ENT);
        return TPWM_OK;
    }
    if (high == period) {            /* 상수 활성 — 위와 같이 미확인 */
        TMR_WR(b, TMR_TCSR0, TCSR_PWM | TCSR_ENT);
        TMR_WR(b, TMR_TCSR1, TCSR_PWM);
        return TPWM_OK;
    }
    if (period < 2 || high < 2) return TPWM_ERR_ARGS;
    TMR_WR(b, TMR_TCSR0, TCSR_PWM);                  /* 정지 */
    TMR_WR(b, TMR_TCSR1, TCSR_PWM);
    TMR_WR(b, TMR_TLR0, period - 2);                 /* TLR0 = period - 2 */
    TMR_WR(b, TMR_TCSR0, TCSR_PWM | TCSR_LOAD);
    TMR_WR(b, TMR_TLR1, high - 2);                   /* TLR1 = high - 2 */
    TMR_WR(b, TMR_TCSR1, TCSR_PWM | TCSR_LOAD);
    TMR_WR(b, TMR_TCSR0, TCSR_PWM);                  /* 시작: 타이머1 에 ENALL */
    TMR_WR(b, TMR_TCSR1, TCSR_PWM | TCSR_ENALL);
    return TPWM_OK;
}

int fan_set_percent(uintptr_t b, uint32_t clk_hz, unsigned pct)
{
    if (pct > 100 || clk_hz < 50000) return TPWM_ERR_ARGS;
    uint32_t period = clk_hz / 25000u;
    uint32_t high = (uint32_t)(((uint64_t)period * pct + 50) / 100);
    return tpwm_set_cycles(b, period, high);
}

int buzzer_tone(uintptr_t b, uint32_t clk_hz, uint32_t f)
{
    if (f == 0) return tpwm_set_cycles(b, 2, 0);
    uint32_t period = clk_hz / f;
    if (period < 4) return TPWM_ERR_ARGS;
    return tpwm_set_cycles(b, period, period / 2);
}
