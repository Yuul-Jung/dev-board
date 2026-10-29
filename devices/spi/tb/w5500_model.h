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
#ifndef W5500_MODEL_H
#define W5500_MODEL_H

#define V(msg) do { viol++; printf("  viol: %s\n", msg); } while (0)
static int viol, fails;
#define CHECK(c, msg) do { if (!(c)) { fails++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); } } while (0)

/* ---------- 모델 상태 ---------- */
static uint8_t C[0x40], S[8][0x30];
static uint8_t txb[8][16384], rxb[8][16384];
static uint16_t rx_rd_chip[8];
static uint16_t pkt_end[8][16]; static int npkt[8];   /* UDP: 주입된 패킷의 끝 포인터(패킷 경계) */
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
typedef void (*peer_fn)(int sock, int proto, const uint8_t ip[4], uint16_t port, const uint8_t *d, unsigned len);
static peer_fn peer_hook;
static void (*tick_hook)(void);   /* 시험이 설정: 드라이버가 기다릴 때마다(지연 호출) 불려 데이터를 조금씩 주입하는 데 씀 */
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
    memset(rx_rd_chip, 0, sizeof rx_rd_chip); memset(npkt, 0, sizeof npkt); memset(pend_cmd, 0, sizeof pend_cmd); memset(cr_busy, 0, sizeof cr_busy); memset(send_pend, 0, sizeof send_pend);
    memset(conn_pend, 0, sizeof conn_pend); memset(disc_pend, 0, sizeof disc_pend); memset(tx_busy_left, 0, sizeof tx_busy_left);
    C[0x19] = 0x07; C[0x1A] = 0xD0; C[0x1B] = 0x08; C[0x1C] = 0x28; C[0x26] = 0xFF; C[0x27] = 0xFF;
    C[0x2E] = (uint8_t)(0xB8 | (link_up ? 1 : 0) | 2 | 4);
    C[0x39] = 0x04;
    for (int n = 0; n < 8; n++) { S[n][0x1E] = 2; S[n][0x1F] = 2; S[n][0x2C] = 0xFF; S[n][0x16] = 0x80; memset(&S[n][6], 0xFF, 6); }
}
static void reset_model(void)
{
    viol = 0; peer_hook = 0; tick_hook = 0; nflog = 0; nwire = 0; n_recv_cmd = 0; n_cmds = 0; n_discon = 0; n_close = 0; drop_off = -1; drop_bsb = -1; now_us = 1000000; ready_at = 0; rst_lvl = 1; was_reset = 0;
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
void model_delay(void *c, uint32_t us) { (void)c; now_us += us; if (tick_hook) tick_hook(); }

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
        if (peer_hook && len <= 4096) {
            static uint8_t tmp[4096];
            for (unsigned k = 0; k < len; k++) tmp[k] = txb[n][(uint16_t)(rd + k) % txsize(n)];
            peer_hook(n, mr, &S[n][0x0C], rd16s(n, 0x10), tmp, len);
        }
        break; }
    case 0x40: {
        n_recv_cmd++;
        uint16_t was = rx_rd_chip[n], now = rd16s(n, 0x28), wrp = rd16s(n, 0x2A);
        if ((uint16_t)(now - was) > (uint16_t)(wrp - was)) V("RECV: Sn_RX_RD 가 수신된 양보다 많이 전진");
        if ((S[n][0] & 0x0F) == 2 && now != was) {      /* UDP: WIZnet 문서 절차대로 패킷 경계(헤더 8 + 길이)에서만 멈춰야 한다 */
            int hit = -1;
            for (int i = 0; i < npkt[n]; i++) if (pkt_end[n][i] == now) { hit = i; break; }
            if (hit < 0) V("UDP RECV: Sn_RX_RD 가 패킷 경계(8 + 데이터 길이)가 아님");
            else { for (int i = hit + 1; i < npkt[n]; i++) pkt_end[n][i - hit - 1] = pkt_end[n][i]; npkt[n] -= hit + 1; }
        }
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
static __attribute__((unused)) void inject_udp(int n, const uint8_t ip[4], uint16_t port, const uint8_t *d, unsigned len)
{
    uint8_t h[8]; memcpy(h, ip, 4); h[4] = (uint8_t)(port >> 8); h[5] = (uint8_t)port; h[6] = (uint8_t)(len >> 8); h[7] = (uint8_t)len;   /* 상위 바이트 먼저 (D3) */
    inject_rx(n, h, 8); if (len) inject_rx(n, d, len);
    if (npkt[n] < 16) pkt_end[n][npkt[n]++] = rd16s(n, 0x2A);
}
static __attribute__((unused)) void force_tx_ptr(int n, uint16_t p) { wr16s(n, 0x22, p); wr16s(n, 0x24, p); }
static __attribute__((unused)) void force_rx_ptr(int n, uint16_t p) { wr16s(n, 0x28, p); wr16s(n, 0x2A, p); rx_rd_chip[n] = p; npkt[n] = 0; }
static __attribute__((unused)) int open_tcp_established(int n, unsigned port)
{
    if (w5500_sock_open(&W, n, W5500_PROTO_TCP, (uint16_t)port) != 0) return -1;
    return w5500_tcp_connect(&W, n, PEER, 5000, 10000);
}


/* 피어(상대 장치) 훅: 드라이버가 SEND 로 보낸 데이터가 와이어에 올라간 직후 호출된다. 시험이 여기서 응답(inject_rx/inject_udp)을 주입한다. */
#endif
