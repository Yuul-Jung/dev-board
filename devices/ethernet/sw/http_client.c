#include "http.h"
#include <string.h>

#define HTTP_POLL_US 1000u
#define HTTP_RX_STEP 64u

int http_get(w5500_t *w, unsigned sock, uint16_t local_port, const uint8_t ip[4], uint16_t port, const char *host, const char *path, uint32_t timeout_us, http_sink_fn sink, void *ctx, int *status)
{
    if (!w || !ip || !status || local_port == 0 || port == 0) return QSPI_ERR_ARGS;
    *status = 0;
    char req[256];
    int rl = http_build_get(req, sizeof req, host, path);
    if (rl < 0) return rl;
    int e = w5500_sock_open(w, sock, W5500_PROTO_TCP, local_port);
    if (e) return e;
    http_parser_t p; http_parser_init(&p, sink, ctx);
    e = w5500_tcp_connect(w, sock, ip, port, timeout_us);
    if (e) { w5500_close(w, sock); return e; }
    e = w5500_send(w, sock, (const uint8_t *)req, (unsigned)rl, timeout_us);
    if (e < 0) { w5500_close(w, sock); return e; }
    e = HTTP_ERR_TIMEOUT;
    for (uint32_t t = 0; ; ) {
        uint8_t sr, buf[HTTP_RX_STEP];
        int r = w5500_sock_state(w, sock, &sr);
        if (r) { e = r; break; }
        int n = w5500_recv(w, sock, buf, sizeof buf);
        if (n < 0) { e = n; break; }
        if (n > 0) {
            r = http_parser_feed(&p, buf, (unsigned)n);
            if (r == 1) { e = 0; break; }
            if (r < 0) { e = r; break; }
            t = 0;                                      /* 데이터가 오면 멈춘 시간을 다시 셈 */
            continue;                                   /* 받은 만큼 바로 다음 조각 */
        }
        if (sr != W5500_SR_ESTABLISHED) {               /* 상대가 닫았고 읽을 것도 없음 */
            e = http_parser_eof(&p);
            break;
        }
        if (t >= timeout_us) break;
        if (w->delay_us) w->delay_us(w->ctx, HTTP_POLL_US);
        t += HTTP_POLL_US;
    }
    *status = p.status;
    w5500_tcp_disconnect(w, sock, 20000);               /* 이미 CLOSED 면 DISCON 없이 정리만 하고 CLOSE */
    return e;
}
