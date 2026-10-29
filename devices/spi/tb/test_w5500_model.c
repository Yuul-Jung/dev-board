/* W5500 장치 모델 시험. 모델은 W5500 Datasheet v1.0.5(웹 가져오기 요약 경유) 의 규칙을 구현하고, 어기면 viol 에 기록한다.
 * 구현한 규칙: SPI 프레임(주소 2B + 제어 1B + 데이터, SCSn 은 프레임 내내 Low), OM=00(VDM)만 사용, BSB 예약값 금지, 주소 자동 증가,
 * 읽기 전용 레지스터에 쓰기 금지(VERSIONR, Sn_SR, TX_FSR, RX_RSR, TX_RD, RX_WR — 일부는 문서 표기를 기억에 의존, TODO 참조),
 * 버퍼 크기 값(0,1,2,4,8,16)과 합계 16 KB(쓰는 도중에도), RSTn Low ≥ 500 µs · 해제 후 1 ms 안에는 SPI 금지, MR.RST 자동 해제,
 * 소켓 명령 전제(OPEN: MR 프로토콜·Sn_PORT ≠ 0·CLOSED, LISTEN: INIT, CONNECT: INIT+DIPR/DPORT, SEND: ESTABLISHED/UDP, TX 쓰기량 ≤ 여유, UDP 는 DIPR/DPORT),
 * RECV 는 RX_RD 가 수신량을 넘어 전진하면 위반, Sn_CR 자동 해제, Sn_IR 은 1 을 써서 해제, TX_FSR/RX_RSR 은 한 번 읽은 값이 틀릴 수 있음(두 번 읽기 권장).
 * 한 프레임이 소켓 버퍼 블록 경계를 넘으면 위반으로 기록(이 동작은 문서에 없어서 드라이버가 피하는지 확인).
 * 상대 장치(피어)의 응답과 수신 데이터는 시험이 주입한다. 이더넷 물리 구간은 모델링하지 않았다. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../sw/w5500.h"

#define V(msg) do { viol++; printf("  viol: %s\n", msg); } while (0)
static int viol, fails;
#define CHECK(c, msg) do { if (!(c)) { fails++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); } } while (0)

/* ---------- 모델 상태 ---------- */
static uint8_t C[0x40], S[8][0x30];
static uint8_t txb[8][16384], rxb[8][16384];
static uint16_t rx_rd_chip[8];
static uint64_t now_us, ready_at, rst_fell_at; static int rst_lvl = 1, was_reset;
static int mr_rst_cnt;
static uint8_t pend_cmd[8];
static int cr_busy[8], send_pend[8], conn_pend[8], conn_mode_now[8], disc_pend[8];
/* 시험이 조정하는 값 */
static int dead, cr_lat, mr_rst_polls, connect_mode, connect_polls, send_lat, peer_dead, tx_busy_polls, discon_hang, glitch, link_up, open_fail;
enum { CONN_ACCEPT, CONN_REFUSE, CONN_TIMEOUT, CONN_NEVER };
static int tx_busy_left[8];

typedef struct { uint8_t bsb, rw; uint16_t off; int n; uint8_t d[8]; } fr_t;
static fr_t flog[4096]; static int nflog;
typedef struct { int sock, proto, len; uint8_t ip[4]; uint16_t port; uint8_t d[2048]; } wire_t;
static wire_t wire[16]; static int nwire;
static int n_recv_cmd, n_cmds, n_discon, n_close, drop_off = -1, drop_bsb = -1;

static uint16_t rd16s(int n, int o) { return (uint16_t)((S[n][o] << 8) | S[n][o + 1]); }
static void wr16s(int n, int o, uint16_t v) { S[n][o] = (uint8_t)(v >> 8); S[n][o + 1] = (uint8_t)v; }
static unsigned txsize(int n) { return S[n][0x1F] * 1024u; }
static unsigned rxsize(int n) { return S[n][0x1E] * 1024u; }
static uint16_t fsr(int n) { unsigned sz = txsize(n), used = (uint16_t)(rd16s(n, 0x24) - rd16s(n, 0x22)); return (uint16_t)(used > sz ? 0 : sz - used); }
static uint16_t rsr(int n) { return (uint16_t)(rd16s(n, 0x2A) - rd16s(n, 0x28)); }

static void full_reset(void)
{
    memset(C, 0, sizeof C); memset(S, 0, sizeof S); memset(txb, 0, sizeof txb); memset(rxb, 0, sizeof rxb);
    memset(rx_rd_chip, 0, sizeof rx_rd_chip); memset(pend_cmd, 0, sizeof pend_cmd); memset(cr_busy, 0, sizeof cr_busy); memset(send_pend, 0, sizeof send_pend);
    memset(conn_pend, 0, sizeof conn_pend); memset(disc_pend, 0, sizeof disc_pend); memset(tx_busy_left, 0, sizeof tx_busy_left);
    C[0x19] = 0x07; C[0x1A] = 0xD0; C[0x1B] = 0x08; C[0x1C] = 0x28; C[0x26] = 0xFF; C[0x27] = 0xFF;
    C[0x2E] = (uint8_t)(0xB8 | (link_up ? 1 : 0) | 2 | 4);
    C[0x39] = 0x04;
    for (int n = 0; n < 8; n++) { S[n][0x1E] = 2; S[n][0x1F] = 2; S[n][0x2C] = 0xFF; S[n][0x16] = 0x80; memset(&S[n][6], 0xFF, 6); }
}
static void reset_model(void)
{
    viol = 0; nflog = 0; nwire = 0; n_recv_cmd = 0; n_cmds = 0; n_discon = 0; n_close = 0; drop_off = -1; drop_bsb = -1; now_us = 1000000; ready_at = 0; rst_lvl = 1; was_reset = 0;
    dead = 0; cr_lat = 0; mr_rst_polls = 1; connect_mode = CONN_ACCEPT; connect_polls = 1; send_lat = 0; peer_dead = 0; tx_busy_polls = 0; discon_hang = 0; glitch = 0; link_up = 1; open_fail = 0;
    mr_rst_cnt = 0;
    full_reset();
}
static void sum_check(void)
{
    unsigned t = 0, r = 0;
    for (int n = 0; n < 8; n++) { t += S[n][0x1F]; r += S[n][0x1E]; }
    if (t > 16) V("TX 버퍼 크기 합계 16 KB 초과");
    if (r > 16) V("RX 버퍼 크기 합계 16 KB 초과");
}

