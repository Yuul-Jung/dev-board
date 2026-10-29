#include "dhcp.h"
#include <string.h>

static uint32_t be32(const uint8_t *p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }
static void put32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v; }

static const uint8_t PARAMS[] = { DHO_MASK, DHO_ROUTER, DHO_DNS, DHO_LEASE, DHO_T1, DHO_T2 };

static int common(uint8_t *b, unsigned cap, uint32_t xid, const uint8_t mac[6], uint8_t type)
{
    if (!b || !mac) return QSPI_ERR_ARGS;
    if (cap < DHCP_MIN_LEN) return DHCP_ERR_NOSPACE;
    memset(b, 0, DHCP_MIN_LEN);
    b[0] = DHCP_OP_REQUEST; b[1] = DHCP_HTYPE_ETH; b[2] = DHCP_HLEN_ETH;
    put32(b + 4, xid);
    b[10] = (uint8_t)(DHCP_FLAG_BROADCAST >> 8); b[11] = (uint8_t)DHCP_FLAG_BROADCAST;
    memcpy(b + DHCP_CHADDR_OFF, mac, 6);
    put32(b + DHCP_COOKIE_OFF, DHCP_COOKIE);
    b[DHCP_OPT_OFF] = DHO_MSGTYPE; b[DHCP_OPT_OFF + 1] = 1; b[DHCP_OPT_OFF + 2] = type;
    return (int)(DHCP_OPT_OFF + 3);
}
static unsigned tail(uint8_t *b, unsigned o)
{
    b[o++] = DHO_PARAMS; b[o++] = sizeof PARAMS; memcpy(b + o, PARAMS, sizeof PARAMS); o += sizeof PARAMS;
    b[o++] = DHO_END;
    return o;
}

int dhcp_build_discover(uint8_t *b, unsigned cap, uint32_t xid, const uint8_t mac[6])
{
    int o = common(b, cap, xid, mac, DHCP_DISCOVER);
    if (o < 0) return o;
    tail(b, (unsigned)o);
    return (int)DHCP_MIN_LEN;
}

int dhcp_build_request(uint8_t *b, unsigned cap, uint32_t xid, const uint8_t mac[6], const uint8_t req_ip[4], const uint8_t server[4])
{
    if (!req_ip || !server) return QSPI_ERR_ARGS;
    int oi = common(b, cap, xid, mac, DHCP_REQUEST);
    if (oi < 0) return oi;
    unsigned o = (unsigned)oi;
    b[o++] = DHO_REQ_IP; b[o++] = 4; memcpy(b + o, req_ip, 4); o += 4;
    b[o++] = DHO_SERVER; b[o++] = 4; memcpy(b + o, server, 4); o += 4;
    tail(b, o);
    return (int)DHCP_MIN_LEN;
}

static int addr_ok(const uint8_t a[4]) { return a[0] != 0 && a[0] != 127 && a[0] < 224; }
static int mask_ok(const uint8_t a[4]) { uint32_t v = be32(a), n = ~v; return v != 0 && (n & (n + 1)) == 0; }

int dhcp_parse(const uint8_t *m, unsigned len, uint32_t xid, const uint8_t mac[6], dhcp_info_t *o)
{
    if (!m || !mac || !o) return QSPI_ERR_ARGS;
    if (len < DHCP_OPT_OFF) return DHCP_ERR_FORMAT;
    if (m[0] != DHCP_OP_REPLY || m[1] != DHCP_HTYPE_ETH || m[2] != DHCP_HLEN_ETH) return DHCP_ERR_FORMAT;
    if (be32(m + 4) != xid || memcmp(m + DHCP_CHADDR_OFF, mac, 6) != 0) return DHCP_ERR_NOTMINE;
    if (be32(m + DHCP_COOKIE_OFF) != DHCP_COOKIE) return DHCP_ERR_FORMAT;
    dhcp_info_t r; memset(&r, 0, sizeof r);
    memcpy(r.yiaddr, m + 16, 4);
    unsigned i = DHCP_OPT_OFF;
    while (i < len) {
        unsigned c = m[i++];
        if (c == DHO_PAD) continue;
        if (c == DHO_END) break;
        if (i >= len) return DHCP_ERR_FORMAT;
        unsigned l = m[i++];
        if (i + l > len) return DHCP_ERR_FORMAT;
        const uint8_t *v = m + i;
        switch (c) {
        case DHO_MSGTYPE: if (l != 1) return DHCP_ERR_FORMAT; if (!r.type) r.type = v[0]; break;
        case DHO_SERVER: if (l != 4) return DHCP_ERR_FORMAT; if (!r.has_server) { memcpy(r.server, v, 4); r.has_server = 1; } break;
        case DHO_LEASE: if (l != 4) return DHCP_ERR_FORMAT; if (!r.has_lease) { r.lease = be32(v); r.has_lease = 1; } break;
        case DHO_T1: if (l != 4) return DHCP_ERR_FORMAT; if (!r.has_t1) { r.t1 = be32(v); r.has_t1 = 1; } break;
        case DHO_T2: if (l != 4) return DHCP_ERR_FORMAT; if (!r.has_t2) { r.t2 = be32(v); r.has_t2 = 1; } break;
        case DHO_MASK: if (l != 4) return DHCP_ERR_FORMAT; if (!r.has_mask) { memcpy(r.mask, v, 4); r.has_mask = 1; } break;
        case DHO_ROUTER: if (l < 4 || (l & 3)) return DHCP_ERR_FORMAT; if (!r.has_router) { memcpy(r.router, v, 4); r.has_router = 1; } break;
        case DHO_DNS: if (l < 4 || (l & 3)) return DHCP_ERR_FORMAT; if (!r.has_dns) { memcpy(r.dns, v, 4); r.has_dns = 1; } break;
        default: break;
        }
        i += l;
    }
    if (!r.type) return DHCP_ERR_OPTIONS;
    if (r.has_mask && !mask_ok(r.mask)) return DHCP_ERR_OPTIONS;
    if (r.type == DHCP_OFFER || r.type == DHCP_ACK) {
        if (!addr_ok(r.yiaddr) || !r.has_server || !r.has_lease) return DHCP_ERR_OPTIONS;
    }
    *o = r;
    return 0;
}

void dhcp_times(const dhcp_info_t *i, uint32_t *lease, uint32_t *t1, uint32_t *t2)
{
    uint32_t l = i->lease;
    *lease = l;
    if (l == 0xFFFFFFFFu) { *t1 = *t2 = 0xFFFFFFFFu; return; }
    uint32_t d1 = l / 2, d2 = (uint32_t)((uint64_t)l * 7 / 8);
    uint32_t a = i->has_t1 ? i->t1 : d1, b = i->has_t2 ? i->t2 : d2;
    if (!(a < b && b < l)) { a = d1; b = d2; }
    *t1 = a; *t2 = b;
}
