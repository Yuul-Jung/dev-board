#include "mcp2515.h"
#include <string.h>

int mcp2515_calc_timing(uint32_t fosc, uint32_t bitrate, uint8_t *cnf1, uint8_t *cnf2, uint8_t *cnf3)
{
    if (!fosc || !bitrate || !cnf1 || !cnf2 || !cnf3) return QSPI_ERR_ARGS;
    int found = 0; uint32_t best_err = 0xFFFFFFFFu; uint8_t b1 = 0, b2 = 0, b3 = 0;
    for (unsigned brp = 0; brp < 64; brp++) {
        uint64_t den = 2ull * (brp + 1) * bitrate;           /* Tq 개수 = fosc / den, 나누어떨어져야 한다 */
        if (fosc % den) continue;
        uint32_t n = (uint32_t)(fosc / den);
        if (n < 8 || n > 25) continue;
        /* 표본 점 = (1 + PRSEG + PHSEG1) / n 을 87.5% 에 가깝게. S = PRSEG + PHSEG1 */
        for (unsigned s = 2; s <= 16; s++) {
            int phseg2 = (int)n - 1 - (int)s;
            if (phseg2 < 2 || phseg2 > 8) continue;
            unsigned ph1 = s / 2 < 1 ? 1 : (s / 2 > 8 ? 8 : s / 2), pr = s - ph1;
            if (pr < 1 || pr > 8) { ph1 = s - 8 > 1 ? s - 8 : 1; pr = s - ph1; if (pr < 1 || pr > 8 || ph1 > 8) continue; }
            uint32_t sp_x1000 = (uint32_t)((1 + s) * 1000u / n);
            uint32_t err = sp_x1000 > 875 ? sp_x1000 - 875 : 875 - sp_x1000;
            if (err < best_err) {
                best_err = err; found = 1;
                unsigned sjw = (unsigned)phseg2 < 4 ? (unsigned)phseg2 : 4;
                b1 = (uint8_t)(((sjw - 1) << 6) | brp);
                b2 = (uint8_t)(0x80 | ((ph1 - 1) << 3) | (pr - 1));   /* BTLMODE=1: PHSEG2 를 CNF3 에서 가져옴. 표본 1회(SAM=0) */
                b3 = (uint8_t)(phseg2 - 1);
            }
        }
    }
    if (!found) return MCP_ERR_TIMING;
    *cnf1 = b1; *cnf2 = b2; *cnf3 = b3;
    return QSPI_OK;
}

int mcp2515_pack_tx(const mcp_frame_t *f, uint8_t out[13])
{
    if (!f || !out || f->dlc > 8) return QSPI_ERR_ARGS;
    if (f->ext ? (f->id > 0x1FFFFFFFu) : (f->id > 0x7FFu)) return QSPI_ERR_ARGS;
    uint32_t sid, eid = 0;
    if (f->ext) { sid = f->id >> 18; eid = f->id & 0x3FFFFu; } else sid = f->id;
    out[0] = (uint8_t)(sid >> 3);
    out[1] = (uint8_t)(((sid & 7u) << 5) | (f->ext ? 1u << 3 : 0u) | ((eid >> 16) & 3u));
    out[2] = (uint8_t)(eid >> 8);
    out[3] = (uint8_t)eid;
    out[4] = (uint8_t)((f->rtr ? 1u << 6 : 0u) | f->dlc);
    if (!f->rtr) memcpy(out + 5, f->data, f->dlc);
    return 5 + (f->rtr ? 0 : f->dlc);
}

static int cmd(qspi_t *s, unsigned cs, const uint8_t *tx, uint8_t *rx, size_t n)
{
    qspi_cs(s, cs, 1);
    int r = qspi_xfer(s, tx, rx, n);
    qspi_cs(s, cs, 0);
    return r;
}

int mcp2515_reset(qspi_t *s, unsigned cs) { uint8_t t = MCP_OP_RESET; return cmd(s, cs, &t, 0, 1); }

int mcp2515_read_regs(qspi_t *s, unsigned cs, uint8_t addr, uint8_t *buf, unsigned n)
{
    uint8_t tx[2 + 16] = { MCP_OP_READ, addr }, rx[2 + 16];
    if (!buf || n == 0 || n > 16) return QSPI_ERR_ARGS;
    int r = cmd(s, cs, tx, rx, 2 + n);
    if (r) return r;
    memcpy(buf, rx + 2, n);
    return QSPI_OK;
}

int mcp2515_bit_modify(qspi_t *s, unsigned cs, uint8_t addr, uint8_t mask, uint8_t val)
{
    uint8_t tx[4] = { MCP_OP_BIT_MODIFY, addr, mask, val };
    return cmd(s, cs, tx, 0, 4);
}

int mcp2515_load_tx(qspi_t *s, unsigned cs, unsigned idx, const mcp_frame_t *f)
{
    uint8_t tx[1 + 13];
    if (idx > 2) return QSPI_ERR_ARGS;
    int n = mcp2515_pack_tx(f, tx + 1);
    if (n < 0) return n;
    tx[0] = (uint8_t)(MCP_OP_LOAD_TX | (2u * idx));
    return cmd(s, cs, tx, 0, 1u + (unsigned)n);
}

int mcp2515_rts(qspi_t *s, unsigned cs, unsigned idx)
{
    if (idx > 2) return QSPI_ERR_ARGS;
    uint8_t t = (uint8_t)(MCP_OP_RTS | (1u << idx));
    return cmd(s, cs, &t, 0, 1);
}

int mcp2515_read_status(qspi_t *s, unsigned cs, uint8_t *st)
{
    uint8_t tx[2] = { MCP_OP_READ_STATUS, 0 }, rx[2];
    if (!st) return QSPI_ERR_ARGS;
    int r = cmd(s, cs, tx, rx, 2);
    if (r) return r;
    *st = rx[1];
    return QSPI_OK;
}
