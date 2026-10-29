#ifndef MOCK_TIMER_H
#define MOCK_TIMER_H
#include <stdint.h>
uint32_t mock_t_rd(uint32_t off);
void mock_t_wr(uint32_t off, uint32_t v);
#define TMR_RD(base, off)    ((void)(base), mock_t_rd((uint32_t)(off)))
#define TMR_WR(base, off, v) ((void)(base), mock_t_wr((uint32_t)(off), (uint32_t)(v)))
#endif
