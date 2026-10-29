#ifndef MOCK_UART_H
#define MOCK_UART_H
#include <stdint.h>
uint32_t mock_ul_rd(uint32_t off);
void mock_ul_wr(uint32_t off, uint32_t v);
#define UL_RD(base, off)    ((void)(base), mock_ul_rd((uint32_t)(off)))
#define UL_WR(base, off, v) ((void)(base), mock_ul_wr((uint32_t)(off), (uint32_t)(v)))
#endif
