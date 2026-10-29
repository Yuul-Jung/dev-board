#ifndef MOCK_CAN_H
#define MOCK_CAN_H
#include <stdint.h>
uint32_t mock_can_rd(uint32_t off);
void mock_can_wr(uint32_t off, uint32_t v);
#define CAN_RD(base, off)    ((void)(base), mock_can_rd((uint32_t)(off)))
#define CAN_WR(base, off, v) ((void)(base), mock_can_wr((uint32_t)(off), (uint32_t)(v)))
#endif
