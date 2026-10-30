/* motor/tb/test_dcmotor.c — dcmotor.c 호스트 모의 시험.
 * 모의 레지스터는 dcmotor_axil_regs.v 머리말의 맵(CTRL 0x00, DUTY 0x04, PERIOD 0x08, STATUS 0x0C)을 리터럴로 쓰고,
 * DUTY 폭(32비트 / 예전 16비트)을 바꿔 가며 같은 드라이버를 시험한다. 한계: 모델은 RTL 주석을 읽고 만든 것(RTL 과 연결되지 않음).
 * Pmod HB5 는 DIR+EN 2핀이라 브레이크가 없다 — 이 시험은 드라이버 레이어만 본다(핀 차이는 하드웨어 설계 단계). */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../sw/dcmotor.h"

volatile uint32_t mock_mot_reg[4];
uint32_t mock_duty_mask = 0xFFFFFFFFu;
uint32_t mock_wlog[32]; int mock_nw;

uint32_t mock_mot_rd(uint32_t off) { return mock_mot_reg[off >> 2]; }
void mock_mot_wr(uint32_t off, uint32_t v)
{
    if (mock_nw < 32) mock_wlog[mock_nw++] = (off << 24) | (off == 0x04 ? 0 : (v & 0xFFFFFF));
    if (off == 0x04) v &= mock_duty_mask;
    if (off == 0x0C) return;                              /* STATUS 는 읽기 전용 */
    mock_mot_reg[off >> 2] = v;
}

static int fails, checks;
#define CHK(c) do { checks++; if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
static void reset_regs(uint32_t period_clk)
{
    mock_mot_reg[0] = 0; mock_mot_reg[1] = 0; mock_mot_reg[2] = period_clk; mock_mot_reg[3] = 0;
    mock_duty_mask = 0xFFFFFFFFu; mock_nw = 0;
}
#define CTRL mock_mot_reg[0]
#define DUTY mock_mot_reg[1]

