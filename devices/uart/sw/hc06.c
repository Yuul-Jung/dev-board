#include "hc06.h"
#include <string.h>
#include <stdio.h>

static int fmt(int n, size_t cap) { return (n < 0 || (size_t)n >= cap) ? BT_ERR_NOSPACE : n; }
static int printable(const char *s, size_t minl, size_t maxl)
{
    size_t n = strlen(s);
    if (n < minl || n > maxl) return 0;
    for (size_t i = 0; i < n; i++) if ((unsigned char)s[i] < 0x21 || (unsigned char)s[i] > 0x7E) return 0;
    return 1;
}

static const unsigned BAUDS[12] = { HC06_BAUD_1, HC06_BAUD_2, HC06_BAUD_3, HC06_BAUD_4, HC06_BAUD_5, HC06_BAUD_6, HC06_BAUD_7, HC06_BAUD_8, HC06_BAUD_9, HC06_BAUD_A, HC06_BAUD_B, HC06_BAUD_C };
int hc06_baud_code(unsigned baud)
{
    for (unsigned i = 0; i < 12; i++) if (BAUDS[i] == baud) return i < 9 ? '1' + (int)i : 'A' + (int)(i - 9);
    return -1;
}

int hc06_cmd_at(char *o, size_t cap) { if (!o) return BT_ERR_ARGS; return fmt(snprintf(o, cap, "AT"), cap); }
int hc06_cmd_version(char *o, size_t cap) { if (!o) return BT_ERR_ARGS; return fmt(snprintf(o, cap, "AT+VERSION"), cap); }
int hc06_cmd_name(char *o, size_t cap, const char *name)
{
    if (!o || !name) return BT_ERR_ARGS;
    if (!printable(name, 1, HC06_NAME_MAX)) return BT_ERR_VALUE;
    return fmt(snprintf(o, cap, "AT+NAME%s", name), cap);
}
int hc06_cmd_pin(char *o, size_t cap, const char *pin)
{
    if (!o || !pin) return BT_ERR_ARGS;
    if (strlen(pin) != HC06_PIN_LEN) return BT_ERR_VALUE;
    for (unsigned i = 0; i < HC06_PIN_LEN; i++) if (pin[i] < '0' || pin[i] > '9') return BT_ERR_VALUE;
    return fmt(snprintf(o, cap, "AT+PIN%s", pin), cap);
}
int hc06_cmd_baud(char *o, size_t cap, unsigned baud)
{
    if (!o) return BT_ERR_ARGS;
    int c = hc06_baud_code(baud);
    if (c < 0) return BT_ERR_VALUE;
    return fmt(snprintf(o, cap, "AT+BAUD%c", c), cap);
}
int hc06_cmd_parity(char *o, size_t cap, char k)
{
    if (!o) return BT_ERR_ARGS;
    if (k != 'N' && k != 'E' && k != 'O') return BT_ERR_VALUE;
    return fmt(snprintf(o, cap, "AT+P%c", k), cap);
}
int hc06_cmd_led(char *o, size_t cap, int on) { if (!o) return BT_ERR_ARGS; return fmt(snprintf(o, cap, "AT+LED%d", on ? 1 : 0), cap); }
int hc06_cmd_role(char *o, size_t cap, char r)
{
    if (!o) return BT_ERR_ARGS;
    if (r != 'S' && r != 'M') return BT_ERR_VALUE;
    return fmt(snprintf(o, cap, "AT+ROLE=%c", r), cap);
}

static int eq(const char *r, size_t n, const char *w) { return r && n == strlen(w) && memcmp(r, w, n) == 0; }
int hc06_reply_ok(const char *r, size_t n) { return eq(r, n, "OK") ? 0 : BT_ERR_REPLY; }
int hc06_reply_name(const char *r, size_t n) { return eq(r, n, "OKsetname") ? 0 : BT_ERR_REPLY; }
int hc06_reply_pin(const char *r, size_t n) { return eq(r, n, "OKsetPIN") ? 0 : BT_ERR_REPLY; }
int hc06_reply_version(const char *r, size_t n, char *out, size_t cap)
{
    if (!r || !out || cap == 0) return BT_ERR_ARGS;
    if (n < 3 || r[0] != 'O' || r[1] != 'K') return BT_ERR_REPLY;
    for (size_t k = 2; k < n; k++) if ((unsigned char)r[k] < 0x20 || (unsigned char)r[k] > 0x7E) return BT_ERR_REPLY;   /* 쓰레기·NUL 거절(설계안) */
    if (n - 2 >= cap) return BT_ERR_NOSPACE;
    memcpy(out, r + 2, n - 2); out[n - 2] = 0;
    return 0;
}
int hc06_reply_baud(const char *r, size_t n, unsigned baud)
{
    char w[16]; snprintf(w, sizeof w, "OK%u", baud);
    return eq(r, n, w) ? 0 : BT_ERR_REPLY;
}

static int run(const bt_io_t *io, const char *cmd, int cl, char *resp, size_t cap, size_t *n)
{
    if (cl < 0) return cl;
    return bt_xfer(io, cmd, (size_t)cl, resp, cap, n);
}
int hc06_probe(const bt_io_t *io)
{
    char c[16], r[16]; size_t n; int e = run(io, c, hc06_cmd_at(c, sizeof c), r, sizeof r, &n);
    return e ? e : hc06_reply_ok(r, n);
}
int hc06_get_version(const bt_io_t *io, char *out, size_t cap)
{
    char c[16], r[48]; size_t n; int e = run(io, c, hc06_cmd_version(c, sizeof c), r, sizeof r, &n);
    return e ? e : hc06_reply_version(r, n, out, cap);
}
int hc06_set_name(const bt_io_t *io, const char *name)
{
    char c[40], r[24]; size_t n; int e = run(io, c, hc06_cmd_name(c, sizeof c, name), r, sizeof r, &n);
    return e ? e : hc06_reply_name(r, n);
}
int hc06_set_pin(const bt_io_t *io, const char *pin)
{
    char c[16], r[24]; size_t n; int e = run(io, c, hc06_cmd_pin(c, sizeof c, pin), r, sizeof r, &n);
    return e ? e : hc06_reply_pin(r, n);
}
int hc06_set_baud(const bt_io_t *io, unsigned baud)
{
    char c[16], r[24]; size_t n; int e = run(io, c, hc06_cmd_baud(c, sizeof c, baud), r, sizeof r, &n);
    return e ? e : hc06_reply_baud(r, n, baud);
}
