#include "dhcp.h"
#include <string.h>

#define DHCP_POLL_US 1000u
#define DHCP_RX_MAX  640u

static const uint8_t BCAST[4] = { 255, 255, 255, 255 };

/* 허용한 종류(want_mask 의 비트 = 종류 번호)의 우리 응답이 올 때까지 기다린다. 0: 받음, TIMEOUT, 또는 하위 오류 */
static int wait_reply(w5500_t *w, unsigned sock, const uint8_t mac[6], uint32_t xid, uint32_t timeout_us, unsigned want_mask, dhcp_info_t *info)
{
    for (uint32_t t = 0; ; ) {
        uint8_t buf[DHCP_RX_MAX], src[4]; uint16_t sp;
        int n = w5500_udp_recvfrom(w, sock, src, &sp, buf, sizeof buf);
        if (n >= 0) {
            w5500_clear_ir(w, sock, W5500_IR_RECV);
            if (sp == DHCP_SERVER_PORT && dhcp_parse(buf, (unsigned)n, xid, mac, info) == 0 && info->type < 32 && ((1u << info->type) & want_mask)) return 0;
            continue;
        }
        if (n != W5500_ERR_NOPKT) return n;
        if (t >= timeout_us) return DHCP_ERR_TIMEOUT;
        if (w->delay_us) w->delay_us(w->ctx, DHCP_POLL_US);
        t += DHCP_POLL_US;
    }
}

int dhcp_acquire(w5500_t *w, unsigned sock, const uint8_t mac[6], uint32_t xid, uint32_t timeout_us, unsigned tries, dhcp_lease_t *out)
{
    if (!w || !mac || !out || tries == 0) return QSPI_ERR_ARGS;
    uint8_t pkt[DHCP_MIN_LEN];
    int e = dhcp_build_discover(pkt, sizeof pkt, xid, mac);
    if (e < 0) return e;
    e = w5500_sock_open(w, sock, W5500_PROTO_UDP, DHCP_CLIENT_PORT);
    if (e) return e;
    dhcp_info_t offer, ack;
    e = DHCP_ERR_TIMEOUT;
    for (unsigned k = 0; k < tries && e == DHCP_ERR_TIMEOUT; k++) {
        int r = w5500_udp_sendto(w, sock, BCAST, DHCP_SERVER_PORT, pkt, DHCP_MIN_LEN, timeout_us);
        if (r < 0) { e = r; break; }
        e = wait_reply(w, sock, mac, xid, timeout_us, 1u << DHCP_OFFER, &offer);
    }
    if (e == 0) {
        e = dhcp_build_request(pkt, sizeof pkt, xid, mac, offer.yiaddr, offer.server);
        if (e >= 0) e = DHCP_ERR_TIMEOUT;
        for (unsigned k = 0; k < tries && e == DHCP_ERR_TIMEOUT; k++) {
            int r = w5500_udp_sendto(w, sock, BCAST, DHCP_SERVER_PORT, pkt, DHCP_MIN_LEN, timeout_us);
            if (r < 0) { e = r; break; }
            for (;;) {
                e = wait_reply(w, sock, mac, xid, timeout_us, (1u << DHCP_ACK) | (1u << DHCP_NAK), &ack);
                if (e != 0) break;
                if (ack.has_server && memcmp(ack.server, offer.server, 4) != 0) continue;     /* 다른 서버의 응답은 무시 */
                if (ack.type == DHCP_NAK) { e = DHCP_ERR_NAK; break; }
                if (memcmp(ack.yiaddr, offer.yiaddr, 4) != 0) { e = DHCP_ERR_MISMATCH; break; }
                if (!ack.has_mask) { e = DHCP_ERR_NOMASK; break; }
                memset(out, 0, sizeof *out);
                memcpy(out->ip, ack.yiaddr, 4); memcpy(out->mask, ack.mask, 4); memcpy(out->server, ack.server, 4);
                if ((out->has_router = ack.has_router) != 0) memcpy(out->router, ack.router, 4);
                if ((out->has_dns = ack.has_dns) != 0) memcpy(out->dns, ack.dns, 4);
                dhcp_times(&ack, &out->lease_s, &out->t1_s, &out->t2_s);
                e = 0; break;
            }
        }
    }
    w5500_close(w, sock);
    return e;
}

int dhcp_apply(w5500_t *w, const dhcp_lease_t *l)
{
    if (!w || !l) return QSPI_ERR_ARGS;
    static const uint8_t zero[4] = { 0, 0, 0, 0 };
    const uint8_t *gw = l->has_router ? l->router : zero;
    struct { uint16_t off; const uint8_t *v; } regs[3] = { { W5500_GAR, gw }, { W5500_SUBR, l->mask }, { W5500_SIPR, l->ip } };
    for (int i = 0; i < 3; i++) {
        uint8_t back[4];
        int e = w5500_write(w, W5500_BSB_COMMON, regs[i].off, regs[i].v, 4);
        if (e) return e;
        e = w5500_read(w, W5500_BSB_COMMON, regs[i].off, back, 4);
        if (e) return e;
        if (memcmp(back, regs[i].v, 4) != 0) return W5500_ERR_VERIFY;
    }
    return QSPI_OK;
}
