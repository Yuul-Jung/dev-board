#include "w5500.h"
#include <string.h>

#define TRY(x) do { int e_ = (x); if (e_ != QSPI_OK) return e_; } while (0)
#define POLL_STEP_US 50u

static void us(w5500_t *w, uint32_t t) { if (w->delay_us) w->delay_us(w->ctx, t); }

/* 프레임 하나: 주소 2바이트(MSB 먼저) + 제어 1바이트 + 데이터. SCSn 은 프레임 내내 Low (p.14, p.19, p.22) */
static int frame(w5500_t *w, uint8_t bsb, uint16_t off, int wr, const uint8_t *tx, uint8_t *rx, size_t len)
{
    uint8_t h[3] = { (uint8_t)(off >> 8), (uint8_t)off, W5500_CTRL(bsb, wr ? W5500_RWB_WRITE : W5500_RWB_READ, W5500_OM_VDM) };
    qspi_cs(w->s, w->cs, 1);
    int r = qspi_xfer(w->s, h, 0, 3);
    if (r == QSPI_OK && len) r = wr ? qspi_xfer(w->s, tx, 0, len) : qspi_xfer(w->s, 0, rx, len);
    qspi_cs(w->s, w->cs, 0);
    return r;
}
int w5500_read(w5500_t *w, uint8_t bsb, uint16_t off, uint8_t *buf, size_t len)
{
    if (!w || !buf || !len || bsb > 0x1F) return QSPI_ERR_ARGS;
    return frame(w, bsb, off, 0, 0, buf, len);
}
int w5500_write(w5500_t *w, uint8_t bsb, uint16_t off, const uint8_t *buf, size_t len)
{
    if (!w || !buf || !len || bsb > 0x1F) return QSPI_ERR_ARGS;
    return frame(w, bsb, off, 1, buf, 0, len);
}
int w5500_rd8(w5500_t *w, uint8_t bsb, uint16_t off, uint8_t *v) { return w5500_read(w, bsb, off, v, 1); }
int w5500_wr8(w5500_t *w, uint8_t bsb, uint16_t off, uint8_t v) { return w5500_write(w, bsb, off, &v, 1); }
int w5500_wr16(w5500_t *w, uint8_t bsb, uint16_t off, uint16_t v)
{
    uint8_t b[2] = { (uint8_t)(v >> 8), (uint8_t)v };
    return w5500_write(w, bsb, off, b, 2);
}
/* TX_FSR·RX_RSR 은 두 번 이상 읽어 같은 값이 나올 때까지 (p.57, p.58 권장) */
int w5500_rd16(w5500_t *w, uint8_t bsb, uint16_t off, uint16_t *v)
{
    uint16_t prev = 0;
    for (int i = 0; i < 8; i++) {
        uint8_t b[2];
        TRY(w5500_read(w, bsb, off, b, 2));
        uint16_t cur = (uint16_t)((b[0] << 8) | b[1]);
        if (i > 0 && cur == prev) { *v = cur; return QSPI_OK; }
        prev = cur;
    }
    return W5500_ERR_VERIFY;
}

static int wr_verify(w5500_t *w, uint8_t bsb, uint16_t off, const uint8_t *v, unsigned len)
{
    uint8_t b[6];
    TRY(w5500_write(w, bsb, off, v, len));
    TRY(w5500_read(w, bsb, off, b, len));
    return memcmp(b, v, len) == 0 ? QSPI_OK : W5500_ERR_VERIFY;
}

static int kb_ok(uint8_t k) { return k == 0 || k == 1 || k == 2 || k == 4 || k == 8 || k == 16; }

