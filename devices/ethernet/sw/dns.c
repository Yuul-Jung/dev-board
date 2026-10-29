#include "dns.h"
#include <string.h>

static unsigned be16(const uint8_t *p) { return ((unsigned)p[0] << 8) | p[1]; }
static void put16(uint8_t *p, unsigned v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static int lc(int c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }
static int name_char_ok(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_'; }

/* 이름을 검사하고 소문자 점 표기(끝 점 없음)로 out 에 담는다. 반환: 0 또는 DNS_ERR_NAME */
static int norm_name(const char *s, char out[DNS_MAX_NAME_WIRE + 1])
{
    if (!s) return DNS_ERR_NAME;
    unsigned n = 0, ll = 0, wire = 1;                 /* wire: 끝의 0 바이트 */
    for (; *s; s++) {
        if (*s == '.') {
            if (ll == 0) return DNS_ERR_NAME;
            if (s[1] == 0) break;                      /* 끝의 점 하나 */
            out[n++] = '.'; ll = 0; continue;
        }
        if (!name_char_ok(*s)) return DNS_ERR_NAME;
        if (++ll > DNS_MAX_LABEL) return DNS_ERR_NAME;
        if (ll == 1) wire++;                           /* 길이 바이트 */
        wire++;
        if (wire > DNS_MAX_NAME_WIRE) return DNS_ERR_NAME;
        out[n++] = (char)lc(*s);
    }
    if (n == 0) return DNS_ERR_NAME;
    out[n] = 0;
    return 0;
}

int dns_build_query(uint8_t *buf, unsigned cap, uint16_t id, const char *name)
{
    if (!buf) return QSPI_ERR_ARGS;
    char nm[DNS_MAX_NAME_WIRE + 1];
    int e = norm_name(name, nm);
    if (e) return e;
    unsigned need = DNS_HDR_LEN + (unsigned)strlen(nm) + 2 + 4;       /* 점 개수 + 1 개의 길이 바이트 + 끝 0 = 글자수 + 2 */
    if (need > cap) return DNS_ERR_NOSPACE;
    memset(buf, 0, DNS_HDR_LEN);
    put16(buf, id); put16(buf + 2, DNS_FLAG_RD); put16(buf + 4, 1);
    unsigned o = DNS_HDR_LEN;
    const char *p = nm;
    while (*p) {
        const char *d = p; while (*d && *d != '.') d++;
        unsigned l = (unsigned)(d - p);
        buf[o++] = (uint8_t)l; memcpy(buf + o, p, l); o += l;
        p = *d ? d + 1 : d;
    }
    buf[o++] = 0;
    put16(buf + o, DNS_TYPE_A); put16(buf + o + 2, DNS_CLASS_IN); o += 4;
    return (int)o;
}

/* 이름을 풀어 소문자 점 표기로 out 에 담는다(루트 = 빈 문자열). *next = 이 이름 다음 바이트 위치. 반환 0 또는 -1 */
static int name_decode(const uint8_t *m, unsigned len, unsigned off, char *out, unsigned cap, unsigned *next)
{
    unsigned pos = off, jumps = 0, o = 0, wire = 0; int jumped = 0;
    for (;;) {
        if (pos >= len) return -1;
        unsigned c = m[pos];
        if ((c & 0xC0) == 0xC0) {
            if (pos + 1 >= len) return -1;
            unsigned ptr = ((c & 0x3Fu) << 8) | m[pos + 1];
            if (!jumped) { *next = pos + 2; jumped = 1; }
            if (++jumps > 16 || ptr >= len) return -1;
            pos = ptr; continue;
        }
        if (c & 0xC0) return -1;                      /* 01, 10: 예약 */
        if (c == 0) { if (!jumped) *next = pos + 1; break; }
        pos++;
        if (pos + c > len) return -1;
        wire += c + 1;
        if (wire > DNS_MAX_NAME_WIRE - 1) return -1;
        if (o) { if (o + 1 >= cap) return -1; out[o++] = '.'; }
        if (o + c >= cap) return -1;
        for (unsigned k = 0; k < c; k++) out[o++] = (char)lc(m[pos + k]);
        pos += c;
    }
    out[o] = 0;
    return 0;
}

static int bad(unsigned flags, int e) { return (flags & DNS_FLAG_TC) ? DNS_ERR_TRUNC : e; }

int dns_parse_response(const uint8_t *m, unsigned len, uint16_t id, const char *name, uint8_t ip[4], uint32_t *ttl)
{
    if (!m || !ip) return QSPI_ERR_ARGS;
    char qn[DNS_MAX_NAME_WIRE + 1];
    int e = norm_name(name, qn);
    if (e) return e;
    if (len < DNS_HDR_LEN) return DNS_ERR_FORMAT;
    unsigned flags = be16(m + 2), qd = be16(m + 4), an = be16(m + 6);
    if (!(flags & DNS_FLAG_QR)) return DNS_ERR_FORMAT;
    if ((flags >> 11) & 0xF) return DNS_ERR_FORMAT;
    if (be16(m) != id) return DNS_ERR_ID;
    switch (flags & 0xF) {
    case 0: break;
    case DNS_RCODE_SERVFAIL: return DNS_ERR_SERVFAIL;
    case DNS_RCODE_NXDOMAIN: return DNS_ERR_NXDOMAIN;
    default: return DNS_ERR_RCODE;
    }
    if (qd != 1) return DNS_ERR_FORMAT;
    char nm[DNS_MAX_NAME_WIRE + 1], cur[DNS_MAX_NAME_WIRE + 1];
    unsigned off = DNS_HDR_LEN, nx;
    if (name_decode(m, len, off, nm, sizeof nm, &nx)) return DNS_ERR_FORMAT;
    off = nx;
    if (off + 4 > len) return DNS_ERR_FORMAT;
    if (strcmp(nm, qn) != 0 || be16(m + off) != DNS_TYPE_A || be16(m + off + 2) != DNS_CLASS_IN) return DNS_ERR_QUESTION;
    off += 4;
    memcpy(cur, qn, strlen(qn) + 1);
    unsigned hops = 0;
    for (unsigned i = 0; i < an; i++) {
        if (name_decode(m, len, off, nm, sizeof nm, &nx)) return bad(flags, DNS_ERR_FORMAT);
        off = nx;
        if (off + 10 > len) return bad(flags, DNS_ERR_FORMAT);
        unsigned type = be16(m + off), cls = be16(m + off + 2), rdl = be16(m + off + 8);
        uint32_t t = ((uint32_t)m[off + 4] << 24) | ((uint32_t)m[off + 5] << 16) | ((uint32_t)m[off + 6] << 8) | m[off + 7];
        off += 10;
        if (off + rdl > len) return bad(flags, DNS_ERR_FORMAT);
        if (cls == DNS_CLASS_IN && strcmp(nm, cur) == 0) {
            if (type == DNS_TYPE_A) {
                if (rdl != 4) return DNS_ERR_FORMAT;
                memcpy(ip, m + off, 4); if (ttl) *ttl = t;
                return 0;
            }
            if (type == DNS_TYPE_CNAME) {
                if (++hops > 8) return DNS_ERR_NOANSWER;
                if (name_decode(m, len, off, cur, sizeof cur, &nx) || nx != off + rdl) return bad(flags, DNS_ERR_FORMAT);
            }
        }
        off += rdl;
    }
    return (flags & DNS_FLAG_TC) ? DNS_ERR_TRUNC : DNS_ERR_NOANSWER;
}

int dns_parse_ipv4(const char *s, uint8_t ip[4])
{
    if (!s || !ip) return 0;
    uint8_t v[4];
    for (int k = 0; k < 4; k++) {
        if (*s < '0' || *s > '9') return 0;
        unsigned n = 0, d = 0; int lead0 = (*s == '0');
        while (*s >= '0' && *s <= '9') { n = n * 10 + (unsigned)(*s - '0'); if (++d > 3 || n > 255) return 0; s++; }
        if (lead0 && d > 1) return 0;
        v[k] = (uint8_t)n;
        if (k < 3) { if (*s != '.') return 0; s++; }
    }
    if (*s) return 0;
    memcpy(ip, v, 4);
    return 1;
}
