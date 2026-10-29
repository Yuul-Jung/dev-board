/* DNS 응답 해석기 무작위 시험(고정 시드): 유효한 응답을 무작위로 망가뜨리거나 자르거나 이어 붙여도
 * (1) ASan/UBSan 오류 없이 끝나고 (2) 반환값이 정의된 집합 안에 있으며 (3) 성공(0)이면 돌려준 IP 4바이트가 메시지 안에 실제로 있고 질문은 보낸 이름과 같은지 확인한다. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../sw/dns.h"

static uint32_t rs = 987654321u;
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
static const uint8_t seed1[] = { 0xAB, 0xCD, 0x81, 0x80, 0, 1, 0, 2, 0, 0, 0, 0,
    3, 'w', 'w', 'w', 7, 'e', 'x', 'a', 'm', 'p', 'l', 'e', 3, 'c', 'o', 'm', 0, 0, 1, 0, 1,
    0xC0, 12, 0, 5, 0, 1, 0, 0, 0, 60, 0, 6, 3, 'c', 'd', 'n', 0xC0, 20,
    0xC0, 41, 0, 1, 0, 1, 0, 0, 1, 0x2C, 0, 4, 198, 51, 100, 77 };
static const uint8_t seed2[] = { 0xAB, 0xCD, 0x81, 0x80, 0, 1, 0, 1, 0, 0, 0, 0,
    3, 'w', 'w', 'w', 7, 'e', 'x', 'a', 'm', 'p', 'l', 'e', 3, 'c', 'o', 'm', 0, 0, 1, 0, 1,
    0xC0, 12, 0, 1, 0, 1, 0, 0, 0, 5, 0, 4, 10, 1, 2, 3 };
int main(void)
{
    unsigned total = 0, okc = 0, bad = 0;
    for (unsigned it = 0; it < 400000; it++) {
        uint8_t buf[300]; unsigned len;
        if (rnd() & 1) { len = sizeof seed1; memcpy(buf, seed1, len); } else { len = sizeof seed2; memcpy(buf, seed2, len); }
        unsigned nm = rnd() % 5;
        for (unsigned k = 0; k < nm; k++) {
            unsigned op = rnd() % 5, pos = rnd() % len;
            if (op == 0) buf[pos] = (uint8_t)rnd();
            else if (op == 1) buf[pos] = (uint8_t)(rnd() & 1 ? 0xC0 : rnd() % 70);
            else if (op == 2 && len > 1) { memmove(buf + pos, buf + pos + 1, len - pos - 1); len--; }
            else if (op == 3 && len < sizeof buf - 1) { memmove(buf + pos + 1, buf + pos, len - pos); buf[pos] = (uint8_t)rnd(); len++; }
            else buf[pos] ^= (uint8_t)(1u << (rnd() % 8));
        }
        if (rnd() % 6 == 0) len = rnd() % (len + 1);
        uint8_t ip[4] = { 0xEE, 0xEE, 0xEE, 0xEE }; uint32_t ttl = 0xEEEEEEEE;
        int r = dns_parse_response(buf, len, 0xABCD, "www.example.com", ip, &ttl);
        total++;
        int known = r == 0 || r == DNS_ERR_FORMAT || r == DNS_ERR_ID || r == DNS_ERR_NXDOMAIN || r == DNS_ERR_SERVFAIL || r == DNS_ERR_RCODE || r == DNS_ERR_TRUNC || r == DNS_ERR_NOANSWER || r == DNS_ERR_QUESTION;
        if (!known) { if (bad++ < 5) printf("UNKNOWN code %d it=%u\n", r, it); continue; }
        if (r == 0) {
            okc++;
            int found = 0; for (unsigned i = 0; i + 4 <= len; i++) if (memcmp(buf + i, ip, 4) == 0) found = 1;
            if (!found) { if (bad++ < 5) printf("IP not in message it=%u\n", it); }
        } else if (ip[0] != 0xEE || ip[1] != 0xEE || ip[2] != 0xEE || ip[3] != 0xEE || ttl != 0xEEEEEEEE) { if (bad++ < 5) printf("output touched on error %d it=%u\n", r, it); }
    }
    printf("%u cases, %u successes, %u problems\n", total, okc, bad);
    if (bad) return 1;
    printf("ALL PASS (dns fuzz)\n");
    return 0;
}