/* ---------- 콜백 ---------- */
void model_rst(void *c, int lvl)
{
    (void)c;
    if (!lvl && rst_lvl) rst_fell_at = now_us;
    if (lvl && !rst_lvl) {
        if (now_us - rst_fell_at < 500) V("RSTn Low 가 500 µs 미만 (TRC)");
        full_reset(); ready_at = now_us + 1000; was_reset = 1;
    }
    rst_lvl = lvl;
}
void model_delay(void *c, uint32_t us) { (void)c; now_us += us; }

/* ---------- 소켓 명령 ---------- */
static void apply_command(int n, uint8_t cr)
{
    uint8_t sr = S[n][3], mr = S[n][0] & 0x0F;
    n_cmds++;
    switch (cr) {
    case 0x01:
        if (sr != 0x00) V("OPEN 인데 소켓이 CLOSED 가 아님");
        if (mr != 1 && mr != 2) V("OPEN 전에 Sn_MR 프로토콜 미설정");
        if (rd16s(n, 4) == 0) V("OPEN 전에 Sn_PORT 미설정 (p.53)");
        if (open_fail) S[n][3] = (mr == 1) ? 0x22 : 0x13; else S[n][3] = (mr == 1) ? 0x13 : 0x22;
        break;
    case 0x02:
        if (sr != 0x13) V("LISTEN 인데 INIT 이 아님");
        else S[n][3] = 0x14;
        break;
    case 0x04:
        if (sr != 0x13 || mr != 1) V("CONNECT 인데 TCP INIT 이 아님");
        if (S[n][0x0C] == 0 && S[n][0x0D] == 0 && S[n][0x0E] == 0 && S[n][0x0F] == 0) V("CONNECT 전에 Sn_DIPR 미설정");
        if (rd16s(n, 0x10) == 0) V("CONNECT 전에 Sn_DPORT 미설정");
        S[n][3] = 0x15; conn_pend[n] = connect_polls; conn_mode_now[n] = connect_mode;
        break;
    case 0x08:
        if (mr != 1) V("DISCON 은 TCP 전용");
        n_discon++;
        if (!discon_hang) S[n][3] = 0x00;
        break;
    case 0x10:
        n_close++;
        S[n][3] = 0x00; send_pend[n] = 0; conn_pend[n] = 0;
        break;
    case 0x20: case 0x21: {
        if (cr == 0x21) { if (mr != 2) V("SEND_MAC 은 UDP 전용"); else { int ff = 1; for (int k = 0; k < 6; k++) if (S[n][6 + k] != 0xFF) ff = 0; if (ff) V("SEND_MAC 인데 Sn_DHAR 미설정"); } }
        if ((mr == 1 && sr != 0x17) || (mr == 2 && sr != 0x22) || (mr != 1 && mr != 2)) { V("SEND 인데 소켓 상태가 ESTABLISHED/UDP 가 아님"); break; }
        if (mr == 2 && (S[n][0x0C] + S[n][0x0D] + S[n][0x0E] + S[n][0x0F] == 0 || rd16s(n, 0x10) == 0)) V("UDP SEND 전에 DIPR/DPORT 미설정");
        uint16_t rd = rd16s(n, 0x22), wr = rd16s(n, 0x24);
        unsigned len = (uint16_t)(wr - rd);
        if (len == 0) V("SEND 인데 보낼 데이터 없음 (Sn_TX_WR 이 안 움직임)");
        if (len > txsize(n)) V("TX 버퍼 크기보다 많이 씀");
        if (nwire < 16 && len <= sizeof wire[0].d) {
            wire_t *w = &wire[nwire++]; w->sock = n; w->proto = mr; w->len = (int)len; memcpy(w->ip, &S[n][0x0C], 4); w->port = rd16s(n, 0x10);
            for (unsigned k = 0; k < len; k++) w->d[k] = txb[n][(uint16_t)(rd + k) % txsize(n)];
        }
        wr16s(n, 0x22, wr);                  /* 전송이 끝나면 TX_RD 가 TX_WR 을 따라감 */
        send_pend[n] = send_lat + 1;
        break; }
    case 0x40: {
        n_recv_cmd++;
        uint16_t was = rx_rd_chip[n], now = rd16s(n, 0x28), wrp = rd16s(n, 0x2A);
        if ((uint16_t)(now - was) > (uint16_t)(wrp - was)) V("RECV: Sn_RX_RD 가 수신된 양보다 많이 전진");
        rx_rd_chip[n] = now;
        break; }
    default: V("알 수 없는 Sn_CR 명령"); break;
    }
}

/* ---------- SPI 프레임 처리 ---------- */
static struct { int in, pos; uint8_t bsb, rw, om; uint16_t off; int nd; uint8_t first[8]; } F;
static int bsb_valid(uint8_t b) { return b == 0 || (b % 4) != 0; }

