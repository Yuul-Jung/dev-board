#include "stepper.h"

/* 8단계 코일 표. 비트 0 = IN1 ... 비트 3 = IN4. A..D 는 IN1..IN4 (대응은 설계안, stepper.h 참고) */
static const uint8_t PAT[8] = {
    STEPPER_PAT_A, STEPPER_PAT_AB, STEPPER_PAT_B, STEPPER_PAT_BC,
    STEPPER_PAT_C, STEPPER_PAT_CD, STEPPER_PAT_D, STEPPER_PAT_DA
};

#define GEAR_MIN_MICRO 1000000u        /* 1:1 */
#define GEAR_MAX_MICRO 1000000000u     /* 1000:1 (계산 범위 제한) */
#define MRPM_MAX       1000000u        /* 1000 rpm (계산 범위 제한) */
#define MAX_DEG10      3600000         /* ±1000 회전 (계산 범위 제한) */

uint8_t stepper_pattern(unsigned idx) { return PAT[idx & 7u]; }

static uint32_t pin_mask(const stepper_t *s) { return 0xFu << s->shift; }

static int write_pat(stepper_t *s, uint8_t pat)
{
    return gpio_write_mask(s->g, s->ch, pin_mask(s), (uint32_t)pat << s->shift) == GPIO_OK ? STEPPER_OK : STEPPER_ERR_ARGS;
}

int stepper_init(stepper_t *s, gpio_t *g, unsigned ch, unsigned shift)
{
    if (!s || !g || ch > 1 || shift > 28) return STEPPER_ERR_ARGS;
    s->g = g; s->ch = ch; s->shift = shift;
    s->mode = STEPPER_FULL; s->idx = 1; s->energized = 0; s->release_on_done = 0;
    s->gear_micro = STEPPER_GEAR_NOMINAL_MICRO; s->max_hz = STEPPER_HZ_PULL_IN;
    s->interval_us = 0; s->next_us = 0; s->remaining = 0; s->pos = 0;
    for (unsigned i = 0; i < STEPPER_PHASES; i++)
        if (gpio_pin_mode(g, ch, shift + i, 1, 0) != GPIO_OK) return STEPPER_ERR_ARGS;
    return STEPPER_OK;
}

uint64_t stepper_steps_per_rev_micro(const stepper_t *s)
{
    uint64_t motor = s->mode == STEPPER_HALF ? STEPPER_MOTOR_STEPS_HALF : STEPPER_MOTOR_STEPS_FULL;
    return motor * s->gear_micro;
}

int stepper_set_mode(stepper_t *s, stepper_mode_t mode)
{
    if (!s || (unsigned)mode > STEPPER_HALF) return STEPPER_ERR_ARGS;
    if (s->remaining != 0) return STEPPER_ERR_BUSY;
    s->mode = mode;                              /* idx 는 로터의 실제 위치(마지막으로 낸 상태)이므로 건드리지 않는다 — 다음 스텝이 짝/홀을 맞춘다 */
    s->pos = 0;
    s->interval_us = 0;                          /* 스텝 수가 달라졌으므로 속도를 다시 정해야 한다 */
    return STEPPER_OK;
}

int stepper_set_gear_micro(stepper_t *s, uint64_t gear_micro)
{
    if (!s || gear_micro < GEAR_MIN_MICRO || gear_micro > GEAR_MAX_MICRO) return STEPPER_ERR_ARGS;
    if (s->remaining != 0) return STEPPER_ERR_BUSY;
    s->gear_micro = gear_micro;
    s->interval_us = 0;
    return STEPPER_OK;
}

static int rate_ok(uint32_t interval_us, uint32_t max_hz) { return interval_us != 0 && (uint64_t)interval_us * max_hz >= 1000000u; }

int stepper_set_max_hz(stepper_t *s, uint32_t hz)
{
    if (!s || hz < 1 || hz > STEPPER_HZ_PULL_OUT) return STEPPER_ERR_ARGS;
    if (s->interval_us != 0 && !rate_ok(s->interval_us, hz)) return STEPPER_ERR_SPEED;
    s->max_hz = hz;
    return STEPPER_OK;
}

