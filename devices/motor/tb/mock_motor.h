#ifndef MOCK_MOTOR_H
#define MOCK_MOTOR_H
#include <stdint.h>
/* 모의 레지스터: MOT_RD/MOT_WR 를 배열 접근으로 대체한다. */
extern volatile uint32_t mock_mot_reg[4]; /* [0]=CTRL [1]=DUTY [2]=PERIOD [3]=STATUS */
#define MOT_RD(base, off)    ((void)(base), mock_mot_reg[(uint32_t)(off) >> 2])
#define MOT_WR(base, off, v) ((void)(base), (void)(mock_mot_reg[(uint32_t)(off) >> 2] = (uint32_t)(v)))
#endif