int w5500_init(w5500_t *w, const w5500_cfg_t *c)
{
    if (!w || !c) return QSPI_ERR_ARGS;
    unsigned st = 0, sr = 0;
    for (int i = 0; i < W5500_NSOCK; i++) {
        if (!kb_ok(c->tx_kb[i]) || !kb_ok(c->rx_kb[i])) return QSPI_ERR_ARGS;
        st += c->tx_kb[i]; sr += c->rx_kb[i];
    }
    if (st > 16 || sr > 16) return QSPI_ERR_ARGS;           /* p.32, p.56: 합계 16 KB 초과 시 오류 */
    int keep = (st == 0 && sr == 0);

    if (w->rst) {
        w->rst(w->ctx, 0); us(w, W5500_RESET_LOW_US);          /* TRC ≥ 500 µs */
        w->rst(w->ctx, 1); us(w, W5500_RESET_WAIT_US);         /* TPL ≤ 1 ms */
    } else {
        TRY(w5500_wr8(w, W5500_BSB_COMMON, W5500_MR, W5500_MR_RST));
        int done = 0;                                          /* 소프트 리셋 후 대기 시간은 문서에 없음 → MR.RST 자동 해제를 폴링 */
        for (int i = 0; i < 200 && !done; i++) {
            uint8_t m;
            TRY(w5500_rd8(w, W5500_BSB_COMMON, W5500_MR, &m));
            if (!(m & W5500_MR_RST)) done = 1; else us(w, 100);
        }
        if (!done) return W5500_ERR_TIMEOUT;
    }
    uint8_t ver;
    TRY(w5500_rd8(w, W5500_BSB_COMMON, W5500_VERSIONR, &ver));
    if (ver != W5500_VERSION) return W5500_ERR_VERSION;

    TRY(wr_verify(w, W5500_BSB_COMMON, W5500_GAR, c->gw, 4));
    TRY(wr_verify(w, W5500_BSB_COMMON, W5500_SUBR, c->mask, 4));
    TRY(wr_verify(w, W5500_BSB_COMMON, W5500_SHAR, c->mac, 6));
    TRY(wr_verify(w, W5500_BSB_COMMON, W5500_SIPR, c->ip, 4));
    if (c->rtr) {
        uint8_t b[2] = { (uint8_t)(c->rtr >> 8), (uint8_t)c->rtr };
        TRY(wr_verify(w, W5500_BSB_COMMON, W5500_RTR, b, 2));
    }
    if (c->rcr) TRY(wr_verify(w, W5500_BSB_COMMON, W5500_RCR, &c->rcr, 1));

    for (int i = 0; i < W5500_NSOCK; i++) { w->txsz[i] = 2048; w->rxsz[i] = 2048; w->proto[i] = 0; }   /* 리셋 값 2 KB (p.55, p.56) */
    if (!keep) {
        /* 합계가 도중에 16 KB 를 넘지 않게: 먼저 리셋 값(2 KB) 이하로 줄이는 소켓, 그 다음 늘리는 소켓 */
        for (int pass = 0; pass < 2; pass++)
            for (int i = 0; i < W5500_NSOCK; i++) {
                uint8_t rxv = c->rx_kb[i], txv = c->tx_kb[i];
                if (pass == 0) { if (rxv > 2) rxv = 2; if (txv > 2) txv = 2; }          /* 1차: 전부 2 KB 이하로 */
                else if (rxv <= 2 && txv <= 2) continue;                                 /* 2차: 2 KB 를 넘는 소켓만 늘림 */
                TRY(wr_verify(w, W5500_BSB_SOCK_REG(i), W5500_Sn_RXBUF_SIZE, &rxv, 1));
                TRY(wr_verify(w, W5500_BSB_SOCK_REG(i), W5500_Sn_TXBUF_SIZE, &txv, 1));
                w->rxsz[i] = (uint16_t)(rxv * 1024u); w->txsz[i] = (uint16_t)(txv * 1024u);
            }
    }
    return QSPI_OK;
}

int w5500_link(w5500_t *w, int *up, int *speed100, int *full)
{
    uint8_t p;
    TRY(w5500_rd8(w, W5500_BSB_COMMON, W5500_PHYCFGR, &p));
    if (up) *up = !!(p & W5500_PHY_LNK);
    if (speed100) *speed100 = !!(p & W5500_PHY_SPD);
    if (full) *full = !!(p & W5500_PHY_DPX);
    return QSPI_OK;
}

int w5500_sock_state(w5500_t *w, unsigned n, uint8_t *sr)
{
    if (!w || !sr || n >= W5500_NSOCK) return QSPI_ERR_ARGS;
    return w5500_rd8(w, W5500_BSB_SOCK_REG(n), W5500_Sn_SR, sr);
}
int w5500_clear_ir(w5500_t *w, unsigned n, uint8_t mask)
{
    if (!w || n >= W5500_NSOCK) return QSPI_ERR_ARGS;
    return w5500_wr8(w, W5500_BSB_SOCK_REG(n), W5500_Sn_IR, (uint8_t)(mask & W5500_IR_ALL));   /* 1 을 써서 해제 */
}

