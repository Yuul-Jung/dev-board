#include "ps2.h"

int ps2_read_byte(uintptr_t base)
{
    uint32_t v = PS2_RD(base, PS2_DATA);
    return (v & 0x100) ? (int)(v & 0xFF) : -1;
}
void ps2_flush(uintptr_t base) { PS2_WR(base, PS2_CTRL, 0x2u | (PS2_RD(base, PS2_CTRL) & 1u)); }
uint32_t ps2_take_errors(uintptr_t base)
{
    uint32_t e = PS2_RD(base, PS2_STAT) & PS2_ST_ERRMASK;
    if (e) PS2_WR(base, PS2_STAT, e);
    return e;
}

int ps2_decode(ps2_dec_t *s, uint8_t b, ps2_key_t *k)
{
    if (b == 0xE0) { s->ext_pending = 1; return 0; }
    if (b == 0xF0) { s->brk_pending = 1; return 0; }
    k->code = b; k->ext = s->ext_pending; k->release = s->brk_pending;
    s->ext_pending = s->brk_pending = 0;
    return 1;
}