static uint8_t read_byte(uint8_t bsb, uint16_t a, int idx)
{
    if (dead) return 0xFF;
    if (bsb == 0) {
        if (a >= 0x40) return 0;
        if (a == 0 && (C[0] & 0x80)) { if (mr_rst_cnt-- <= 0) { full_reset(); } return C[0]; }
        return C[a];
    }
    int n = (bsb - 1) / 4, t = (bsb - 1) % 4;
    if (t == 0) {
        if (a >= 0x30) return 0;
        if (a == 0x01) {                      /* 명령은 Sn_CR 이 0 으로 돌아올 때(칩이 받아들일 때) 효과가 나타남 */
            if (cr_busy[n] > 0) { cr_busy[n]--; return S[n][1]; }
            if (pend_cmd[n]) { uint8_t c = pend_cmd[n]; pend_cmd[n] = 0; apply_command(n, c); }
            S[n][1] = 0; return 0;
        }
        if (a == 0x03) {
            if (conn_pend[n] > 0 && --conn_pend[n] == 0) {
                if (conn_mode_now[n] == CONN_ACCEPT) { S[n][3] = 0x17; S[n][2] |= 0x01; }
                else if (conn_mode_now[n] == CONN_REFUSE) S[n][3] = 0x00;
                else if (conn_mode_now[n] == CONN_TIMEOUT) { S[n][3] = 0x00; S[n][2] |= 0x08; }
                else conn_pend[n] = 1000000;
            }
            return S[n][3];
        }
        if (a == 0x02 && send_pend[n] > 0 && --send_pend[n] == 0) {
            if (peer_dead) { S[n][2] |= 0x08; if (S[n][0] == 1) S[n][3] = 0x00; } else S[n][2] |= 0x10;
        }
        if (a == 0x20 || a == 0x21 || a == 0x26 || a == 0x27) {
            uint16_t v = (a < 0x24) ? fsr(n) : rsr(n);
            if (a < 0x24 && tx_busy_left[n] > 0 && a == 0x20) tx_busy_left[n]--;
            if (a < 0x24 && tx_busy_left[n] > 0) v = 0;
            if (glitch > 0 && idx == 0 && (a == 0x20 || a == 0x26)) { glitch--; v ^= 0x0100; }
            return (uint8_t)((a & 1) ? v : v >> 8);
        }
        return S[n][a];
    }
    if (t == 1 || t == 2) return 0;            /* 버퍼 읽기: 이 시험에서는 RX 버퍼만 의미 있음 (아래) */
    return 0;
}
static uint8_t read_rx(int n, uint16_t a)
{
    unsigned sz = rxsize(n);
    if (!sz) { V("RX 버퍼 크기 0 인 소켓의 버퍼 읽기"); return 0; }
    return rxb[n][a % sz];
}
static void write_byte(uint8_t bsb, uint16_t a, uint8_t b)
{
    if (bsb == 0) {
        if (a >= 0x40) return;
        if (a == 0x39 || a == 0x2E) { V("읽기 전용 공통 레지스터에 씀 (VERSIONR/PHYCFGR)"); return; }
        if (a == 0) { C[0] = b; if (b & 0x80) mr_rst_cnt = mr_rst_polls; return; }
        C[a] = b; return;
    }
    int n = (bsb - 1) / 4, t = (bsb - 1) % 4;
    if (t == 0) {
        if (a >= 0x30) return;
        if (a == 0x03 || a == 0x20 || a == 0x21 || a == 0x22 || a == 0x23 || a == 0x26 || a == 0x27 || a == 0x2A || a == 0x2B) { V("읽기 전용 소켓 레지스터에 씀 (SR/TX_FSR/TX_RD/RX_RSR/RX_WR)"); return; }
        if (a == 0x01) {
            if (pend_cmd[n]) V("이전 명령이 받아들여지기 전에 Sn_CR 에 씀");
            pend_cmd[n] = b; S[n][1] = b; cr_busy[n] = cr_lat; return;
        }
        if (a == 0x02) { if (b & 0xE0) V("Sn_IR 예약 비트(7:5)에 1 을 씀"); S[n][2] = (uint8_t)(S[n][2] & ~b); return; }
        if (a == 0x1E || a == 0x1F) {
            if (b != 0 && b != 1 && b != 2 && b != 4 && b != 8 && b != 16) V("버퍼 크기 값이 0,1,2,4,8,16 이 아님");
            S[n][a] = b; sum_check(); return;
        }
        S[n][a] = b;
        if (a == 0x24 || a == 0x25) { /* TX_WR 갱신: 여유보다 많이 쓰면 위반은 SEND 에서 */ }
        return;
    }
    if (t == 1) {
        unsigned sz = txsize(n);
        if (!sz) { V("TX 버퍼 크기 0 인 소켓의 버퍼 쓰기"); return; }
        txb[n][a % sz] = b; return;
    }
    V("RX 버퍼에 호스트가 씀");
}

static int dropped(uint8_t bsb, uint16_t a) { return drop_off >= 0 && (int)a == drop_off && (int)bsb == drop_bsb; }
void qspi_cs(qspi_t *s, unsigned i, int a)
{
    (void)s; (void)i;
    if (a) {
        if (F.in) V("CS 가 이미 Low 인데 또 내림");
        if (now_us < ready_at) V("RSTn 해제 후 PLL 잠금(1 ms) 전에 SPI 시작");
        if (!rst_lvl) V("RSTn Low 중에 SPI");
        F.in = 1; F.pos = 0; F.nd = 0; memset(F.first, 0, sizeof F.first);
    } else {
        if (!F.in) { V("CS 해제인데 Low 가 아니었음"); return; }
        if (F.pos < 3) V("프레임이 3바이트 헤더보다 짧음");
        else {
            if (nflog < 4096) { fr_t *f = &flog[nflog++]; f->bsb = F.bsb; f->rw = F.rw; f->off = F.off; f->n = F.nd; memcpy(f->d, F.first, 8); }
            if ((F.bsb % 4) == 2 || (F.bsb % 4) == 3) {
                int n = (F.bsb - 1) / 4; unsigned sz = (F.bsb % 4 == 2) ? txsize(n) : rxsize(n);
                if (sz && F.nd && (F.off % sz) + (unsigned)F.nd > sz) V("한 프레임이 소켓 버퍼 블록 경계를 넘음 (문서에 동작 없음)");
            }
        }
        F.in = 0;
    }
}
int qspi_xfer(qspi_t *s, const uint8_t *tx, uint8_t *rx, size_t n)
{
    (void)s;
    if (!F.in) V("CS 밖에서 전송");
    for (size_t i = 0; i < n; i++) {
        uint8_t b = tx ? tx[i] : 0x00, out = 0;
        if (F.pos == 0) F.off = (uint16_t)(b << 8);
        else if (F.pos == 1) F.off |= b;
        else if (F.pos == 2) {
            F.bsb = b >> 3; F.rw = (b >> 2) & 1; F.om = b & 3;
            if (F.om != 0) V("OM != 00 (이 드라이버는 VDM 만 씀)");
            if (!bsb_valid(F.bsb)) V("예약된 BSB");
        } else {
            uint16_t a = (uint16_t)(F.off + (F.pos - 3));       /* 주소 자동 증가 */
            if (F.nd < 8) { if (F.rw) F.first[F.nd] = b; }
            if (F.rw) { if (!dropped(F.bsb, a)) write_byte(F.bsb, a, b); }
            else {
                if (F.bsb != 0 && (F.bsb - 1) % 4 == 2) out = read_rx((F.bsb - 1) / 4, a);
                else out = read_byte(F.bsb, a, F.pos - 3);
                if (F.nd < 8) F.first[F.nd] = out;
            }
            F.nd++;
        }
        F.pos++;
        if (rx) rx[i] = out;
    }
    return 0;
}

