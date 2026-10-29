/* SNTP 시험: 순수 패킷 함수(손으로 계산한 값) + W5500 모델 위의 질의(상대 서버 응답은 이 파일의 훅이 만든다).
 * 한계: 응답 패킷은 내가 만든 것이지 실제 NTP 서버 캡처가 아니다. RFC 5905 원문을 열지 못해 필드 규칙은 D3(OpenBSD·chrony·RIOT) 기준. */
#include "../../spi/tb/w5500_model.h"
#include "../sw/ntp.h"

static const uint8_t SRV[4] = { 203, 0, 113, 5 }, OTHER[4] = { 203, 0, 113, 99 };
static int hook_mode; static int hook_calls;
enum { H_NORMAL, H_NONE, H_BAD_ORIGIN, H_OTHER_IP, H_OTHER_PORT, H_JUNK_THEN_OK, H_SHORT_THEN_OK, H_KOD, H_LONG, H_ALARM, H_CLIENT_MODE, H_TX0 };
static uint32_t rep_sec = 3913056000u, rep_frac = 0x80000000u;

static void mkrep(uint8_t *r, const uint8_t *req, uint8_t b0, uint8_t stratum)
{
    memset(r, 0, 48);
    r[0] = b0; r[1] = stratum; r[2] = 6; r[3] = 0xEC;
    memcpy(r + 24, req + 40, 8);                       /* 원점 = 요청의 전송 시각 */
    r[32] = 0xE9; r[36] = 0x11;                        /* 수신 시각(의미 없는 값) */
    r[40] = (uint8_t)(rep_sec >> 24); r[41] = (uint8_t)(rep_sec >> 16); r[42] = (uint8_t)(rep_sec >> 8); r[43] = (uint8_t)rep_sec;
    r[44] = (uint8_t)(rep_frac >> 24); r[45] = (uint8_t)(rep_frac >> 16); r[46] = (uint8_t)(rep_frac >> 8); r[47] = (uint8_t)rep_frac;
}
static void hook(int sock, int proto, const uint8_t ip[4], uint16_t port, const uint8_t *d, unsigned len)
{
    hook_calls++;
    if (proto != 2 || port != 123 || len != 48 || memcmp(ip, SRV, 4) != 0) return;
    uint8_t r[80]; unsigned n = 48;
    switch (hook_mode) {
    case H_NONE: return;
    case H_NORMAL: mkrep(r, d, 0x24, 2); break;
    case H_BAD_ORIGIN: mkrep(r, d, 0x24, 2); r[31] ^= 1; break;
    case H_OTHER_IP: mkrep(r, d, 0x24, 2); inject_udp(sock, OTHER, 123, r, 48); return;
    case H_OTHER_PORT: mkrep(r, d, 0x24, 2); inject_udp(sock, SRV, 124, r, 48); return;
    case H_JUNK_THEN_OK: { uint8_t j[48]; mkrep(j, d, 0x24, 2); inject_udp(sock, OTHER, 123, j, 48); mkrep(r, d, 0x24, 3); break; }
    case H_SHORT_THEN_OK: { uint8_t j[20] = { 0x24 }; inject_udp(sock, SRV, 123, j, 20); mkrep(r, d, 0x24, 3); break; }
    case H_KOD: mkrep(r, d, 0x24, 0); break;
    case H_LONG: mkrep(r, d, 0x24, 2); memset(r + 48, 0xAA, 20); n = 68; break;
    case H_ALARM: mkrep(r, d, 0xE4, 2); break;
    case H_CLIENT_MODE: mkrep(r, d, 0x23, 2); break;
    case H_TX0: mkrep(r, d, 0x24, 2); memset(r + 40, 0, 8); break;
    }
    inject_udp(sock, SRV, 123, r, n);
}

