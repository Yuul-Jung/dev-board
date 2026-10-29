#include "nmea.h"
#include <string.h>

void nmea_asm_init(nmea_asm_t *a) { a->n = 0; a->in = 0; }

int nmea_feed(nmea_asm_t *a, char c)
{
    if (c == '$') { a->n = 0; a->in = 1; return 0; }
    if (!a->in) return 0;
    if (c == '\r' || c == '\n') {
        a->in = 0;
        if (a->n == 0) return 0;
        a->buf[a->n] = 0;
        return 1;
    }
    if (a->n >= NMEA_MAX - 1) { a->in = 0; a->n = 0; return -1; }
    a->buf[a->n++] = c;
    return 0;
}

static int hexv(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

int nmea_check(const char *line)
{
    uint8_t x = 0; const char *p = line;
    while (*p && *p != '*') x ^= (uint8_t)*p++;
    if (*p != '*') return 0;
    int h = hexv(p[1]), l = hexv(p[2]);
    if (h < 0 || l < 0 || p[3] != 0) return 0;
    return x == (uint8_t)(h * 16 + l);
}

/* 필드 k(0 시작)의 시작 포인터와 길이. 없으면 NULL */
static const char *field(const char *line, int k, int *len)
{
    const char *p = line;
    while (k--) { while (*p && *p != ',' && *p != '*') p++; if (*p != ',') return 0; p++; }
    const char *e = p;
    while (*e && *e != ',' && *e != '*') e++;
    *len = (int)(e - p);
    return p;
}

static int dig(const char *s, int n, int *ok)
{
    int v = 0;
    for (int i = 0; i < n; i++) { if (s[i] < '0' || s[i] > '9') { *ok = 0; return 0; } v = v * 10 + s[i] - '0'; }
    return v;
}

/* 위도·경도: 소수점 앞 마지막 두 자리부터가 분. ddmm 과 dddmm 모두 처리(문서의 RMC 표기 상충 대응). 결과 1e-7 도 */
static int coord(const char *s, int len, int32_t *out)
{
    int dot = -1;
    for (int i = 0; i < len; i++) if (s[i] == '.') { dot = i; break; }
    if (dot < 0) dot = len;
    if (dot < 3) return 0;
    int ok = 1;
    int deg = dig(s, dot - 2, &ok);
    int mi  = dig(s + dot - 2, 2, &ok);
    int64_t frac = 0, scale = 1;               /* 분의 소수부, 최대 5자리 */
    for (int i = dot + 1; i < len && scale < 100000; i++) {
        if (s[i] < '0' || s[i] > '9') return 0;
        frac = frac * 10 + (s[i] - '0'); scale *= 10;
    }
    if (!ok) return 0;
    /* 분 = mi + frac/scale → 도 = deg + 분/60 ; 1e-7 도 단위 */
    int64_t min_e5 = (int64_t)mi * scale + frac;           /* 분 × scale */
    int64_t d_e7 = (int64_t)deg * 10000000 + (min_e5 * 10000000) / (60 * scale);
    *out = (int32_t)d_e7;
    return 1;
}

int nmea_parse_rmc(const char *line, nmea_rmc_t *r)
{
    int n; const char *f;
    if (strlen(line) < 5 || strncmp(line + 2, "RMC,", 4) != 0) return 0;
    if (!nmea_check(line)) return -1;
    memset(r, 0, sizeof *r);
    int ok = 1;
    if (!(f = field(line, 1, &n))) return -1;
    if (n >= 6) { r->hh = dig(f, 2, &ok); r->mm = dig(f + 2, 2, &ok); r->ss = dig(f + 4, 2, &ok); }
    if (!(f = field(line, 2, &n))) return -1;
    r->valid = (n == 1 && f[0] == 'A');
    int32_t v;
    if ((f = field(line, 3, &n)) && n && coord(f, n, &v)) {
        r->lat_e7 = v;
        const char *h = field(line, 4, &n); if (h && n == 1 && h[0] == 'S') r->lat_e7 = -v;
    }
    if ((f = field(line, 5, &n)) && n && coord(f, n, &v)) {
        r->lon_e7 = v;
        const char *h = field(line, 6, &n); if (h && n == 1 && h[0] == 'W') r->lon_e7 = -v;
    }
    if ((f = field(line, 7, &n)) && n) {
        int64_t w = 0, sc = 1; int i = 0;
        for (; i < n && f[i] != '.'; i++) { if (f[i] < '0' || f[i] > '9') { ok = 0; break; } w = w * 10 + f[i] - '0'; }
        int fr = 0, fd = 0;
        if (i < n && f[i] == '.') for (i++; i < n && fd < 2; i++, fd++) { fr = fr * 10 + (f[i] - '0'); }
        while (fd++ < 2) fr *= 10;
        (void)sc;
        r->speed_cent_kn = (int32_t)(w * 100 + fr);
    }
    if ((f = field(line, 9, &n)) && n >= 6) { r->day = dig(f, 2, &ok); r->mon = dig(f + 2, 2, &ok); r->year2 = dig(f + 4, 2, &ok); }
    return ok ? 1 : -1;
}
