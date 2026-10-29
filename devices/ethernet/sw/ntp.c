#include "ntp.h"
#include <string.h>

static void put32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v; }
static uint32_t get32(const uint8_t *p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }

int ntp_build_request(uint8_t pkt[NTP_PACKET_LEN], uint32_t xmit_sec, uint32_t xmit_frac)
{
    if (!pkt) return QSPI_ERR_ARGS;
    memset(pkt, 0, NTP_PACKET_LEN);
    pkt[0] = (uint8_t)((0u << 6) | (NTP_VERSION << 3) | NTP_MODE_CLIENT);
    put32(pkt + NTP_OFF_TRANSMIT, xmit_sec);
    put32(pkt + NTP_OFF_TRANSMIT + 4, xmit_frac);
    return QSPI_OK;
}

int ntp_parse(const uint8_t *p, unsigned len, ntp_pkt_t *o)
{
    if (!p || !o) return QSPI_ERR_ARGS;
    if (len < NTP_PACKET_LEN) return NTP_ERR_LEN;
    o->li = (uint8_t)(p[0] >> 6); o->vn = (uint8_t)((p[0] >> 3) & 7); o->mode = (uint8_t)(p[0] & 7);
    o->stratum = p[1]; o->poll = (int8_t)p[2]; o->precision = (int8_t)p[3];
    o->root_delay = get32(p + 4); o->root_disp = get32(p + 8); o->ref_id = get32(p + 12);
    o->ref_sec = get32(p + 16); o->ref_frac = get32(p + 20);
    o->orig_sec = get32(p + 24); o->orig_frac = get32(p + 28);
    o->rx_sec = get32(p + 32); o->rx_frac = get32(p + 36);
    o->tx_sec = get32(p + 40); o->tx_frac = get32(p + 44);
    return QSPI_OK;
}

int ntp_check_reply(const ntp_pkt_t *r, uint32_t orig_sec, uint32_t orig_frac)
{
    if (!r) return QSPI_ERR_ARGS;
    if (r->mode != NTP_MODE_SERVER) return NTP_ERR_MODE;
    if (r->orig_sec != orig_sec || r->orig_frac != orig_frac) return NTP_ERR_ORIGIN;
    if (r->stratum == 0 || r->stratum > 15) return NTP_ERR_STRATUM;
    if (r->li == 3) return NTP_ERR_LI;
    if (r->tx_sec == 0 && r->tx_frac == 0) return NTP_ERR_TX0;
    return 0;
}

int64_t ntp_to_unix(uint32_t s)
{
    return s >= NTP_UNIX_OFFSET ? (int64_t)s - NTP_UNIX_OFFSET : (int64_t)s + 4294967296LL - NTP_UNIX_OFFSET;
}

uint32_t ntp_frac_to_usec(uint32_t frac)
{
    return (uint32_t)(((uint64_t)frac * 1000000u) >> 32);
}
