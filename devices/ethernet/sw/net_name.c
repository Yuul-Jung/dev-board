#include "net_name.h"
#include <string.h>

static int resolve(w5500_t *w, unsigned sock, uint16_t lp, const uint8_t dns[4], uint16_t id, const char *name, uint32_t to, uint8_t ip[4])
{
    if (dns_parse_ipv4(name, ip)) return 0;
    if (!dns) return QSPI_ERR_ARGS;
    return dns_resolve(w, sock, lp, dns, name, id, to, NET_DNS_TRIES, ip, 0);
}

int ntp_query_name(w5500_t *w, unsigned sock, uint16_t local_port, const uint8_t dns_server[4], uint16_t dns_id, const char *server_name,
                   uint32_t nonce_sec, uint32_t nonce_frac, uint32_t timeout_us, ntp_time_t *out)
{
    uint8_t ip[4];
    if (!w || !server_name) return QSPI_ERR_ARGS;
    int e = resolve(w, sock, local_port, dns_server, dns_id, server_name, timeout_us, ip);
    if (e) return e;
    return ntp_query(w, sock, local_port, ip, nonce_sec, nonce_frac, timeout_us, out);
}

int http_get_name(w5500_t *w, unsigned sock, uint16_t local_port, const uint8_t dns_server[4], uint16_t dns_id, uint16_t port, const char *host, const char *path,
                  uint32_t timeout_us, http_sink_fn sink, void *ctx, int *status)
{
    uint8_t ip[4];
    char hh[DNS_MAX_NAME_WIRE + 8];
    if (!w || !host || !status) return QSPI_ERR_ARGS;
    *status = 0;
    size_t hl = strlen(host);
    if (hl == 0 || hl > DNS_MAX_NAME_WIRE) return HTTP_ERR_ARG;
    int e = resolve(w, sock, local_port, dns_server, dns_id, host, timeout_us, ip);
    if (e) return e;
    memcpy(hh, host, hl); hh[hl] = 0;
    if (port != 80) {
        char d[6]; int nd = 0; unsigned v = port;
        do { d[nd++] = (char)('0' + v % 10); v /= 10; } while (v);
        hh[hl++] = ':';
        while (nd) hh[hl++] = d[--nd];
        hh[hl] = 0;
    }
    return http_get(w, sock, local_port, ip, port, hh, path, timeout_us, sink, ctx, status);
}
