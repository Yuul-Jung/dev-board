/* PS/2 수신 레지스터 접근과 키 이벤트 분리. 근거: Digilent Pmod PS2 문서
 * (F0 = 키 뗌, E0 = 확장 키 접두, 확장 키 뗌 = E0 F0). 스캔 코드 -> 문자 표는 문서가 도해만 가리켜 만들지 않았다. */
#ifndef PS2_H
#define PS2_H
#include <stdint.h>

#ifndef PS2_RD
#define PS2_RD(base, off)    (*(volatile uint32_t *)((uintptr_t)(base) + (off)))
#define PS2_WR(base, off, v) (*(volatile uint32_t *)((uintptr_t)(base) + (off)) = (v))
#endif
#define PS2_DATA 0x00
#define PS2_STAT 0x04
#define PS2_CTRL 0x08
#define PS2_CNT  0x0C
#define PS2_ST_EMPTY (1u << 0)
#define PS2_ST_FULL  (1u << 1)
#define PS2_ST_ERRMASK 0x3Cu   /* overflow, parity, frame, timeout (1을 쓰면 지움) */

typedef struct { uint8_t ext_pending, brk_pending; } ps2_dec_t;
typedef struct { uint8_t code; uint8_t ext; uint8_t release; } ps2_key_t;

int  ps2_read_byte(uintptr_t base);                    /* 없으면 -1 */
void ps2_flush(uintptr_t base);
uint32_t ps2_take_errors(uintptr_t base);              /* STATUS 오류 비트를 읽고 지움 */
/* 바이트 하나 투입. 키 이벤트가 완성되면 1, 아직이면 0 */
int  ps2_decode(ps2_dec_t *s, uint8_t b, ps2_key_t *k);
#endif
