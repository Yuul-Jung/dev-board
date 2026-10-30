#ifndef MOCK_US_H
#define MOCK_US_H
#include <stdint.h>
uint32_t mock_us_rd(uint32_t off);
void mock_us_wr(uint32_t off, uint32_t v);
#define US_RD(base, off)    ((void)(base), mock_us_rd((uint32_t)(off)))
#define US_WR(base, off, v) ((void)(base), mock_us_wr((uint32_t)(off), (uint32_t)(v)))
#endif
