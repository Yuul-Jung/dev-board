/* nRF24L01+ 장치 모델 시험. 모델은 Nordic Product Specification v1.0 의 규칙을 구현하고, 규칙을 어기면 viol 에 기록한다.
 * 구현한 규칙: Table 20 명령, Table 28 레지스터(예약 비트에 0 외 쓰기 금지, 읽기 전용, STATUS 1 쓰기 해제, EN_CRC 강제), FEATURE 없이 쓴 R_RX_PL_WID/W_ACK_PAYLOAD/NOACK 금지,
 * DPL_Px 는 EN_DPL 과 ENAA_Px 필요, 수신 모드(PWR_UP·PRIM_RX·CE)에서 STATUS 외 레지스터 쓰기 금지, PWR_UP 뒤 1.5 ms 전에 CE 상승 금지(Tpd2stby 하한),
 * CE High 10 µs 미만이면 송신 안 됨, CE 상승 4 µs 이내 CSN 하강 금지(Tpece2csn), 수신은 CE 상승 130 µs 뒤부터, FIFO 3단, MAX_RT 가 서 있으면 송신 안 함.
 * 송신 결과(ACK 성공/실패)와 수신 패킷은 시험이 주입한다. 공중(RF) 구간은 모델링하지 않았다. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../sw/nrf24l01.h"

typedef struct { uint8_t pipe, len, d[32], noack; } pkt_t;
static uint8_t A[0x20][5];
static pkt_t txq[3], rxq[3], ackq[3]; static int ntx, nrx, nack;
static uint64_t now_us, t_pwrup, t_ce_rise; static int ce_lvl, viol, cs_low, ntrans;
static int air_ok = 1, air_ackpay_len = 0, dead = 0;
static uint8_t log_buf[512][34]; static uint8_t log_len[512]; static int nlog;
#define V(msg) do { viol++; printf("  viol: %s\n", msg); } while (0)

static void reset_model(void)
{
    memset(A, 0, sizeof A); ntx = nrx = nack = 0; viol = 0; nlog = 0; ntrans = 0; ce_lvl = 0; now_us = 100000; t_pwrup = 0; t_ce_rise = 0; dead = 0; air_ok = 1; air_ackpay_len = 0;
    A[0][0] = 0x08; A[1][0] = 0x3F; A[2][0] = 0x03; A[3][0] = 0x03; A[4][0] = 0x03; A[5][0] = 0x02; A[6][0] = 0x0E;
    memset(A[0x0A], 0xE7, 5); memset(A[0x0B], 0xC2, 5); A[0x0C][0] = 0xC3; A[0x0D][0] = 0xC4; A[0x0E][0] = 0xC5; A[0x0F][0] = 0xC6; memset(A[0x10], 0xE7, 5);
}
static uint8_t cfg_eff(void) { uint8_t c = A[0][0]; if (A[1][0] & 0x3F) c |= 0x08; return c; }       /* EN_AA 가 하나라도 켜지면 EN_CRC 강제 1 */
static uint8_t status(void)
{
    uint8_t s = (uint8_t)(A[7][0] & 0x70);
    s |= (uint8_t)((nrx ? rxq[0].pipe : 7) << 1);
    if (ntx == 3) s |= 1;
    return s;
}
static uint8_t fifo_status(void)
{
    return (uint8_t)((ntx == 3 ? 0x20 : 0) | (ntx == 0 ? 0x10 : 0) | (nrx == 3 ? 0x02 : 0) | (nrx == 0 ? 0x01 : 0));
}
static int rx_mode(void) { return (cfg_eff() & 3) == 3 && ce_lvl; }
static int reg_w(uint8_t r) { return (r <= 0x10 && r != 8 && r != 9 && r != 7) || (r >= 0x11 && r <= 0x16) || r == 0x1C || r == 0x1D; }
static int reg_width(uint8_t r) { return (r == 0x0A || r == 0x0B || r == 0x10) ? 5 : 1; }
static const uint8_t RSV[0x20] = { [0]=0x80, [1]=0xC0, [2]=0xC0, [3]=0xFC, [5]=0x80, [6]=0x40, [0x11]=0xC0,[0x12]=0xC0,[0x13]=0xC0,[0x14]=0xC0,[0x15]=0xC0,[0x16]=0xC0, [0x1C]=0xC0, [0x1D]=0xF8 };
static void tx_attempt(void)
{
    if (!ntx || (A[7][0] & 0x10)) return;
    pkt_t p = txq[0];
    int ok = air_ok || p.noack;
    if (ok) {
        memmove(&txq[0], &txq[1], sizeof(pkt_t) * 2); ntx--;
        A[7][0] |= 0x20;
        if (!p.noack && air_ackpay_len && nrx < 3) { rxq[nrx].pipe = 0; rxq[nrx].len = (uint8_t)air_ackpay_len; for (int i = 0; i < air_ackpay_len; i++) rxq[nrx].d[i] = (uint8_t)(0xA0 + i); nrx++; A[7][0] |= 0x40; }
    } else { A[7][0] |= 0x10; if ((A[8][0] >> 4) < 15) A[8][0] += 0x10; }
}
void model_ce(void *c, int lvl)
{
    (void)c;
    if (lvl && !ce_lvl) {
        t_ce_rise = now_us;
        if ((A[0][0] & 2) && now_us - t_pwrup < 1500) V("CE 상승이 PWR_UP 후 1.5 ms 이내 (Tpd2stby)");
    }
    if (!lvl && ce_lvl && (A[0][0] & 3) == 2) {                       /* PTX, PWR_UP */
        if (now_us - t_ce_rise >= 10) tx_attempt();
    }
    ce_lvl = lvl;
}
void model_delay(void *c, uint32_t us) { (void)c; now_us += us; }
void qspi_cs(qspi_t *s, unsigned i, int a)
{
    (void)s; (void)i;
    if (a) { if (ce_lvl && now_us - t_ce_rise < 4) V("CE 상승 4 µs 이내 CSN 하강 (Tpece2csn)"); cs_low = 1; } else cs_low = 0;
}
int qspi_xfer(qspi_t *s, const uint8_t *tx, uint8_t *rx, size_t n)
{
    (void)s;
    if (!cs_low) V("CSN 없이 전송");
    if (nlog < 512 && n <= 34) { memcpy(log_buf[nlog], tx, n); log_len[nlog++] = (uint8_t)n; }
    ntrans++;
    uint8_t st = status(), op = tx[0];
    if (rx) { memset(rx, 0, n); rx[0] = dead ? 0xFF : st; }
    if (dead) { if (rx) memset(rx, 0xFF, n); return 0; }
    if ((op & 0xE0) == 0x00) {            /* R_REGISTER */
        uint8_t r = op & 0x1F; if (n - 1 > (size_t)reg_width(r)) V("레지스터 폭 초과 읽기");
        for (size_t i = 1; i < n; i++) { uint8_t v = A[r][i - 1];
            if (r == 0) v = cfg_eff();
            if (r == 7) v = st;
            if (r == 0x17) v = fifo_status();
            rx[i] = v; }
        return 0;
    }
    if ((op & 0xE0) == 0x20) {                                                          /* W_REGISTER */
        uint8_t r = op & 0x1F;
        if (!reg_w(r) && r != 7) { V("쓰기 불가 레지스터"); return 0; }
        if (r != 7 && rx_mode()) V("수신 모드에서 STATUS 외 레지스터 쓰기");
        if (n - 1 > (size_t)reg_width(r)) V("레지스터 폭 초과 쓰기");
        for (size_t i = 1; i < n; i++) {
            uint8_t v = tx[i];
            if (r == 7) { A[7][0] &= (uint8_t)~(v & 0x70); continue; }
            if (v & RSV[r] && i == 1) V("예약 비트에 1 쓰기");
            if (r == 0x1C && (v & 0x3F)) for (int p = 0; p < 6; p++) if ((v >> p & 1) && !((A[0x1D][0] & 4) && (A[1][0] >> p & 1))) V("DPL_Px 는 EN_DPL 과 ENAA_Px 필요");
            if (r == 0 && !(A[0][0] & 2) && (v & 2)) t_pwrup = now_us;
            if (r == 5) A[8][0] &= 0x0F;                                                /* RF_CH 쓰기가 PLOS_CNT 초기화 */
            A[r][i - 1] = v;
        }
        return 0;
    }
    if (op == NRF_CMD_W_TX_PAYLOAD || op == NRF_CMD_W_TX_NOACK) {
        if (n < 2 || n > 33) V("TX 페이로드 길이 1..32 아님");
        if (op == NRF_CMD_W_TX_NOACK && !(A[0x1D][0] & 1)) { V("EN_DYN_ACK 없이 NOACK"); return 0; }
        if (ntx < 3 && n >= 2) { txq[ntx].noack = op == NRF_CMD_W_TX_NOACK; txq[ntx].len = (uint8_t)(n - 1); memcpy(txq[ntx].d, tx + 1, n - 1); ntx++; } else V("TX FIFO 가득 참에 쓰기");
        return 0;
    }
    if (op == NRF_CMD_R_RX_PAYLOAD) {
        if (!nrx) { V("빈 RX FIFO 읽기"); return 0; }
        if (n - 1 != rxq[0].len) V("RX 읽기 길이가 페이로드와 다름");
        for (size_t i = 1; i < n && i - 1 < 32; i++) rx[i] = rxq[0].d[i - 1];
        memmove(&rxq[0], &rxq[1], sizeof(pkt_t) * 2); nrx--;
        return 0;
    }
    if (op == NRF_CMD_R_RX_PL_WID) { if (!(A[0x1D][0] & 4)) V("EN_DPL 없이 R_RX_PL_WID"); rx[1] = nrx ? rxq[0].len : 0; return 0; }
    if ((op & 0xF8) == NRF_CMD_W_ACK_PAYLOAD) {
        if (!(A[0x1D][0] & 2)) V("EN_ACK_PAY 없이 W_ACK_PAYLOAD");
        if (nack < 3) { ackq[nack].pipe = op & 7; ackq[nack].len = (uint8_t)(n - 1); memcpy(ackq[nack].d, tx + 1, n - 1); nack++; }
        return 0;
    }
    if (op == NRF_CMD_FLUSH_TX) { ntx = 0; return 0; }
    if (op == NRF_CMD_FLUSH_RX) { nrx = 0; return 0; }
    if (op == NRF_CMD_NOP) return 0;
    V("모르는 명령"); return -9;
}
/* 공중에서 온 패킷 주입. 수신 조건을 만족하지 않으면 버려진다 */
static int air_rx(unsigned pipe, const uint8_t *d, unsigned len)
{
    if (!rx_mode() || now_us - t_ce_rise < 130 || !(A[2][0] >> pipe & 1) || nrx >= 3) return 0;
    rxq[nrx].pipe = (uint8_t)pipe; rxq[nrx].len = (uint8_t)len; memcpy(rxq[nrx].d, d, len); nrx++; A[7][0] |= 0x40; return 1;
}

