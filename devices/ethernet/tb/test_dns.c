/* DNS 클라이언트 시험: 질의 바이트(손으로 계산), 응답 해석(압축 포인터·CNAME·오류 코드·악성 포인터), IPv4 문자열, W5500 모델 위 dns_resolve, 이름으로 NTP·HTTP.
 * 한계: 응답은 내가 손으로 조립한 것이지 실제 서버 캡처가 아니다. RFC 1035 원문은 열지 못했고 Chromium·iPXE·zytrax(D3) 요약 기준이다. */
#include "../../spi/tb/w5500_model.h"
#include "../sw/net_name.h"

/* ---------- 메시지 조립 도우미 ---------- */
typedef struct { uint8_t b[700]; unsigned n; } msg_t;
static void u8(msg_t *m, unsigned v) { m->b[m->n++] = (uint8_t)v; }
static void u16(msg_t *m, unsigned v) { u8(m, v >> 8); u8(m, v); }
static void u32(msg_t *m, uint32_t v) { u16(m, v >> 16); u16(m, v & 0xFFFF); }
static void hdr(msg_t *m, unsigned id, unsigned flags, unsigned qd, unsigned an) { m->n = 0; u16(m, id); u16(m, flags); u16(m, qd); u16(m, an); u16(m, 0); u16(m, 0); }
static void nm(msg_t *m, const char *s)
{
    while (*s) { const char *d = s; while (*d && *d != '.') d++; u8(m, (unsigned)(d - s)); for (; s < d; s++) u8(m, (unsigned char)*s); if (*d) s++; }
    u8(m, 0);
}
static void question(msg_t *m, const char *name, unsigned type, unsigned cls) { nm(m, name); u16(m, type); u16(m, cls); }
static void rr_ptr(msg_t *m, unsigned ptr, unsigned type, unsigned cls, uint32_t ttl, unsigned rdl) { u16(m, 0xC000 | ptr); u16(m, type); u16(m, cls); u32(m, ttl); u16(m, rdl); }
static void rr_a(msg_t *m, unsigned ptr, uint32_t ttl, const uint8_t ip[4]) { rr_ptr(m, ptr, 1, 1, ttl, 4); for (int i = 0; i < 4; i++) u8(m, ip[i]); }

static const uint8_t R1[4] = { 198, 51, 100, 77 }, R2[4] = { 203, 0, 113, 9 };
static int parse(const msg_t *m, unsigned id, const char *name, uint8_t *ip, uint32_t *ttl) { return dns_parse_response(m->b, m->n, (uint16_t)id, name, ip, ttl); }
static msg_t std_reply(unsigned id, unsigned flags, const char *qn, unsigned an) { msg_t m; hdr(&m, id, flags, 1, an); question(&m, qn, 1, 1); return m; }

static void t_build(void)
{
    printf("[질의 만들기]\n");
    uint8_t b[300];
    const uint8_t want[] = { 0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             7, 'e', 'x', 'a', 'm', 'p', 'l', 'e', 3, 'c', 'o', 'm', 0, 0x00, 0x01, 0x00, 0x01 };
    int n = dns_build_query(b, sizeof b, 0x1234, "example.com");
    CHECK(n == (int)sizeof want && memcmp(b, want, sizeof want) == 0, "example.com 질의 29바이트(헤더 12 + 이름 13 + 4)");
    CHECK(dns_build_query(b, sizeof b, 0x1234, "example.com.") == n && memcmp(b, want, sizeof want) == 0, "끝 점 하나는 같은 결과");
    n = dns_build_query(b, sizeof b, 0xBEEF, "A-b_1.Example.COM");
    CHECK(n > 0 && b[0] == 0xBE && b[1] == 0xEF && b[12] == 5 && memcmp(b + 13, "a-b_1", 5) == 0 && b[18] == 7 && memcmp(b + 19, "example", 7) == 0, "소문자로 바꿔 인코딩, - 와 _ 허용");
    CHECK(dns_build_query(b, 29, 1, "example.com") == 29 && dns_build_query(b, 28, 1, "example.com") == DNS_ERR_NOSPACE, "버퍼 경계: 29 가능, 28 NOSPACE");
    CHECK(dns_build_query(b, sizeof b, 1, "a") == 12 + 3 + 4, "한 글자 이름");
    char l63[80], l64[80]; memset(l63, 'a', 63); l63[63] = 0; memset(l64, 'a', 64); l64[64] = 0;
    CHECK(dns_build_query(b, sizeof b, 1, l63) == 12 + 1 + 63 + 1 + 4, "레이블 63자 허용");
    CHECK(dns_build_query(b, sizeof b, 1, l64) == DNS_ERR_NAME, "레이블 64자 거절");
    /* 이름 전체: 길이 바이트 포함 255 (끝 0 포함). 63+63+63+... 레이블 3개(각 63) = 3*64 + 1 = 193, 여기에 레이블 하나 더: 61자 → 193 + 62 = 255 */
    char big[300]; unsigned k = 0;
    for (int i = 0; i < 3; i++) { memset(big + k, 'a', 63); k += 63; big[k++] = '.'; }
    memset(big + k, 'b', 61); k += 61; big[k] = 0;
    CHECK(dns_build_query(b, sizeof b, 1, big) == 12 + 255 + 4, "이름 255바이트(끝 0 포함) 허용");
    big[k] = 'b'; big[k + 1] = 0;
    CHECK(dns_build_query(b, sizeof b, 1, big) == DNS_ERR_NAME, "이름 256바이트 거절");
    CHECK(dns_build_query(b, sizeof b, 1, "-.") > 0, "'-' 하나짜리 레이블은 허용(호스트 이름 규칙 검사는 하지 않음 — 설계안)");
    CHECK(dns_build_query(b, sizeof b, 1, "") == DNS_ERR_NAME && dns_build_query(b, sizeof b, 1, ".") == DNS_ERR_NAME && dns_build_query(b, sizeof b, 1, "a..b") == DNS_ERR_NAME && dns_build_query(b, sizeof b, 1, ".a") == DNS_ERR_NAME, "빈 이름·점만·빈 레이블");
    CHECK(dns_build_query(b, sizeof b, 1, "a b") == DNS_ERR_NAME && dns_build_query(b, sizeof b, 1, "a/b") == DNS_ERR_NAME && dns_build_query(b, sizeof b, 1, "ex\x80.com") == DNS_ERR_NAME && dns_build_query(b, sizeof b, 1, "a.\n.b") == DNS_ERR_NAME, "공백·슬래시·비ASCII·개행");
    CHECK(dns_build_query(b, sizeof b, 1, "a.b..") == DNS_ERR_NAME && dns_build_query(b, sizeof b, 1, "a.") == 12 + 3 + 4, "끝 점 둘은 거절, 하나는 허용");
    CHECK(dns_build_query(b, sizeof b, 1, 0) == DNS_ERR_NAME && dns_build_query(0, 30, 1, "a") == QSPI_ERR_ARGS, "NULL");
}

