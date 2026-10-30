#ifndef MOCK_OW_H
#define MOCK_OW_H
#include <stdint.h>
uint32_t mock_ow_rd(uint32_t off);
void mock_ow_wr(uint32_t off, uint32_t v);
#define OW_RD(base, off)    ((void)(base), mock_ow_rd((uint32_t)(off)))
#define OW_WR(base, off, v) ((void)(base), mock_ow_wr((uint32_t)(off), (uint32_t)(v)))
#endif
