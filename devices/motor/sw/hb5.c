/* Pmod HB5 드라이버 구현. */
#include "hb5.h"

int hb5_init(hb5_t *h, gpio_t *g, unsigned gpio_ch, unsigned gpio_bit,
             uintptr_t pwm_base, uint32_t period_cycles)
{
    if (!h || !g || period_cycles < 2) return HB5_ERR_ARGS;
    h->gpio      = g;
    h->gpio_ch   = gpio_ch;
    h->gpio_bit  = gpio_bit;
    h->pwm_base  = pwm_base;
    h->period    = period_cycles;

    /* DIR 핀 출력으로, 초기값 Low (FWD) */
    gpio_pin_mode(g, gpio_ch, gpio_bit, 1, 0);
    /* EN = 0 (high==0 → 상수 Low) */
    tpwm_set_cycles(pwm_base, period_cycles, 0);
    return HB5_OK;
}

int hb5_stop(hb5_t *h)
{
    if (!h || !h->gpio) return HB5_ERR_ARGS;
    return tpwm_set_cycles(h->pwm_base, h->period, 0) == TPWM_OK ? HB5_OK : HB5_ERR_ARGS;
}

int hb5_set_dir(hb5_t *h, hb5_dir_t dir)
{
    if (!h || !h->gpio) return HB5_ERR_ARGS;
    /* 안전: EN 먼저 0 */
    tpwm_set_cycles(h->pwm_base, h->period, 0);
    gpio_write(h->gpio, h->gpio_ch, h->gpio_bit, (dir == HB5_REV) ? 1 : 0);
    return HB5_OK;
}

int hb5_set_speed(hb5_t *h, unsigned percent)
{
    if (!h || !h->gpio) return HB5_ERR_ARGS;
    if (percent > 100) return HB5_ERR_SPEED;
    uint32_t high;
    if (percent == 0)        high = 0;
    else if (percent == 100) high = h->period;
    else                     high = (uint32_t)((uint64_t)h->period * percent / 100u);
    return tpwm_set_cycles(h->pwm_base, h->period, high) == TPWM_OK ? HB5_OK : HB5_ERR_ARGS;
}

int hb5_run(hb5_t *h, hb5_dir_t dir, unsigned percent)
{
    if (!h || !h->gpio) return HB5_ERR_ARGS;
    if (percent > 100) return HB5_ERR_SPEED;
    /* 안전: EN 내린 뒤 DIR 변경 */
    tpwm_set_cycles(h->pwm_base, h->period, 0);
    gpio_write(h->gpio, h->gpio_ch, h->gpio_bit, (dir == HB5_REV) ? 1 : 0);
    return hb5_set_speed(h, percent);
}