static void t_ipv4(void)
{
    printf("[IPv4 문자열]\n");
    uint8_t ip[4] = { 9, 9, 9, 9 };
    CHECK(dns_parse_ipv4("192.168.0.1", ip) == 1 && ip[0] == 192 && ip[1] == 168 && ip[2] == 0 && ip[3] == 1, "192.168.0.1");
    CHECK(dns_parse_ipv4("0.0.0.0", ip) == 1 && ip[0] == 0 && ip[3] == 0, "0.0.0.0");
    CHECK(dns_parse_ipv4("255.255.255.255", ip) == 1 && ip[0] == 255 && ip[3] == 255, "255.255.255.255");
    CHECK(dns_parse_ipv4("1.2.3.4", ip) == 1 && ip[0] == 1 && ip[1] == 2 && ip[2] == 3 && ip[3] == 4, "각 칸 구분");
    memset(ip, 7, 4);
    const char *no[] = { "", "1.2.3", "1.2.3.4.5", "256.1.1.1", "1.256.1.1", "1.1.1.256", "1.2.3.4 ", " 1.2.3.4", "a.b.c.d", "1..2.3", "01.2.3.4", "1.2.3.04", "1.2.3.", ".1.2.3", "1000.1.1.1", "1.2.3.4x", "example.com", "1.2.3.-4" };
    int ok = 1; for (unsigned i = 0; i < sizeof no / sizeof no[0]; i++) if (dns_parse_ipv4(no[i], ip) != 0) { ok = 0; printf("    (거절되어야 함: \"%s\")\n", no[i]); }
    CHECK(ok, "IPv4 가 아닌 문자열 18개 거절");
    CHECK(ip[0] == 7 && ip[3] == 7, "거절하면 ip 를 건드리지 않음");
    CHECK(dns_parse_ipv4("10.0.0.1", ip) == 1 && dns_parse_ipv4(0, ip) == 0 && dns_parse_ipv4("1.2.3.4", 0) == 0, "NULL");
    CHECK(dns_parse_ipv4("0.10.100.200", ip) == 1 && ip[1] == 10 && ip[2] == 100 && ip[3] == 200, "0, 10, 100, 200");
}

