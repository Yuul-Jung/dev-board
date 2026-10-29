/* DHCP 시험: 요청 바이트(손으로 계산한 오프셋), 응답 해석(옵션 경계·중복·오류), 시간 기본값, W5500 모델 위 dhcp_acquire·dhcp_apply.
 * 한계: 응답은 내가 손으로 조립한 것이지 실제 서버 캡처가 아니다. RFC 2131 원문은 열지 못했고 dhcpcd 사본(D3)·WIZnet 문서(D1 요약) 기준이다.
 * W5500 이 SIPR=0.0.0.0 상태에서 255.255.255.255 로 보내고 유니캐스트/브로드캐스트 응답을 받는지는 이 모델이 검증하지 못한다(모델은 그 규칙을 내가 가정한 것). */
#include "../../spi/tb/w5500_model.h"
#include "../sw/dhcp.h"
#include <stdlib.h>

static const uint8_t SMAC[6] = { 0x00, 0x08, 0xDC, 0x01, 0x02, 0x03 };      /* w5500_model.h 의 MAC 과 같음 */
static const uint8_t OMAC[6] = { 0x00, 0x08, 0xDC, 0x01, 0x02, 0x04 };
static const uint8_t SRV[4] = { 192, 168, 7, 1 }, SRV2[4] = { 192, 168, 7, 2 }, OFFERED[4] = { 192, 168, 7, 55 }, OFFERED2[4] = { 192, 168, 7, 99 };
static const uint8_t BC[4] = { 255, 255, 255, 255 };

/* ---------- 응답 조립 ---------- */
typedef struct { uint8_t b[700]; unsigned n; } msg_t;
static void ob(msg_t *m, unsigned v) { m->b[m->n++] = (uint8_t)v; }
static void ob4(msg_t *m, unsigned code, const uint8_t v[4]) { ob(m, code); ob(m, 4); for (int i = 0; i < 4; i++) ob(m, v[i]); }
static void ou32(msg_t *m, unsigned code, uint32_t v) { ob(m, code); ob(m, 4); ob(m, v >> 24); ob(m, v >> 16); ob(m, v >> 8); ob(m, v); }
static void otype(msg_t *m, unsigned t) { ob(m, 53); ob(m, 1); ob(m, t); }
static void head(msg_t *m, uint32_t xid, const uint8_t yi[4], const uint8_t mac[6])
{
    memset(m->b, 0, 240); m->n = 240;
    m->b[0] = 2; m->b[1] = 1; m->b[2] = 6;
    m->b[4] = (uint8_t)(xid >> 24); m->b[5] = (uint8_t)(xid >> 16); m->b[6] = (uint8_t)(xid >> 8); m->b[7] = (uint8_t)xid;
    if (yi) memcpy(m->b + 16, yi, 4);
    memcpy(m->b + 28, mac, 6);
    m->b[236] = 0x63; m->b[237] = 0x82; m->b[238] = 0x53; m->b[239] = 0x63;
}
static const uint8_t MASK24[4] = { 255, 255, 255, 0 }, RT1[4] = { 192, 168, 7, 254 }, RT2[4] = { 192, 168, 7, 253 }, DNS1[4] = { 8, 8, 8, 8 }, DNS2[4] = { 8, 8, 4, 4 };
/* 표준 응답: 종류, 주소, 서버, 임대 3600, 마스크, 라우터 2개, DNS 2개 */
static msg_t full(unsigned type, uint32_t xid, const uint8_t yi[4], const uint8_t srv[4])
{
    msg_t m; head(&m, xid, yi, SMAC);
    otype(&m, type); ob4(&m, 54, srv); ou32(&m, 51, 3600); ob4(&m, 1, MASK24);
    ob(&m, 3); ob(&m, 8); for (int i = 0; i < 4; i++) ob(&m, RT1[i]); for (int i = 0; i < 4; i++) ob(&m, RT2[i]);
    ob(&m, 6); ob(&m, 8); for (int i = 0; i < 4; i++) ob(&m, DNS1[i]); for (int i = 0; i < 4; i++) ob(&m, DNS2[i]);
    ob(&m, 255);
    return m;
}
static int P(const msg_t *m, dhcp_info_t *o) { return dhcp_parse(m->b, m->n, 0x3903F326u, SMAC, o); }
#define XID 0x3903F326u

