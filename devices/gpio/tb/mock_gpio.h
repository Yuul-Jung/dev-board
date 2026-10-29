#ifndef MOCK_GPIO_H
#define MOCK_GPIO_H
#include <stdint.h>
uint32_t mock_g_rd(uint32_t off);
void mock_g_wr(uint32_t off, uint32_t v);
#define GPIO_RD(base, off)    ((void)(base), mock_g_rd((uint32_t)(off)))
#define GPIO_WR(base, off, v) ((void)(base), mock_g_wr((uint32_t)(off), (uint32_t)(v)))
#endif
