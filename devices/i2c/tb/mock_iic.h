/* 호스트 모의 테스트용: 레지스터 접근을 모의 함수로 바꾼다. gcc -include tb/mock_iic.h 로 사용 */
#ifndef MOCK_IIC_H
#define MOCK_IIC_H
#include <stdint.h>
uint32_t mock_rd(uint32_t off);
void mock_wr(uint32_t off, uint32_t v);
#define AXI_IIC_RD(base, off)    ((void)(base), mock_rd((uint32_t)(off)))
#define AXI_IIC_WR(base, off, v) ((void)(base), mock_wr((uint32_t)(off), (uint32_t)(v)))
#endif
