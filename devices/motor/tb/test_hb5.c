/* motor/tb/test_hb5.c — Pmod HB5 드라이버 호스트 모의 시험.
 *
 * 빌드:
 *   cc -include gpio/tb/mock_gpio.h -include gpio/tb/mock_timer.h \
 *      -I gpio/sw -I motor/sw \
 *      motor/tb/test_hb5.c gpio/sw/axi_gpio.c gpio/sw/axi_timer_pwm.c \
 *      motor/sw/hb5.c -o /tmp/hb5
 * SAN:
 *   cc -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all \
 *      -include gpio/tb/mock_gpio.h -include gpio/tb/mock_timer.h \
 *      -I gpio/sw -I motor/sw \
 *      motor/tb/test_hb5.c gpio/sw/axi_gpio.c gpio/sw/axi_timer_pwm.c \
 *      motor/sw/hb5.c -o /tmp/hb5
 *
 * 확인 항목:
 *   1. hb5_init: DIR 핀 출력 설정, EN = 0
 *   2. hb5_run(FWD, 50): EN 을 먼저 0 으로 내린 뒤 DIR=0, EN duty = 50%
 *   3. hb5_run(REV, 75): EN 먼저 0, DIR=1, EN duty = 75%
 *   4. hb5_stop: EN = 0 (DIR 유지)
 *   5. hb5_set_dir: EN 먼저 0 → DIR 변경
 *   6. hb5_set_speed(0/100 %) 경계
 *   7. percent > 100 → HB5_ERR_SPEED
 *   8. NULL 인수 → HB5_ERR_ARGS
 */
#include <stdio.h>
#include <stdint.h>
#include "../sw/hb5.h"

/* ── GPIO mock 구현 ───────────────────────────────────────────────────── */
/* DATA/TRI 레지스터만 필요 (채널 1만, 인터럽트 없음) */
static uint32_t g_reg[4]; /* [0]=DATA [1]=TRI [2]=DATA2 [3]=TRI2 */
uint32_t mock_g_rd(uint32_t off) { return g_reg[off >> 2]; }
void     mock_g_wr(uint32_t off, uint32_t v) { g_reg[off >> 2] = v; }

/* ── Timer mock 구현 ───────────────────────────────────────────────────── */
static uint32_t tlr0, tlr1, tcsr0, tcsr1;
uint32_t mock_t_rd(uint32_t off) {
    if (off == TMR_TCSR0) return tcsr0;
    if (off == TMR_TCSR1) return tcsr1;
    return 0;
}
void mock_t_wr(uint32_t off, uint32_t v) {
    if (off == TMR_TLR0)  tlr0  = v;
    else if (off == TMR_TLR1)  tlr1  = v;
    else if (off == TMR_TCSR0) tcsr0 = v;
    else if (off == TMR_TCSR1) tcsr1 = v;
}

static void reset_hw(void) {
    for (int i = 0; i < 4; i++) g_reg[i] = 0;
    tlr0 = tlr1 = tcsr0 = tcsr1 = 0;
}

static int fails;
#define CHK(c, msg) do { if (!(c)) { fails++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); } } while(0)

/* DIR 핀(채널0, 비트0)의 현재 출력 값 */
static int dir_pin(void) { return (int)(g_reg[0] & 1u); }

/* EN 이 꺼져 있는지: tpwm_set_cycles(p, 0) → TCSR0 에 ENALL 없고 TCSR1 에 ENT */
static int en_off(void) {
    /* tpwm_set_cycles(..., 0) 는 상수 Low: TCSR0 에 ENALL 없어야 */
    return !(tcsr0 & TCSR_ENALL);
}
/* EN duty 가 period 의 percent% 인지 확인 (TLR1 기반) */
static int en_duty_approx(uint32_t period, unsigned pct) {
    /* tpwm_set_cycles 에서 TLR1 = high - 2 = (period*pct/100) - 2 */
    if (pct == 0)   return en_off();
    if (pct == 100) {
        /* 상수 High: tcsr0 & ENT */
        return (int)(tcsr0 & TCSR_ENT);
    }
    uint32_t high = (uint32_t)((uint64_t)period * pct / 100u);
    return (int)(tlr1 == high - 2u);
}

