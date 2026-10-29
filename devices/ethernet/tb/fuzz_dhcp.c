/* DHCP 응답 해석기 무작위 시험(고정 시드): 유효한 OFFER 를 무작위로 망가뜨리거나 자르거나 이어 붙여도
 * (1) ASan/UBSan 오류 없이 끝나고 (2) 반환값이 정의된 집합 안에 있으며 (3) 성공(0)이면 yiaddr·서버 ID·마스크가 메시지 안에 실제로 있고 종류는 존재하며 (4) 실패하면 출력을 건드리지 않는지 확인한다. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../sw/dhcp.h"

static uint32_t rs = 246813579u;
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
static const uint8_t MAC[6] = { 0x00, 0x08, 0xDC, 0x01, 0x02, 0x03 };
int main(void)
{
    uint8_t seed[300]; unsigned sl = 0;
    memset(seed, 0, sizeof seed);
    seed[0] = 2; seed[1] = 1; seed[2] = 6; seed[4] = 0x39; seed[5] = 0x03; seed[6] = 0xF3; seed[7] = 0x26;
    seed[16] = 192; seed[17] = 168; seed[18] = 7; seed[19] = 55; memcpy(seed + 28, MAC, 6);
    seed[236] = 0x63; seed[237] = 0x82; seed[238] = 0x53; seed[239] = 0x63;
    const uint8_t opt[] = { 53, 1, 2, 54, 4, 192, 168, 7, 1, 51, 4, 0, 0, 0x0E, 0x10, 1, 4, 255, 255, 255, 0,
                            3, 8, 192, 168, 7, 254, 192, 168, 7, 253, 6, 8, 8, 8, 8, 8, 8, 8, 4, 4, 58, 4, 0, 0, 7, 8, 59, 4, 0, 0, 12, 78, 0, 12, 2, 'a', 'b', 255 };
    memcpy(seed + 240, opt, sizeof opt); sl = 240 + (unsigned)sizeof opt;
    unsigned total = 0, okc = 0, bad = 0;
    for (unsigned it = 0; it < 400000; it++) {
        uint8_t buf[400]; unsigned len = sl; memcpy(buf, seed, len);
        unsigned nm = rnd() % 5;
        for (unsigned k = 0; k < nm; k++) {
            unsigned op = rnd() % 5, pos = rnd() % len;
            if (op == 0) buf[pos] = (uint8_t)rnd();
            else if (op == 1) buf[pos] = (uint8_t)(240 + rnd() % 16);
            else if (op == 2 && len > 1) { memmove(buf + pos, buf + pos + 1, len - pos - 1); len--; }
            else if (op == 3 && len < sizeof buf - 1) { memmove(buf + pos + 1, buf + pos, len - pos); buf[pos] = (uint8_t)rnd(); len++; }
            else buf[pos] ^= (uint8_t)(1u << (rnd() % 8));
        }
        if (rnd() % 6 == 0) len = rnd() % (len + 1);
        dhcp_info_t o; memset(&o, 0xEE, sizeof o);
        int r = dhcp_parse(buf, len, 0x3903F326u, MAC, &o);
        total++;
        int known = r == 0 || r == DHCP_ERR_FORMAT || r == DHCP_ERR_NOTMINE || r == DHCP_ERR_OPTIONS;
        if (!known) { if (bad++ < 5) printf("UNKNOWN code %d it=%u\n", r, it); continue; }
        if (r == 0) {
            okc++;
            int f1 = 0, f2 = 0, f3 = 1;
            for (unsigned i = 0; i + 4 <= len; i++) { if (memcmp(buf + i, o.yiaddr, 4) == 0) f1 = 1; if (!o.has_server || memcmp(buf + i, o.server, 4) == 0) f2 = 1; }
            if (o.has_mask) { f3 = 0; for (unsigned i = 0; i + 4 <= len; i++) if (memcmp(buf + i, o.mask, 4) == 0) f3 = 1; }
            if (!f1 || !f2 || !f3 || o.type == 0 || len < 240) { if (bad++ < 5) printf("value not in message it=%u\n", it); }
            if (o.type == DHCP_OFFER || o.type == DHCP_ACK) { if (!o.has_server || !o.has_lease || o.yiaddr[0] == 0 || o.yiaddr[0] == 127 || o.yiaddr[0] >= 224) { if (bad++ < 5) printf("OFFER/ACK invariant it=%u\n", it); } }
            uint32_t l, a, b; dhcp_times(&o, &l, &a, &b);
            if (l != 0xFFFFFFFFu && !(a <= b && b <= l)) { if (bad++ < 5) printf("times order it=%u\n", it); }
        } else {
            const uint8_t *p = (const uint8_t *)&o; int touched = 0; for (unsigned i = 0; i < sizeof o; i++) if (p[i] != 0xEE) touched = 1;
            if (touched) { if (bad++ < 5) printf("output touched on error %d it=%u\n", r, it); }
        }
    }
    printf("%u cases, %u successes, %u problems\n", total, okc, bad);
    if (bad) return 1;
    printf("ALL PASS (dhcp fuzz)\n");
    return 0;
}
