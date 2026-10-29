#ifndef MOCK_TIMER_B_H
#define MOCK_TIMER_B_H
#include <stdint.h>
/* 기준 주소를 구분하는 AXI Timer 모의 (RGB 처럼 인스턴스가 여러 개인 시험용) */
uint32_t mock_tb_rd(uintptr_t base, uint32_t off);
void mock_tb_wr(uintptr_t base, uint32_t off, uint32_t v);
#define TMR_RD(base, off)    mock_tb_rd((uintptr_t)(base), (uint32_t)(off))
#define TMR_WR(base, off, v) mock_tb_wr((uintptr_t)(base), (uint32_t)(off), (uint32_t)(v))
#endif