static void t_parse(void)
{
    printf("[응답 해석]\n");
    uint8_t ip[4]; uint32_t ttl; msg_t m;
    /* 기본: A 레코드, 질문 이름을 포인터(0xC00C)로 가리킴 */
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1); rr_a(&m, 12, 300, R1);
    memset(ip, 0, 4); ttl = 0;
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == 0 && memcmp(ip, R1, 4) == 0 && ttl == 300, "A 레코드, TTL 300");
    CHECK(parse(&m, 0xABCD, "WWW.Example.COM.", ip, &ttl) == 0, "질의 이름 대소문자·끝 점 무시");
    CHECK(dns_parse_response(m.b, m.n, 0xABCD, "www.example.com", ip, 0) == 0, "ttl NULL 허용");
    ttl = 0xFFFFFFFFu; m = std_reply(0xABCD, 0x8180, "www.example.com", 1); rr_a(&m, 12, 0x01020304u, R1);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == 0 && ttl == 0x01020304u, "TTL 4바이트 빅엔디안");
    CHECK(parse(&m, 0xABCE, "www.example.com", ip, &ttl) == DNS_ERR_ID, "ID 불일치");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1); rr_a(&m, 12, 300, R1);
    CHECK(parse(&m, 0xABCD, "www.example.org", ip, &ttl) == DNS_ERR_QUESTION, "질문 이름 불일치");
    m = std_reply(0xABCD, 0x0100, "www.example.com", 1); rr_a(&m, 12, 300, R1);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "QR=0(질의)");
    m = std_reply(0xABCD, 0x8980, "www.example.com", 1); rr_a(&m, 12, 300, R1);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "OPCODE=1");
    m = std_reply(0xABCD, 0xF980, "www.example.com", 1); rr_a(&m, 12, 300, R1);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "OPCODE=15");
    m = std_reply(0xABCD, 0x8580, "www.example.com", 1); rr_a(&m, 12, 300, R1);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == 0, "AA 비트는 무관");
    m = std_reply(0xABCD, 0x8190, "www.example.com", 0);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_NOANSWER, "RCODE 는 하위 4비트만 본다(0x8190 → RCODE 0, 답변 없음)");
    for (unsigned rc = 1; rc <= 5; rc++) {
        m = std_reply(0xABCD, 0x8180 | rc, "www.example.com", 0);
        int exp = rc == 2 ? DNS_ERR_SERVFAIL : rc == 3 ? DNS_ERR_NXDOMAIN : DNS_ERR_RCODE;
        int r = parse(&m, 0xABCD, "www.example.com", ip, &ttl);
        CHECK(r == exp, "RCODE → 오류 코드");
        if (r != exp) printf("    (rcode %u: %d)\n", rc, r);
    }
    m = std_reply(0xABCD, 0x8183, "www.example.com", 0); m.n = 12;           /* NXDOMAIN 인데 질문을 안 돌려줌 */
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_NXDOMAIN, "NXDOMAIN 은 질문 없이도 인식");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 0);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_NOANSWER, "답변 0개 → NOANSWER");
    m = std_reply(0xABCD, 0x8380, "www.example.com", 0);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_TRUNC, "TC + 답변 없음 → TRUNC");
    m = std_reply(0xABCD, 0x8380, "www.example.com", 1); rr_a(&m, 12, 300, R1);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == 0 && memcmp(ip, R1, 4) == 0, "TC 이지만 A 가 있으면 사용");
    m = std_reply(0xABCD, 0x8380, "www.example.com", 2); rr_a(&m, 12, 300, R1); rr_a(&m, 12, 300, R2); m.n -= 2;
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == 0, "TC, 첫 A 는 완전하고 뒤가 잘림 → 첫 A 사용");
    m = std_reply(0xABCD, 0x8380, "www.example.com", 1); rr_ptr(&m, 12, 1, 1, 1, 4); u8(&m, 1);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_TRUNC, "TC, 레코드가 잘림 → TRUNC");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1); rr_ptr(&m, 12, 1, 1, 1, 4); u8(&m, 1);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "TC 없이 잘림 → FORMAT");

    /* 답변 여러 개: 첫 번째 사용, 다른 이름·AAAA·다른 CLASS 건너뜀 */
    m = std_reply(0xABCD, 0x8180, "www.example.com", 4);
    rr_ptr(&m, 12, 28, 1, 60, 16); for (int i = 0; i < 16; i++) u8(&m, 0xAA);          /* AAAA 건너뜀 */
    rr_ptr(&m, 12, 1, 3, 60, 4); for (int i = 0; i < 4; i++) u8(&m, 0xBB);             /* CLASS CH 건너뜀 */
    u16(&m, 0xC000 | 16); u16(&m, 1); u16(&m, 1); u32(&m, 60); u16(&m, 4); for (int i = 0; i < 4; i++) u8(&m, 0xCC);   /* 소유자 이름이 "example.com"(질문 이름의 16번째부터) → 건너뜀 */
    rr_a(&m, 12, 77, R2);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == 0 && memcmp(ip, R2, 4) == 0 && ttl == 77, "AAAA·다른 CLASS·다른 소유자 이름은 건너뛰고 맞는 A 선택");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 2); rr_a(&m, 12, 10, R1); rr_a(&m, 12, 20, R2);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == 0 && memcmp(ip, R1, 4) == 0 && ttl == 10, "A 가 여럿이면 첫 번째");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1); rr_ptr(&m, 12, 28, 1, 60, 16); for (int i = 0; i < 16; i++) u8(&m, 0xAA);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_NOANSWER, "AAAA 만 있으면 NOANSWER");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1); rr_ptr(&m, 12, 1, 1, 60, 5); for (int i = 0; i < 5; i++) u8(&m, 1);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "A 의 RDLENGTH ≠ 4");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1); rr_ptr(&m, 12, 1, 1, 60, 3); for (int i = 0; i < 3; i++) u8(&m, 1);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "A 의 RDLENGTH 3");

    /* 압축 없는 이름과 압축된 이름 모두 */
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1); nm(&m, "www.example.com"); u16(&m, 1); u16(&m, 1); u32(&m, 5); u16(&m, 4); for (int i = 0; i < 4; i++) u8(&m, R1[i]);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == 0 && memcmp(ip, R1, 4) == 0, "압축 없는 소유자 이름");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1); u8(&m, 3); u8(&m, 'w'); u8(&m, 'w'); u8(&m, 'w'); u16(&m, 0xC000 | 16);   /* "www" + 포인터("example.com") */
    u16(&m, 1); u16(&m, 1); u32(&m, 5); u16(&m, 4); for (int i = 0; i < 4; i++) u8(&m, R1[i]);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == 0, "레이블 + 포인터 섞인 소유자 이름");

    /* CNAME */
    m = std_reply(0xABCD, 0x8180, "www.example.com", 2);
    { unsigned cn_off; rr_ptr(&m, 12, 5, 1, 120, 0); unsigned rdl_pos = m.n - 2; unsigned s = m.n; cn_off = m.n; nm(&m, "edge.cdn.net"); m.b[rdl_pos] = 0; m.b[rdl_pos + 1] = (uint8_t)(m.n - s);
      rr_a(&m, cn_off, 33, R2); }
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == 0 && memcmp(ip, R2, 4) == 0 && ttl == 33, "CNAME 뒤 A (A 의 소유자 이름은 CNAME 대상을 가리키는 포인터)");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 2);
    { rr_ptr(&m, 12, 5, 1, 120, 0); unsigned rdl_pos = m.n - 2; unsigned s = m.n; nm(&m, "edge.cdn.net"); m.b[rdl_pos + 1] = (uint8_t)(m.n - s);
      rr_a(&m, 12, 33, R2); }
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_NOANSWER, "CNAME 대상과 다른 이름(원래 이름)의 A 는 건너뜀");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 3);
    { unsigned a1, a2;
      rr_ptr(&m, 12, 5, 1, 1, 0); unsigned p1 = m.n - 2, s1 = m.n; a1 = m.n; nm(&m, "b.example.com"); m.b[p1 + 1] = (uint8_t)(m.n - s1);
      rr_ptr(&m, a1, 5, 1, 1, 0); unsigned p2 = m.n - 2, s2 = m.n; a2 = m.n; nm(&m, "c.example.com"); m.b[p2 + 1] = (uint8_t)(m.n - s2);
      rr_a(&m, a2, 9, R2); }
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == 0 && memcmp(ip, R2, 4) == 0, "CNAME 두 번");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 2);
    { rr_ptr(&m, 12, 5, 1, 1, 0); unsigned p1 = m.n - 2, s1 = m.n; unsigned a1 = m.n; nm(&m, "b.example.com"); m.b[p1 + 1] = (uint8_t)(m.n - s1);
      rr_ptr(&m, a1, 5, 1, 1, 0); unsigned p2 = m.n - 2, s2 = m.n; nm(&m, "www.example.com"); m.b[p2 + 1] = (uint8_t)(m.n - s2); }
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_NOANSWER, "CNAME 순환(www→b→www) → A 없음");
    {   /* 9번 CNAME 을 따라가면 거절(최대 8) */
        for (unsigned hops = 8; hops <= 9; hops++) {
            m = std_reply(0xABCD, 0x8180, "n0.example.com", hops + 1);
            unsigned prev = 12;
            for (unsigned i = 1; i <= hops; i++) {
                char t[24]; sprintf(t, "n%u.example.com", i);
                rr_ptr(&m, prev, 5, 1, 1, 0); unsigned p = m.n - 2, s = m.n; prev = m.n; nm(&m, t); m.b[p + 1] = (uint8_t)(m.n - s);
            }
            rr_a(&m, prev, 4, R2);
            int r = parse(&m, 0xABCD, "n0.example.com", ip, &ttl);
            CHECK(hops == 8 ? r == 0 : r == DNS_ERR_NOANSWER, hops == 8 ? "CNAME 8번은 따라감" : "CNAME 9번은 거절");
        }
    }
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1);
    { rr_ptr(&m, 12, 5, 1, 1, 4); u8(&m, 1); u8(&m, 'a'); u8(&m, 0); u8(&m, 0); }
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "CNAME RDLENGTH 가 이름 길이와 다름");

    /* 악성·깨진 입력 */
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1); u16(&m, 0xC000 | (m.n)); u16(&m, 1); u16(&m, 1); u32(&m, 1); u16(&m, 4); for (int i = 0; i < 4; i++) u8(&m, 1);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "자기 자신을 가리키는 포인터(무한 루프)");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1); { unsigned at = m.n; u16(&m, 0xC000 | (at + 2)); u16(&m, 0xC000 | at); u16(&m, 1); u16(&m, 1); u32(&m, 1); u16(&m, 4); for (int i = 0; i < 4; i++) u8(&m, 1); }
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "두 포인터가 서로 가리킴");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1); u16(&m, 0xC000 | 0x3FFF); u16(&m, 1); u16(&m, 1); u32(&m, 1); u16(&m, 4); for (int i = 0; i < 4; i++) u8(&m, 1);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "메시지 밖을 가리키는 포인터");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1); u8(&m, 0x40); u8(&m, 'a'); u8(&m, 0); u16(&m, 1); u16(&m, 1); u32(&m, 1); u16(&m, 4); for (int i = 0; i < 4; i++) u8(&m, 1);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "예약된 레이블 형식(01)");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1); u8(&m, 0x80); u8(&m, 'a'); u8(&m, 0); u16(&m, 1); u16(&m, 1); u32(&m, 1); u16(&m, 4); for (int i = 0; i < 4; i++) u8(&m, 1);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "예약된 레이블 형식(10)");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1); u8(&m, 20); u8(&m, 'a');
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "레이블이 메시지 끝을 넘음");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1); u8(&m, 0xC0);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "포인터 둘째 바이트가 없음");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1); rr_ptr(&m, 12, 1, 1, 1, 4); u8(&m, 1); u8(&m, 1); u8(&m, 1);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "RDATA 가 메시지 끝을 넘음");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1); m.n = 20;
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "질문이 도중에 끊김");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 1); m.n -= 2;
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "질문의 CLASS 가 없음");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 3); rr_a(&m, 12, 1, R1);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == 0, "ANCOUNT 가 실제보다 커도 앞에서 A 를 찾으면 성공");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 3); rr_ptr(&m, 12, 28, 1, 1, 0);
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "ANCOUNT 가 실제보다 크고 A 도 없음 → FORMAT");
    m.n = 11;
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "헤더보다 짧음");
    m.n = 0;
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "빈 메시지");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 0); m.b[5] = 2;
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "질문이 2개");
    m.b[5] = 0;
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_FORMAT, "질문이 0개(RCODE 0)");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 0); m.b[m.n - 3] = 28;
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_QUESTION, "질문 TYPE 이 AAAA");
    m = std_reply(0xABCD, 0x8180, "www.example.com", 0); m.b[m.n - 1] = 3;
    CHECK(parse(&m, 0xABCD, "www.example.com", ip, &ttl) == DNS_ERR_QUESTION, "질문 CLASS 가 CH");
    CHECK(dns_parse_response(0, 5, 1, "a", ip, &ttl) == QSPI_ERR_ARGS && dns_parse_response(m.b, m.n, 1, "a", 0, &ttl) == QSPI_ERR_ARGS && dns_parse_response(m.b, m.n, 1, "a b", ip, &ttl) == DNS_ERR_NAME, "NULL, 나쁜 이름");

    /* 포인터가 뒤쪽(앞으로) 가리키는 정상 압축 */
    m = std_reply(0xABCD, 0x8180, "a.example.com", 1);
    { unsigned lab = m.n; (void)lab; u8(&m, 1); u8(&m, 'a'); u8(&m, 0xC0); u8(&m, 14); u16(&m, 1); u16(&m, 1); u32(&m, 2); u16(&m, 4); for (int i = 0; i < 4; i++) u8(&m, R2[i]); }
    CHECK(parse(&m, 0xABCD, "a.example.com", ip, &ttl) == 0 && memcmp(ip, R2, 4) == 0, "질문의 두 번째 레이블부터를 가리키는 포인터(a + example.com)");
}