/* 명령을 쓰고 칩이 받아들여 Sn_CR 을 0 으로 되돌릴 때까지 기다림 (p.47) */
static int command(w5500_t *w, unsigned n, uint8_t cr)
{
    TRY(w5500_wr8(w, W5500_BSB_SOCK_REG(n), W5500_Sn_CR, cr));
    for (uint32_t t = 0; t <= 20000u; t += 10u) {
        uint8_t v;
        TRY(w5500_rd8(w, W5500_BSB_SOCK_REG(n), W5500_Sn_CR, &v));
        if (v == 0) return QSPI_OK;
        us(w, 10);
    }
    return W5500_ERR_TIMEOUT;
}

int w5500_sock_open(w5500_t *w, unsigned n, uint8_t proto, uint16_t port)
{
    if (!w || n >= W5500_NSOCK || (proto != W5500_PROTO_TCP && proto != W5500_PROTO_UDP) || port == 0) return QSPI_ERR_ARGS;
    if (!w->txsz[n] || !w->rxsz[n]) return QSPI_ERR_ARGS;
    uint8_t sr;
    TRY(w5500_sock_state(w, n, &sr));
    if (sr != W5500_SR_CLOSED) return W5500_ERR_STATE;
    TRY(w5500_wr8(w, W5500_BSB_SOCK_REG(n), W5500_Sn_MR, proto));
    TRY(w5500_wr16(w, W5500_BSB_SOCK_REG(n), W5500_Sn_PORT, port));      /* OPEN 전에 설정 (p.53) */
    TRY(w5500_clear_ir(w, n, W5500_IR_ALL));
    TRY(command(w, n, W5500_CR_OPEN));
    TRY(w5500_sock_state(w, n, &sr));
    uint8_t want = proto == W5500_PROTO_TCP ? W5500_SR_INIT : W5500_SR_UDP;
    if (sr != want) { command(w, n, W5500_CR_CLOSE); return W5500_ERR_STATE; }
    w->proto[n] = proto;
    return QSPI_OK;
}

int w5500_tcp_listen(w5500_t *w, unsigned n)
{
    if (!w || n >= W5500_NSOCK) return QSPI_ERR_ARGS;
    uint8_t sr;
    TRY(w5500_sock_state(w, n, &sr));
    if (sr != W5500_SR_INIT || w->proto[n] != W5500_PROTO_TCP) return W5500_ERR_STATE;
    TRY(command(w, n, W5500_CR_LISTEN));
    TRY(w5500_sock_state(w, n, &sr));
    return sr == W5500_SR_LISTEN ? QSPI_OK : W5500_ERR_STATE;
}

int w5500_tcp_connect(w5500_t *w, unsigned n, const uint8_t ip[4], uint16_t port, uint32_t timeout_us)
{
    if (!w || !ip || n >= W5500_NSOCK || port == 0) return QSPI_ERR_ARGS;
    uint8_t sr;
    TRY(w5500_sock_state(w, n, &sr));
    if (sr != W5500_SR_INIT || w->proto[n] != W5500_PROTO_TCP) return W5500_ERR_STATE;
    TRY(w5500_write(w, W5500_BSB_SOCK_REG(n), W5500_Sn_DIPR, ip, 4));
    TRY(w5500_wr16(w, W5500_BSB_SOCK_REG(n), W5500_Sn_DPORT, port));
    TRY(command(w, n, W5500_CR_CONNECT));
    for (uint32_t t = 0; t <= timeout_us; t += POLL_STEP_US) {
        uint8_t ir;
        TRY(w5500_sock_state(w, n, &sr));
        TRY(w5500_rd8(w, W5500_BSB_SOCK_REG(n), W5500_Sn_IR, &ir));
        if (sr == W5500_SR_ESTABLISHED) { TRY(w5500_clear_ir(w, n, W5500_IR_CON)); return QSPI_OK; }
        if (ir & W5500_IR_TIMEOUT) { TRY(w5500_clear_ir(w, n, W5500_IR_TIMEOUT)); return W5500_ERR_PEER; }
        if (sr == W5500_SR_CLOSED) return W5500_ERR_PEER;
        us(w, POLL_STEP_US);
    }
    return W5500_ERR_TIMEOUT;
}

