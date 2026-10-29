/* SNTP 클라이언트(UDP 123). 패킷 만들기·해석은 입출력이 없는 순수 함수(ntp.c), W5500 위의 질의는 ntp_client.c.
 * 근거(등급): 패킷 48바이트·필드 순서·시각 오프셋 40·1900→1970 오프셋 2208988800·첫 바이트 LI(7:6) VN(5:3) Mode(2:0)·클라이언트 모드 3·서버 모드 4·포트 123·
 *   요청의 전송 시각 칸에 임의 값을 넣고 응답의 원점(originate) 칸과 비교 — OpenBSD rdate ntp.c, chrony ntp.h, RIOT/EPICS 헤더(모두 D3, WebFetch 요약 경유).
 *   RFC 5905 원문은 이 환경에서 열지 못했다(미확인). 스트라툼 0(KoD)·LI=3 거절, 응답을 보낸 IP·포트 일치 검사, 시대(era) 처리는 설계안.
 * 시각 해석: 초 단위 32비트는 2036-02-07 에 한 바퀴 돈다. 이 코드는 값이 2208988800(1970) 이상이면 1900~2036 시대, 미만이면 다음 시대(2036~2104)로 본다(설계안). */
#ifndef NTP_H
#define NTP_H
#include <stdint.h>
#include "w5500.h"

#define NTP_PORT        123u
#define NTP_PACKET_LEN  48u
#define NTP_VERSION     4u
#define NTP_MODE_CLIENT 3u
#define NTP_MODE_SERVER 4u
#define NTP_UNIX_OFFSET 2208988800u   /* 1900-01-01 → 1970-01-01 초 */
#define NTP_OFF_TRANSMIT 40u

#define NTP_ERR_LEN     (-80)   /* 패킷이 48바이트 미만 */
#define NTP_ERR_MODE    (-81)   /* 서버 모드(4)가 아님 */
#define NTP_ERR_STRATUM (-82)   /* 스트라툼 0(거절 신호) 또는 15 초과 */
#define NTP_ERR_ORIGIN  (-83)   /* 원점 시각이 우리가 보낸 값과 다름(엉뚱한/위조 응답) */
#define NTP_ERR_LI      (-84)   /* 시계 동기 안 됨 표시(LI=3) */
#define NTP_ERR_TX0     (-85)   /* 서버 전송 시각이 0 */
#define NTP_ERR_TIMEOUT (-86)   /* 시간 안에 응답 없음 */

typedef struct {
    uint8_t li, vn, mode, stratum;
    int8_t poll, precision;
    uint32_t root_delay, root_disp, ref_id;
    uint32_t ref_sec, ref_frac, orig_sec, orig_frac, rx_sec, rx_frac, tx_sec, tx_frac;
} ntp_pkt_t;

typedef struct { int64_t unix_sec; uint32_t usec; uint8_t stratum; } ntp_time_t;

int ntp_build_request(uint8_t pkt[NTP_PACKET_LEN], uint32_t xmit_sec, uint32_t xmit_frac);   /* LI 0, VN 4, 모드 3, 나머지 0, 전송 시각 = 인자 */
int ntp_parse(const uint8_t *p, unsigned len, ntp_pkt_t *o);                                 /* 길이만 검사 */
int ntp_check_reply(const ntp_pkt_t *r, uint32_t orig_sec, uint32_t orig_frac);              /* 0 이면 받아들일 만함 */
int64_t ntp_to_unix(uint32_t ntp_sec);
uint32_t ntp_frac_to_usec(uint32_t frac);

/* W5500 소켓 n 을 UDP 로 열고(로컬 포트 local_port), 서버에 요청을 보내 응답을 기다린 뒤 소켓을 닫는다. nonce 는 호출자가 주는 매번 다른 값(예: 타이머 값).
 * 다른 IP·포트에서 온 패킷, 해석에 실패한 패킷은 무시하고 timeout_us 까지 계속 기다린다. 성공하면 0. */
int ntp_query(w5500_t *w, unsigned sock, uint16_t local_port, const uint8_t server[4], uint32_t nonce_sec, uint32_t nonce_frac, uint32_t timeout_us, ntp_time_t *out);
#endif