static void t_pure(void)
{
    printf("[순수 함수]\n");
    uint8_t p[48]; memset(p, 0x77, sizeof p);
    CHECK(ntp_build_request(p, 0x01020304u, 0x05060708u) == 0, "요청 만들기");
    CHECK(p[0] == 0x23, "첫 바이트 = LI 0, VN 4, 모드 3 = 0x23");
    int z = 1; for (int i = 1; i < 40; i++) if (p[i]) z = 0;
    CHECK(z, "1~39 번째는 0");
    CHECK(p[40] == 1 && p[41] == 2 && p[42] == 3 && p[43] == 4 && p[44] == 5 && p[45] == 6 && p[46] == 7 && p[47] == 8, "전송 시각은 40 번째부터 빅엔디안");
    CHECK(ntp_build_request(0, 1, 1) == QSPI_ERR_ARGS, "NULL 거절");

    /* 모든 필드가 다른 값인 패킷: 첫 바이트 0x64 = LI 1, VN 4, 모드 4 */
    uint8_t r[48];
    for (int i = 0; i < 48; i++) r[i] = (uint8_t)(0x10 + i);
    r[0] = 0x64; r[1] = 3; r[2] = 0xFA; r[3] = 0xEC;
    ntp_pkt_t o; memset(&o, 0xCC, sizeof o);
    CHECK(ntp_parse(r, 48, &o) == 0, "해석");
    CHECK(o.li == 1 && o.vn == 4 && o.mode == 4 && o.stratum == 3 && o.poll == -6 && o.precision == -20, "LI·VN·모드·스트라툼·폴·정밀도(부호 있는 값)");
    CHECK(o.root_delay == 0x14151617u && o.root_disp == 0x18191A1Bu && o.ref_id == 0x1C1D1E1Fu, "루트 지연·분산·참조 ID (오프셋 4, 8, 12)");
    CHECK(o.ref_sec == 0x20212223u && o.ref_frac == 0x24252627u, "참조 시각 (16)");
    CHECK(o.orig_sec == 0x28292A2Bu && o.orig_frac == 0x2C2D2E2Fu, "원점 시각 (24)");
    CHECK(o.rx_sec == 0x30313233u && o.rx_frac == 0x34353637u, "수신 시각 (32)");
    CHECK(o.tx_sec == 0x38393A3Bu && o.tx_frac == 0x3C3D3E3Fu, "전송 시각 (40)");
    CHECK(ntp_parse(r, 47, &o) == NTP_ERR_LEN && ntp_parse(r, 0, &o) == NTP_ERR_LEN, "48바이트 미만 거절");
    CHECK(ntp_parse(r, 68, &o) == 0, "더 긴 패킷(확장 필드)은 앞 48바이트만 해석");
    CHECK(ntp_parse(0, 48, &o) == QSPI_ERR_ARGS && ntp_parse(r, 48, 0) == QSPI_ERR_ARGS, "NULL 거절");
    r[0] = 0xDB;   /* 11 011 011 */
    ntp_parse(r, 48, &o);
    CHECK(o.li == 3 && o.vn == 3 && o.mode == 3, "비트 경계: 0xDB → LI 3, VN 3, 모드 3");
    r[0] = 0x1C;   /* 00 011 100 */
    ntp_parse(r, 48, &o);
    CHECK(o.li == 0 && o.vn == 3 && o.mode == 4, "비트 경계: 0x1C → LI 0, VN 3, 모드 4");

    /* 응답 검사 */
    ntp_pkt_t g = { .li = 0, .vn = 4, .mode = 4, .stratum = 2, .orig_sec = 100, .orig_frac = 200, .tx_sec = 5, .tx_frac = 6 };
    CHECK(ntp_check_reply(&g, 100, 200) == 0, "정상 응답");
    ntp_pkt_t b = g; b.mode = 3;       CHECK(ntp_check_reply(&b, 100, 200) == NTP_ERR_MODE, "모드 3 거절");
    b = g; b.mode = 5;                 CHECK(ntp_check_reply(&b, 100, 200) == NTP_ERR_MODE, "모드 5 거절");
    b = g; b.mode = 0;                 CHECK(ntp_check_reply(&b, 100, 200) == NTP_ERR_MODE, "모드 0 거절");
    CHECK(ntp_check_reply(&g, 101, 200) == NTP_ERR_ORIGIN, "원점 초가 다름");
    CHECK(ntp_check_reply(&g, 100, 201) == NTP_ERR_ORIGIN, "원점 분수가 다름");
    b = g; b.stratum = 0;              CHECK(ntp_check_reply(&b, 100, 200) == NTP_ERR_STRATUM, "스트라툼 0 거절");
    b = g; b.stratum = 16;             CHECK(ntp_check_reply(&b, 100, 200) == NTP_ERR_STRATUM, "스트라툼 16 거절");
    b = g; b.stratum = 15;             CHECK(ntp_check_reply(&b, 100, 200) == 0, "스트라툼 15 허용");
    b = g; b.stratum = 1;              CHECK(ntp_check_reply(&b, 100, 200) == 0, "스트라툼 1 허용");
    b = g; b.li = 3;                   CHECK(ntp_check_reply(&b, 100, 200) == NTP_ERR_LI, "LI 3 거절");
    b = g; b.li = 1;                   CHECK(ntp_check_reply(&b, 100, 200) == 0, "LI 1 허용");
    b = g; b.li = 2;                   CHECK(ntp_check_reply(&b, 100, 200) == 0, "LI 2 허용");
    b = g; b.tx_sec = 0; b.tx_frac = 0; CHECK(ntp_check_reply(&b, 100, 200) == NTP_ERR_TX0, "전송 시각 0 거절");
    b = g; b.tx_sec = 0; b.tx_frac = 1; CHECK(ntp_check_reply(&b, 100, 200) == 0, "초만 0 이면 허용");
    b = g; b.tx_sec = 1; b.tx_frac = 0; CHECK(ntp_check_reply(&b, 100, 200) == 0, "분수만 0 이면 허용");
    CHECK(ntp_check_reply(0, 1, 1) == QSPI_ERR_ARGS, "NULL 거절");

    /* 시각 변환 — 손으로 계산: 2024-01-01 00:00:00 UTC = Unix 1704067200 = NTP 1704067200 + 2208988800 = 3913056000 */
    CHECK(ntp_to_unix(2208988800u) == 0, "1970-01-01 → 0");
    CHECK(ntp_to_unix(3913056000u) == 1704067200LL, "2024-01-01 → 1704067200");
    CHECK(ntp_to_unix(2208988801u) == 1, "1970 + 1 초");
    CHECK(ntp_to_unix(0xFFFFFFFFu) == 2085978495LL, "시대 0 의 마지막 초 (2036-02-07 06:28:15)");
    CHECK(ntp_to_unix(0) == 2085978496LL, "시대 1 의 첫 초 (2036-02-07 06:28:16)");
    CHECK(ntp_to_unix(2208988799u) == 4294967295LL, "2106-02-07 06:28:15 (32비트 Unix 의 끝)");
    CHECK(ntp_to_unix(1) == 2085978497LL, "시대 1, 1 초");
    CHECK(ntp_frac_to_usec(0) == 0 && ntp_frac_to_usec(0x80000000u) == 500000 && ntp_frac_to_usec(0x40000000u) == 250000 && ntp_frac_to_usec(0xFFFFFFFFu) == 999999, "분수 → µs: 0, 1/2, 1/4, 최댓값 999999");
    CHECK(ntp_frac_to_usec(0x00000100u) == 0 && ntp_frac_to_usec(0x000010C7u) == 1 && ntp_frac_to_usec(0x10000000u) == 62500, "분수 → µs: 작은 값(0x10C7 에서 처음 1), 1/16 = 62500");
}