int w5500_close(w5500_t *w, unsigned n)
{
    if (!w || n >= W5500_NSOCK) return QSPI_ERR_ARGS;
    TRY(command(w, n, W5500_CR_CLOSE));            /* DISCON 없이 Sn_SR 만 CLOSED (p.48) */
    TRY(w5500_clear_ir(w, n, W5500_IR_ALL));
    w->proto[n] = 0;
    return QSPI_OK;
}

int w5500_tcp_disconnect(w5500_t *w, unsigned n, uint32_t timeout_us)
{
    if (!w || n >= W5500_NSOCK) return QSPI_ERR_ARGS;
    uint8_t sr;
    TRY(w5500_sock_state(w, n, &sr));
    if (w->proto[n] != W5500_PROTO_TCP) return W5500_ERR_STATE;
    if (sr != W5500_SR_CLOSED) {
        TRY(command(w, n, W5500_CR_DISCON));
        for (uint32_t t = 0; t <= timeout_us; t += POLL_STEP_US) {
            TRY(w5500_sock_state(w, n, &sr));
            if (sr == W5500_SR_CLOSED) break;
            us(w, POLL_STEP_US);
        }
    }
    return w5500_close(w, n);                      /* 끝났어도 CLOSE 로 IR·상태를 정리 */
}

/* 버퍼 접근: 소켓 버퍼 블록의 경계에서 프레임을 나눈다. 주소는 포인터 값 그대로(0x0000~0xFFFF 어디든 접근 가능, p.28).
 * 한 프레임 안에서 블록 끝을 넘을 때의 동작은 문서에 없어서 경계마다 프레임을 새로 시작한다(설계). */
static int buf_xfer(w5500_t *w, uint8_t bsb, unsigned sz, uint16_t ptr, int wr, const uint8_t *tx, uint8_t *rx, unsigned len)
{
    while (len) {
        unsigned room = sz - (ptr % sz);
        unsigned c = len < room ? len : room;
        TRY(frame(w, bsb, ptr, wr, tx, rx, c));
        ptr = (uint16_t)(ptr + c); len -= c;
        if (tx) tx += c;
        if (rx) rx += c;
    }
    return QSPI_OK;
}

int w5500_send(w5500_t *w, unsigned n, const uint8_t *data, unsigned len, uint32_t timeout_us)
{
    if (!w || !data || n >= W5500_NSOCK || len == 0 || len > w->txsz[n]) return QSPI_ERR_ARGS;
    uint8_t want = w->proto[n] == W5500_PROTO_TCP ? W5500_SR_ESTABLISHED : w->proto[n] == W5500_PROTO_UDP ? W5500_SR_UDP : 0xFF;
    uint32_t t;
    uint16_t fsr = 0;
    for (t = 0; ; t += POLL_STEP_US) {
        uint8_t sr;
        TRY(w5500_sock_state(w, n, &sr));
        if (sr != want) return W5500_ERR_STATE;
        TRY(w5500_rd16(w, W5500_BSB_SOCK_REG(n), W5500_Sn_TX_FSR, &fsr));
        if (fsr >= len) break;
        if (t >= timeout_us) return W5500_ERR_NOSPACE;
        us(w, POLL_STEP_US);
    }
    uint16_t wr;
    TRY(w5500_rd16(w, W5500_BSB_SOCK_REG(n), W5500_Sn_TX_WR, &wr));
    TRY(buf_xfer(w, W5500_BSB_SOCK_TX(n), w->txsz[n], wr, 1, data, 0, len));
    TRY(w5500_wr16(w, W5500_BSB_SOCK_REG(n), W5500_Sn_TX_WR, (uint16_t)(wr + len)));     /* 16비트 올림은 버림 (p.57) */
    TRY(command(w, n, W5500_CR_SEND));
    for (t = 0; t <= timeout_us; t += POLL_STEP_US) {
        uint8_t ir;
        TRY(w5500_rd8(w, W5500_BSB_SOCK_REG(n), W5500_Sn_IR, &ir));
        if (ir & W5500_IR_SEND_OK) { TRY(w5500_clear_ir(w, n, W5500_IR_SEND_OK)); return (int)len; }
        if (ir & W5500_IR_TIMEOUT) { TRY(w5500_clear_ir(w, n, W5500_IR_TIMEOUT)); return W5500_ERR_PEER; }
        us(w, POLL_STEP_US);
    }
    return W5500_ERR_TIMEOUT;
}

