#ifndef MOCK_SERVO_H
#define MOCK_SERVO_H
#include <stdint.h>
uint32_t mock_srv_rd(uint32_t off);
void mock_srv_wr(uint32_t off, uint32_t v);
#define SRV_RD(base, off)    ((void)(base), mock_srv_rd((uint32_t)(off)))
#define SRV_WR(base, off, v) ((void)(base), mock_srv_wr((uint32_t)(off), (uint32_t)(v)))
#endif
