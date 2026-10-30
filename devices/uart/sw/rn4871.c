/* Pmod BLE — RN4871 드라이버 구현. */
#include "rn4871.h"
#include <string.h>
#include <stdio.h>

static int fmt(int n, size_t cap) { return (n < 0 || (size_t)n >= cap) ? BT_ERR_NOSPACE : n; }
static int printable(const char *s, size_t maxl)
{
    size_t n = strlen(s);
    if (n == 0 || n > maxl) return 0;
    for (size_t i = 0; i < n; i++) if ((unsigned char)s[i] < 0x21 || (unsigned char)s[i] > 0x7E) return 0;
    return 1;
}

/* 보레이트 코드: RN-42 와 같은 형식을 가정(D3/미확인) */
static const struct { unsigned baud; const char code[4]; } baud_tab[] = {
    {1200u,   "12"}, {2400u,  "24"}, {4800u,  "48"}, {9600u,  "96"},
    {19200u,  "19"}, {38400u, "38"}, {57600u, "57"}, {115200u,"11"},
    {230400u, "23"}, {460800u,"46"}, {921600u,"92"},
};
#define BAUD_N (sizeof baud_tab / sizeof baud_tab[0])

const char *rn4871_baud_code(unsigned baud)
{
    for (unsigned i = 0; i < BAUD_N; i++) if (baud_tab[i].baud == baud) return baud_tab[i].code;
    return NULL;
}

/* ── 명령 만들기 ──────────────────────────────────────────────────────────── */
int rn4871_cmd_enter(char *o, size_t cap)   { if (!o) return BT_ERR_ARGS; return fmt(snprintf(o, cap, "$$$"), cap); }
int rn4871_cmd_exit(char *o, size_t cap)    { if (!o) return BT_ERR_ARGS; return fmt(snprintf(o, cap, "---\r"), cap); }
int rn4871_cmd_at(char *o, size_t cap)      { if (!o) return BT_ERR_ARGS; return fmt(snprintf(o, cap, "+\r"), cap); }
int rn4871_cmd_name(char *o, size_t cap, const char *name)
{
    if (!o || !name) return BT_ERR_ARGS;
    if (!printable(name, RN4871_NAME_MAX)) return BT_ERR_VALUE;
    return fmt(snprintf(o, cap, "SN,%s\r", name), cap);
}
int rn4871_cmd_baud(char *o, size_t cap, unsigned baud)
{
    if (!o) return BT_ERR_ARGS;
    const char *code = rn4871_baud_code(baud);
    if (!code) return BT_ERR_VALUE;
    return fmt(snprintf(o, cap, "SB,%s\r", code), cap);
}
int rn4871_cmd_factory(char *o, size_t cap) { if (!o) return BT_ERR_ARGS; return fmt(snprintf(o, cap, "SF,1\r"), cap); }
int rn4871_cmd_reboot(char *o, size_t cap)  { if (!o) return BT_ERR_ARGS; return fmt(snprintf(o, cap, "R,1\r"), cap); }

/* ── 응답 확인 ────────────────────────────────────────────────────────────── */
static int contains(const char *r, size_t n, const char *tok)
{
    if (!r || !tok) return 0;
    size_t tl = strlen(tok);
    if (tl == 0 || n < tl) return 0;
    for (size_t i = 0; i + tl <= n; i++) if (memcmp(r + i, tok, tl) == 0) return 1;
    return 0;
}
int rn4871_reply_ok(const char *r, size_t n)  { return contains(r, n, "AOK") ? 0 : BT_ERR_REPLY; }
int rn4871_reply_cmd(const char *r, size_t n) { return contains(r, n, "CMD>") ? 0 : BT_ERR_REPLY; }
int rn4871_reply_end(const char *r, size_t n) { return contains(r, n, "END") ? 0 : BT_ERR_REPLY; }
int rn4871_reply_err(const char *r, size_t n) { return contains(r, n, "Err") ? 0 : BT_ERR_REPLY; }

/* ── IO 고수준 ────────────────────────────────────────────────────────────── */
static int run(const bt_io_t *io, const char *cmd, int cl, char *resp, size_t cap, size_t *n)
{
    if (cl < 0) return cl;
    return bt_xfer(io, cmd, (size_t)cl, resp, cap, n);
}
int rn4871_enter_cmd(const bt_io_t *io)
{
    char c[8], r[16]; size_t n;
    int e = run(io, c, rn4871_cmd_enter(c, sizeof c), r, sizeof r, &n);
    return e ? e : rn4871_reply_cmd(r, n);
}
int rn4871_exit_cmd(const bt_io_t *io)
{
    char c[8], r[16]; size_t n;
    int e = run(io, c, rn4871_cmd_exit(c, sizeof c), r, sizeof r, &n);
    return e ? e : rn4871_reply_end(r, n);
}
int rn4871_probe(const bt_io_t *io)
{
    char c[8], r[16]; size_t n;
    int e = run(io, c, rn4871_cmd_at(c, sizeof c), r, sizeof r, &n);
    return e ? e : rn4871_reply_ok(r, n);
}
int rn4871_set_name(const bt_io_t *io, const char *name)
{
    char c[40], r[16]; size_t n;
    int e = run(io, c, rn4871_cmd_name(c, sizeof c, name), r, sizeof r, &n);
    return e ? e : rn4871_reply_ok(r, n);
}
int rn4871_set_baud(const bt_io_t *io, unsigned baud)
{
    char c[16], r[16]; size_t n;
    int e = run(io, c, rn4871_cmd_baud(c, sizeof c, baud), r, sizeof r, &n);
    return e ? e : rn4871_reply_ok(r, n);
}
int rn4871_factory_reset(const bt_io_t *io)
{
    char c[16], r[16]; size_t n;
    int e = run(io, c, rn4871_cmd_factory(c, sizeof c), r, sizeof r, &n);
    return e ? e : rn4871_reply_ok(r, n);
}
int rn4871_reboot(const bt_io_t *io)
{
    char c[16], r[16]; size_t n;
    int e = run(io, c, rn4871_cmd_reboot(c, sizeof c), r, sizeof r, &n);
    return (e == BT_ERR_TIMEOUT || e == BT_OK) ? 0 : e;
}
