#ifndef MOCK_MOTOR_H
#define MOCK_MOTOR_H
#include <stdint.h>
/* 모의 레지스터. 읽기/쓰기를 함수로 받아 DUTY 폭(mock_duty_mask)을 흉내 내고 쓰기 순서를 기록한다.
 * [0]=CTRL [1]=DUTY [2]=PERIOD [3]=STATUS (오프셋은 rtl/dcmotor_axil_regs.v 머리말의 리터럴 0x00/0x04/0x08/0x0C) */
extern volatile uint32_t mock_mot_reg[4];
extern uint32_t mock_duty_mask;          /* 0xFFFFFFFF = 현재 RTL, 0xFFFF = 예전 16비트 RTL */
extern uint32_t mock_wlog[32]; extern int mock_nw;
uint32_t mock_mot_rd(uint32_t off);
void mock_mot_wr(uint32_t off, uint32_t v);
#define MOT_RD(base, off)    ((void)(base), mock_mot_rd((uint32_t)(off)))
#define MOT_WR(base, off, v) ((void)(base), mock_mot_wr((uint32_t)(off), (uint32_t)(v)))
#endif