int w5500_udp_sendto(w5500_t *w, unsigned n, const uint8_t ip[4], uint16_t port, const uint8_t *data, unsigned len, uint32_t timeout_us)
{
    if (!w || !ip || n >= W5500_NSOCK || port == 0) return QSPI_ERR_ARGS;
    if (w->proto[n] != W5500_PROTO_UDP) return W5500_ERR_STATE;
    TRY(w5500_write(w, W5500_BSB_SOCK_REG(n), W5500_Sn_DIPR, ip, 4));
    TRY(w5500_wr16(w, W5500_BSB_SOCK_REG(n), W5500_Sn_DPORT, port));
    return w5500_send(w, n, data, len, timeout_us);
}

int w5500_rx_avail(w5500_t *w, unsigned n, uint16_t *avail)
{
    if (!w || !avail || n >= W5500_NSOCK) return QSPI_ERR_ARGS;
    return w5500_rd16(w, W5500_BSB_SOCK_REG(n), W5500_Sn_RX_RSR, avail);
}

int w5500_recv(w5500_t *w, unsigned n, uint8_t *buf, unsigned maxlen)
{
    if (!w || !buf || n >= W5500_NSOCK || maxlen == 0) return QSPI_ERR_ARGS;
    uint16_t avail, rd;
    TRY(w5500_rx_avail(w, n, &avail));
    if (avail == 0) return 0;
    unsigned len = avail < maxlen ? avail : maxlen;
    TRY(w5500_rd16(w, W5500_BSB_SOCK_REG(n), W5500_Sn_RX_RD, &rd));
    TRY(buf_xfer(w, W5500_BSB_SOCK_RX(n), w->rxsz[n], rd, 0, 0, buf, len));
    TRY(w5500_wr16(w, W5500_BSB_SOCK_REG(n), W5500_Sn_RX_RD, (uint16_t)(rd + len)));
    TRY(command(w, n, W5500_CR_RECV));                 /* 새 Sn_RX_RD 를 칩에 알림 (p.58) */
    return (int)len;
}

int w5500_udp_recvfrom(w5500_t *w, unsigned n, uint8_t ip[4], uint16_t *port, uint8_t *buf, unsigned maxlen)
{
    if (!w || !ip || !port || !buf || n >= W5500_NSOCK || maxlen == 0) return QSPI_ERR_ARGS;
    if (w->proto[n] != W5500_PROTO_UDP) return W5500_ERR_STATE;
    uint16_t avail, rd;
    TRY(w5500_rx_avail(w, n, &avail));
    if (avail < W5500_UDP_HDR) return W5500_ERR_NOPKT;
    TRY(w5500_rd16(w, W5500_BSB_SOCK_REG(n), W5500_Sn_RX_RD, &rd));
    uint8_t h[W5500_UDP_HDR];
    TRY(buf_xfer(w, W5500_BSB_SOCK_RX(n), w->rxsz[n], rd, 0, 0, h, W5500_UDP_HDR));
    unsigned dlen = ((unsigned)h[6] << 8) | h[7];
    if (W5500_UDP_HDR + dlen > w->rxsz[n]) return W5500_ERR_FRAME;
    if (avail < W5500_UDP_HDR + dlen) return W5500_ERR_NOPKT;
    unsigned c = dlen < maxlen ? dlen : maxlen;
    if (c) TRY(buf_xfer(w, W5500_BSB_SOCK_RX(n), w->rxsz[n], (uint16_t)(rd + W5500_UDP_HDR), 0, 0, buf, c));
    memcpy(ip, h, 4);
    *port = (uint16_t)(((unsigned)h[4] << 8) | h[5]);
    TRY(w5500_wr16(w, W5500_BSB_SOCK_REG(n), W5500_Sn_RX_RD, (uint16_t)(rd + W5500_UDP_HDR + dlen)));
    TRY(command(w, n, W5500_CR_RECV));
    return (int)c;
}
