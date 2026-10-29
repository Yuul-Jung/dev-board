#include "json.h"
#include <string.h>

typedef struct { const char *s; unsigned n, i; } cur_t;

static int at(const cur_t *c) { return c->i < c->n ? (unsigned char)c->s[c->i] : -1; }
static void ws(cur_t *c)
{
    while (c->i < c->n) { char ch = c->s[c->i]; if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') c->i++; else break; }
}
static int hexv(int ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}
static int isdig(int ch) { return ch >= '0' && ch <= '9'; }

static int str_scan(cur_t *c)      /* 여는 따옴표 위치에서 시작, 닫는 따옴표 다음에서 끝 */
{
    c->i++;
    for (;;) {
        int ch = at(c);
        if (ch <= (int)JSON_CTRL_MAX) return JSON_ERR_SYNTAX;      /* 끝(-1) 또는 제어 문자 */
        c->i++;
        if (ch == '"') return 0;
        if (ch == '\\') {
            int e = at(c);
            if (e < 0) return JSON_ERR_SYNTAX;
            c->i++;
            switch (e) {
            case '"': case '\\': case '/': case 'b': case 'f': case 'n': case 'r': case 't': break;
            case 'u': for (unsigned k = 0; k < JSON_HEX_DIGITS; k++) { if (hexv(at(c)) < 0) return JSON_ERR_SYNTAX; c->i++; } break;
            default: return JSON_ERR_SYNTAX;
            }
        }
    }
}

static int num_scan(cur_t *c)
{
    if (at(c) == '-') c->i++;
    int d = at(c);
    if (d == '0') c->i++;
    else if (d >= '1' && d <= '9') { while (isdig(at(c))) c->i++; }
    else return JSON_ERR_SYNTAX;
    if (at(c) == '.') { c->i++; if (!isdig(at(c))) return JSON_ERR_SYNTAX; while (isdig(at(c))) c->i++; }
    if (at(c) == 'e' || at(c) == 'E') {
        c->i++;
        if (at(c) == '+' || at(c) == '-') c->i++;
        if (!isdig(at(c))) return JSON_ERR_SYNTAX;
        while (isdig(at(c))) c->i++;
    }
    return 0;
}

static int lit(cur_t *c, const char *w)
{
    unsigned l = (unsigned)strlen(w);
    if (c->n - c->i < l || memcmp(c->s + c->i, w, l) != 0) return JSON_ERR_SYNTAX;
    c->i += l;
    return 0;
}

static int value(cur_t *c, unsigned depth, json_val_t *o);

static int arr_scan(cur_t *c, unsigned depth)
{
    if (depth >= JSON_MAX_DEPTH) return JSON_ERR_DEPTH;
    c->i++; ws(c);
    if (at(c) == ']') { c->i++; return 0; }
    for (;;) {
        int e = value(c, depth + 1, 0);
        if (e) return e;
        ws(c);
        int ch = at(c);
        if (ch == ',') { c->i++; continue; }
        if (ch == ']') { c->i++; return 0; }
        return JSON_ERR_SYNTAX;
    }
}

static int obj_scan(cur_t *c, unsigned depth)
{
    if (depth >= JSON_MAX_DEPTH) return JSON_ERR_DEPTH;
    c->i++; ws(c);
    if (at(c) == '}') { c->i++; return 0; }
    for (;;) {
        ws(c);
        if (at(c) != '"') return JSON_ERR_SYNTAX;
        int e = str_scan(c);
        if (e) return e;
        ws(c);
        if (at(c) != ':') return JSON_ERR_SYNTAX;
        c->i++;
        e = value(c, depth + 1, 0);
        if (e) return e;
        ws(c);
        int ch = at(c);
        if (ch == ',') { c->i++; continue; }
        if (ch == '}') { c->i++; return 0; }
        return JSON_ERR_SYNTAX;
    }
}

static int value(cur_t *c, unsigned depth, json_val_t *o)
{
    ws(c);
    int ch = at(c);
    if (ch < 0) return JSON_ERR_SYNTAX;
    unsigned start = c->i, type; int e;
    switch (ch) {
    case '"': type = JSON_STR; e = str_scan(c); break;
    case '{': type = JSON_OBJ; e = obj_scan(c, depth); break;
    case '[': type = JSON_ARR; e = arr_scan(c, depth); break;
    case 't': type = JSON_TRUE; e = lit(c, "true"); break;
    case 'f': type = JSON_FALSE; e = lit(c, "false"); break;
    case 'n': type = JSON_NULL; e = lit(c, "null"); break;
    default:
        if (ch != '-' && !isdig(ch)) return JSON_ERR_SYNTAX;
        type = JSON_NUM; e = num_scan(c); break;
    }
    if (e) return e;
    if (o) { o->p = c->s + start; o->len = c->i - start; o->type = type; }
    return 0;
}

int json_parse(const char *s, unsigned n, json_val_t *root)
{
    if (!s || !root) return JSON_ERR_ARGS;
    cur_t c = { s, n, 0 };
    if (n >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF) c.i = 3;
    json_val_t v;
    int e = value(&c, 0, &v);
    if (e) return e;
    ws(&c);
    if (c.i != n) return JSON_ERR_SYNTAX;
    *root = v;
    return 0;
}
int json_validate(const char *s, unsigned n) { json_val_t r; return json_parse(s, n, &r); }

/* ---------- 문자열 풀기 ---------- */
static int hex4(const char *p, unsigned avail)
{
    if (avail < JSON_HEX_DIGITS) return -1;
    int v = 0;
    for (unsigned k = 0; k < JSON_HEX_DIGITS; k++) { int h = hexv((unsigned char)p[k]); if (h < 0) return -1; v = v * 16 + h; }
    return v;
}
static int unesc(const char *r, unsigned n, char *out, unsigned cap, unsigned *olen)
{
    unsigned o = 0, i = 0;
#define PUT(b) do { if (o + 1 >= cap) return JSON_ERR_NOSPACE; out[o++] = (char)(b); } while (0)
    while (i < n) {
        unsigned char ch = (unsigned char)r[i++];
        if (ch != '\\') { PUT(ch); continue; }
        if (i >= n) return JSON_ERR_SYNTAX;
        char e = r[i++];
        switch (e) {
        case 'b': PUT(8); break;
        case 'f': PUT(12); break;
        case 'n': PUT(10); break;
        case 'r': PUT(13); break;
        case 't': PUT(9); break;
        case '"': case '\\': case '/': PUT(e); break;
        case 'u': {
            int cp = hex4(r + i, n - i);
            if (cp < 0) return JSON_ERR_SYNTAX;
            i += JSON_HEX_DIGITS;
            if (cp >= 0xD800 && cp <= 0xDBFF) {
                if (i + 2 > n || r[i] != '\\' || r[i + 1] != 'u') return JSON_ERR_UTF16;
                int lo = hex4(r + i + 2, n - i - 2);
                if (lo < 0xDC00 || lo > 0xDFFF) return lo < 0 ? JSON_ERR_SYNTAX : JSON_ERR_UTF16;
                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                i += JSON_ESC_U_LEN;
            } else if (cp >= 0xDC00 && cp <= 0xDFFF) return JSON_ERR_UTF16;
            if (cp < 0x80) PUT(cp);
            else if (cp < 0x800) { PUT(0xC0 | (cp >> 6)); PUT(0x80 | (cp & 0x3F)); }
            else if (cp < 0x10000) { PUT(0xE0 | (cp >> 12)); PUT(0x80 | ((cp >> 6) & 0x3F)); PUT(0x80 | (cp & 0x3F)); }
            else { PUT(0xF0 | (cp >> 18)); PUT(0x80 | ((cp >> 12) & 0x3F)); PUT(0x80 | ((cp >> 6) & 0x3F)); PUT(0x80 | (cp & 0x3F)); }
            break; }
        default: return JSON_ERR_SYNTAX;
        }
    }
#undef PUT
    if (o >= cap) return JSON_ERR_NOSPACE;
    out[o] = 0;
    if (olen) *olen = o;
    return 0;
}

int json_str(const json_val_t *v, char *out, unsigned cap, unsigned *len)
{
    if (!v || !out || cap == 0) return JSON_ERR_ARGS;
    if (v->type != JSON_STR || v->len < 2) return JSON_ERR_TYPE;
    return unesc(v->p + 1, v->len - 2, out, cap, len);
}

/* ---------- 숫자 ---------- */
int json_int(const json_val_t *v, int32_t *out)
{
    if (!v || !out) return JSON_ERR_ARGS;
    if (v->type != JSON_NUM) return JSON_ERR_TYPE;
    unsigned i = 0; int neg = 0;
    if (v->p[0] == '-') { neg = 1; i = 1; }
    uint64_t m = 0; const uint64_t lim = neg ? 2147483648ull : 2147483647ull;
    int range = 0;
    for (; i < v->len; i++) {
        char ch = v->p[i];
        if (ch == '.' || ch == 'e' || ch == 'E') return JSON_ERR_TYPE;
        m = m * 10 + (uint64_t)(ch - '0');
        if (m > lim) { m = lim + 1; range = 1; }
    }
    if (range) return JSON_ERR_RANGE;
    *out = neg ? (int32_t)(-(int64_t)m) : (int32_t)m;
    return 0;
}

int json_fixed(const json_val_t *v, unsigned decimals, int32_t *out)
{
    if (!v || !out || decimals > 9) return JSON_ERR_ARGS;
    if (v->type != JSON_NUM) return JSON_ERR_TYPE;
    unsigned i = 0; int neg = 0;
    if (v->p[0] == '-') { neg = 1; i = 1; }
    uint64_t m = 0; int sig = 0; long e10 = 0;
    for (; i < v->len && isdig((unsigned char)v->p[i]); i++) {
        unsigned d = (unsigned)(v->p[i] - '0');
        if (sig < 18) { m = m * 10 + d; if (m) sig++; } else e10++;
    }
    if (i < v->len && v->p[i] == '.') {
        for (i++; i < v->len && isdig((unsigned char)v->p[i]); i++) {
            unsigned d = (unsigned)(v->p[i] - '0');
            if (sig < 18) { m = m * 10 + d; if (m) sig++; e10--; }
        }
    }
    if (i < v->len && (v->p[i] == 'e' || v->p[i] == 'E')) {
        i++;
        int es = 1;
        if (i < v->len && (v->p[i] == '+' || v->p[i] == '-')) { es = v->p[i] == '-' ? -1 : 1; i++; }
        long ev = 0;
        for (; i < v->len && isdig((unsigned char)v->p[i]); i++) { ev = ev * 10 + (v->p[i] - '0'); if (ev > 100000) ev = 100000; }
        e10 += es * ev;
    }
    const uint64_t lim = neg ? 2147483648ull : 2147483647ull;
    if (m == 0) { *out = 0; return 0; }
    long shift = e10 + (long)decimals;
    if (shift >= 0) {
        if (shift > 10) return JSON_ERR_RANGE;
        for (long k = 0; k < shift; k++) { if (m > lim / 10) return JSON_ERR_RANGE; m *= 10; }
    } else {
        if (-shift > 19) m = 0;
        else for (long k = 0; k < -shift; k++) m /= 10;
    }
    if (m > lim) return JSON_ERR_RANGE;
    *out = neg ? (int32_t)(-(int64_t)m) : (int32_t)m;
    return 0;
}

/* ---------- 경로 ---------- */
static int member(const json_val_t *obj, const char *key, unsigned klen, json_val_t *out)
{
    cur_t c = { obj->p, obj->len, 1 };
    ws(&c);
    if (at(&c) == '}') return JSON_ERR_NOTFOUND;
    for (;;) {
        ws(&c);
        unsigned ns = c.i + 1;
        int e = str_scan(&c);
        if (e) return e;
        unsigned ne = c.i - 1;
        ws(&c); c.i++;                      /* ':' */
        json_val_t mv;
        e = value(&c, 0, &mv);
        if (e) return e;
        char tmp[64]; unsigned tl = 0;
        if (unesc(c.s + ns, ne - ns, tmp, sizeof tmp, &tl) == 0 && tl == klen && memcmp(tmp, key, klen) == 0) { *out = mv; return 0; }
        ws(&c);
        if (at(&c) == ',') { c.i++; continue; }
        return JSON_ERR_NOTFOUND;
    }
}
static int element(const json_val_t *arr, unsigned idx, json_val_t *out)
{
    cur_t c = { arr->p, arr->len, 1 };
    ws(&c);
    if (at(&c) == ']') return JSON_ERR_NOTFOUND;
    for (unsigned k = 0; ; k++) {
        json_val_t ev;
        int e = value(&c, 0, &ev);
        if (e) return e;
        if (k == idx) { *out = ev; return 0; }
        ws(&c);
        if (at(&c) == ',') { c.i++; continue; }
        return JSON_ERR_NOTFOUND;
    }
}

/* 경로를 한 토큰씩 읽는다: 반환 1 = 키(k, klen), 2 = 인덱스(idx), 0 = 끝, 오류면 JSON_ERR_PATH */
static int path_next(const char **pp, int *first, const char **k, unsigned *klen, unsigned *idx)
{
    const char *p = *pp;
    if (!*p) return 0;
    if (*p == '[') {
        p++;
        unsigned v = 0, nd = 0;
        while (isdig((unsigned char)*p)) { v = v * 10 + (unsigned)(*p - '0'); p++; if (++nd > 9) return JSON_ERR_PATH; }
        if (nd == 0 || *p != ']') return JSON_ERR_PATH;
        *pp = p + 1; *idx = v; *first = 0;
        return 2;
    }
    if (!*first) { if (*p != '.') return JSON_ERR_PATH; p++; }
    const char *b = p;
    while (*p && *p != '.' && *p != '[' && *p != ']') p++;
    if (*p == ']' || p == b) return JSON_ERR_PATH;
    *pp = p; *k = b; *klen = (unsigned)(p - b); *first = 0;
    return 1;
}

int json_get(const char *s, unsigned n, const char *path, json_val_t *v)
{
    if (!s || !path || !v) return JSON_ERR_ARGS;
    const char *k = 0; unsigned kl = 0, idx = 0; int first = 1, t;
    for (const char *q = path; (t = path_next(&q, &first, &k, &kl, &idx)) != 0; ) if (t < 0) return t;   /* 경로 문법을 먼저 전부 확인 */
    json_val_t cur;
    int e = json_parse(s, n, &cur);
    if (e) return e;
    const char *p = path; first = 1;
    while ((t = path_next(&p, &first, &k, &kl, &idx)) > 0) {
        if (t == 2) { if (cur.type != JSON_ARR) return JSON_ERR_TYPE; e = element(&cur, idx, &cur); }
        else { if (cur.type != JSON_OBJ) return JSON_ERR_TYPE; e = member(&cur, k, kl, &cur); }
        if (e) return e;
    }
    *v = cur;
    return 0;
}

int json_get_int(const char *s, unsigned n, const char *path, int32_t *out)
{ json_val_t v; int e = json_get(s, n, path, &v); return e ? e : json_int(&v, out); }
int json_get_fixed(const char *s, unsigned n, const char *path, unsigned decimals, int32_t *out)
{ json_val_t v; int e = json_get(s, n, path, &v); return e ? e : json_fixed(&v, decimals, out); }
int json_get_str(const char *s, unsigned n, const char *path, char *out, unsigned cap, unsigned *len)
{ json_val_t v; int e = json_get(s, n, path, &v); return e ? e : json_str(&v, out, cap, len); }