/* ---------- 시험 도우미 ---------- */
static qspi_t Q; static w5500_t W;
static w5500_cfg_t cfg;
static const uint8_t MAC[6] = { 0x00, 0x08, 0xDC, 0x01, 0x02, 0x03 }, IP[4] = { 192, 168, 0, 11 }, GW[4] = { 192, 168, 0, 1 }, MASK[4] = { 255, 255, 255, 0 }, PEER[4] = { 192, 168, 0, 20 };

static void make_cfg(void)
{
    memset(&cfg, 0, sizeof cfg);
    memcpy(cfg.mac, MAC, 6); memcpy(cfg.ip, IP, 4); memcpy(cfg.gw, GW, 4); memcpy(cfg.mask, MASK, 4);
}
static int start(int with_rst)
{
    reset_model(); make_cfg();
    memset(&W, 0, sizeof W);
    W.s = &Q; W.cs = 0; W.delay_us = model_delay; W.rst = with_rst ? model_rst : 0;
    return w5500_init(&W, &cfg);
}
static void inject_rx(int n, const uint8_t *d, unsigned len)
{
    uint16_t wr = rd16s(n, 0x2A); unsigned sz = rxsize(n);
    for (unsigned i = 0; i < len; i++) rxb[n][(uint16_t)(wr + i) % sz] = d[i];
    wr16s(n, 0x2A, (uint16_t)(wr + len)); S[n][2] |= 0x04;
}
static void force_tx_ptr(int n, uint16_t p) { wr16s(n, 0x22, p); wr16s(n, 0x24, p); }
static void force_rx_ptr(int n, uint16_t p) { wr16s(n, 0x28, p); wr16s(n, 0x2A, p); rx_rd_chip[n] = p; }
static int open_tcp_established(int n, unsigned port)
{
    if (w5500_sock_open(&W, n, W5500_PROTO_TCP, (uint16_t)port) != 0) return -1;
    return w5500_tcp_connect(&W, n, PEER, 5000, 10000);
}

