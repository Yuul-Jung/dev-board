/* HTTP 응답 해석기 무작위 시험: 유효한 응답을 무작위로 망가뜨려 (1) 어떤 입력에도 ASan/UBSan 오류가 없고
 * (2) 입력을 어떻게 쪼개 넣어도(통째, 1바이트, 무작위 조각) 결과(반환값·본문·상태)가 같은지 확인한다. 결정적 난수(고정 시드). */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../sw/http.h"

static uint32_t rs = 12345u;
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
typedef struct { uint8_t b[4096]; unsigned n; } acc_t;
static int sink(void *c, const uint8_t *d, unsigned n) { acc_t *a = c; if (a->n + n > sizeof a->b) return 1; memcpy(a->b + a->n, d, n); a->n += n; return 0; }
static const char *seeds[] = {
    "HTTP/1.1 200 OK\r\nContent-Length: 13\r\nContent-Type: text/plain\r\n\r\nHello, world!",
    "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nHello\r\n7;x=1\r\n, world\r\n0\r\nT: 1\r\n\r\n",
    "HTTP/1.0 200 OK\r\nServer: a\r\n\r\nbody until close",
    "HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 204 No Content\r\n\r\n",
    "HTTP/1.1 301 Moved\nLocation: /x\nContent-Length: 0\n\n",
};
static int one(const uint8_t *in, unsigned len, unsigned mode, acc_t *a, http_parser_t *p)
{
    memset(a, 0, sizeof *a); http_parser_init(p, sink, a);
    int r = 0; unsigned i = 0;
    while (i < len && r == 0) {
        unsigned c = mode == 0 ? len : mode == 1 ? 1 : 1 + rnd() % 17;
        if (c > len - i) c = len - i;
        r = http_parser_feed(p, in + i, c); i += c;
    }
    return r >= 0 && r != 1 ? http_parser_eof(p) + 1000 : r;      /* 완료/오류/eof 결과를 한 숫자로 */
}
int main(void)
{
    unsigned bad = 0, total = 0, ok = 0, errs = 0;
    for (unsigned it = 0; it < 150000; it++) {
        const char *s = seeds[rnd() % (sizeof seeds / sizeof seeds[0])];
        uint8_t buf[512]; unsigned len = (unsigned)strlen(s); memcpy(buf, s, len);
        unsigned nm = rnd() % 4;
        for (unsigned k = 0; k < nm; k++) {
            unsigned op = rnd() % 4, pos = rnd() % len;
            if (op == 0) buf[pos] = (uint8_t)rnd();
            else if (op == 1) buf[pos] = "\r\n:; 0123456789abcdefABCDEFxX"[rnd() % 29];
            else if (op == 2 && len > 1) { memmove(buf + pos, buf + pos + 1, len - pos - 1); len--; }
            else if (len < sizeof buf - 1) { memmove(buf + pos + 1, buf + pos, len - pos); buf[pos] = (uint8_t)rnd(); len++; }
        }
        if (rnd() % 5 == 0) len = rnd() % (len + 1);
        acc_t a0, a1, a2; http_parser_t p0, p1, p2;
        int r0 = one(buf, len, 0, &a0, &p0), r1 = one(buf, len, 1, &a1, &p1), r2 = one(buf, len, 2, &a2, &p2);
        total++;
        if (r0 == 1 || r0 == 1000) ok++; else if (r0 < 0) errs++;
        if (r0 != r1 || r0 != r2 || a0.n != a1.n || a0.n != a2.n || memcmp(a0.b, a1.b, a0.n) || memcmp(a0.b, a2.b, a0.n) || p0.status != p1.status || p0.status != p2.status) {
            if (bad++ < 5) printf("MISMATCH it=%u r=%d/%d/%d n=%u/%u/%u\n", it, r0, r1, r2, a0.n, a1.n, a2.n);
        }
    }
    printf("%u cases, %u complete/ok, %u errors, %u mismatches\n", total, ok, errs, bad);
    if (bad) return 1;
    printf("ALL PASS (http fuzz)\n");
    return 0;
}
