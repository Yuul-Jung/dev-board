/* motor/tb/test_dcmotor.c — dcmotor.c 호스트 모의 시험
 *
 * 확인 항목:
 *  1. duty 계산 (50 %, 0 %, 100 %)
 *  2. duty 클램프 (음수 → 0, 101 → 100)
 *  3. mode 비트 배치 (CTRL[2:1])
 *  4. PERIOD=0 일 때 duty=0 (나누기 0 방지)
 *  5. motor_stop() → CTRL=0
 *
 * Pmod HB5 비고:
 *  HB5 는 DIR+EN 2핀이며 브레이크 핀이 없다. RTL 은 여전히 IN1/IN2 를
 *  사용하므로 MOT_BRAKE 를 보내도 RTL 측은 동작하지만 HB5 핀에는
 *  브레이크가 적용되지 않는다. 이 시험은 드라이버 레이어(소프트웨어)를
 *  검증하며, HB5 와 RTL 사이의 핀 차이는 하드웨어 설계 단계에서 해결한다.
 */
#include <stdio.h>
#include "../sw/dcmotor.h"

volatile uint32_t mock_mot_reg[4]; /* CTRL, DUTY, PERIOD, STATUS */

static int fails;
#define CHK(c) do { \
    if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } \
} while (0)

/* 편의 함수 */
static void reset_regs(uint32_t period_clk)
{
    mock_mot_reg[0] = 0;           /* CTRL */
    mock_mot_reg[1] = 0;           /* DUTY */
    mock_mot_reg[2] = period_clk;  /* PERIOD */
    mock_mot_reg[3] = 0;           /* STATUS */
}

int main(void)
{
    /* 1. 50% 정회전 — period=100000(100MHz·1kHz), duty=50000 */
    reset_regs(100000u);
    motor_set(0, MOT_FWD, 50);
    CHK(mock_mot_reg[1] == 50000u);                     /* DUTY */
    CHK((mock_mot_reg[0] & 0x1u) == 1u);               /* enable=1 */
    CHK(((mock_mot_reg[0] >> 1) & 0x3u) == MOT_FWD);  /* mode=FWD */

    /* 2. 0% 역회전 — duty=0 */
    motor_set(0, MOT_REV, 0);
    CHK(mock_mot_reg[1] == 0u);
    CHK(((mock_mot_reg[0] >> 1) & 0x3u) == MOT_REV);

    /* 3. 100% 정회전 — duty=period */
    motor_set(0, MOT_FWD, 100);
    CHK(mock_mot_reg[1] == 100000u);

    /* 4. 클램프: 음수 → 0 */
    motor_set(0, MOT_FWD, -1);
    CHK(mock_mot_reg[1] == 0u);

    /* 5. 클램프: 101 → 100 */
    motor_set(0, MOT_FWD, 101);
    CHK(mock_mot_reg[1] == 100000u);

    /* 6. COAST 모드 */
    motor_set(0, MOT_COAST, 50);
    CHK(((mock_mot_reg[0] >> 1) & 0x3u) == MOT_COAST);

    /* 7. BRAKE 모드 — enable=1, mode=3 */
    motor_set(0, MOT_BRAKE, 50);
    CHK(((mock_mot_reg[0] >> 1) & 0x3u) == MOT_BRAKE);
    CHK((mock_mot_reg[0] & 0x1u) == 1u);

    /* 8. PERIOD=0 일 때 duty=0 (나누기 0 방지) */
    reset_regs(0u);
    motor_set(0, MOT_FWD, 75);
    CHK(mock_mot_reg[1] == 0u);

    /* 9. motor_stop() → CTRL=0 */
    reset_regs(100000u);
    motor_set(0, MOT_FWD, 50);
    motor_stop(0);
    CHK(mock_mot_reg[0] == 0u);

    /* 10. 25% 정밀 계산 — period=200000, duty=50000 */
    reset_regs(200000u);
    motor_set(0, MOT_FWD, 25);
    CHK(mock_mot_reg[1] == 50000u);

    printf(fails ? "FAILED %d\n" : "ALL PASS\n", fails);
    return fails != 0;
}