static int sock_sr(int n) { return S[n][3]; }
static void t_query(void)
{
    printf("[질의]\n");
    CHECK(start(1) == 0, "init");
    peer_hook = hook; hook_mode = H_NORMAL; hook_calls = 0;
    ntp_time_t t; memset(&t, 0xCC, sizeof t);
    CHECK(ntp_query(&W, 1, 4000, SRV, 0xA1B2C3D4u, 0x0F1E2D3Cu, 100000, &t) == 0, "정상 질의");
    CHECK(t.unix_sec == 1704067200LL && t.usec == 500000 && t.stratum == 2, "서버 전송 시각이 Unix 초·µs 로 변환됨 (2024-01-01 00:00:00.5)");
    CHECK(nwire == 1 && wire[0].proto == 2 && wire[0].port == 123 && wire[0].len == 48 && memcmp(wire[0].ip, SRV, 4) == 0, "요청: UDP, 서버 IP, 포트 123, 48바이트");
    CHECK(wire[0].d[0] == 0x23 && wire[0].d[40] == 0xA1 && wire[0].d[41] == 0xB2 && wire[0].d[42] == 0xC3 && wire[0].d[43] == 0xD4 && wire[0].d[44] == 0x0F && wire[0].d[47] == 0x3C, "요청 내용: 0x23, 전송 시각 칸에 nonce");
    CHECK(rd16s(1, 4) == 4000, "로컬 포트 4000");
    CHECK(sock_sr(1) == 0x00, "끝나면 소켓이 닫힘");
    CHECK(viol == 0, "위반 없음");

    CHECK(ntp_query(&W, 1, 1, SRV, 1, 2, 100000, &t) == 0 && rd16s(1, 4) == 1, "로컬 포트 1 도 허용(0 만 거절)");
    /* 다시 쓸 수 있음 */
    CHECK(ntp_query(&W, 1, 4001, SRV, 1, 2, 100000, &t) == 0 && t.unix_sec == 1704067200LL, "두 번째 질의");

    /* 응답 없음 */
    hook_mode = H_NONE; uint64_t t0 = now_us;
    CHECK(ntp_query(&W, 1, 4000, SRV, 1, 2, 50000, &t) == NTP_ERR_TIMEOUT, "응답 없음 → TIMEOUT");
    CHECK(now_us - t0 >= 50000 && now_us - t0 < 120000, "기다린 시간이 제한 시간 근처");
    CHECK(sock_sr(1) == 0x00, "실패해도 소켓을 닫음");

    const struct { int m; const char *msg; } bad[] = {
        { H_BAD_ORIGIN, "원점이 다른 응답은 무시" }, { H_OTHER_IP, "다른 IP 의 응답은 무시" }, { H_OTHER_PORT, "다른 포트의 응답은 무시" },
        { H_KOD, "스트라툼 0 응답은 무시" }, { H_ALARM, "LI 3 응답은 무시" }, { H_CLIENT_MODE, "모드 3 응답은 무시" }, { H_TX0, "전송 시각 0 응답은 무시" } };
    for (unsigned i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        hook_mode = bad[i].m;
        CHECK(ntp_query(&W, 1, 4000, SRV, 0x11, 0x22, 30000, &t) == NTP_ERR_TIMEOUT, bad[i].msg);
        CHECK(sock_sr(1) == 0, "닫힘");
    }
    hook_mode = H_JUNK_THEN_OK; memset(&t, 0, sizeof t);
    t0 = now_us;
    CHECK(ntp_query(&W, 1, 4000, SRV, 0x11, 0x22, 30000, &t) == 0 && t.stratum == 3 && t.unix_sec == 1704067200LL, "남의 패킷 뒤에 오는 정상 응답을 받음");
    CHECK(now_us - t0 < 30000, "남의 패킷을 버리는 데 제한 시간을 쓰지 않음");
    hook_mode = H_SHORT_THEN_OK; memset(&t, 0, sizeof t);
    CHECK(ntp_query(&W, 1, 4000, SRV, 0x11, 0x22, 30000, &t) == 0 && t.stratum == 3, "짧은 패킷을 버리고 정상 응답을 받음");
    hook_mode = H_LONG; memset(&t, 0, sizeof t);
    CHECK(ntp_query(&W, 1, 4000, SRV, 0x11, 0x22, 30000, &t) == 0 && t.stratum == 2, "확장 필드가 붙은 68바이트 응답");
    CHECK(viol == 0, "위반 없음");

    /* 시대 1: 서버가 2036 이후 시각을 보냄 */
    hook_mode = H_NORMAL; rep_sec = 100; rep_frac = 0xFFFFFFFFu;
    CHECK(ntp_query(&W, 1, 4000, SRV, 1, 2, 30000, &t) == 0 && t.unix_sec == 2085978496LL + 100 && t.usec == 999999, "시대 1 시각");
    rep_sec = 3913056000u; rep_frac = 0x80000000u;

    /* 송신 실패(ARP/재전송 한도) */
    peer_dead = 1; hook_mode = H_NORMAL;
    CHECK(ntp_query(&W, 1, 4000, SRV, 1, 2, 30000, &t) == W5500_ERR_PEER, "송신 실패 → PEER");
    CHECK(sock_sr(1) == 0, "닫힘");
    peer_dead = 0;

    /* 이미 쓰는 소켓 */
    CHECK(w5500_sock_open(&W, 2, W5500_PROTO_UDP, 5000) == 0, "소켓 2 사용 중");
    CHECK(ntp_query(&W, 2, 4000, SRV, 1, 2, 30000, &t) != 0 && sock_sr(2) == 0x22, "열려 있는 소켓에는 실패하고 그 소켓을 닫지 않음");
    CHECK(ntp_query(&W, 8, 4000, SRV, 1, 2, 30000, &t) != 0, "없는 소켓 번호");

    CHECK(ntp_query(0, 1, 4000, SRV, 1, 2, 3, &t) == QSPI_ERR_ARGS && ntp_query(&W, 1, 0, SRV, 1, 2, 3, &t) == QSPI_ERR_ARGS && ntp_query(&W, 1, 4000, 0, 1, 2, 3, &t) == QSPI_ERR_ARGS && ntp_query(&W, 1, 4000, SRV, 1, 2, 3, 0) == QSPI_ERR_ARGS, "인자 검사");
    CHECK(viol == 0, "위반 없음");
}

int main(void)
{
    t_pure(); t_query();
    if (fails) { printf("FAIL: %d\n", fails); return 1; }
    printf("ALL PASS (ntp)\n");
    return 0;
}