int main(void)
{
    gpio_t g;
    hb5_t  h;
    const uint32_t PERIOD = 4000u; /* 100 MHz / 25 kHz */

    printf("[hb5_init]\n");
    reset_hw();
    CHK(hb5_init(&h, &g, 0, 0, 0, PERIOD) == HB5_OK, "init OK");
    CHK((g_reg[1] & 1u) == 0u, "DIR 핀 TRI=0 (출력)");
    CHK(dir_pin() == 0, "DIR 초기값 Low (FWD)");
    CHK(en_off(), "EN 초기값 0");

    printf("[hb5_run FWD 50%%]\n");
    reset_hw(); gpio_init(&g, 0, 1); gpio_pin_mode(&g, 0, 0, 1, 0); /* 재초기화 */
    CHK(hb5_run(&h, HB5_FWD, 50) == HB5_OK, "run OK");
    CHK(dir_pin() == 0, "DIR=Low(FWD)");
    CHK(en_duty_approx(PERIOD, 50), "EN 50%");

    printf("[hb5_run REV 75%%]\n");
    CHK(hb5_run(&h, HB5_REV, 75) == HB5_OK, "run OK");
    CHK(dir_pin() == 1, "DIR=High(REV)");
    CHK(en_duty_approx(PERIOD, 75), "EN 75%");

    printf("[hb5_stop]\n");
    CHK(hb5_stop(&h) == HB5_OK, "stop OK");
    CHK(en_off(), "EN=0 후 정지");
    CHK(dir_pin() == 1, "DIR 유지(정지 시 변경 없음)");

    printf("[hb5_set_dir]\n");
    /* REV → FWD: EN 을 먼저 0 으로 내린 뒤 DIR 변경하는지 확인 */
    hb5_run(&h, HB5_REV, 80);         /* 먼저 REV 80% 으로 */
    CHK(hb5_set_dir(&h, HB5_FWD) == HB5_OK, "set_dir OK");
    CHK(dir_pin() == 0, "DIR=FWD 후");
    CHK(en_off(), "set_dir 후 EN=0");

    printf("[hb5_set_speed 경계]\n");
    CHK(hb5_set_speed(&h, 0)   == HB5_OK,      "speed 0%");
    CHK(en_off(),                               "speed 0% → EN off");
    CHK(hb5_set_speed(&h, 100) == HB5_OK,      "speed 100%");
    CHK((tcsr0 & TCSR_ENT),                    "speed 100% → 상수 High");
    CHK(hb5_set_speed(&h, 50)  == HB5_OK,      "speed 50%");
    CHK(en_duty_approx(PERIOD, 50),            "speed 50% duty");

    printf("[오류 처리]\n");
    CHK(hb5_run(&h, HB5_FWD, 101) == HB5_ERR_SPEED, "percent>100 오류");
    CHK(hb5_set_speed(&h, 200)     == HB5_ERR_SPEED, "set_speed>100 오류");
    CHK(hb5_init(NULL, &g, 0, 0, 0, PERIOD) == HB5_ERR_ARGS, "NULL hb5");
    CHK(hb5_init(&h, NULL, 0, 0, 0, PERIOD) == HB5_ERR_ARGS, "NULL gpio");
    CHK(hb5_init(&h, &g, 0, 0, 0, 1)        == HB5_ERR_ARGS, "period<2");
    CHK(hb5_stop(NULL)                        == HB5_ERR_ARGS, "stop NULL");
    CHK(hb5_set_dir(NULL, HB5_FWD)           == HB5_ERR_ARGS, "set_dir NULL");
    CHK(hb5_set_speed(NULL, 50)               == HB5_ERR_ARGS, "set_speed NULL");
    CHK(hb5_run(NULL, HB5_FWD, 50)           == HB5_ERR_ARGS, "run NULL");

    printf(fails ? "FAILED %d\n" : "ALL PASS\n", fails);
    return fails != 0;
}
