/* DNS 클라이언트(UDP 53, A 레코드만). 질의 만들기·응답 해석은 입출력 없는 순수 함수(dns.c), W5500 위의 질의는 dns_client.c, 이름으로 NTP·HTTP 를 부르는 얇은 함수는 net_name.c.
 * 근거(등급):
 *   - 헤더 12바이트(id, flags, qdcount, ancount, nscount, arcount 각 16비트 빅엔디안), QR=0x8000·RD=0x0100·TC=0x0200, RCODE 0~5, TYPE A=1·CNAME=5, CLASS IN=1,
 *     레이블 상위 2비트 11 = 압축 포인터(14비트 오프셋), 이름 전체 255바이트 이하, UDP 메시지 512바이트, 포트 53 — Chromium dns_protocol.h(D3, WebFetch 요약 경유), 포트·헤더 구조·압축 매크로는 iPXE dns.h(D3)와 일치.
 *   - QNAME = 길이 바이트 + 글자들의 반복, 0 으로 끝, 레이블 최대 63(상위 2비트 00), 포인터는 이름을 끝낸다, TC 비트 의미, flags 의 QR·OPCODE(4)·AA·TC·RD·RA 위치 — zytrax DNS 해설(D3, 요약 경유).
 *   RFC 1035 원문은 이 환경에서 열지 못했다(미확인). RA 비트(0x0080)와 RCODE 폭(하위 4비트)은 zytrax 표와 일반 지식으로 두었고, 이 코드는 RA 를 쓰지 않는다.
 * 설계안(문서로 확인하지 못한 선택): 이름 총길이 255 에 끝의 0 바이트까지 포함 / 응답의 질문이 우리가 보낸 것(이름 대소문자 무시, A, IN)과 다르면 거부 /
 *   답변 순서대로 한 번만 훑으며 CNAME 은 최대 8번 따라감(A 가 CNAME 보다 앞서 나오는 응답은 못 찾음) / 같은 이름의 A 가 여럿이면 첫 번째 / A 의 RDLENGTH ≠ 4 는 형식 오류 /
 *   TC 가 켜져 있어도 쓸 수 있는 A 가 있으면 사용, 없으면 TRUNC(TCP 재시도는 하지 않음) / 포인터는 최대 16번 따라감 / 이름 글자는 영문자·숫자·'-'·'_' 만 허용 /
 *   ID 와 보낸 쪽 IP·포트가 맞는 응답만 받음(16비트 ID 라 위조 방지로는 약함 — 로컬 포트를 호출자가 정함).
 * 하지 않는 것: AAAA(IPv6), TCP 폴백, EDNS, DNSSEC, 캐시, 재귀 해석(공용·공유기 DNS 서버에 재귀를 맡김). */
#ifndef DNS_H
#define DNS_H
#include <stdint.h>
#include "w5500.h"

#define DNS_PORT        53u
#define DNS_HDR_LEN     12u
#define DNS_MAX_UDP     512u
#define DNS_MAX_NAME_WIRE 255u
#define DNS_MAX_LABEL   63u
#define DNS_TYPE_A      1u
#define DNS_TYPE_CNAME  5u
#define DNS_CLASS_IN    1u
#define DNS_FLAG_QR     0x8000u
#define DNS_FLAG_TC     0x0200u
#define DNS_FLAG_RD     0x0100u
#define DNS_RCODE_SERVFAIL 2u
#define DNS_RCODE_NXDOMAIN 3u

#define DNS_ERR_FORMAT   (-100)   /* 메시지 형식 오류(짧음, 포인터 이상, 경계 초과, QR=0, OPCODE≠0 …) */
#define DNS_ERR_ID       (-101)   /* ID 불일치 */
#define DNS_ERR_NXDOMAIN (-102)   /* RCODE 3: 그런 이름 없음 */
#define DNS_ERR_SERVFAIL (-103)   /* RCODE 2 */
#define DNS_ERR_RCODE    (-104)   /* 그 밖의 RCODE(1 형식 오류, 4 미구현, 5 거절 …) */
#define DNS_ERR_TRUNC    (-105)   /* TC 가 켜져 있고 쓸 A 레코드가 없음 */
#define DNS_ERR_NOANSWER (-106)   /* 오류는 아니지만 이 이름의 A 레코드가 없음 */
#define DNS_ERR_NAME     (-107)   /* 이름이 규칙에 어긋남(빈 이름, 빈 레이블, 63자 초과 레이블, 255바이트 초과, 허용 안 되는 글자) */
#define DNS_ERR_NOSPACE  (-108)   /* 질의 버퍼가 작음 */
#define DNS_ERR_QUESTION (-109)   /* 응답의 질문이 보낸 질문과 다름 */
#define DNS_ERR_TIMEOUT  (-110)

int dns_build_query(uint8_t *buf, unsigned cap, uint16_t id, const char *name);   /* 길이 또는 음수. 이름 끝의 '.' 하나는 허용 */
int dns_parse_response(const uint8_t *msg, unsigned len, uint16_t id, const char *name, uint8_t ip[4], uint32_t *ttl);   /* 0 이면 ip·ttl 채움 */
int dns_parse_ipv4(const char *s, uint8_t ip[4]);   /* 점 십진 IPv4 면 1(ip 채움), 아니면 0. 각 칸 0~255, 앞에 0 을 붙인 칸("01")은 거절 */

/* 서버(DNS 서버 IP)에 질의를 보내고 응답을 기다린다. tries 번까지 다시 보냄(각 시도마다 timeout_us). 다른 ID·IP·포트나 형식이 틀린 패킷은 무시하고 계속 기다린다.
 * NXDOMAIN·SERVFAIL·RCODE·NOANSWER·TRUNC 처럼 서버의 정상 응답이 오류를 알리면 바로 그 오류를 돌려준다. 소켓은 열고 닫는다. */
int dns_resolve(w5500_t *w, unsigned sock, uint16_t local_port, const uint8_t server[4], const char *name, uint16_t id, uint32_t timeout_us, unsigned tries, uint8_t ip[4], uint32_t *ttl);
#endif
