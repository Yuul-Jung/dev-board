/* DC 모터 드라이버 예시. 컴파일과 보드 시험은 아직 하지 않았다. */
#include "dcmotor.h"

/* percent: 0..100. PERIOD 레지스터에서 주기를 읽어 DUTY 를 계산한다. */
void motor_set(uintptr_t base, int mode, int percent)
{
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;
    uint32_t period = MOT_RD(base, MOT_OFF_PERIOD);
    uint32_t duty   = period ? (period * (uint32_t)percent) / 100u : 0u;
    MOT_WR(base, MOT_OFF_DUTY, duty);
    MOT_WR(base, MOT_OFF_CTRL, 1u | ((uint32_t)mode << 1));
}

void motor_stop(uintptr_t base)
{
    MOT_WR(base, MOT_OFF_CTRL, 0u);
}