static void t_build(void)
{
    printf("[요청 만들기]\n");
    uint8_t b[400]; memset(b, 0xEE, sizeof b);
    int n = dhcp_build_discover(b, sizeof b, XID, SMAC);
    CHECK(n == 300, "DISCOVER 300바이트");
    CHECK(b[0] == 1 && b[1] == 1 && b[2] == 6 && b[3] == 0, "op=1 htype=1 hlen=6 hops=0");
    CHECK(b[4] == 0x39 && b[5] == 0x03 && b[6] == 0xF3 && b[7] == 0x26, "xid 빅엔디안");
    CHECK(b[8] == 0 && b[9] == 0 && b[10] == 0x80 && b[11] == 0x00, "secs 0, flags 0x8000(BROADCAST)");
    int z = 1; for (int i = 12; i < 28; i++) if (b[i]) z = 0;
    CHECK(z, "ciaddr·yiaddr·siaddr·giaddr 0");
    CHECK(memcmp(b + 28, SMAC, 6) == 0, "chaddr 오프셋 28");
    z = 1; for (int i = 34; i < 236; i++) if (b[i]) z = 0;
    CHECK(z, "chaddr 나머지 10 + sname 64 + file 128 = 0");
    CHECK(b[236] == 0x63 && b[237] == 0x82 && b[238] == 0x53 && b[239] == 0x63, "쿠키 오프셋 236");
    const uint8_t opt[] = { 53, 1, 1, 55, 6, 1, 3, 6, 51, 58, 59, 255 };
    CHECK(memcmp(b + 240, opt, sizeof opt) == 0, "옵션: 종류 DISCOVER, 요청 목록(마스크·라우터·DNS·임대·T1·T2), 끝");
    z = 1; for (unsigned i = 240 + sizeof opt; i < 300; i++) if (b[i]) z = 0;
    CHECK(z && b[300] == 0xEE, "나머지 0 으로 채움, 300 바이트 이후는 안 건드림");
    CHECK(dhcp_build_discover(b, 299, XID, SMAC) == DHCP_ERR_NOSPACE && dhcp_build_discover(b, 300, XID, SMAC) == 300, "버퍼 경계 300");

    n = dhcp_build_request(b, sizeof b, XID, SMAC, OFFERED, SRV);
    CHECK(n == 300 && b[0] == 1 && memcmp(b + 4, "\x39\x03\xF3\x26", 4) == 0 && b[10] == 0x80, "REQUEST 같은 머리");
    const uint8_t ropt[] = { 53, 1, 3, 50, 4, 192, 168, 7, 55, 54, 4, 192, 168, 7, 1, 55, 6, 1, 3, 6, 51, 58, 59, 255 };
    CHECK(memcmp(b + 240, ropt, sizeof ropt) == 0, "옵션: 종류 REQUEST, 요청 IP(50), 서버 ID(54), 요청 목록, 끝");
    z = 1; for (unsigned i = 240 + sizeof ropt; i < 300; i++) if (b[i]) z = 0;
    CHECK(z, "나머지 0");
    CHECK(b[12] == 0 && b[15] == 0, "ciaddr 0 (SELECTING 상태: 요청 IP 는 옵션 50 으로)");
    CHECK(dhcp_build_request(b, 299, XID, SMAC, OFFERED, SRV) == DHCP_ERR_NOSPACE, "REQUEST 버퍼 경계");
    CHECK(dhcp_build_discover(0, 300, XID, SMAC) == QSPI_ERR_ARGS && dhcp_build_discover(b, 300, XID, 0) == QSPI_ERR_ARGS
          && dhcp_build_request(b, 300, XID, SMAC, 0, SRV) == QSPI_ERR_ARGS && dhcp_build_request(b, 300, XID, SMAC, OFFERED, 0) == QSPI_ERR_ARGS, "NULL");
}

