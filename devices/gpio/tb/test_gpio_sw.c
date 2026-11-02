#include <stdio.h>
#include "../sw/axi_timer_pwm.h"
// (추가) 독립 모의 CTRL 레지스터로 INVERT 수정 시 IRQ_EN 보존을 검증한다.
static uint32_t quad_regs[3];
#define QD_RD(base, off)    ((void)(base), quad_regs[(off) / 4])
#define QD_WR(base, off, v) ((void)(base), quad_regs[(off) / 4] = (v))
#include "../sw/quad.h"

static uint32_t tlr0, tlr1, tcsr0, tcsr1; static int nw;
uint32_t mock_t_rd(uint32_t off) { (void)off; return 0; }
void mock_t_wr(uint32_t off, uint32_t v)
{
    nw++;
    if (off == TMR_TLR0) tlr0 = v; else if (off == TMR_TLR1) tlr1 = v;
    else if (off == TMR_TCSR0) tcsr0 = v; else if (off == TMR_TCSR1) tcsr1 = v;
}
static int fails;
#define CHK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
int main(void)
{
    /* 100 MHz, 팬 25 kHz → 주기 4000 사이클, 50 % → 하이 2000 → TLR0=3998, TLR1=1998 */
    CHK(fan_set_percent(0, 100000000, 50) == TPWM_OK);
    CHK(tlr0 == 3998 && tlr1 == 1998 && (tcsr1 & TCSR_ENALL) && (tcsr0 & TCSR_PWMA) && TCSR_PWM == 0x216);
    CHK(fan_set_percent(0, 100000000, 101) == TPWM_ERR_ARGS);
    CHK(fan_set_percent(0, 100000000, 0) == TPWM_OK && !(tcsr0 & TCSR_ENALL) && (tcsr1 & TCSR_ENT));
    CHK(fan_set_percent(0, 100000000, 100) == TPWM_OK && (tcsr0 & TCSR_ENT));
    /* 부저 440 Hz: 주기 227272, 하이 113636 */
    CHK(buzzer_tone(0, 100000000, 440) == TPWM_OK && tlr0 == 227270 && tlr1 == 113634);
    CHK(buzzer_tone(0, 100000000, 0) == TPWM_OK && (tcsr1 & TCSR_ENT));
    CHK(buzzer_tone(0, 100, 50) == TPWM_ERR_ARGS);
    CHK(tpwm_set_cycles(0, 10, 1) == TPWM_ERR_ARGS);
    // (추가) 기존 IRQ_EN 비트를 유지한 채 invert만 변경.
    quad_regs[2] = 2u;
    quad_set_invert(0, 1);
    CHK(quad_regs[2] == 3u);
    quad_set_invert(0, 0);
    CHK(quad_regs[2] == 2u);
    printf(fails ? "FAILED %d\n" : "ALL PASS\n", fails);
    return fails != 0;
}
