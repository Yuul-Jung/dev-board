#include <stdio.h>
#include "../sw/axi_timer_pwm.h"
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
    printf(fails ? "FAILED %d\n" : "ALL PASS\n", fails);
    return fails != 0;
}
