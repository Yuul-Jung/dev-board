#include "hm10.h"
#include <string.h>
#include <stdio.h>

static int fmt(int n, size_t cap) { return (n < 0 || (size_t)n >= cap) ? BT_ERR_NOSPACE : n; }
static int in_list(const char *w, const char *const *l, unsigned n) { for (unsigned i = 0; i < n; i++) if (strcmp(w, l[i]) == 0) return 1; return 0; }

int hm10_cmd_at(char *o, size_t cap) { if (!o) return BT_ERR_ARGS; return fmt(snprintf(o, cap, "AT"), cap); }

int hm10_cmd_query(char *o, size_t cap, const char *what)
{
    static const char *const L[] = { "NAME", "ADDR", "VERS", "BAUD", "ROLE", "MODE", "PASS", "TYPE", "POWE", "NOTI" };
    if (!o || !what) return BT_ERR_ARGS;
    if (!in_list(what, L, sizeof L / sizeof L[0])) return BT_ERR_VALUE;
    return fmt(snprintf(o, cap, "AT+%s?", what), cap);
}
int hm10_cmd_simple(char *o, size_t cap, const char *what)
{
    static const char *const L[] = { "RESET", "RENEW", "SLEEP", "START" };
    if (!o || !what) return BT_ERR_ARGS;
    if (!in_list(what, L, sizeof L / sizeof L[0])) return BT_ERR_VALUE;
    return fmt(snprintf(o, cap, "AT+%s", what), cap);
}
int hm10_cmd_name(char *o, size_t cap, const char *name)
{
    if (!o || !name) return BT_ERR_ARGS;
    size_t n = strlen(name);
    if (n < 1 || n > HM10_NAME_MAX) return BT_ERR_VALUE;
    for (size_t i = 0; i < n; i++) if ((unsigned char)name[i] < 0x21 || (unsigned char)name[i] > 0x7E || name[i] == '?') return BT_ERR_VALUE;
    return fmt(snprintf(o, cap, "AT+NAME%s", name), cap);
}

static const unsigned BAUDS[9] = { HM10_BAUD_0, HM10_BAUD_1, HM10_BAUD_2, HM10_BAUD_3, HM10_BAUD_4, HM10_BAUD_5, HM10_BAUD_6, HM10_BAUD_7, HM10_BAUD_8 };
int hm10_baud_code(unsigned baud, int v545)
{
    unsigned last = v545 ? 8 : 4;
    for (unsigned i = 0; i <= last; i++) if (BAUDS[i] == baud && i != 7) return (int)i;
    return -1;
}
int hm10_cmd_baud(char *o, size_t cap, unsigned baud, int v545)
{
    if (!o) return BT_ERR_ARGS;
    int c = hm10_baud_code(baud, v545);
    if (c < 0) return BT_ERR_VALUE;
    return fmt(snprintf(o, cap, "AT+BAUD%d", c), cap);
}
static int num(char *o, size_t cap, const char *name, unsigned v, unsigned max)
{
    if (!o) return BT_ERR_ARGS;
    if (v > max) return BT_ERR_VALUE;
    return fmt(snprintf(o, cap, "AT+%s%u", name, v), cap);
}
int hm10_cmd_role(char *o, size_t cap, unsigned r) { return num(o, cap, "ROLE", r, 1); }
int hm10_cmd_mode(char *o, size_t cap, unsigned m) { return num(o, cap, "MODE", m, 2); }
int hm10_cmd_type(char *o, size_t cap, unsigned t) { return num(o, cap, "TYPE", t, 3); }
int hm10_cmd_power(char *o, size_t cap, unsigned p) { return num(o, cap, "POWE", p, 3); }
int hm10_cmd_notify(char *o, size_t cap, int on) { return num(o, cap, "NOTI", on ? 1u : 0u, 1); }
int hm10_cmd_pass(char *o, size_t cap, const char *p)
{
    if (!o || !p) return BT_ERR_ARGS;
    if (strlen(p) != HM10_PASS_LEN) return BT_ERR_VALUE;
    for (unsigned i = 0; i < HM10_PASS_LEN; i++) if (p[i] < '0' || p[i] > '9') return BT_ERR_VALUE;
    return fmt(snprintf(o, cap, "AT+PASS%s", p), cap);
}