int stepper_set_speed_mrpm(stepper_t *s, uint32_t mrpm)
{
    if (!s) return STEPPER_ERR_ARGS;
    if (mrpm == 0 || mrpm > MRPM_MAX) return STEPPER_ERR_SPEED;
    /* 간격[µs] = 60e6 / (스텝/회전 × rpm) = 6e16 / (steps_per_rev_micro × mrpm), 반올림 */
    uint64_t den = stepper_steps_per_rev_micro(s) * mrpm;
    uint64_t iv = (60000000000000000ull + den / 2) / den;
    if (iv == 0 || iv > 0xFFFFFFFFull || !rate_ok((uint32_t)iv, s->max_hz)) return STEPPER_ERR_SPEED;
    s->interval_us = (uint32_t)iv;
    return STEPPER_OK;
}

int stepper_step(stepper_t *s, int dir)
{
    if (!s || dir == 0) return STEPPER_ERR_ARGS;
    /* 풀·웨이브는 WAVE = 짝수, FULL = 홀수 상태만 쓴다. 모드를 바꾼 직후처럼 짝/홀이 안 맞으면 요청한 방향으로 반 스텝만 가서 맞춘다 */
    unsigned inc = (s->mode == STEPPER_HALF || (s->idx & 1u) != (unsigned)s->mode) ? 1u : 2u;
    uint8_t ni = (uint8_t)((s->idx + (dir > 0 ? inc : 8u - inc)) & 7u);
    int r = write_pat(s, PAT[ni]);
    if (r) return r;
    s->idx = ni; s->energized = 1;
    s->pos = (int32_t)((uint32_t)s->pos + (dir > 0 ? 1u : 0xFFFFFFFFu));
    return STEPPER_OK;
}

int stepper_move(stepper_t *s, int32_t steps, uint32_t now_us)
{
    if (!s) return STEPPER_ERR_ARGS;
    if (s->interval_us == 0) return STEPPER_ERR_SPEED;
    if (s->remaining != 0) return STEPPER_ERR_BUSY;
    s->remaining = steps;
    s->next_us = now_us;
    return STEPPER_OK;
}

int stepper_move_deg10(stepper_t *s, int32_t deg10, uint32_t now_us)
{
    if (!s || deg10 > MAX_DEG10 || deg10 < -MAX_DEG10) return STEPPER_ERR_ARGS;
    uint64_t a = deg10 < 0 ? (uint64_t)(-(int64_t)deg10) : (uint64_t)deg10;
    const uint64_t den = 3600ull * 1000000ull;                  /* 0.1° 단위 3600 = 1회전, 비율은 1e6 단위 */
    uint64_t n = (a * stepper_steps_per_rev_micro(s) + den / 2) / den;
    if (n > 0x7FFFFFFFull) return STEPPER_ERR_ARGS;
    return stepper_move(s, deg10 < 0 ? -(int32_t)n : (int32_t)n, now_us);
}

int stepper_poll(stepper_t *s, uint32_t now_us)
{
    if (!s || s->remaining == 0) return 0;
    if ((int32_t)(now_us - s->next_us) < 0) return 0;
    int dir = s->remaining > 0 ? 1 : -1;
    if (stepper_step(s, dir) != STEPPER_OK) return 0;
    s->remaining -= dir;
    uint32_t nominal = s->next_us + s->interval_us;
    s->next_us = ((int32_t)(now_us - nominal) >= 0) ? now_us + s->interval_us : nominal;
    if (s->remaining == 0 && s->release_on_done) (void)stepper_release(s);
    return 1;
}

int32_t stepper_remaining(const stepper_t *s) { return s ? s->remaining : 0; }

void stepper_stop(stepper_t *s) { if (s) s->remaining = 0; }

int stepper_release(stepper_t *s)
{
    if (!s) return STEPPER_ERR_ARGS;
    s->remaining = 0;
    int r = write_pat(s, 0);
    if (r == STEPPER_OK) s->energized = 0;
    return r;
}

int stepper_hold(stepper_t *s)
{
    if (!s) return STEPPER_ERR_ARGS;
    int r = write_pat(s, PAT[s->idx]);
    if (r == STEPPER_OK) s->energized = 1;
    return r;
}

void stepper_set_release_on_done(stepper_t *s, int on) { if (s) s->release_on_done = on ? 1 : 0; }