static void t_parse(void)
{
    printf("[응답 해석]\n");
    dhcp_info_t o; msg_t m;
    m = full(DHCP_OFFER, XID, OFFERED, SRV);
    memset(&o, 0, sizeof o);
    CHECK(P(&m, &o) == 0 && o.type == 2 && memcmp(o.yiaddr, OFFERED, 4) == 0 && memcmp(o.server, SRV, 4) == 0 && o.lease == 3600 && o.has_lease && o.has_server, "OFFER: 종류·주소·서버·임대");
    CHECK(o.has_mask && memcmp(o.mask, MASK24, 4) == 0 && o.has_router && memcmp(o.router, RT1, 4) == 0 && o.has_dns && memcmp(o.dns, DNS1, 4) == 0, "마스크, 라우터·DNS 는 첫 번째 값");
    CHECK(!o.has_t1 && !o.has_t2, "T1/T2 없음");
    m = full(DHCP_ACK, XID, OFFERED, SRV);
    CHECK(P(&m, &o) == 0 && o.type == 5, "ACK");

    /* NAK: 주소·서버·임대 없이도 통과 */
    head(&m, XID, 0, SMAC); otype(&m, DHCP_NAK); ob(&m, 255);
    CHECK(P(&m, &o) == 0 && o.type == 6, "NAK 은 주소·서버·임대 없이 통과");
    /* 다른 종류(DISCOVER 에코 등)도 해석만 되고 걸러내는 건 호출자 */
    head(&m, XID, 0, SMAC); otype(&m, 1); ob(&m, 255);
    CHECK(P(&m, &o) == 0 && o.type == 1, "종류 1 은 해석만 됨");

    /* 머리 오류 */
    m = full(DHCP_OFFER, XID, OFFERED, SRV);
    CHECK(dhcp_parse(m.b, 239, XID, SMAC, &o) == DHCP_ERR_FORMAT && dhcp_parse(m.b, 0, XID, SMAC, &o) == DHCP_ERR_FORMAT, "240바이트 미만");
    { msg_t h; head(&h, XID, OFFERED, SMAC); CHECK(dhcp_parse(h.b, 240, XID, SMAC, &o) == DHCP_ERR_OPTIONS, "정확히 240바이트(옵션 없음): 종류 없음"); }
    m = full(DHCP_OFFER, XID, OFFERED, SRV); m.b[0] = 1;
    CHECK(P(&m, &o) == DHCP_ERR_FORMAT, "op=REQUEST 거절");
    m = full(DHCP_OFFER, XID, OFFERED, SRV); m.b[0] = 3;
    CHECK(P(&m, &o) == DHCP_ERR_FORMAT, "op=3 거절");
    m = full(DHCP_OFFER, XID, OFFERED, SRV); m.b[1] = 6;
    CHECK(P(&m, &o) == DHCP_ERR_FORMAT, "htype≠1 거절");
    m = full(DHCP_OFFER, XID, OFFERED, SRV); m.b[2] = 8;
    CHECK(P(&m, &o) == DHCP_ERR_FORMAT, "hlen≠6 거절");
    m = full(DHCP_OFFER, XID, OFFERED, SRV); m.b[239] ^= 1;
    CHECK(P(&m, &o) == DHCP_ERR_FORMAT, "쿠키 마지막 바이트 틀림");
    m = full(DHCP_OFFER, XID, OFFERED, SRV); m.b[236] ^= 0x80;
    CHECK(P(&m, &o) == DHCP_ERR_FORMAT, "쿠키 첫 바이트 틀림");
    m = full(DHCP_OFFER, XID + 1, OFFERED, SRV);
    CHECK(P(&m, &o) == DHCP_ERR_NOTMINE, "xid 다름");
    m = full(DHCP_OFFER, XID ^ 0x80000000u, OFFERED, SRV);
    CHECK(P(&m, &o) == DHCP_ERR_NOTMINE, "xid 최상위 비트만 다름");
    m = full(DHCP_OFFER, XID, OFFERED, SRV); m.b[33] ^= 1;
    CHECK(P(&m, &o) == DHCP_ERR_NOTMINE, "chaddr 마지막 바이트 다름");
    m = full(DHCP_OFFER, XID, OFFERED, SRV); m.b[28] ^= 1;
    CHECK(P(&m, &o) == DHCP_ERR_NOTMINE, "chaddr 첫 바이트 다름");

    /* 주소·마스크 검사 */
    const uint8_t bad_addr[][4] = { { 0, 0, 0, 0 }, { 0, 1, 2, 3 }, { 127, 0, 0, 1 }, { 224, 0, 0, 1 }, { 255, 255, 255, 255 } };
    for (unsigned i = 0; i < 5; i++) {
        m = full(DHCP_OFFER, XID, bad_addr[i], SRV);
        CHECK(P(&m, &o) == DHCP_ERR_OPTIONS, "OFFER 의 부적합 주소 거절(0/127/224 이상)");
    }
    const uint8_t ok_addr[][4] = { { 1, 0, 0, 1 }, { 126, 255, 255, 254 }, { 128, 0, 0, 1 }, { 223, 255, 255, 254 } };
    for (unsigned i = 0; i < 4; i++) { m = full(DHCP_OFFER, XID, ok_addr[i], SRV); CHECK(P(&m, &o) == 0, "경계의 정상 주소 허용"); }
    { const uint8_t z[4] = { 0, 0, 0, 0 }; m = full(DHCP_NAK, XID, z, SRV); CHECK(P(&m, &o) == 0, "NAK 은 yiaddr 0 이어도 통과"); }
    const uint8_t bad_mask[][4] = { { 0, 0, 0, 0 }, { 255, 0, 255, 0 }, { 0, 255, 255, 255 }, { 255, 255, 255, 1 }, { 255, 255, 127, 0 }, { 254, 255, 0, 0 } };
    for (unsigned i = 0; i < 6; i++) {
        msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob4(&h, 54, SRV); ou32(&h, 51, 100); ob4(&h, 1, bad_mask[i]); ob(&h, 255);
        CHECK(P(&h, &o) == DHCP_ERR_OPTIONS, "연속이 아니거나 0 인 마스크 거절");
    }
    const uint8_t ok_mask[][4] = { { 255, 255, 255, 255 }, { 255, 255, 255, 252 }, { 255, 0, 0, 0 }, { 128, 0, 0, 0 }, { 255, 255, 254, 0 } };
    for (unsigned i = 0; i < 5; i++) {
        msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob4(&h, 54, SRV); ou32(&h, 51, 100); ob4(&h, 1, ok_mask[i]); ob(&h, 255);
        CHECK(P(&h, &o) == 0 && memcmp(o.mask, ok_mask[i], 4) == 0, "정상 마스크 허용");
    }

    /* 필수 옵션 */
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ou32(&h, 51, 100); ob(&h, 255);   CHECK(P(&h, &o) == DHCP_ERR_OPTIONS, "OFFER 에 서버 ID 없음"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 5); ob4(&h, 54, SRV); ob(&h, 255);   CHECK(P(&h, &o) == DHCP_ERR_OPTIONS, "ACK 에 임대 시간 없음"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); ob4(&h, 54, SRV); ou32(&h, 51, 100); ob(&h, 255);   CHECK(P(&h, &o) == DHCP_ERR_OPTIONS, "메시지 종류 없음"); }

    /* 옵션 경계 */
    { msg_t h = full(DHCP_OFFER, XID, OFFERED, SRV); h.n -= 1;   CHECK(P(&h, &o) == 0, "END 가 없어도(길이로 끝) 통과"); }
    { msg_t h = full(DHCP_OFFER, XID, OFFERED, SRV); h.n -= 2;   CHECK(P(&h, &o) == DHCP_ERR_FORMAT, "마지막 DNS 옵션이 잘림 → 형식 오류"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob(&h, 54);   CHECK(P(&h, &o) == DHCP_ERR_FORMAT, "코드만 있고 길이 바이트가 없음"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob(&h, 54); ob(&h, 4); ob(&h, 1); ob(&h, 2); ob(&h, 3);   CHECK(P(&h, &o) == DHCP_ERR_FORMAT, "길이 4 인데 3바이트만 있음"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob(&h, 54); ob(&h, 4); ob(&h, 1); ob(&h, 2); ob(&h, 3); ob(&h, 4);
      ob4(&h, 54, SRV2); ou32(&h, 51, 7); ob(&h, 255); CHECK(P(&h, &o) == 0 && memcmp(o.server, "\x01\x02\x03\x04", 4) == 0, "정확히 끝에 맞는 옵션 허용, 같은 옵션이 또 오면 첫 값"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); ob(&h, 0); ob(&h, 0); otype(&h, 2); ob(&h, 0); ob4(&h, 54, SRV); ou32(&h, 51, 77); ob(&h, 255);   CHECK(P(&h, &o) == 0 && o.lease == 77, "PAD 건너뜀"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob4(&h, 54, SRV); ou32(&h, 51, 77); ob(&h, 255); ob(&h, 3); ob(&h, 200);   CHECK(P(&h, &o) == 0, "END 뒤의 쓰레기는 읽지 않음"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob4(&h, 54, SRV); ou32(&h, 51, 77); ob(&h, 12); ob(&h, 3); ob(&h, 'a'); ob(&h, 'b'); ob(&h, 'c'); ob(&h, 255);   CHECK(P(&h, &o) == 0, "모르는 옵션(길이 3)은 건너뜀"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob(&h, 12); ob(&h, 0); ob4(&h, 54, SRV); ou32(&h, 51, 77); ob(&h, 255);   CHECK(P(&h, &o) == 0 && o.lease == 77, "길이 0 옵션"); }
    /* 길이 오류 */
    { msg_t h; head(&h, XID, OFFERED, SMAC); ob(&h, 53); ob(&h, 2); ob(&h, 2); ob(&h, 2); ob(&h, 255);   CHECK(P(&h, &o) == DHCP_ERR_FORMAT, "종류 옵션 길이 2"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob(&h, 54); ob(&h, 3); ob(&h, 1); ob(&h, 2); ob(&h, 3); ob(&h, 255);   CHECK(P(&h, &o) == DHCP_ERR_FORMAT, "서버 ID 길이 3"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob(&h, 51); ob(&h, 3); ob(&h, 1); ob(&h, 2); ob(&h, 3); ob(&h, 255);   CHECK(P(&h, &o) == DHCP_ERR_FORMAT, "임대 길이 3"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob(&h, 1); ob(&h, 5); for (int i = 0; i < 5; i++) ob(&h, 255); ob(&h, 255);   CHECK(P(&h, &o) == DHCP_ERR_FORMAT, "마스크 길이 5"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob(&h, 3); ob(&h, 0); ob(&h, 255);   CHECK(P(&h, &o) == DHCP_ERR_FORMAT, "라우터 길이 0"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob(&h, 3); ob(&h, 6); for (int i = 0; i < 6; i++) ob(&h, 1); ob(&h, 255);   CHECK(P(&h, &o) == DHCP_ERR_FORMAT, "라우터 길이 6(4의 배수 아님)"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob(&h, 6); ob(&h, 2); ob(&h, 1); ob(&h, 2); ob(&h, 255);   CHECK(P(&h, &o) == DHCP_ERR_FORMAT, "DNS 길이 2"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob(&h, 6); ob(&h, 4); for (int i = 0; i < 4; i++) ob(&h, 1); ob(&h, 6); ob(&h, 4); for (int i = 0; i < 4; i++) ob(&h, 2);
      ob4(&h, 54, SRV); ou32(&h, 51, 5); ob(&h, 255);   CHECK(P(&h, &o) == 0 && o.dns[0] == 1, "DNS 옵션이 두 번 오면 첫 값"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob4(&h, 54, SRV); ou32(&h, 51, 5); ou32(&h, 58, 100); ou32(&h, 59, 200); ob(&h, 255);
      CHECK(P(&h, &o) == 0 && o.has_t1 && o.t1 == 100 && o.has_t2 && o.t2 == 200, "T1/T2 읽음"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob4(&h, 54, SRV); ou32(&h, 51, 0xFFFFFFFFu); ob(&h, 255);
      CHECK(P(&h, &o) == 0 && o.lease == 0xFFFFFFFFu, "임대 0xFFFFFFFF(무기한), 32비트 전체를 읽음"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob4(&h, 54, SRV); ou32(&h, 51, 0x80000001u); ob(&h, 255);
      CHECK(P(&h, &o) == 0 && o.lease == 0x80000001u, "임대 상위 비트"); }

    /* 같은 옵션이 반복되면 첫 값 */
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); otype(&h, 5); ob4(&h, 54, SRV); ou32(&h, 51, 100); ou32(&h, 51, 200); ob4(&h, 1, MASK24); ob4(&h, 1, ok_mask[2]);
      ob4(&h, 3, RT1); ob4(&h, 3, RT2); ob(&h, 255);
      CHECK(P(&h, &o) == 0 && o.type == 2 && o.lease == 100 && memcmp(o.mask, MASK24, 4) == 0 && memcmp(o.router, RT1, 4) == 0, "종류·임대·마스크·라우터가 반복돼도 첫 값"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob4(&h, 54, SRV); ou32(&h, 51, 100); ou32(&h, 58, 10); ou32(&h, 58, 20); ou32(&h, 59, 30); ou32(&h, 59, 40); ob(&h, 255);
      CHECK(P(&h, &o) == 0 && o.t1 == 10 && o.t2 == 30, "T1·T2 반복도 첫 값"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob4(&h, 54, SRV); ou32(&h, 51, 100); ob(&h, 58); ob(&h, 3); ob(&h, 1); ob(&h, 2); ob(&h, 3); ob(&h, 255);
      CHECK(P(&h, &o) == DHCP_ERR_FORMAT, "T1 길이 3"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob4(&h, 54, SRV); ou32(&h, 51, 100); ob(&h, 59); ob(&h, 3); ob(&h, 1); ob(&h, 2); ob(&h, 3); ob(&h, 255);
      CHECK(P(&h, &o) == DHCP_ERR_FORMAT, "T2 길이 3"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob(&h, 6); ob(&h, 6); for (int i = 0; i < 6; i++) ob(&h, 1); ob(&h, 255);
      CHECK(P(&h, &o) == DHCP_ERR_FORMAT, "DNS 길이 6(4의 배수 아님)"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob(&h, 6); ob(&h, 0); ob(&h, 255);
      CHECK(P(&h, &o) == DHCP_ERR_FORMAT, "DNS 길이 0"); }
    { msg_t h; head(&h, XID, OFFERED, SMAC); otype(&h, 2); ob(&h, 3); ob(&h, 8); for (int i = 0; i < 8; i++) ob(&h, 1); ob4(&h, 54, SRV); ou32(&h, 51, 5); ob(&h, 255);
      CHECK(P(&h, &o) == 0 && o.router[0] == 1, "라우터 길이 8(2개)은 허용"); }

    /* 잘림 훑기: 정확한 크기의 힙 버퍼로 모든 접두를 해석(ASan 이 한 바이트 넘어 읽기를 잡음). 반환값은 정해진 집합 안 */
    { msg_t h = full(DHCP_OFFER, XID, OFFERED, SRV); int bad = 0, okc = 0;
      for (unsigned n = 0; n <= h.n; n++) {
          uint8_t *c = malloc(n ? n : 1); memcpy(c, h.b, n);
          int r = dhcp_parse(c, n, XID, SMAC, &o);
          if (r != 0 && r != DHCP_ERR_FORMAT && r != DHCP_ERR_OPTIONS) bad++;
          if (r == 0) okc++;
          free(c);
      }
      CHECK(bad == 0, "모든 접두 길이에서 반환값이 FORMAT/OPTIONS/0");
      CHECK(okc >= 1, "전체 길이에서는 성공"); }
    /* 실패하면 o 를 건드리지 않음 */
    memset(&o, 0x5A, sizeof o); m = full(DHCP_OFFER, XID + 1, OFFERED, SRV);
    CHECK(P(&m, &o) != 0 && o.type == 0x5A && o.lease == 0x5A5A5A5Au, "실패하면 출력을 건드리지 않음");
    CHECK(dhcp_parse(0, 300, XID, SMAC, &o) == QSPI_ERR_ARGS && dhcp_parse(m.b, m.n, XID, 0, &o) == QSPI_ERR_ARGS && dhcp_parse(m.b, m.n, XID, SMAC, 0) == QSPI_ERR_ARGS, "NULL");
    { /* 다른 MAC 의 클라이언트로 해석하면 NOTMINE */
        m = full(DHCP_OFFER, XID, OFFERED, SRV); CHECK(dhcp_parse(m.b, m.n, XID, OMAC, &o) == DHCP_ERR_NOTMINE, "MAC 다르면 NOTMINE"); }
}

static void t_times(void)
{
    printf("[시간 기본값]\n");
    dhcp_info_t i; uint32_t l, a, b;
    memset(&i, 0, sizeof i); i.lease = 3600;
    dhcp_times(&i, &l, &a, &b);
    CHECK(l == 3600 && a == 1800 && b == 3150, "기본: T1 = 1/2, T2 = 7/8");
    i.has_t1 = 1; i.t1 = 1000; i.has_t2 = 1; i.t2 = 2000; dhcp_times(&i, &l, &a, &b);
    CHECK(a == 1000 && b == 2000, "서버가 준 T1/T2 사용");
    i.t1 = 2000; dhcp_times(&i, &l, &a, &b);
    CHECK(a == 1800 && b == 3150, "T1 = T2 이면 기본값");
    i.t1 = 2500; dhcp_times(&i, &l, &a, &b);
    CHECK(a == 1800 && b == 3150, "T1 > T2 이면 기본값");
    i.t1 = 1000; i.t2 = 3600; dhcp_times(&i, &l, &a, &b);
    CHECK(a == 1800 && b == 3150, "T2 = 임대 이면 기본값");
    i.t2 = 5000; dhcp_times(&i, &l, &a, &b);
    CHECK(a == 1800 && b == 3150, "T2 > 임대 이면 기본값");
    i.t2 = 1999; i.t1 = 1998; dhcp_times(&i, &l, &a, &b);
    CHECK(a == 1998 && b == 1999, "T1 + 1 = T2 는 정상");
    i.has_t2 = 0; i.t1 = 3200; dhcp_times(&i, &l, &a, &b);
    CHECK(a == 1800 && b == 3150, "T1 만 오고 기본 T2(3150) 이상이면 둘 다 기본값");
    i.t1 = 3149; dhcp_times(&i, &l, &a, &b);
    CHECK(a == 3149 && b == 3150, "T1 만 오고 기본 T2 바로 아래면 그대로");
    i.t1 = 100; dhcp_times(&i, &l, &a, &b);
    CHECK(a == 100 && b == 3150, "T1 만 오면 T2 는 기본값");
    i.has_t1 = 0; i.has_t2 = 1; i.t2 = 3000; dhcp_times(&i, &l, &a, &b);
    CHECK(a == 1800 && b == 3000, "T2 만 오면 T1 은 기본값");
    memset(&i, 0, sizeof i); i.lease = 0xFFFFFFFFu; dhcp_times(&i, &l, &a, &b);
    CHECK(l == 0xFFFFFFFFu && a == 0xFFFFFFFFu && b == 0xFFFFFFFFu, "무기한은 모두 무기한");
    i.has_t1 = 1; i.t1 = 5; i.has_t2 = 1; i.t2 = 9; dhcp_times(&i, &l, &a, &b);
    CHECK(a == 0xFFFFFFFFu && b == 0xFFFFFFFFu, "무기한이면 서버 T1/T2 무시");
    memset(&i, 0, sizeof i); i.lease = 0xFFFFFFFEu; dhcp_times(&i, &l, &a, &b);
    CHECK(a == 0x7FFFFFFFu && b == (uint32_t)((uint64_t)0xFFFFFFFEu * 7 / 8), "매우 긴 임대: 32비트 넘침 없이 7/8 (= 3758096381)");
    memset(&i, 0, sizeof i); i.lease = 8; dhcp_times(&i, &l, &a, &b);
    CHECK(a == 4 && b == 7, "임대 8초: 4, 7");
}

/* ---------- W5500 위 클라이언트 ---------- */
enum { H_OK, H_NONE, H_OFFER_SECOND, H_BAD_XID, H_BAD_PORT, H_NAK, H_MISMATCH, H_OTHER_SRV_ACK, H_NO_MASK, H_NO_ACK, H_JUNK_THEN_OK, H_ACK_FIRST, H_TWO_OFFERS, H_INFINITE, H_NO_ROUTER_DNS, H_BAD_OFFER_THEN_OK, H_ACK_SECOND, H_WEIRD_TYPE, H_FRAME, H_NAK_NOSRV };
static int hmode, ndisc, nreq, ir_stale;
static uint32_t last_xid;

static unsigned msgtype_of(const uint8_t *d, unsigned len)
{
    for (unsigned i = 240; i + 2 < len; ) { if (d[i] == 255) break; if (d[i] == 0) { i++; continue; } if (d[i] == 53) return d[i + 2]; i += 2u + d[i + 1]; }
    return 0;
}
static void send_msg(int sock, const msg_t *m, const uint8_t from[4], uint16_t port) { inject_udp(sock, from, port, m->b, m->n); }
static void ack_for(int sock, const uint8_t *d, unsigned len, const uint8_t yi[4], const uint8_t srv[4], int variant)
{
    (void)len;
    uint32_t xid = (uint32_t)d[4] << 24 | (uint32_t)d[5] << 16 | (uint32_t)d[6] << 8 | d[7];
    msg_t m = full(DHCP_ACK, xid, yi, srv);
    if (variant == 1) { head(&m, xid, 0, SMAC); otype(&m, DHCP_NAK); ob4(&m, 54, srv); ob(&m, 255); }
    if (variant == 4) { head(&m, xid, 0, SMAC); otype(&m, DHCP_NAK); ob(&m, 255); }
    if (variant == 2) { head(&m, xid, yi, SMAC); otype(&m, DHCP_ACK); ob4(&m, 54, srv); ou32(&m, 51, 3600); ob(&m, 255); }
    if (variant == 3) { head(&m, xid, yi, SMAC); otype(&m, DHCP_ACK); ob4(&m, 54, srv); ou32(&m, 51, 0xFFFFFFFFu); ob4(&m, 1, MASK24); ob(&m, 255); }
    send_msg(sock, &m, srv, 67);
}
static void hhook(int sock, int proto, const uint8_t ip[4], uint16_t port, const uint8_t *d, unsigned len)
{
    if (proto != 2 || port != 67 || memcmp(ip, BC, 4) != 0 || len < 240) return;
    unsigned t = msgtype_of(d, len);
    uint32_t xid = (uint32_t)d[4] << 24 | (uint32_t)d[5] << 16 | (uint32_t)d[6] << 8 | d[7];
    last_xid = xid;
    if (t == DHCP_DISCOVER) {
        ndisc++;
        msg_t o = full(DHCP_OFFER, xid, OFFERED, SRV);
        switch (hmode) {
        case H_NONE: return;
        case H_OFFER_SECOND: if (ndisc < 2) return; send_msg(sock, &o, SRV, 67); return;
        case H_BAD_XID: o = full(DHCP_OFFER, xid + 1, OFFERED, SRV); send_msg(sock, &o, SRV, 67); return;
        case H_BAD_PORT: send_msg(sock, &o, SRV, 6767); return;
        case H_JUNK_THEN_OK: { uint8_t j[5] = { 1, 2, 3, 4, 5 }; inject_udp(sock, SRV, 67, j, 5); send_msg(sock, &o, SRV, 67); return; }
        case H_ACK_FIRST: { msg_t a = full(DHCP_ACK, xid, OFFERED2, SRV2); send_msg(sock, &a, SRV2, 67); send_msg(sock, &o, SRV, 67); return; }
        case H_TWO_OFFERS: { msg_t o2 = full(DHCP_OFFER, xid, OFFERED2, SRV2); send_msg(sock, &o, SRV, 67); send_msg(sock, &o2, SRV2, 67); return; }
        case H_WEIRD_TYPE: { msg_t w = full(34, xid, OFFERED2, SRV2); send_msg(sock, &w, SRV2, 67); send_msg(sock, &o, SRV, 67); return; }
        case H_FRAME: { uint8_t h[8] = { 1, 2, 3, 4, 0, 67, 0x0B, 0xB8 }; inject_rx(sock, h, 8); return; }
        case H_BAD_OFFER_THEN_OK: { msg_t b = full(DHCP_OFFER, xid, OFFERED2, SRV2); b.b[236] = 0; send_msg(sock, &b, SRV2, 67); send_msg(sock, &o, SRV, 67); return; }
        default: send_msg(sock, &o, SRV, 67); return;
        }
    }
    if (t == DHCP_REQUEST) {
        nreq++;
        if (hmode == H_OK && (S[sock][2] & 0x04)) ir_stale = 1;
        switch (hmode) {
        case H_NO_ACK: return;
        case H_NAK: ack_for(sock, d, len, OFFERED, SRV, 1); return;
        case H_NAK_NOSRV: ack_for(sock, d, len, OFFERED, SRV, 4); return;
        case H_MISMATCH: ack_for(sock, d, len, OFFERED2, SRV, 0); return;
        case H_OTHER_SRV_ACK: ack_for(sock, d, len, OFFERED2, SRV2, 1); ack_for(sock, d, len, OFFERED, SRV, 0); return;
        case H_NO_MASK: ack_for(sock, d, len, OFFERED, SRV, 2); return;
        case H_INFINITE: ack_for(sock, d, len, OFFERED, SRV, 3); return;
        case H_ACK_SECOND: if (nreq < 2) return; ack_for(sock, d, len, OFFERED, SRV, 0); return;
        case H_NO_ROUTER_DNS: { msg_t m; head(&m, xid, OFFERED, SMAC); otype(&m, DHCP_ACK); ob4(&m, 54, SRV); ou32(&m, 51, 600); ob4(&m, 1, MASK24); ob(&m, 255); send_msg(sock, &m, SRV, 67); return; }
        case H_JUNK_THEN_OK: { uint8_t j[3] = { 9, 9, 9 }; inject_udp(sock, SRV, 67, j, 3); ack_for(sock, d, len, OFFERED, SRV, 0); return; }
        default: ack_for(sock, d, len, OFFERED, SRV, 0); return;
        }
    }
}
static int dstart(void)
{
    reset_model(); make_cfg(); memset(cfg.ip, 0, 4); memset(cfg.gw, 0, 4); memset(cfg.mask, 0, 4);
    memset(&W, 0, sizeof W);
    W.s = &Q; W.cs = 0; W.delay_us = model_delay; W.rst = model_rst;
    return w5500_init(&W, &cfg);
}
static void hsetup(int mode) { nwire = 0; hmode = mode; ndisc = nreq = 0; ir_stale = 0; peer_hook = hhook; }

static void t_client(void)
{
    printf("[dhcp_acquire / dhcp_apply]\n");
    CHECK(dstart() == 0, "init (IP 0.0.0.0 으로 시작)");
    dhcp_lease_t L; uint64_t t0;
    hsetup(H_OK); memset(&L, 0xAA, sizeof L);
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 100000, 2, &L) == 0, "정상 거래");
    CHECK(memcmp(L.ip, OFFERED, 4) == 0 && memcmp(L.mask, MASK24, 4) == 0 && memcmp(L.server, SRV, 4) == 0 && L.has_router && memcmp(L.router, RT1, 4) == 0 && L.has_dns && memcmp(L.dns, DNS1, 4) == 0, "임대: IP·마스크·서버·라우터(첫 값)·DNS(첫 값)");
    CHECK(L.lease_s == 3600 && L.t1_s == 1800 && L.t2_s == 3150, "임대 3600, T1 1800, T2 3150");
    CHECK(nwire == 2 && ndisc == 1 && nreq == 1, "송신 2번(DISCOVER, REQUEST)");
    CHECK(wire[0].proto == 2 && wire[0].port == 67 && memcmp(wire[0].ip, BC, 4) == 0 && wire[0].len == 300 && msgtype_of(wire[0].d, 300) == 1, "1번째: UDP, 255.255.255.255:67, 300바이트, DISCOVER");
    CHECK(wire[1].proto == 2 && wire[1].port == 67 && memcmp(wire[1].ip, BC, 4) == 0 && wire[1].len == 300 && msgtype_of(wire[1].d, 300) == 3, "2번째: REQUEST 도 브로드캐스트");
    CHECK(memcmp(wire[0].d + 4, wire[1].d + 4, 4) == 0 && last_xid == XID, "같은 xid");
    CHECK(memcmp(wire[1].d + 243, "\x32\x04\xC0\xA8\x07\x37\x36\x04\xC0\xA8\x07\x01", 12) == 0, "REQUEST 가 OFFER 의 주소(192.168.7.55)·서버(192.168.7.1)를 담음");
    CHECK(rd16s(1, 4) == 68 && S[1][3] == 0 && W.proto[1] == 0, "로컬 포트 68, 끝나면 소켓 닫힘");
    CHECK(viol == 0 && ir_stale == 0, "모델 위반 없음, OFFER 를 읽은 뒤 Sn_IR 의 RECV 비트를 지움");
    CHECK(C[0x0F] == 0 && C[0x10] == 0 && C[0x11] == 0 && C[0x12] == 0, "acquire 는 W5500 의 IP 를 바꾸지 않음(SIPR 그대로 0)");

    /* 적용 */
    CHECK(dhcp_apply(&W, &L) == 0, "dhcp_apply");
    CHECK(memcmp(&C[0x0F], OFFERED, 4) == 0 && memcmp(&C[0x05], MASK24, 4) == 0 && memcmp(&C[0x01], RT1, 4) == 0, "SIPR·SUBR·GAR 에 기록(0x0F, 0x05, 0x01)");
    { dhcp_lease_t N = L; N.has_router = 0; memset(&C[0x01], 0x77, 4);
      CHECK(dhcp_apply(&W, &N) == 0 && C[0x01] == 0 && C[0x04] == 0, "라우터가 없으면 GAR 0.0.0.0"); }
    /* 쓰기를 한 바이트(주소 시작)만 버리는 모델: 기존 값이 달라야 검출되므로 먼저 레지스터를 다른 값으로 채움 */
    memset(&C[0x01], 0x11, 0x12); drop_bsb = 0; drop_off = 0x0F; CHECK(dhcp_apply(&W, &L) == W5500_ERR_VERIFY, "SIPR 쓰기가 반영 안 되면 VERIFY"); 
    memset(&C[0x01], 0x11, 0x12); drop_off = 0x05; CHECK(dhcp_apply(&W, &L) == W5500_ERR_VERIFY, "SUBR 쓰기가 반영 안 되면 VERIFY");
    memset(&C[0x01], 0x11, 0x12); drop_off = 0x01; CHECK(dhcp_apply(&W, &L) == W5500_ERR_VERIFY, "GAR 쓰기가 반영 안 되면 VERIFY");
    drop_off = -1; drop_bsb = -1;
    CHECK(dhcp_apply(0, &L) == QSPI_ERR_ARGS && dhcp_apply(&W, 0) == QSPI_ERR_ARGS, "NULL");

    /* 재시도·타임아웃 */
    hsetup(H_OFFER_SECOND); t0 = now_us;
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 50000, 3, &L) == 0 && ndisc == 2, "첫 DISCOVER 에 응답이 없으면 다시 보냄");
    CHECK(now_us - t0 >= 50000 && now_us - t0 < 110000, "첫 시도 대기 시간만큼 기다림");
    hsetup(H_NONE); t0 = now_us;
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 3, &L) == DHCP_ERR_TIMEOUT && ndisc == 3 && nreq == 0, "OFFER 가 없으면 tries 번 보내고 TIMEOUT");
    CHECK(now_us - t0 == 60000 && S[1][3] == 0 && W.proto[1] == 0, "대기 시간 = 시도 × 제한, 소켓 닫힘");
    hsetup(H_NONE); t0 = now_us;
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 1, &L) == DHCP_ERR_TIMEOUT && ndisc == 1, "tries 1");
    CHECK(now_us - t0 == 20000, "대기 시간이 정확히 제한(20000us) — 모델의 송신은 시간을 쓰지 않으므로 1000us 간격 폴링 20번");
    hsetup(H_NO_ACK); t0 = now_us;
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 3, &L) == DHCP_ERR_TIMEOUT && ndisc == 1 && nreq == 3, "ACK 가 없으면 REQUEST 를 tries 번 보내고 TIMEOUT(DISCOVER 는 한 번)");
    CHECK(S[1][3] == 0 && W.proto[1] == 0, "닫힘");
    hsetup(H_ACK_SECOND);
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 3, &L) == 0 && nreq == 2, "첫 REQUEST 에 응답이 없으면 다시 보냄");

    /* 무시해야 하는 응답 */
    hsetup(H_BAD_XID);
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 1, &L) == DHCP_ERR_TIMEOUT && nreq == 0 && W.proto[1] == 0, "xid 다른 OFFER 는 무시");
    hsetup(H_BAD_PORT);
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 1, &L) == DHCP_ERR_TIMEOUT && nreq == 0, "서버 포트가 67 이 아닌 OFFER 는 무시");
    hsetup(H_JUNK_THEN_OK); t0 = now_us;
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 1, &L) == 0 && memcmp(L.ip, OFFERED, 4) == 0 && now_us - t0 < 20000, "쓰레기 패킷을 버리고 정상 응답을 받음(시간 안 씀)");
    hsetup(H_BAD_OFFER_THEN_OK);
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 1, &L) == 0 && memcmp(L.server, SRV, 4) == 0, "쿠키가 깨진 OFFER 를 버리고 정상 OFFER 를 받음");
    hsetup(H_WEIRD_TYPE);
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 1, &L) == 0 && memcmp(L.ip, OFFERED, 4) == 0 && memcmp(L.server, SRV, 4) == 0, "종류 번호가 32 이상인 이상한 메시지는 무시(시프트 넘침으로 OFFER 로 오인하지 않음)");
    hsetup(H_FRAME);
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 3, &L) == W5500_ERR_FRAME && ndisc == 1 && S[1][3] == 0 && W.proto[1] == 0, "수신 버퍼보다 큰 길이의 헤더 → FRAME 반환(재시도 안 함), 닫힘");
    force_rx_ptr(1, 0);   /* 모델은 CLOSE 가 수신 버퍼를 비우는지 모른다(실제 칩 동작 미확인) → 시험이 직접 비움 */
    hsetup(H_NAK_NOSRV);
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 3, &L) == DHCP_ERR_NAK && nreq == 1, "서버 ID 없는 NAK 도 받아들임(xid·chaddr 일치) — 설계안");
    hsetup(H_ACK_FIRST);
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 1, &L) == 0 && memcmp(L.ip, OFFERED, 4) == 0 && memcmp(L.server, SRV, 4) == 0, "OFFER 기다리는 중에 온 ACK 는 무시");
    hsetup(H_TWO_OFFERS);
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 1, &L) == 0 && memcmp(L.ip, OFFERED, 4) == 0 && memcmp(L.server, SRV, 4) == 0 && wire[1].d[245] == 192 && wire[1].d[248] == 55 && wire[1].d[254] == 1, "OFFER 가 둘이면 먼저 온 것을 선택, REQUEST 에 그 서버");
    hsetup(H_OTHER_SRV_ACK);
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 1, &L) == 0 && memcmp(L.ip, OFFERED, 4) == 0 && memcmp(L.server, SRV, 4) == 0, "다른 서버의 NAK 은 무시하고 선택한 서버의 ACK 를 받음");

    /* 종료 오류 */
    hsetup(H_NAK);
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 3, &L) == DHCP_ERR_NAK && nreq == 1 && S[1][3] == 0 && W.proto[1] == 0, "NAK → 바로 반환(재시도 없음), 닫힘");
    hsetup(H_MISMATCH);
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 3, &L) == DHCP_ERR_MISMATCH && nreq == 1 && W.proto[1] == 0, "ACK 주소가 OFFER 와 다르면 MISMATCH");
    hsetup(H_NO_MASK);
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 3, &L) == DHCP_ERR_NOMASK && W.proto[1] == 0, "마스크 없는 ACK → NOMASK");
    hsetup(H_INFINITE);
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 1, &L) == 0 && L.lease_s == 0xFFFFFFFFu && L.t1_s == 0xFFFFFFFFu && L.t2_s == 0xFFFFFFFFu && !L.has_router && !L.has_dns, "무기한 임대, 라우터·DNS 없음");
    hsetup(H_NO_ROUTER_DNS); memset(&L, 0xAA, sizeof L);
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 1, &L) == 0 && !L.has_router && !L.has_dns && L.lease_s == 600 && L.t1_s == 300 && L.t2_s == 525, "라우터·DNS 옵션이 없으면 has_* = 0, 임대 600 → 300/525");
    CHECK(L.router[0] == 0 && L.dns[0] == 0, "없는 값은 0");

    /* 인자·상태 */
    hsetup(H_OK); { int nc = n_cmds;
        CHECK(dhcp_acquire(0, 1, SMAC, XID, 5, 1, &L) == QSPI_ERR_ARGS && dhcp_acquire(&W, 1, 0, XID, 5, 1, &L) == QSPI_ERR_ARGS && dhcp_acquire(&W, 1, SMAC, XID, 5, 0, &L) == QSPI_ERR_ARGS && dhcp_acquire(&W, 1, SMAC, XID, 5, 1, 0) == QSPI_ERR_ARGS, "인자 검사");
        CHECK(n_cmds == nc, "인자 오류는 소켓 명령 없음"); }
    peer_dead = 1; hsetup(H_OK); peer_dead = 1;
    CHECK(dhcp_acquire(&W, 1, SMAC, XID, 20000, 2, &L) == W5500_ERR_PEER && W.proto[1] == 0, "송신 실패 → PEER, 닫힘");
    peer_dead = 0;
    hsetup(H_OK);
    CHECK(w5500_sock_open(&W, 2, W5500_PROTO_UDP, 6000) == 0 && dhcp_acquire(&W, 2, SMAC, XID, 5000, 1, &L) != 0 && S[2][3] == 0x22, "사용 중인 소켓은 건드리지 않음");
    CHECK(viol == 0, "모델 위반 없음");
}

int main(void)
{
    (void)start;
    t_build(); t_parse(); t_times(); t_client();
    if (fails) { printf("FAIL: %d\n", fails); return 1; }
    printf("ALL PASS (dhcp)\n");
    return 0;
}