int hm10_parse(const char *r, size_t n, hm10_reply_t *o)
{
    if (!r || !o) return BT_ERR_ARGS;
    if (n < 2 || r[0] != 'O' || r[1] != 'K') return BT_ERR_REPLY;
    hm10_reply_t t; memset(&t, 0, sizeof t);
    if (n == 2) { t.ok_only = 1; *o = t; return 0; }
    if (r[2] != '+') return BT_ERR_REPLY;
    for (size_t k = 3; k < n; k++) if ((unsigned char)r[k] < 0x20 || (unsigned char)r[k] > 0x7E) return BT_ERR_REPLY;   /* 보레이트가 어긋난 쓰레기·NUL 은 성공으로 읽지 않음(설계안) */
    size_t i = 3, tl = 0;
    while (i < n && r[i] != ':') { if (tl >= sizeof t.tag - 1) return BT_ERR_NOSPACE; t.tag[tl++] = r[i++]; }
    if (tl == 0) return BT_ERR_REPLY;
    if (i < n) {
        i++;
        size_t vl = 0;
        while (i < n) { if (vl >= sizeof t.value - 1) return BT_ERR_NOSPACE; t.value[vl++] = r[i++]; }
    }
    *o = t;
    return 0;
}

const char *hm10_value(const hm10_reply_t *t, const char *want)
{
    if (!t || !want || t->ok_only) return 0;
    size_t wl = strlen(want);
    if (wl == 0 || strncmp(t->tag, want, wl) != 0) return 0;
    if (t->value[0]) return t->tag[wl] == 0 ? t->value : 0;
    return t->tag + wl;
}

static int run(const bt_io_t *io, const char *cmd, int cl, char *resp, size_t cap, size_t *n)
{
    if (cl < 0) return cl;
    return bt_xfer(io, cmd, (size_t)cl, resp, cap, n);
}
int hm10_probe(const bt_io_t *io)
{
    char c[8], r[16]; size_t n; hm10_reply_t t;
    int e = run(io, c, hm10_cmd_at(c, sizeof c), r, sizeof r, &n);
    if (e) return e;
    e = hm10_parse(r, n, &t);
    return e ? e : (t.ok_only ? 0 : BT_ERR_REPLY);
}
static int is_set(const hm10_reply_t *t) { return !t->ok_only && strncmp(t->tag, "Set", 3) == 0; }
int hm10_set_name(const bt_io_t *io, const char *name)
{
    char c[24], r[64]; size_t n; hm10_reply_t t;
    int e = run(io, c, hm10_cmd_name(c, sizeof c, name), r, sizeof r, &n);
    if (e) return e;
    e = hm10_parse(r, n, &t);
    return e ? e : (is_set(&t) ? 0 : BT_ERR_REPLY);
}
int hm10_set_baud(const bt_io_t *io, unsigned baud, int v545)
{
    char c[16], r[32]; size_t n; hm10_reply_t t;
    int e = run(io, c, hm10_cmd_baud(c, sizeof c, baud, v545), r, sizeof r, &n);
    if (e) return e;
    e = hm10_parse(r, n, &t);
    return e ? e : (is_set(&t) ? 0 : BT_ERR_REPLY);
}
int hm10_query(const bt_io_t *io, const char *what, hm10_reply_t *o)
{
    char c[16], r[64]; size_t n;
    int e = run(io, c, hm10_cmd_query(c, sizeof c, what), r, sizeof r, &n);
    return e ? e : hm10_parse(r, n, o);
}