/* ---------- 시험 ---------- */
static void t_init_stream(void)
{
    printf("[init 바이트열]\n");
    reset_model(); make_cfg();
    cfg.tx_kb[0] = 16;
    for (int i = 0; i < 4; i++) cfg.rx_kb[i] = 4;
    memset(&W, 0, sizeof W); W.s = &Q; W.delay_us = model_delay; W.rst = model_rst;
    CHECK(w5500_init(&W, &cfg) == 0, "init 성공");
    CHECK(viol == 0, "init 중 위반 없음");
    CHECK(was_reset, "하드웨어 리셋을 수행");
    /* 손으로 계산한 기대 프레임: (bsb, rw, off, 길이) */
    struct { uint8_t bsb, rw; uint16_t off; int n; uint8_t d[6]; } E[] = {
        { 0, 0, 0x0039, 1, { 0 } },
        { 0, 1, 0x0001, 4, { 192, 168, 0, 1 } },   { 0, 0, 0x0001, 4, { 192, 168, 0, 1 } },
        { 0, 1, 0x0005, 4, { 255, 255, 255, 0 } }, { 0, 0, 0x0005, 4, { 255, 255, 255, 0 } },
        { 0, 1, 0x0009, 6, { 0x00, 0x08, 0xDC, 0x01, 0x02, 0x03 } }, { 0, 0, 0x0009, 6, { 0x00, 0x08, 0xDC, 0x01, 0x02, 0x03 } },
        { 0, 1, 0x000F, 4, { 192, 168, 0, 11 } },  { 0, 0, 0x000F, 4, { 192, 168, 0, 11 } },
    };
    int ne = (int)(sizeof E / sizeof E[0]);
    CHECK(nflog >= ne, "프레임 수 충분");
    for (int i = 0; i < ne && i < nflog; i++) {
        int ok = flog[i].bsb == E[i].bsb && flog[i].rw == E[i].rw && flog[i].off == E[i].off && flog[i].n == E[i].n;
        if (ok && i > 0) ok = memcmp(flog[i].d, E[i].d, (size_t)E[i].n) == 0;
        if (!ok) printf("  프레임 %d 불일치: bsb=%u rw=%u off=%04X n=%d\n", i, flog[i].bsb, flog[i].rw, flog[i].off, flog[i].n);
        CHECK(ok, "init 앞부분 프레임이 손 계산과 같음");
    }
    /* 버퍼 크기 쓰기 순서: 1차(전부 2 KB 이하로), 2차(늘림). 합계는 도중에도 16 KB 이하(모델이 검사) */
    static const uint8_t RX0[8] = { 2, 2, 2, 2, 0, 0, 0, 0 }, TX0[8] = { 2, 0, 0, 0, 0, 0, 0, 0 };
    static const uint8_t RX1[4] = { 4, 4, 4, 4 }, TX1[4] = { 16, 0, 0, 0 };
    struct { int n; uint16_t off; uint8_t v; } X[8 * 2 + 4 * 2]; int nx = 0;
    for (int i = 0; i < 8; i++) { X[nx].n = i; X[nx].off = 0x1E; X[nx++].v = RX0[i]; X[nx].n = i; X[nx].off = 0x1F; X[nx++].v = TX0[i]; }
    for (int i = 0; i < 4; i++) { X[nx].n = i; X[nx].off = 0x1E; X[nx++].v = RX1[i]; X[nx].n = i; X[nx].off = 0x1F; X[nx++].v = TX1[i]; }
    int k = 0;
    for (int i = 0; i < nflog; i++) {
        if (flog[i].rw != 1 || flog[i].bsb == 0 || (flog[i].bsb - 1) % 4 != 0) continue;
        if (flog[i].off != 0x1E && flog[i].off != 0x1F) continue;
        if (k < nx) {
            int ok = (flog[i].bsb - 1) / 4 == X[k].n && flog[i].off == X[k].off && flog[i].d[0] == X[k].v;
            if (!ok) printf("  버퍼 크기 쓰기 %d 불일치: 소켓 %d off %02X 값 %u (기대 %d %02X %u)\n", k, (flog[i].bsb - 1) / 4, flog[i].off, flog[i].d[0], X[k].n, X[k].off, X[k].v);
            CHECK(ok, "버퍼 크기 쓰기 순서·값");
        }
        k++;
    }
    CHECK(k == nx, "버퍼 크기 쓰기 횟수");
    CHECK(W.txsz[0] == 16384 && W.rxsz[3] == 4096 && W.txsz[1] == 0, "드라이버가 소켓 버퍼 크기를 기억");
    CHECK(S[0][0x1F] == 16 && S[2][0x1E] == 4, "칩 레지스터 값");
}
static void t_init_variants(void)
{
    printf("[init 변형·오류]\n");
    CHECK(start(1) == 0, "기본 설정(크기 0 = 리셋 값 유지)");
    CHECK(W.txsz[5] == 2048 && W.rxsz[7] == 2048, "리셋 값 2 KB 유지");
    for (int i = 0; i < nflog; i++) CHECK(!((flog[i].bsb % 4) == 1 && flog[i].rw == 1 && (flog[i].off == 0x1E || flog[i].off == 0x1F)), "크기 0 이면 버퍼 크기 레지스터를 쓰지 않음");
    CHECK(viol == 0, "위반 없음");

    reset_model(); make_cfg(); memset(&W, 0, sizeof W); W.s = &Q; W.delay_us = model_delay; W.rst = model_rst; dead = 1;
    CHECK(w5500_init(&W, &cfg) == W5500_ERR_VERSION, "칩 응답 없음(0xFF) → ERR_VERSION");

    /* 잘못된 설정: SPI 통신 없이 거절 */
    int bad = 0;
    for (int c = 0; c < 4; c++) {
        reset_model(); make_cfg(); memset(&W, 0, sizeof W); W.s = &Q; W.delay_us = model_delay; W.rst = model_rst;
        if (c == 0) cfg.tx_kb[0] = 3;
        if (c == 1) cfg.rx_kb[2] = 5;
        if (c == 2) { cfg.tx_kb[0] = 16; cfg.tx_kb[1] = 1; }
        if (c == 3) { for (int i = 0; i < 8; i++) cfg.rx_kb[i] = 4; }
        int e = w5500_init(&W, &cfg);
        if (e != QSPI_ERR_ARGS || nflog != 0) bad++;
    }
    CHECK(bad == 0, "잘못된 크기 설정 4종은 ARGS, SPI 통신 없음");
    CHECK(w5500_init(0, &cfg) == QSPI_ERR_ARGS && w5500_init(&W, 0) == QSPI_ERR_ARGS, "NULL 인자");

    /* 쓴 값이 읽혀 오지 않으면 ERR_VERIFY */
    { struct { uint8_t bsb; uint16_t off; } D[] = { { 0, 0x0001 }, { 0, 0x0005 }, { 0, 0x000A }, { 0, 0x000F }, { 1, 0x001E }, { 1, 0x001F } };
      int ok = 0;
      for (unsigned i = 0; i < sizeof D / sizeof D[0]; i++) {
          reset_model(); make_cfg(); cfg.tx_kb[0] = 4; cfg.rx_kb[0] = 4; memset(&W, 0, sizeof W); W.s = &Q; W.delay_us = model_delay; W.rst = model_rst;
          drop_bsb = D[i].bsb; drop_off = D[i].off;
          if (w5500_init(&W, &cfg) == W5500_ERR_VERIFY) ok++;
      }
      CHECK(ok == 6, "GAR/SUBR/SHAR/SIPR/RXBUF/TXBUF 쓰기가 반영 안 되면 ERR_VERIFY"); }

    /* 소프트 리셋 경로 */
    reset_model(); make_cfg(); memset(&W, 0, sizeof W); W.s = &Q; W.delay_us = model_delay; mr_rst_polls = 3;
    CHECK(w5500_init(&W, &cfg) == 0, "소프트 리셋: MR.RST 자동 해제를 기다림");
    CHECK(viol == 0, "소프트 리셋 위반 없음");
    reset_model(); make_cfg(); memset(&W, 0, sizeof W); W.s = &Q; W.delay_us = model_delay; mr_rst_polls = 1000000;
    CHECK(w5500_init(&W, &cfg) == W5500_ERR_TIMEOUT, "MR.RST 가 안 풀리면 TIMEOUT");

    /* rtr/rcr */
    reset_model(); make_cfg(); cfg.rtr = 4000; cfg.rcr = 5; memset(&W, 0, sizeof W); W.s = &Q; W.delay_us = model_delay; W.rst = model_rst;
    CHECK(w5500_init(&W, &cfg) == 0 && C[0x19] == 0x0F && C[0x1A] == 0xA0 && C[0x1B] == 5, "RTR = 4000(0x0FA0) 상위 먼저, RCR");
}
static void t_link(void)
{
    printf("[링크]\n");
    int up, sp, fu;
    CHECK(start(1) == 0, "init");
    link_up = 1; full_reset(); C[0x39] = 4;
    CHECK(w5500_link(&W, &up, &sp, &fu) == 0 && up == 1 && sp == 1 && fu == 1, "링크 업·100M·전이중 (LNK=bit0, SPD=bit1, DPX=bit2)");
    C[0x2E] = 0xB8 | 0x02;
    CHECK(w5500_link(&W, &up, &sp, &fu) == 0 && up == 0 && sp == 1 && fu == 0, "링크 다운");
    C[0x2E] = 0xB8 | 0x01;
    CHECK(w5500_link(&W, &up, &sp, 0) == 0 && up == 1 && sp == 0, "10M");
}
static void t_open(void)
{
    printf("[소켓 열기·TCP 연결]\n");
    CHECK(start(1) == 0, "init");
    CHECK(w5500_sock_open(&W, 0, W5500_PROTO_TCP, 0) == QSPI_ERR_ARGS, "포트 0 거절");
    CHECK(w5500_sock_open(&W, 0, 3, 80) == QSPI_ERR_ARGS && w5500_sock_open(&W, 8, W5500_PROTO_TCP, 80) == QSPI_ERR_ARGS, "프로토콜·소켓 번호 범위");
    CHECK(w5500_sock_open(&W, 0, W5500_PROTO_TCP, 5000) == 0 && S[0][3] == 0x13, "TCP OPEN → INIT");
    CHECK(S[0][4] == 0x13 && S[0][5] == 0x88, "Sn_PORT = 5000 → 0x13, 0x88 (p.53 예제)");
    CHECK(w5500_sock_open(&W, 0, W5500_PROTO_TCP, 5000) == W5500_ERR_STATE, "이미 열린 소켓");
    CHECK(w5500_sock_open(&W, 1, W5500_PROTO_UDP, 6000) == 0 && S[1][3] == 0x22, "UDP OPEN → UDP");
    open_fail = 1; CHECK(w5500_sock_open(&W, 2, W5500_PROTO_TCP, 80) == W5500_ERR_STATE, "OPEN 후 상태가 기대와 다르면 오류"); open_fail = 0;
    CHECK(S[2][3] == 0 && n_close >= 1, "실패 후 CLOSE 명령으로 CLOSED 로 정리");
    CHECK(viol == 0, "위반 없음");

    /* TCP 연결 */
    CHECK(start(1) == 0, "init");
    cr_lat = 2; connect_polls = 3;
    CHECK(open_tcp_established(0, 5000) == 0 && S[0][3] == 0x17, "연결 성공 (SR 폴링, Sn_CR 해제 대기)");
    CHECK(S[0][0x0C] == 192 && S[0][0x0F] == 20 && S[0][0x10] == 0x13 && S[0][0x11] == 0x88, "DIPR 192.168.0.20, DPORT 5000");
    CHECK((S[0][2] & 1) == 0, "성공 후 Sn_IR.CON 해제");
    CHECK(viol == 0, "위반 없음");

    connect_mode = CONN_REFUSE; CHECK(start(1) == 0, "init"); connect_mode = CONN_REFUSE;
    CHECK(open_tcp_established(0, 5000) == W5500_ERR_PEER, "연결 거부(CLOSED) → ERR_PEER");
    connect_mode = CONN_TIMEOUT; CHECK(start(1) == 0, "init"); connect_mode = CONN_TIMEOUT;
    CHECK(open_tcp_established(0, 5000) == W5500_ERR_PEER && (S[0][2] & 0x08) == 0, "Sn_IR.TIMEOUT → ERR_PEER, 플래그 해제");
    CHECK(start(1) == 0, "init"); connect_mode = CONN_NEVER;
    CHECK(open_tcp_established(0, 5000) == W5500_ERR_TIMEOUT, "응답 없음 → ERR_TIMEOUT");
    CHECK(start(1) == 0, "init");
    CHECK(w5500_tcp_connect(&W, 0, PEER, 80, 1000) == W5500_ERR_STATE, "INIT 아니면 CONNECT 거절");
    CHECK(w5500_sock_open(&W, 0, W5500_PROTO_UDP, 80) == 0 && w5500_tcp_connect(&W, 0, PEER, 80, 1000) == W5500_ERR_STATE, "UDP 소켓에 CONNECT 거절");
    CHECK(w5500_tcp_listen(&W, 0) == W5500_ERR_STATE, "UDP 소켓에 LISTEN 거절");
    CHECK(w5500_sock_open(&W, 1, W5500_PROTO_TCP, 80) == 0 && w5500_tcp_listen(&W, 1) == 0 && S[1][3] == 0x14, "LISTEN");
    CHECK(w5500_tcp_listen(&W, 1) == W5500_ERR_STATE, "이미 LISTEN 인 소켓에 LISTEN 거절");
    CHECK(w5500_sock_open(&W, 2, W5500_PROTO_TCP, 81) == 0 && w5500_tcp_connect(&W, 2, PEER, 81, 10000) == 0, "소켓 2 연결");
    CHECK(w5500_tcp_connect(&W, 2, PEER, 81, 1000) == W5500_ERR_STATE && w5500_tcp_listen(&W, 2) == W5500_ERR_STATE, "연결된(INIT 아닌) TCP 소켓에 CONNECT/LISTEN 거절");
    CHECK(viol == 0, "위반 없음");
}
static void t_send(void)
{
    printf("[송신]\n");
    CHECK(start(1) == 0, "init");
    CHECK(open_tcp_established(0, 5000) == 0, "연결");
    uint8_t d[40]; for (int i = 0; i < 40; i++) d[i] = (uint8_t)(0x30 + i);
    CHECK(w5500_send(&W, 0, d, 10, 5000) == 10, "10바이트 송신");
    CHECK(nwire == 1 && wire[0].len == 10 && memcmp(wire[0].d, d, 10) == 0 && wire[0].sock == 0, "상대에게 간 바이트가 같음");
    CHECK(rd16s(0, 0x24) == 10 && rd16s(0, 0x22) == 10, "TX_WR 전진, TX_RD 따라감");
    CHECK((S[0][2] & 0x10) == 0, "SEND_OK 해제");
    CHECK(n_cmds >= 3, "명령 발행");

    /* 버퍼 경계(2 KB): 0x07F0 에서 40바이트 → 16 + 24 로 나뉨 */
    force_tx_ptr(0, 0x07F0); nwire = 0;
    CHECK(w5500_send(&W, 0, d, 40, 5000) == 40, "경계 걸친 송신");
    CHECK(nwire == 1 && wire[0].len == 40 && memcmp(wire[0].d, d, 40) == 0, "경계에서 나눠 써도 순서대로 도착");
    CHECK(rd16s(0, 0x24) == 0x0818, "TX_WR = 0x07F0 + 40");
    { int cross = 0, a = 0, b = 0; for (int i = 0; i < nflog; i++) if (flog[i].bsb == 2 && flog[i].rw == 1) { cross++; if (flog[i].off == 0x07F0 && flog[i].n == 16) a = 1; if (flog[i].off == 0x0800 && flog[i].n == 24) b = 1; }
      CHECK(a && b, "TX 버퍼 쓰기 프레임: (0x07F0,16) 와 (0x0800,24)"); CHECK(cross >= 2, "프레임이 둘 이상"); }
    /* 16비트 포인터 올림: 0xFFF0 + 32 → 0x0010 */
    force_tx_ptr(0, 0xFFF0); nwire = 0;
    CHECK(w5500_send(&W, 0, d, 32, 5000) == 32 && rd16s(0, 0x24) == 0x0010, "16비트 올림 버림 (p.57)");
    CHECK(nwire == 1 && memcmp(wire[0].d, d, 32) == 0, "올림 경계 송신 데이터");

    { static uint8_t big[2048]; for (int i = 0; i < 2048; i++) big[i] = (uint8_t)(i ^ 0x5A); force_tx_ptr(0, 0x0123); nwire = 0;
      CHECK(w5500_send(&W, 0, big, 2048, 5000) == 2048, "버퍼 크기와 같은 길이(2048) 송신 (여유 == 길이)");
      CHECK(nwire == 1 && wire[0].len == 2048 && memcmp(wire[0].d, big, 2048) == 0, "2048바이트가 순서대로 도착"); }
    CHECK(w5500_send(&W, 0, d, 0, 100) == QSPI_ERR_ARGS && w5500_send(&W, 0, d, 2049, 100) == QSPI_ERR_ARGS, "길이 0, 버퍼보다 큼 거절");
    CHECK(w5500_send(&W, 0, 0, 5, 100) == QSPI_ERR_ARGS && w5500_send(&W, 9, d, 5, 100) == QSPI_ERR_ARGS, "NULL·소켓 번호");
    CHECK(w5500_send(&W, 3, d, 5, 100) == W5500_ERR_STATE, "열리지 않은 소켓");

    /* 여유 공간 대기 */
    tx_busy_left[0] = 5;
    CHECK(w5500_send(&W, 0, d, 8, 5000) == 8, "TX_FSR 이 잠시 0 이어도 기다렸다 보냄");
    tx_busy_left[0] = 100000;
    CHECK(w5500_send(&W, 0, d, 8, 500) == W5500_ERR_NOSPACE, "계속 가득이면 ERR_NOSPACE");
    tx_busy_left[0] = 0;

    /* 전송 완료 지연·실패 */
    nwire = 0; send_lat = 4;
    CHECK(w5500_send(&W, 0, d, 8, 5000) == 8, "SEND_OK 가 늦게 와도 기다림");
    send_lat = 100000;
    CHECK(w5500_send(&W, 0, d, 8, 300) == W5500_ERR_TIMEOUT, "SEND_OK 가 안 오면 ERR_TIMEOUT");
    CHECK(start(1) == 0 && open_tcp_established(0, 5000) == 0, "재시작");
    peer_dead = 1;
    CHECK(w5500_send(&W, 0, d, 8, 5000) == W5500_ERR_PEER && (S[0][2] & 0x08) == 0, "Sn_IR.TIMEOUT → ERR_PEER, 플래그 해제");
    CHECK(w5500_send(&W, 0, d, 8, 500) == W5500_ERR_STATE, "끊어진 뒤에는 STATE");

    /* TX_FSR 첫 읽기가 틀려도 두 번 읽기로 바로잡음 */
    CHECK(start(1) == 0 && open_tcp_established(0, 5000) == 0, "재시작");
    glitch = 1;
    CHECK(w5500_send(&W, 0, d, 8, 5000) == 8, "TX_FSR 읽기 오류(첫 값이 다름)를 넘김");
    CHECK(viol == 0, "위반 없음");
}
static void t_udp(void)
{
    printf("[UDP]\n");
    CHECK(start(1) == 0, "init");
    CHECK(w5500_sock_open(&W, 2, W5500_PROTO_UDP, 6000) == 0, "UDP 열기");
    uint8_t d[20]; for (int i = 0; i < 20; i++) d[i] = (uint8_t)(0xA0 + i);
    CHECK(w5500_udp_sendto(&W, 2, PEER, 7000, d, 20, 5000) == 20, "UDP 송신");
    CHECK(nwire == 1 && wire[0].proto == 2 && wire[0].port == 7000 && memcmp(wire[0].ip, PEER, 4) == 0 && memcmp(wire[0].d, d, 20) == 0, "목적지 IP·포트·데이터");
    CHECK(w5500_udp_sendto(&W, 2, PEER, 0, d, 20, 5000) == QSPI_ERR_ARGS, "포트 0 거절");
    CHECK(open_tcp_established(0, 80) == 0 && w5500_udp_sendto(&W, 0, PEER, 7000, d, 20, 5000) == W5500_ERR_STATE, "연결된 TCP 소켓에 sendto 거절");
    CHECK(viol == 0, "위반 없음");
}
static void t_recv(void)
{
    printf("[수신]\n");
    CHECK(start(1) == 0, "init");
    CHECK(open_tcp_established(0, 5000) == 0, "연결");
    uint8_t in[100], out[100]; for (int i = 0; i < 100; i++) in[i] = (uint8_t)(i * 3 + 1);
    uint16_t av;
    CHECK(w5500_rx_avail(&W, 0, &av) == 0 && av == 0, "처음엔 없음");
    int rc0 = n_recv_cmd;
    CHECK(w5500_recv(&W, 0, out, 10) == 0 && n_recv_cmd == rc0, "없으면 0 반환, RECV 명령 안 보냄");
    inject_rx(0, in, 100);
    CHECK(w5500_rx_avail(&W, 0, &av) == 0 && av == 100, "수신량 100");
    CHECK(w5500_recv(&W, 0, out, 30) == 30 && memcmp(out, in, 30) == 0, "30바이트");
    CHECK(rd16s(0, 0x28) == 30 && n_recv_cmd == rc0 + 1, "RX_RD 전진 후 RECV 명령");
    CHECK(w5500_recv(&W, 0, out, 30) == 30 && memcmp(out, in + 30, 30) == 0, "다음 30바이트");
    CHECK(w5500_recv(&W, 0, out, 100) == 40 && memcmp(out, in + 60, 40) == 0, "남은 40바이트만 반환");
    CHECK(w5500_recv(&W, 0, out, 100) == 0, "다 읽음");
    CHECK((S[0][2] & 0x04) != 0, "Sn_IR.RECV 는 건드리지 않음(호출자가 해제)");
    CHECK(w5500_clear_ir(&W, 0, W5500_IR_RECV) == 0 && (S[0][2] & 0x04) == 0, "clear_ir");

    /* 버퍼 경계(2 KB) 걸침 + 16비트 올림 */
    force_rx_ptr(0, 0x07F8); inject_rx(0, in, 20);
    CHECK(w5500_recv(&W, 0, out, 20) == 20 && memcmp(out, in, 20) == 0, "경계 걸친 수신");
    CHECK(rd16s(0, 0x28) == 0x080C, "RX_RD = 0x07F8 + 20");
    force_rx_ptr(0, 0xFFF8); inject_rx(0, in, 20);
    CHECK(w5500_recv(&W, 0, out, 20) == 20 && memcmp(out, in, 20) == 0 && rd16s(0, 0x28) == 0x000C, "16비트 올림 경계 수신");

    glitch = 1; inject_rx(0, in, 12);
    CHECK(w5500_recv(&W, 0, out, 100) == 12 && memcmp(out, in, 12) == 0, "RX_RSR 첫 읽기가 틀려도 두 번 읽기로 바로잡음");
    CHECK(w5500_recv(&W, 0, 0, 5) == QSPI_ERR_ARGS && w5500_recv(&W, 0, out, 0) == QSPI_ERR_ARGS && w5500_recv(&W, 8, out, 5) == QSPI_ERR_ARGS, "인자 검사");
    CHECK(viol == 0, "위반 없음");
}
static void t_close(void)
{
    printf("[연결 종료]\n");
    CHECK(start(1) == 0 && open_tcp_established(0, 5000) == 0, "연결");
    CHECK(w5500_tcp_disconnect(&W, 0, 2000) == 0 && S[0][3] == 0 && W.proto[0] == 0, "DISCON → CLOSED");
    CHECK(n_discon == 1, "DISCON 명령을 보냄");
    CHECK(w5500_send(&W, 0, (const uint8_t *)"x", 1, 100) == W5500_ERR_STATE, "닫힌 뒤 송신 거절");
    CHECK(start(1) == 0 && open_tcp_established(0, 5000) == 0, "재연결");
    discon_hang = 1;
    CHECK(w5500_tcp_disconnect(&W, 0, 1000) == 0 && S[0][3] == 0, "DISCON 이 안 끝나면 CLOSE 로 정리");
    CHECK(n_discon == 1 && n_close >= 1, "DISCON 후 CLOSE 명령");
    CHECK(start(1) == 0 && w5500_sock_open(&W, 1, W5500_PROTO_UDP, 9) == 0, "UDP 소켓");
    S[1][2] = 0x1F;
    CHECK(w5500_close(&W, 1) == 0 && S[1][3] == 0 && S[1][2] == 0, "CLOSE 후 IR 해제");
    CHECK(w5500_tcp_disconnect(&W, 1, 100) == W5500_ERR_STATE, "UDP 소켓에 DISCON 거절");
    CHECK(viol == 0, "위반 없음");
}
static void t_frames(void)
{
    printf("[프레임 형식]\n");
    CHECK(start(1) == 0, "init");
    nflog = 0;
    uint8_t v; uint8_t b[3];
    CHECK(w5500_rd8(&W, W5500_BSB_COMMON, W5500_VERSIONR, &v) == 0 && v == 4, "VERSIONR = 0x04");
    CHECK(nflog == 1 && flog[0].bsb == 0 && flog[0].rw == 0 && flog[0].off == 0x0039, "읽기 프레임: BSB 0, RWB 0, 주소 0x0039");
    CHECK(w5500_wr8(&W, W5500_BSB_SOCK_REG(3), W5500_Sn_TTL, 0x55) == 0 && S[3][0x16] == 0x55, "소켓 3 레지스터 쓰기");
    CHECK(flog[1].bsb == 13 && flog[1].rw == 1, "소켓 3 레지스터 BSB = 4×3+1 = 13, RWB 1");
    CHECK(W5500_CTRL(W5500_BSB_SOCK_REG(0), W5500_RWB_WRITE, 0) == 0x0C && W5500_CTRL(W5500_BSB_SOCK_TX(0), W5500_RWB_WRITE, 0) == 0x14 && W5500_CTRL(W5500_BSB_SOCK_RX(0), W5500_RWB_READ, 0) == 0x18, "제어 바이트: 소켓 0 레지스터 쓰기 0x0C, TX 쓰기 0x14, RX 읽기 0x18");
    CHECK(W5500_CTRL(W5500_BSB_SOCK_REG(7), W5500_RWB_WRITE, 0) == 0xEC && W5500_CTRL(W5500_BSB_SOCK_TX(7), W5500_RWB_WRITE, 0) == 0xF4 && W5500_CTRL(W5500_BSB_SOCK_RX(7), W5500_RWB_READ, 0) == 0xF8, "소켓 7: 0xEC, 0xF4, 0xF8");
    CHECK(w5500_read(&W, 0, 0, b, 0) == QSPI_ERR_ARGS && w5500_read(&W, 32, 0, b, 1) == QSPI_ERR_ARGS && w5500_write(&W, 0, 0, 0, 1) == QSPI_ERR_ARGS, "인자 검사");
    CHECK(viol == 0, "위반 없음");
}

int main(void)
{
    t_init_stream(); t_init_variants(); t_link(); t_open(); t_send(); t_udp(); t_recv(); t_close(); t_frames();
    if (fails) { printf("FAIL: %d\n", fails); return 1; }
    printf("ALL PASS (w5500 model)\n");
    return 0;
}
