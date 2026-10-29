/* DHCP 클라이언트(IPv4, 처음 주소 받기). 메시지 만들기·해석은 입출력 없는 순수 함수(dhcp.c), W5500 위의 DISCOVER→OFFER→REQUEST→ACK 는 dhcp_client.c.
 * 근거(등급):
 *   - 메시지 배치: op, htype, hlen, hops(각 1) · xid(4) · secs(2) · flags(2) · ciaddr·yiaddr·siaddr·giaddr(각 4) · chaddr(16) · sname(64) · file(128) · 쿠키(4) · 옵션 → 쿠키는 오프셋 236, 옵션은 240.
 *     쿠키 0x63825363, BROADCAST 플래그 0x8000, 포트 서버 67 / 클라이언트 68, op BOOTREQUEST 1 / BOOTREPLY 2, 메시지 종류 DISCOVER 1 … NAK 6,
 *     옵션 번호 0 패드 · 1 서브넷 마스크 · 3 라우터 · 6 DNS · 50 요청 IP · 51 임대 시간 · 53 메시지 종류 · 54 서버 ID · 55 요청 목록 · 58 T1 · 59 T2 · 255 끝 — dhcpcd `dhcp.h`(Chromium OS 사본, D3, WebFetch 요약 경유).
 *   - W5500: 목적 IP 255.255.255.255 로 보내면 ARP 없이 브로드캐스트되고, 소켓은 255.255.255.255 로 오는 브로드캐스트도 받는다 — WIZnet 문서 UDP 쪽(D1, 요약 경유). 그 문서는 SIPR 을 0.0.0.0 으로 둔 채 보내는 것과 Sn_MR 의 브로드캐스트 차단 비트는 다루지 않는다(미확인).
 *   RFC 2131 원문은 열지 못했다(미확인). 아래는 설계안 또는 일반 지식(미확인): 요청 메시지를 300바이트로 채움(BOOTP 최소 크기 관례), 한 거래 동안 xid 고정, 요청에 BROADCAST 플래그를 켬(주소 받기 전에는 유니캐스트 응답을 못 받는다고 보고),
 *   T1 = 임대/2, T2 = 임대의 7/8 을 기본값으로, 클라이언트 식별자(61) 옵션은 쓰지 않고 chaddr 만 사용, sname/file 안의 옵션(옵션 52)은 읽지 않음, 같은 옵션이 반복되면 첫 번째 값, OFFER·ACK 에는 서버 ID(54)와 임대 시간(51)이 있어야 함.
 * 하지 않는 것: 임대 갱신(T1/T2 에 호출자가 dhcp_acquire 를 다시 부르면 됨), DECLINE·RELEASE, ARP 충돌 검사, INIT-REBOOT, 릴레이, 옵션 중첩(52), 클라이언트 식별자, 호스트 이름. */
#ifndef DHCP_H
#define DHCP_H
#include <stdint.h>
#include "w5500.h"

#define DHCP_SERVER_PORT 67u
#define DHCP_CLIENT_PORT 68u
#define DHCP_COOKIE_OFF  236u
#define DHCP_OPT_OFF     240u
#define DHCP_MIN_LEN     300u
#define DHCP_OP_REQUEST  1u
#define DHCP_OP_REPLY    2u
#define DHCP_HTYPE_ETH   1u
#define DHCP_HLEN_ETH    6u
#define DHCP_FLAG_BROADCAST 0x8000u
#define DHCP_COOKIE      0x63825363u
#define DHCP_CHADDR_OFF  28u

#define DHO_PAD      0u
#define DHO_MASK     1u
#define DHO_ROUTER   3u
#define DHO_DNS      6u
#define DHO_REQ_IP   50u
#define DHO_LEASE    51u
#define DHO_MSGTYPE  53u
#define DHO_SERVER   54u
#define DHO_PARAMS   55u
#define DHO_T1       58u
#define DHO_T2       59u
#define DHO_END      255u

#define DHCP_DISCOVER 1u
#define DHCP_OFFER    2u
#define DHCP_REQUEST  3u
#define DHCP_ACK      5u
#define DHCP_NAK      6u

#define DHCP_ERR_FORMAT   (-120)   /* 길이·op·htype·hlen·쿠키·옵션 경계 오류 */
#define DHCP_ERR_NOTMINE  (-121)   /* xid 또는 chaddr 가 우리 것이 아님 */
#define DHCP_ERR_OPTIONS  (-122)   /* 필요한 옵션이 없거나 값이 이상함(종류 없음, OFFER/ACK 에 서버 ID·임대 시간 없음, 주소가 0/127/224 이상, 마스크가 연속이 아님) */
#define DHCP_ERR_NAK      (-123)   /* 서버가 거절(NAK) */
#define DHCP_ERR_TIMEOUT  (-124)
#define DHCP_ERR_NOSPACE  (-125)
#define DHCP_ERR_MISMATCH (-126)   /* ACK 의 주소가 OFFER 와 다름 */
#define DHCP_ERR_NOMASK   (-127)   /* 임대에 서브넷 마스크가 없어 W5500 에 설정할 수 없음 */

typedef struct {
    uint8_t type, yiaddr[4], server[4], mask[4], router[4], dns[4];
    uint8_t has_mask, has_router, has_dns, has_lease, has_t1, has_t2, has_server;
    uint32_t lease, t1, t2;
} dhcp_info_t;

typedef struct {
    uint8_t ip[4], mask[4], router[4], dns[4], server[4];
    uint8_t has_router, has_dns;
    uint32_t lease_s, t1_s, t2_s;   /* 0xFFFFFFFF = 무기한 */
} dhcp_lease_t;

int dhcp_build_discover(uint8_t *buf, unsigned cap, uint32_t xid, const uint8_t mac[6]);
int dhcp_build_request(uint8_t *buf, unsigned cap, uint32_t xid, const uint8_t mac[6], const uint8_t req_ip[4], const uint8_t server[4]);
int dhcp_parse(const uint8_t *m, unsigned len, uint32_t xid, const uint8_t mac[6], dhcp_info_t *o);
void dhcp_times(const dhcp_info_t *i, uint32_t *lease, uint32_t *t1, uint32_t *t2);   /* 서버 값이 앞뒤가 안 맞으면(T1 < T2 < 임대 가 아니면) 기본값 */

/* 소켓 sock 을 UDP 68 로 열고 DISCOVER → OFFER → REQUEST → ACK 를 진행한다. W5500 의 SIPR 은 호출자가 0.0.0.0 으로 두고(w5500_init 의 ip 를 0 으로) 시작한다.
 * 각 단계에서 tries 번까지 보내고 시도마다 timeout_us 를 기다린다. 소켓은 열고 닫는다. 성공하면 *out 을 채우고 0 (아직 W5500 에 적용되지 않음: dhcp_apply). */
int dhcp_acquire(w5500_t *w, unsigned sock, const uint8_t mac[6], uint32_t xid, uint32_t timeout_us, unsigned tries, dhcp_lease_t *out);
int dhcp_apply(w5500_t *w, const dhcp_lease_t *l);   /* SIPR·SUBR·GAR 를 쓰고 읽어 확인. 라우터가 없으면 GAR 는 0.0.0.0 */
#endif
