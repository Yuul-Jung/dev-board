#include "ntp.h"
#include <string.h>

#define NTP_POLL_US 1000u

int ntp_query(w5500_t *w, unsigned sock, uint16_t local_port, const uint8_t server[4], uint32_t nonce_sec, uint32_t nonce_frac, uint32_t timeout_us, ntp_time_t *out)
{
    if (!w || !server || !out || local_port == 0) return QSPI_ERR_ARGS;
    uint8_t req[NTP_PACKET_LEN];
    int e = ntp_build_request(req, nonce_sec, nonce_frac);
    if (e) return e;
    e = w5500_sock_open(w, sock, W5500_PROTO_UDP, local_port);
    if (e) return e;
    e = w5500_udp_sendto(w, sock, server, NTP_PORT, req, NTP_PACKET_LEN, timeout_us);
    if (e < 0) { w5500_close(w, sock); return e; }
    e = NTP_ERR_TIMEOUT;
    for (uint32_t t = 0; ; ) {
        uint8_t buf[NTP_PACKET_LEN + 20], ip[4]; uint16_t port;
        int n = w5500_udp_recvfrom(w, sock, ip, &port, buf, sizeof buf);
        if (n >= 0) {
            w5500_clear_ir(w, sock, W5500_IR_RECV);
            ntp_pkt_t r;
            if (memcmp(ip, server, 4) == 0 && port == NTP_PORT && ntp_parse(buf, (unsigned)n, &r) == QSPI_OK && ntp_check_reply(&r, nonce_sec, nonce_frac) == 0) {
                out->unix_sec = ntp_to_unix(r.tx_sec); out->usec = ntp_frac_to_usec(r.tx_frac); out->stratum = r.stratum;
                e = QSPI_OK; break;
            }
            continue;                                   /* 남의 패킷·잘못된 응답: 시간을 쓰지 않고 다음 패킷 확인 */
        }
        if (n != W5500_ERR_NOPKT) { e = n; break; }
        if (t >= timeout_us) break;
        if (w->delay_us) w->delay_us(w->ctx, NTP_POLL_US);
        t += NTP_POLL_US;
    }
    w5500_close(w, sock);
    return e;
}
