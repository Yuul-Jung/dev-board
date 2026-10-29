#include "dns.h"
#include <string.h>

#define DNS_POLL_US 1000u

int dns_resolve(w5500_t *w, unsigned sock, uint16_t local_port, const uint8_t server[4], const char *name, uint16_t id, uint32_t timeout_us, unsigned tries, uint8_t ip[4], uint32_t *ttl)
{
    if (!w || !server || !ip || local_port == 0 || tries == 0) return QSPI_ERR_ARGS;
    uint8_t q[DNS_HDR_LEN + DNS_MAX_NAME_WIRE + 4];
    int ql = dns_build_query(q, sizeof q, id, name);
    if (ql < 0) return ql;
    int e = w5500_sock_open(w, sock, W5500_PROTO_UDP, local_port);
    if (e) return e;
    e = DNS_ERR_TIMEOUT;
    for (unsigned attempt = 0; attempt < tries && e == DNS_ERR_TIMEOUT; attempt++) {
        int r = w5500_udp_sendto(w, sock, server, DNS_PORT, q, (unsigned)ql, timeout_us);
        if (r < 0) { e = r; break; }
        for (uint32_t t = 0; ; ) {
            uint8_t buf[DNS_MAX_UDP], src[4]; uint16_t sp;
            int n = w5500_udp_recvfrom(w, sock, src, &sp, buf, sizeof buf);
            if (n >= 0) {
                w5500_clear_ir(w, sock, W5500_IR_RECV);
                if (memcmp(src, server, 4) != 0 || sp != DNS_PORT) continue;
                int pr = dns_parse_response(buf, (unsigned)n, id, name, ip, ttl);
                if (pr == DNS_ERR_ID || pr == DNS_ERR_FORMAT || pr == DNS_ERR_QUESTION) continue;   /* 우리 응답이 아님 */
                e = pr; break;
            }
            if (n != W5500_ERR_NOPKT) { e = n; break; }
            if (t >= timeout_us) break;
            if (w->delay_us) w->delay_us(w->ctx, DNS_POLL_US);
            t += DNS_POLL_US;
        }
    }
    w5500_close(w, sock);
    return e;
}