int main(void)
{
    /* 기본 주기 100000 (100 MHz, 1 kHz) */
    reset_regs(100000u);
    CHK(motor_set(0, MOT_FWD, 50) == MOT_OK);
    CHK(DUTY == 50000u && (CTRL & 1u) == 1u && ((CTRL >> 1) & 3u) == MOT_FWD && CTRL == 3u);
    CHK(mock_nw == 2 && (mock_wlog[0] >> 24) == 0x04 && (mock_wlog[1] >> 24) == 0x00);   /* DUTY 가 먼저, CTRL 이 나중 */
    CHK(motor_set(0, MOT_REV, 0) == MOT_OK && DUTY == 0 && CTRL == 5u);
    CHK(motor_set(0, MOT_FWD, 100) == MOT_OK && DUTY == 100000u);
    CHK(motor_set(0, MOT_FWD, 75) == MOT_OK && DUTY == 75000u);       /* 66 % 를 넘는 값: 16비트였다면 잘렸을 값 */
    CHK(motor_set(0, MOT_FWD, 66) == MOT_OK && DUTY == 66000u);
    CHK(motor_set(0, MOT_FWD, 65) == MOT_OK && DUTY == 65000u);
    CHK(motor_set(0, MOT_FWD, -1) == MOT_OK && DUTY == 0u);
    CHK(motor_set(0, MOT_FWD, 101) == MOT_OK && DUTY == 100000u);
    CHK(motor_set(0, MOT_FWD, 1000000) == MOT_OK && DUTY == 100000u);
    CHK(motor_set(0, MOT_FWD, -1000000) == MOT_OK && DUTY == 0u);
    CHK(motor_set(0, MOT_FWD, 1) == MOT_OK && DUTY == 1000u);
    CHK(motor_set(0, MOT_FWD, 99) == MOT_OK && DUTY == 99000u);
    CHK(motor_set(0, MOT_COAST, 50) == MOT_OK && CTRL == 1u);        /* enable=1, mode=0: RTL 에서는 코스트 */
    CHK(motor_set(0, MOT_BRAKE, 50) == MOT_OK && CTRL == 7u);
    for (int m = 0; m <= 3; m++) { CHK(motor_set(0, m, 10) == MOT_OK && CTRL == (1u | ((uint32_t)m << 1))); }

    /* 모드 검증: 범위 밖이면 아무것도 쓰지 않는다 */
    reset_regs(100000u); CTRL = 0x55; DUTY = 0x66;
    CHK(motor_set(0, 4, 50) == MOT_ERR_ARGS && motor_set(0, -1, 50) == MOT_ERR_ARGS && motor_set(0, 100, 50) == MOT_ERR_ARGS);
    CHK(mock_nw == 0 && CTRL == 0x55 && DUTY == 0x66);

    /* 주기 0 → duty 0, 나누기 0 없음 */
    reset_regs(0u); CHK(motor_set(0, MOT_FWD, 75) == MOT_OK && DUTY == 0u && CTRL == 3u);

    /* 다른 주기: 25 % of 200000, 반올림 없이 내림 */
    reset_regs(200000u); CHK(motor_set(0, MOT_FWD, 25) == MOT_OK && DUTY == 50000u);
    reset_regs(1001u); CHK(motor_set(0, MOT_FWD, 33) == MOT_OK && DUTY == 330u);          /* 330.33 → 330 */
    reset_regs(7u); CHK(motor_set(0, MOT_FWD, 99) == MOT_OK && DUTY == 6u);               /* 6.93 → 6 */

    /* 큰 주기: 32비트 곱셈이면 넘치는 값 (period*percent > 2^32) */
    reset_regs(100000000u); CHK(motor_set(0, MOT_FWD, 50) == MOT_OK && DUTY == 50000000u);   /* 1 Hz @100 MHz */
    reset_regs(0xFFFFFFFFu); CHK(motor_set(0, MOT_FWD, 100) == MOT_OK && DUTY == 0xFFFFFFFFu);
    reset_regs(0xFFFFFFFFu); CHK(motor_set(0, MOT_FWD, 50) == MOT_OK && DUTY == 0x7FFFFFFFu);
    reset_regs(50000000u); CHK(motor_set(0, MOT_FWD, 99) == MOT_OK && DUTY == 49500000u);     /* 50M*99 = 4.95e9 > 2^32 */

    /* 예전 16비트 RTL: 폭 때문에 잘리면 오류를 돌려주고 CTRL=0 으로 멈춘다 */
    reset_regs(100000u); mock_duty_mask = 0xFFFFu; CTRL = 7u;
    CHK(motor_set(0, MOT_FWD, 100) == MOT_ERR_DUTY && CTRL == 0u);                      /* 100000 → 34464 */
    CHK(motor_set(0, MOT_FWD, 75) == MOT_ERR_DUTY && CTRL == 0u);
    CHK(motor_set(0, MOT_FWD, 65) == MOT_OK && DUTY == 65000u && CTRL == 3u);            /* 65535 이하는 통과 */
    reset_regs(65535u); mock_duty_mask = 0xFFFFu; CHK(motor_set(0, MOT_FWD, 100) == MOT_OK && DUTY == 65535u);
    reset_regs(65536u); mock_duty_mask = 0xFFFFu; CHK(motor_set(0, MOT_FWD, 100) == MOT_ERR_DUTY);      /* 65536 → 0 */
    CHK(motor_set(0, MOT_FWD, 99) == MOT_OK && DUTY == 64880u);     /* 65536*99/100 = 64880: 16비트에 들어가므로 통과 */

    /* motor_stop */
    reset_regs(100000u); motor_set(0, MOT_FWD, 50); mock_nw = 0; motor_stop(0);
    CHK(CTRL == 0u && DUTY == 50000u && mock_nw == 1 && (mock_wlog[0] >> 24) == 0x00);   /* CTRL 만 쓴다 */

    /* 속성: 임의 주기·퍼센트에서 DUTY = floor(period*percent/100) (64비트 독립 계산) */
    srand(3);
    for (int n = 0; n < 20000; n++) {
        uint32_t per = ((uint32_t)rand() << 16) ^ (uint32_t)rand(); if (n % 7 == 0) per >>= 12;
        int pc = rand() % 130 - 15; int m = rand() % 4;
        reset_regs(per);
        int r = motor_set(0, m, pc);
        long long cl = pc < 0 ? 0 : pc > 100 ? 100 : pc;
        unsigned long long want = per ? (unsigned long long)per * (unsigned long long)cl / 100ull : 0;
        CHK(r == MOT_OK && DUTY == (uint32_t)want && CTRL == (1u | ((uint32_t)m << 1)));
    }
    CHK(MOT_OK == 0 && MOT_ERR_ARGS == -1 && MOT_ERR_DUTY == -2);
    CHK(MOT_OFF_CTRL == 0 && MOT_OFF_DUTY == 4 && MOT_OFF_PERIOD == 8 && MOT_OFF_STATUS == 12);
    CHK(MOT_COAST == 0 && MOT_FWD == 1 && MOT_REV == 2 && MOT_BRAKE == 3);
    printf("dcmotor: %d 검사, 실패 %d\n", checks, fails);
    if (fails) return 1;
    printf("ALL PASS\n");
    return 0;
}
