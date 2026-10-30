#include "dcmotor.h"

int motor_set(uintptr_t base, int mode, int percent)
{
    if (mode < MOT_COAST || mode > MOT_BRAKE) return MOT_ERR_ARGS;
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;
    uint32_t period = MOT_RD(base, MOT_OFF_PERIOD);
    uint32_t duty   = period ? (uint32_t)(((uint64_t)period * (uint64_t)percent) / 100u) : 0u;   /* 64비트: 주기가 4290만 클럭을 넘어도 넘치지 않음 */
    MOT_WR(base, MOT_OFF_DUTY, duty);
    if (MOT_RD(base, MOT_OFF_DUTY) != duty) {          /* 레지스터가 값을 잘라냈다 → 엉뚱한 속도로 돌지 않도록 멈춘다 */
        MOT_WR(base, MOT_OFF_CTRL, 0u);
        return MOT_ERR_DUTY;
    }
    MOT_WR(base, MOT_OFF_CTRL, 1u | ((uint32_t)mode << 1));
    return MOT_OK;
}

void motor_stop(uintptr_t base)
{
    MOT_WR(base, MOT_OFF_CTRL, 0u);
}
