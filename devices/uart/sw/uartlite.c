#include "uartlite.h"

/* STAT 읽기는 오류 비트(5~7)를 지우므로 읽을 때마다 누적한다. */
static uint32_t stat(uartlite_t *u)
{
    uint32_t s = UL_RD(u->base, UL_STAT);
    u->err |= s & UL_ST_ERRMASK;
    return s;
}

void uartlite_init(uartlite_t *u, uintptr_t base)
{
    u->base = base; u->err = 0;
    UL_WR(base, UL_CTRL, UL_CT_RST_TX | UL_CT_RST_RX); /* 인터럽트 비트 0 */
    (void)stat(u);
}

int uartlite_getc(uartlite_t *u)
{
    if (!(stat(u) & UL_ST_RXVALID)) return -1;
    return (int)(UL_RD(u->base, UL_RX) & 0xFF);
}

int uartlite_putc(uartlite_t *u, uint8_t c)
{
    if (stat(u) & UL_ST_TXFULL) return -1;
    UL_WR(u->base, UL_TX, c);
    return 0;
}

void uartlite_write(uartlite_t *u, const char *s, size_t n)
{
    for (size_t i = 0; i < n; i++)
        while (uartlite_putc(u, (uint8_t)s[i]) != 0) { }
}

uint32_t uartlite_take_errors(uartlite_t *u)
{
    (void)stat(u);
    uint32_t e = u->err; u->err = 0; return e;
}
