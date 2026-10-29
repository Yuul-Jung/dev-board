#ifndef MOCK_QSPI_H
#define MOCK_QSPI_H
#include <stdint.h>
uint32_t mock_q_rd(uint32_t off);
void mock_q_wr(uint32_t off, uint32_t v);
#define QSPI_RD(base, off)    ((void)(base), mock_q_rd((uint32_t)(off)))
#define QSPI_WR(base, off, v) ((void)(base), mock_q_wr((uint32_t)(off), (uint32_t)(v)))
#endif
