#ifndef MOCK_LCD_H
#define MOCK_LCD_H
#include <stdint.h>
uint32_t mock_l_rd(uint32_t off);
void mock_l_wr(uint32_t off, uint32_t v);
#define LCD_RD(base, off)    ((void)(base), mock_l_rd((uint32_t)(off)))
#define LCD_WR(base, off, v) ((void)(base), mock_l_wr((uint32_t)(off), (uint32_t)(v)))
#endif