/* ---------- W5500 모델 위 ---------- */
static const uint8_t DNSIP[4] = { 192, 0, 2, 53 }, OTHER[4] = { 192, 0, 2, 99 }, NTPIP[4] = { 203, 0, 113, 5 };
enum { D_OK, D_NONE, D_SECOND, D_BAD_ID, D_OTHER_IP, D_OTHER_PORT, D_JUNK_THEN_OK, D_NXDOMAIN, D_SERVFAIL, D_NOANSWER, D_WRONG_Q, D_TRUNC, D_CNAME, D_BADFORM_THEN_OK };
static int dmode, dsends, wire_hits_http; static uint8_t answer_ip[4];
static char hostseen[64];

static void reply_dns(int sock, const uint8_t *q, unsigned qlen, int mode)
{
    unsigned id = (unsigned)(q[0] << 8 | q[1]);
    msg_t m;
    msg_t qq; memcpy(qq.b, q, qlen); qq.n = qlen;
    switch (mode) {
    case D_OK: case D_SECOND: m.n = 0; hdr(&m, id, 0x8180, 1, 1); memcpy(m.b + m.n, q + 12, qlen - 12); m.n += qlen - 12; rr_a(&m, 12, 300, answer_ip); inject_udp(sock, DNSIP, 53, m.b, m.n); break;
    case D_BAD_ID: hdr(&m, id ^ 1, 0x8180, 1, 1); memcpy(m.b + m.n, q + 12, qlen - 12); m.n += qlen - 12; rr_a(&m, 12, 300, answer_ip); inject_udp(sock, DNSIP, 53, m.b, m.n); break;
    case D_OTHER_IP: hdr(&m, id, 0x8180, 1, 1); memcpy(m.b + m.n, q + 12, qlen - 12); m.n += qlen - 12; rr_a(&m, 12, 300, answer_ip); inject_udp(sock, OTHER, 53, m.b, m.n); break;
    case D_OTHER_PORT: hdr(&m, id, 0x8180, 1, 1); memcpy(m.b + m.n, q + 12, qlen - 12); m.n += qlen - 12; rr_a(&m, 12, 300, answer_ip); inject_udp(sock, DNSIP, 5353, m.b, m.n); break;
    case D_JUNK_THEN_OK: { uint8_t j[5] = { 1, 2, 3, 4, 5 }; inject_udp(sock, DNSIP, 53, j, 5); hdr(&m, id, 0x8180, 1, 1); memcpy(m.b + m.n, q + 12, qlen - 12); m.n += qlen - 12; rr_a(&m, 12, 300, answer_ip); inject_udp(sock, DNSIP, 53, m.b, m.n); break; }
    case D_BADFORM_THEN_OK: { msg_t b; hdr(&b, id, 0x8180, 1, 1); memcpy(b.b + b.n, q + 12, qlen - 12); b.n += qlen - 12; u8(&b, 0x40); inject_udp(sock, DNSIP, 53, b.b, b.n);
        hdr(&m, id, 0x8180, 1, 1); memcpy(m.b + m.n, q + 12, qlen - 12); m.n += qlen - 12; rr_a(&m, 12, 300, answer_ip); inject_udp(sock, DNSIP, 53, m.b, m.n); break; }
    case D_NXDOMAIN: hdr(&m, id, 0x8183, 1, 0); memcpy(m.b + m.n, q + 12, qlen - 12); m.n += qlen - 12; inject_udp(sock, DNSIP, 53, m.b, m.n); break;
    case D_SERVFAIL: hdr(&m, id, 0x8182, 1, 0); memcpy(m.b + m.n, q + 12, qlen - 12); m.n += qlen - 12; inject_udp(sock, DNSIP, 53, m.b, m.n); break;
    case D_NOANSWER: hdr(&m, id, 0x8180, 1, 0); memcpy(m.b + m.n, q + 12, qlen - 12); m.n += qlen - 12; inject_udp(sock, DNSIP, 53, m.b, m.n); break;
    case D_TRUNC: hdr(&m, id, 0x8380, 1, 0); memcpy(m.b + m.n, q + 12, qlen - 12); m.n += qlen - 12; inject_udp(sock, DNSIP, 53, m.b, m.n); break;
    case D_WRONG_Q: hdr(&m, id, 0x8180, 1, 1); question(&m, "other.example", 1, 1); rr_a(&m, 12, 300, answer_ip); inject_udp(sock, DNSIP, 53, m.b, m.n); break;
    case D_CNAME: { hdr(&m, id, 0x8180, 1, 2); memcpy(m.b + m.n, q + 12, qlen - 12); m.n += qlen - 12;
        rr_ptr(&m, 12, 5, 1, 60, 0); unsigned p = m.n - 2, s = m.n, off = m.n; nm(&m, "real.example.net"); m.b[p + 1] = (uint8_t)(m.n - s); rr_a(&m, off, 300, answer_ip); inject_udp(sock, DNSIP, 53, m.b, m.n); break; }
    }
    (void)qq;
}
static void mkntp(uint8_t *r, const uint8_t *req)
{
    memset(r, 0, 48); r[0] = 0x24; r[1] = 2; memcpy(r + 24, req + 40, 8);
    r[40] = 0xE9; r[41] = 0x3C; r[42] = 0x7F; r[43] = 0x00;   /* 3913056000 = 0xE93C7F00 (2024-01-01) */
}
static void thook(int sock, int proto, const uint8_t ip[4], uint16_t port, const uint8_t *d, unsigned len)
{
    if (proto == 2 && port == 53 && memcmp(ip, DNSIP, 4) == 0 && len >= 17) {
        dsends++;
        int mode = dmode;
        if (dmode == D_SECOND && dsends < 2) return;
        if (dmode == D_NONE) return;
        reply_dns(sock, d, len, mode);
        return;
    }
    if (proto == 2 && port == 123 && len == 48) {
        uint8_t r[48]; mkntp(r, d); if (memcmp(ip, answer_ip, 4) == 0) inject_udp(sock, ip, 123, r, 48); return;
    }
    if (proto == 1 && (port == 80 || port == 8080 || port == 65535)) {
        wire_hits_http++;
        unsigned c = len < sizeof hostseen - 1 ? len : (unsigned)sizeof hostseen - 1; memcpy(hostseen, d, c); hostseen[c] = 0;
        if (memcmp(ip, answer_ip, 4) == 0) { static const char resp[] = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello"; inject_rx(sock, (const uint8_t *)resp, sizeof resp - 1); }
    }
}
static int sink(void *c, const uint8_t *d, unsigned n) { char *p = c; memcpy(p + strlen(p), d, n); p[strlen(p) + n] = 0; return 0; }
static void setup(int mode) { nwire = 0; dmode = mode; dsends = 0; wire_hits_http = 0; peer_hook = thook; memcpy(answer_ip, R1, 4); }

static void t_resolve(void)
{
    printf("[dns_resolve]\n");
    CHECK(start(1) == 0, "init");
    uint8_t ip[4]; uint32_t ttl = 0; uint64_t t0;
    setup(D_OK);
    CHECK(dns_resolve(&W, 1, 5000, DNSIP, "www.example.com", 0x1234, 100000, 2, ip, &ttl) == 0 && memcmp(ip, R1, 4) == 0 && ttl == 300, "정상 질의");
    CHECK(nwire == 1 && wire[0].proto == 2 && wire[0].port == 53 && memcmp(wire[0].ip, DNSIP, 4) == 0 && wire[0].len == 12 + 17 + 4 && wire[0].d[0] == 0x12 && wire[0].d[1] == 0x34 && wire[0].d[2] == 0x01 && wire[0].d[3] == 0x00 && wire[0].d[5] == 1, "질의: UDP, DNS 서버 IP, 포트 53, 33바이트, ID 0x1234, RD, QDCOUNT 1");
    CHECK(S[1][3] == 0x00 && W.proto[1] == 0 && rd16s(1, 4) == 5000, "끝나면 소켓 닫힘, 로컬 포트 5000");
    CHECK(viol == 0, "위반 없음");

    setup(D_SECOND); t0 = now_us;
    CHECK(dns_resolve(&W, 1, 5000, DNSIP, "www.example.com", 7, 50000, 3, ip, &ttl) == 0 && dsends == 2, "첫 질의에 응답이 없으면 다시 보냄(2번째에 성공)");
    CHECK(now_us - t0 >= 50000 && now_us - t0 < 110000, "첫 시도의 대기 시간만큼 기다림");
    setup(D_NONE); t0 = now_us;
    CHECK(dns_resolve(&W, 1, 5000, DNSIP, "www.example.com", 7, 20000, 3, ip, &ttl) == DNS_ERR_TIMEOUT && dsends == 3, "응답이 계속 없으면 tries 번 보내고 TIMEOUT");
    CHECK(now_us - t0 >= 60000 && now_us - t0 < 110000, "대기 시간 = 시도 횟수 × 제한");
    CHECK(S[1][3] == 0x00 && W.proto[1] == 0, "닫힘");
    setup(D_NONE);
    CHECK(dns_resolve(&W, 1, 5000, DNSIP, "www.example.com", 7, 20000, 1, ip, &ttl) == DNS_ERR_TIMEOUT && dsends == 1, "tries 1");

    const struct { int m; const char *msg; } ign[] = { { D_BAD_ID, "ID 가 다른 응답은 무시" }, { D_OTHER_IP, "다른 IP 의 응답은 무시" }, { D_OTHER_PORT, "다른 포트의 응답은 무시" }, { D_WRONG_Q, "질문이 다른 응답은 무시" } };
    for (unsigned i = 0; i < sizeof ign / sizeof ign[0]; i++) {
        setup(ign[i].m);
        CHECK(dns_resolve(&W, 1, 5000, DNSIP, "www.example.com", 7, 20000, 1, ip, &ttl) == DNS_ERR_TIMEOUT, ign[i].msg);
        CHECK(S[1][3] == 0 && W.proto[1] == 0, "닫힘");
    }
    setup(D_JUNK_THEN_OK); t0 = now_us;
    CHECK(dns_resolve(&W, 1, 5000, DNSIP, "www.example.com", 7, 20000, 1, ip, &ttl) == 0 && now_us - t0 < 20000, "쓰레기 패킷을 버리고 그 뒤의 정상 응답을 받음(시간을 쓰지 않음)");
    setup(D_BADFORM_THEN_OK);
    CHECK(dns_resolve(&W, 1, 5000, DNSIP, "www.example.com", 7, 20000, 1, ip, &ttl) == 0, "형식이 깨진 응답을 버리고 정상 응답을 받음");
    const struct { int m; int e; const char *msg; } fin[] = { { D_NXDOMAIN, DNS_ERR_NXDOMAIN, "NXDOMAIN 은 바로 반환(재시도 없음)" }, { D_SERVFAIL, DNS_ERR_SERVFAIL, "SERVFAIL 반환" },
        { D_NOANSWER, DNS_ERR_NOANSWER, "A 없음 반환" }, { D_TRUNC, DNS_ERR_TRUNC, "TC 반환" } };
    for (unsigned i = 0; i < sizeof fin / sizeof fin[0]; i++) {
        setup(fin[i].m); t0 = now_us;
        CHECK(dns_resolve(&W, 1, 5000, DNSIP, "www.example.com", 7, 50000, 3, ip, &ttl) == fin[i].e && dsends == 1 && now_us - t0 < 50000, fin[i].msg);
        CHECK(S[1][3] == 0 && W.proto[1] == 0, "닫힘");
    }
    setup(D_CNAME);
    CHECK(dns_resolve(&W, 1, 5000, DNSIP, "www.example.com", 7, 50000, 1, ip, &ttl) == 0 && memcmp(ip, R1, 4) == 0, "CNAME 응답");
    setup(D_OK); { int nc = n_cmds;
        CHECK(dns_resolve(&W, 1, 5000, DNSIP, "bad name", 7, 50000, 1, ip, &ttl) == DNS_ERR_NAME && n_cmds == nc, "나쁜 이름: 소켓 명령 없음");
        CHECK(dns_resolve(0, 1, 5000, DNSIP, "a.b", 7, 5, 1, ip, &ttl) == QSPI_ERR_ARGS && dns_resolve(&W, 1, 0, DNSIP, "a.b", 7, 5, 1, ip, &ttl) == QSPI_ERR_ARGS && dns_resolve(&W, 1, 5000, 0, "a.b", 7, 5, 1, ip, &ttl) == QSPI_ERR_ARGS
              && dns_resolve(&W, 1, 5000, DNSIP, "a.b", 7, 5, 0, ip, &ttl) == QSPI_ERR_ARGS && dns_resolve(&W, 1, 5000, DNSIP, "a.b", 7, 5, 1, 0, &ttl) == QSPI_ERR_ARGS, "인자 검사"); }
    peer_dead = 1; setup(D_OK); peer_dead = 1;
    CHECK(dns_resolve(&W, 1, 5000, DNSIP, "www.example.com", 7, 20000, 2, ip, &ttl) == W5500_ERR_PEER && S[1][3] == 0 && W.proto[1] == 0, "송신 실패(ARP 등) → PEER, 닫힘");
    peer_dead = 0;
    CHECK(w5500_sock_open(&W, 2, W5500_PROTO_UDP, 6000) == 0 && dns_resolve(&W, 2, 5000, DNSIP, "a.b", 7, 5000, 1, ip, &ttl) != 0 && S[2][3] == 0x22, "사용 중인 소켓은 건드리지 않음");
    CHECK(viol == 0, "위반 없음");
}

static void t_names(void)
{
    printf("[이름으로 NTP·HTTP]\n");
    CHECK(start(1) == 0, "init");
    ntp_time_t t; int st = 0; char body[64];
    setup(D_OK); memcpy(answer_ip, NTPIP, 4);
    CHECK(ntp_query_name(&W, 1, 4000, DNSIP, 0x55AA, "pool.example.org", 1, 2, 50000, &t) == 0 && t.unix_sec == 1704067200LL && dsends == 1, "이름 → DNS → NTP");
    CHECK(nwire == 2 && wire[0].port == 53 && wire[1].port == 123 && memcmp(wire[1].ip, NTPIP, 4) == 0, "와이어: DNS 질의, 그다음 해석된 IP 로 NTP 포트 123");
    setup(D_OK); memcpy(answer_ip, NTPIP, 4);
    CHECK(ntp_query_name(&W, 1, 4000, DNSIP, 0x55AA, "203.0.113.5", 1, 2, 50000, &t) == 0 && dsends == 0 && nwire == 1 && wire[0].port == 123, "IPv4 문자열이면 DNS 를 건너뜀");
    CHECK(ntp_query_name(&W, 1, 4000, 0, 0x55AA, "203.0.113.5", 1, 2, 50000, &t) == 0, "IPv4 문자열이면 DNS 서버 없어도 됨");
    CHECK(ntp_query_name(&W, 1, 4000, 0, 0x55AA, "pool.example.org", 1, 2, 50000, &t) == QSPI_ERR_ARGS, "이름인데 DNS 서버 없음");
    setup(D_NXDOMAIN);
    CHECK(ntp_query_name(&W, 1, 4000, DNSIP, 0x55AA, "nope.example", 1, 2, 50000, &t) == DNS_ERR_NXDOMAIN && nwire == 1, "NXDOMAIN 이면 NTP 를 보내지 않음");
    CHECK(ntp_query_name(0, 1, 4000, DNSIP, 1, "a", 1, 2, 5, &t) == QSPI_ERR_ARGS && ntp_query_name(&W, 1, 4000, DNSIP, 1, 0, 1, 2, 5, &t) == QSPI_ERR_ARGS, "인자 검사");

    setup(D_OK); memcpy(answer_ip, R1, 4); body[0] = 0; hostseen[0] = 0;
    CHECK(http_get_name(&W, 1, 3000, DNSIP, 0x0102, 80, "www.example.com", "/x", 50000, sink, body, &st) == 0 && st == 200 && strcmp(body, "hello") == 0, "이름 → DNS → HTTP");
    CHECK(strcmp(hostseen, "GET /x HTTP/1.1\r\nHost: www.example.com\r\nConnection: close\r\n\r\n") == 0, "Host 헤더는 이름 그대로(포트 80 이면 포트 생략)");
    CHECK(nwire == 2 && wire[0].port == 53 && wire[1].port == 80 && memcmp(wire[1].ip, R1, 4) == 0, "와이어: DNS, 그다음 해석된 IP 의 포트 80");
    setup(D_OK); memcpy(answer_ip, R1, 4); body[0] = 0; hostseen[0] = 0;
    CHECK(http_get_name(&W, 1, 3000, DNSIP, 0x0102, 8080, "www.example.com", "/", 50000, sink, body, &st) == 0 && st == 200 && strstr(hostseen, "\r\nHost: www.example.com:8080\r\n") != 0 && wire[1].port == 8080, "포트 8080 이면 Host 헤더에 :8080");
    setup(D_OK); memcpy(answer_ip, R1, 4); body[0] = 0; hostseen[0] = 0;
    CHECK(http_get_name(&W, 1, 3000, DNSIP, 0x0102, 65535, "a.b", "/", 50000, sink, body, &st) == 0 && strstr(hostseen, "\r\nHost: a.b:65535\r\n") != 0, "포트 65535 의 자릿수");
    setup(D_OK); memcpy(answer_ip, R1, 4); body[0] = 0; hostseen[0] = 0;
    CHECK(http_get_name(&W, 1, 3000, DNSIP, 0x0102, 1, "a.b", "/", 3000, sink, body, &st) != 0 && wire[1].port == 1, "포트 1 로 접속 시도");
    setup(D_OK); memcpy(answer_ip, R1, 4); body[0] = 0; hostseen[0] = 0;
    CHECK(http_get_name(&W, 1, 3000, 0, 0, 80, "198.51.100.77", "/", 50000, sink, body, &st) == 0 && st == 200 && dsends == 0, "IPv4 문자열 호스트는 DNS 없이");
    setup(D_NXDOMAIN); st = 5;
    CHECK(http_get_name(&W, 1, 3000, DNSIP, 1, 80, "nope.example", "/", 50000, sink, body, &st) == DNS_ERR_NXDOMAIN && st == 0 && wire_hits_http == 0, "NXDOMAIN → status 0, 접속 안 함");
    CHECK(http_get_name(&W, 1, 3000, DNSIP, 1, 80, "", "/", 5, sink, body, &st) == HTTP_ERR_ARG && http_get_name(0, 1, 3000, DNSIP, 1, 80, "a", "/", 5, sink, body, &st) == QSPI_ERR_ARGS && http_get_name(&W, 1, 3000, DNSIP, 1, 80, "a", "/", 5, sink, body, 0) == QSPI_ERR_ARGS, "인자 검사");
    CHECK(viol == 0, "위반 없음");
}

int main(void)
{
    t_build(); t_ipv4(); t_parse(); t_resolve(); t_names();
    if (fails) { printf("FAIL: %d\n", fails); return 1; }
    printf("ALL PASS (dns)\n");
    return 0;
}