static int bad;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); bad++; } } while (0)
static qspi_t Q;
static nrf24_t N = { &Q, 0, 0, model_ce, model_delay, 0, 0, 0 };
static nrf24_cfg_t def(void)
{
    nrf24_cfg_t c = { .channel = 76, .rate = NRF_RATE_1M, .power = NRF_PWR_0, .crc_bytes = 2, .addr_width = 5, .ard = 5, .arc = 15, .auto_ack = 1 };
    return c;
}
static int logeq(int i, const uint8_t *e, unsigned n) { return i < nlog && log_len[i] == n && !memcmp(log_buf[i], e, n); }

static void t_init_stream(void)
{
    reset_model(); nrf24_cfg_t c = def();
    CHECK(nrf24_init(&N, &c) == 0);
    /* 손으로 계산한 기대 바이트열: 쓰기(20h|주소) 값, 읽기(주소) 0 */
    static const struct { uint8_t r, v; } w[] = { {3, 0x03}, {4, 0x5F}, {5, 0x4C}, {6, 0x06}, {1, 0x3F}, {0x1D, 0x00}, {0x1C, 0x00}, {0, 0x0E} };
    int i = 0;
    for (unsigned k = 0; k < 8; k++) {
        uint8_t a[2] = { (uint8_t)(0x20 | w[k].r), w[k].v }, b[2] = { w[k].r, 0 };
        CHECK(logeq(i++, a, 2)); CHECK(logeq(i++, b, 2));
    }
    uint8_t f1 = 0xE1, f2 = 0xE2, cl[2] = { 0x27, 0x70 };
    CHECK(logeq(i++, &f1, 1)); CHECK(logeq(i++, &f2, 1)); CHECK(logeq(i++, cl, 2));
    CHECK(nlog == i);
    CHECK(now_us - 100000 >= 4500);                          /* PWR_UP 뒤 Tpd2stby 대기 */
    CHECK(A[0][0] == 0x0E);
    CHECK(viol == 0);
}
static void t_rf_setup_matrix(void)
{
    static const struct { uint8_t rate, pwr, exp; } m[] = { {NRF_RATE_1M,0,0x00}, {NRF_RATE_1M,3,0x06}, {NRF_RATE_2M,0,0x08}, {NRF_RATE_2M,2,0x0C}, {NRF_RATE_250K,0,0x20}, {NRF_RATE_250K,1,0x22} };
    for (unsigned k = 0; k < 6; k++) {
        reset_model(); nrf24_cfg_t c = def(); c.rate = m[k].rate; c.power = m[k].pwr; if (c.rate == NRF_RATE_250K) c.ard = 2;
        CHECK(nrf24_init(&N, &c) == 0); CHECK(A[6][0] == m[k].exp); CHECK(viol == 0);
    }
    /* 주소 폭·CRC·재전송·채널 */
    reset_model(); nrf24_cfg_t c = def(); c.addr_width = 3; c.crc_bytes = 1; c.ard = 15; c.arc = 0; c.channel = 125;
    CHECK(nrf24_init(&N, &c) == 0); CHECK(A[3][0] == 1); CHECK(A[0][0] == 0x0A); CHECK(A[4][0] == 0xF0); CHECK(A[5][0] == 125);
    c.addr_width = 4; reset_model(); CHECK(nrf24_init(&N, &c) == 0); CHECK(A[3][0] == 2);
    /* CRC 끔 + 자동 ACK 끔 */
    reset_model(); c = def(); c.crc_bytes = 0; c.auto_ack = 0; CHECK(nrf24_init(&N, &c) == 0); CHECK(A[0][0] == 0x02 && A[1][0] == 0);
    /* DPL + ACK 페이로드 + dyn_ack */
    reset_model(); c = def(); c.dyn_payload = 1; c.ack_payload = 1; c.dyn_ack = 1; CHECK(nrf24_init(&N, &c) == 0);
    CHECK(A[0x1D][0] == 7 && A[0x1C][0] == 0x3F && viol == 0);
}
static void t_args(void)
{
    nrf24_cfg_t c; struct { nrf24_cfg_t c; const char *why; } t[16]; int k = 0;
    c = def(); c.channel = 126; t[k].c = c; t[k++].why = "채널 126";
    c = def(); c.rate = 3; t[k].c = c; t[k++].why = "속도 3";
    c = def(); c.power = 4; t[k].c = c; t[k++].why = "출력 4";
    c = def(); c.crc_bytes = 3; t[k].c = c; t[k++].why = "CRC 3";
    c = def(); c.addr_width = 2; t[k].c = c; t[k++].why = "주소 2";
    c = def(); c.addr_width = 6; t[k].c = c; t[k++].why = "주소 6";
    c = def(); c.ard = 16; t[k].c = c; t[k++].why = "ARD 16";
    c = def(); c.arc = 16; t[k].c = c; t[k++].why = "ARC 16";
    c = def(); c.crc_bytes = 0; t[k].c = c; t[k++].why = "ACK 있는데 CRC 없음";
    c = def(); c.auto_ack = 0; c.dyn_payload = 1; t[k].c = c; t[k++].why = "DPL 인데 ACK 없음";
    c = def(); c.ack_payload = 1; t[k].c = c; t[k++].why = "ACK 페이로드인데 DPL 없음";
    c = def(); c.rate = NRF_RATE_250K; c.ard = 0; t[k].c = c; t[k++].why = "250 kbps ARD 0";
    c = def(); c.dyn_payload = 1; c.ack_payload = 1; c.ard = 0; t[k].c = c; t[k++].why = "ACK 페이로드 ARD 0";
    for (int i = 0; i < k; i++) { reset_model(); int before = ntrans; if (nrf24_init(&N, &t[i].c) != QSPI_ERR_ARGS || ntrans != before) { printf("FAIL args: %s\n", t[i].why); bad++; } }
    CHECK(nrf24_init(&N, 0) == QSPI_ERR_ARGS);
}
static void t_send(void)
{
    reset_model(); nrf24_cfg_t c = def(); CHECK(nrf24_init(&N, &c) == 0);
    uint8_t a[5] = { 0xB3, 0xB4, 0xB5, 0xB6, 0x05 };
    CHECK(nrf24_set_tx_addr(&N, a) == 0);
    CHECK(!memcmp(A[0x10], a, 5) && !memcmp(A[0x0A], a, 5));
    uint8_t d[32]; for (int i = 0; i < 32; i++) d[i] = (uint8_t)(i * 7 + 1);
    CHECK(nrf24_send(&N, d, 32, 0, 100000) == 0);
    CHECK(ntx == 0 && !(A[7][0] & 0x70) && viol == 0);
    /* 실패 → MAX_RT, FIFO 비움, 플래그 해제 */
    air_ok = 0; CHECK(nrf24_send(&N, d, 5, 0, 100000) == NRF_ERR_MAXRT);
    CHECK(ntx == 0 && !(A[7][0] & 0x70)); CHECK((A[8][0] >> 4) == 1);
    air_ok = 1; CHECK(nrf24_send(&N, d, 1, 0, 100000) == 0);
    CHECK(nrf24_send(&N, d, 0, 0, 1000) == QSPI_ERR_ARGS); CHECK(nrf24_send(&N, d, 33, 0, 1000) == QSPI_ERR_ARGS);
    CHECK(nrf24_send(&N, d, 4, 1, 1000) == QSPI_ERR_ARGS);                     /* dyn_ack 없이 NOACK */
    /* TX FIFO 가득 → NRF_ERR_TXFULL */
    for (int i = 0; i < 3; i++) { txq[i].len = 1; txq[i].noack = 0; } ntx = 3; A[7][0] |= 0; CHECK(nrf24_send(&N, d, 4, 0, 1000) == NRF_ERR_TXFULL); ntx = 0;
    /* 응답 없고 CE 펄스로 전송이 안 되는 경우: 타임아웃 (여기서는 PWR_UP 를 내려 모델이 송신 안 함) */
    A[0][0] &= (uint8_t)~2; CHECK(nrf24_send(&N, d, 4, 0, 500) == NRF_ERR_TIMEOUT); CHECK(viol == 0);
}
static void t_recv(void)
{
    reset_model(); nrf24_cfg_t c = def(); CHECK(nrf24_init(&N, &c) == 0);
    uint8_t a1[5] = { 1, 2, 3, 4, 5 }; CHECK(nrf24_set_rx_pipe(&N, 1, a1, 6) == 0); CHECK(nrf24_set_rx_pipe(&N, 3, a1, 2) == 0);
    CHECK(!memcmp(A[0x0B], a1, 5) && A[0x0D][0] == 1 && A[0x12][0] == 6 && A[0x14][0] == 2 && (A[2][0] & 0x0A) == 0x0A && (A[2][0] & 1));  /* P0 는 리셋 후 켜져 있음 */
    uint8_t buf[32], d1[6] = { 11, 12, 13, 14, 15, 16 }; unsigned len, pipe;
    CHECK(nrf24_recv(&N, buf, &len, &pipe) == NRF_ERR_NODATA);
    CHECK(air_rx(1, d1, 6) == 0);                                               /* CE 꺼져 있으면 수신 안 됨 */
    CHECK(nrf24_rx_start(&N) == 0); CHECK(viol == 0);
    uint8_t d3[2] = { 0xAA, 0xBB };
    CHECK(air_rx(1, d1, 6) && air_rx(3, d3, 2) && air_rx(1, d1, 6)); CHECK(air_rx(1, d1, 6) == 0);           /* 4번째는 FIFO 가득 */
    CHECK(nrf24_recv(&N, buf, &len, &pipe) == 0 && len == 6 && pipe == 1 && !memcmp(buf, d1, 6));
    CHECK(!(A[7][0] & 0x40));
    CHECK(nrf24_recv(&N, buf, &len, &pipe) == 0 && len == 2 && pipe == 3 && buf[0] == 0xAA && buf[1] == 0xBB);
    CHECK(nrf24_recv(&N, buf, &len, &pipe) == 0 && len == 6 && pipe == 1);
    CHECK(nrf24_recv(&N, buf, &len, &pipe) == NRF_ERR_NODATA);
    CHECK(nrf24_rx_stop(&N) == 0);
    /* RX 에서 TX 로 전환 시 PRIM_RX 해제 */
    uint8_t d[4] = { 9, 8, 7, 6 }; uint8_t ta[5] = { 7, 7, 7, 7, 7 }; CHECK(nrf24_set_tx_addr(&N, ta) == 0);
    CHECK(nrf24_send(&N, d, 4, 0, 10000) == 0); CHECK(!(A[0][0] & 1)); CHECK(viol == 0);
    CHECK(nrf24_set_rx_pipe(&N, 6, a1, 1) == QSPI_ERR_ARGS); CHECK(nrf24_set_rx_pipe(&N, 1, a1, 0) == QSPI_ERR_ARGS); CHECK(nrf24_set_rx_pipe(&N, 1, a1, 33) == QSPI_ERR_ARGS);
}
static void t_dpl_ack(void)
{
    reset_model(); nrf24_cfg_t c = def(); c.dyn_payload = 1; c.ack_payload = 1; c.dyn_ack = 1; CHECK(nrf24_init(&N, &c) == 0);
    uint8_t a1[5] = { 1, 2, 3, 4, 5 }; CHECK(nrf24_set_rx_pipe(&N, 1, a1, 0) == 0); CHECK(A[0x12][0] == 0);        /* DPL 이면 RX_PW 를 쓰지 않음 */
    CHECK(nrf24_rx_start(&N) == 0);
    uint8_t d[32], buf[32]; unsigned len, pipe;
    for (unsigned l = 1; l <= 32; l++) {
        for (unsigned i = 0; i < l; i++) d[i] = (uint8_t)(l * 3 + i);
        CHECK(air_rx(1, d, l) == 1); CHECK(nrf24_recv(&N, buf, &len, &pipe) == 0 && len == l && pipe == 1 && !memcmp(buf, d, l));
    }
    /* 비정상 폭(33): FLUSH_RX */
    rxq[0].pipe = 1; rxq[0].len = 33; nrx = 1; A[7][0] |= 0x40; CHECK(nrf24_recv(&N, buf, &len, &pipe) == NRF_ERR_BADLEN); CHECK(nrx == 0 && !(A[7][0] & 0x40));
    rxq[0].pipe = 1; rxq[0].len = 0; nrx = 1; CHECK(nrf24_recv(&N, buf, &len, &pipe) == NRF_ERR_BADLEN && nrx == 0);
    CHECK(nrf24_write_ack_payload(&N, 1, d, 5) == 0 && nack == 1 && ackq[0].pipe == 1 && ackq[0].len == 5);
    CHECK(nrf24_write_ack_payload(&N, 6, d, 5) == QSPI_ERR_ARGS && nrf24_write_ack_payload(&N, 1, d, 0) == QSPI_ERR_ARGS && nrf24_write_ack_payload(&N, 1, d, 33) == QSPI_ERR_ARGS);
    CHECK(nrf24_rx_stop(&N) == 0);
    /* PTX: ACK 에 페이로드가 실려 오면 RX FIFO 로 */
    uint8_t ta[5] = { 5, 5, 5, 5, 5 }; CHECK(nrf24_set_tx_addr(&N, ta) == 0);
    air_ackpay_len = 7; CHECK(nrf24_send(&N, d, 3, 0, 10000) == 0); CHECK(nrx == 1 && (A[7][0] & 0x40));
    CHECK(nrf24_recv(&N, buf, &len, &pipe) == 0 && len == 7 && pipe == 0 && buf[0] == 0xA0 && buf[6] == 0xA6);
    CHECK(nrf24_send(&N, d, 3, 1, 10000) == 0);                                 /* NOACK (TX_DS 시점은 모델의 가정) */
    CHECK(viol == 0);
    /* FEATURE 없이 호출 */
    reset_model(); c = def(); CHECK(nrf24_init(&N, &c) == 0);
    CHECK(nrf24_write_ack_payload(&N, 0, d, 1) == QSPI_ERR_ARGS && viol == 0);
}
static void t_dead(void)
{
    reset_model(); dead = 1; nrf24_cfg_t c = def(); CHECK(nrf24_init(&N, &c) == NRF_ERR_VERIFY);   /* 칩이 없으면 읽기가 0xFF */
    reset_model(); int r = nrf24_read_rpd(&N, &(int){ 0 }); CHECK(r == 0);
    A[9][0] = 1; int rpd = 0; CHECK(nrf24_read_rpd(&N, &rpd) == 0 && rpd == 1);
}
static void t_misc(void)
{
    reset_model(); nrf24_cfg_t c = def(); CHECK(nrf24_init(&N, &c) == 0);
    uint8_t b[5], st = 0; CHECK(nrf24_read_reg(&N, 0x20, b, 1) == QSPI_ERR_ARGS && nrf24_read_reg(&N, 0, b, 0) == QSPI_ERR_ARGS && nrf24_read_reg(&N, 0, b, 6) == QSPI_ERR_ARGS);
    A[7][0] |= 0x40; CHECK(nrf24_nop(&N, &st) == 0 && (st & 0x40)); CHECK(nrf24_clear_irq(&N, 0x40) == 0 && !(A[7][0] & 0x40));
    CHECK(nrf24_clear_irq(&N, 0xFF) == 0);                                      /* 예약 비트에는 0 만 쓴다 */
    CHECK(viol == 0);
}
int main(void)
{
    t_init_stream(); t_rf_setup_matrix(); t_args(); t_send(); t_recv(); t_dpl_ack(); t_dead(); t_misc();
    if (bad) { printf("%d FAIL\n", bad); return 1; }
    printf("ALL PASS (nrf24 model)\n"); return 0;
}
