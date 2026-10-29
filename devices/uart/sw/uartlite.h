/* AXI UART Lite (PG142 v2.0) 드라이버. 레지스터·비트는 PDF 에서 확인한 값.
 * 보레이트는 Vivado 설계 시 고정(런타임 변경 불가). 호스트 모의 테스트만 수행. */
#ifndef UARTLITE_H
#define UARTLITE_H
#include <stdint.h>
#include <stddef.h>

#ifndef UL_RD
#define UL_RD(base, off)     (*(volatile uint32_t *)((uintptr_t)(base) + (off)))
#define UL_WR(base, off, v)  (*(volatile uint32_t *)((uintptr_t)(base) + (off)) = (v))
#endif

#define UL_RX    0x00
#define UL_TX    0x04
#define UL_STAT  0x08
#define UL_CTRL  0x0C

#define UL_ST_RXVALID (1u << 0)
#define UL_ST_RXFULL  (1u << 1)
#define UL_ST_TXEMPTY (1u << 2)
#define UL_ST_TXFULL  (1u << 3)
#define UL_ST_INTEN   (1u << 4)
#define UL_ST_OVERRUN (1u << 5)  /* 읽으면 지워짐 */
#define UL_ST_FRAME   (1u << 6)  /* 읽으면 지워짐 */
#define UL_ST_PARITY  (1u << 7)  /* 읽으면 지워짐 */
#define UL_ST_ERRMASK (UL_ST_OVERRUN | UL_ST_FRAME | UL_ST_PARITY)

#define UL_CT_RST_TX  (1u << 0)
#define UL_CT_RST_RX  (1u << 1)
#define UL_CT_INTEN   (1u << 4)

typedef struct { uintptr_t base; uint32_t err; } uartlite_t; /* err: 누적 오류 비트(5~7) */

void uartlite_init(uartlite_t *u, uintptr_t base);          /* FIFO 리셋, 인터럽트 끔 */
int  uartlite_getc(uartlite_t *u);                          /* 데이터 없으면 -1 (Rx 빈 FIFO 읽기 SLVERR 회피) */
int  uartlite_putc(uartlite_t *u, uint8_t c);               /* TX FIFO 가득이면 -1 */
void uartlite_write(uartlite_t *u, const char *s, size_t n);/* 블로킹 */
uint32_t uartlite_take_errors(uartlite_t *u);               /* 누적 오류 반환 후 지움 */
#endif
