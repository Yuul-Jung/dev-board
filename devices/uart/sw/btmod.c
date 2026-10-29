#include "btmod.h"

int bt_xfer(const bt_io_t *io, const char *cmd, size_t cmdlen, char *resp, size_t cap, size_t *rlen)
{
    if (!io || !io->u || !io->delay_us || !cmd || cmdlen == 0 || !resp || cap < 2) return BT_ERR_ARGS;
    for (unsigned k = 0; k < BT_FLUSH_MAX && uartlite_getc(io->u) >= 0; k++) { }
    (void)uartlite_take_errors(io->u);
    uartlite_write(io->u, cmd, cmdlen);
    size_t n = 0; uint32_t idle = 0; int over = 0;
    for (;;) {
        int c = uartlite_getc(io->u);
        if (c >= 0) {
            if (n < cap - 1) resp[n++] = (char)c; else over = 1;
            idle = 0;
            continue;
        }
        uint32_t lim = (n == 0 && !over) ? io->first_ms : io->quiet_ms;
        if (idle >= lim) break;
        io->delay_us(io->ctx, 1000u);
        idle++;
    }
    resp[n] = 0;
    if (rlen) *rlen = n;
    uint32_t err = uartlite_take_errors(io->u);
    if (over) return BT_ERR_NOSPACE;
    if (n == 0) return BT_ERR_TIMEOUT;
    if (err) return BT_ERR_UART;
    return BT_OK;
}
