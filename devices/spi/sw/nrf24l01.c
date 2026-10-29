#include "nrf24l01.h"
#include <string.h>

static int cmd(nrf24_t *n, const uint8_t *tx, uint8_t *rx, size_t len)
{
    qspi_cs(n->s, n->cs, 1);
    int r = qspi_xfer(n->s, tx, rx, len);
    qspi_cs(n->s, n->cs, 0);
    return r;
}
static void ce(nrf24_t *n, int v) { if (n->ce) n->ce(n->ctx, v); }
static void us(nrf24_t *n, uint32_t t) { if (n->delay_us) n->delay_us(n->ctx, t); }

int nrf24_nop(nrf24_t *n, uint8_t *st)
{
    uint8_t t = NRF_CMD_NOP, r = 0;
    int e = cmd(n, &t, &r, 1);
    if (e == QSPI_OK && st) *st = r;
    return e;
}
int nrf24_read_reg(nrf24_t *n, uint8_t reg, uint8_t *buf, unsigned len)
{
    if (!n || !buf || reg > 0x1F || len < 1 || len > 5) return QSPI_ERR_ARGS;
    uint8_t tx[6] = { (uint8_t)(NRF_CMD_R_REGISTER | reg) }, rx[6];
    int e = cmd(n, tx, rx, len + 1);
    if (e == QSPI_OK) memcpy(buf, rx + 1, len);
    return e;
}
int nrf24_write_reg(nrf24_t *n, uint8_t reg, const uint8_t *buf, unsigned len)
{
    if (!n || !buf || reg > 0x1F || len < 1 || len > 5) return QSPI_ERR_ARGS;
    uint8_t tx[6] = { (uint8_t)(NRF_CMD_W_REGISTER | reg) };
    memcpy(tx + 1, buf, len);
    return cmd(n, tx, 0, len + 1);
}
int nrf24_write_reg8(nrf24_t *n, uint8_t reg, uint8_t v) { return nrf24_write_reg(n, reg, &v, 1); }
int nrf24_read_reg8(nrf24_t *n, uint8_t reg, uint8_t *v) { return nrf24_read_reg(n, reg, v, 1); }
int nrf24_flush_tx(nrf24_t *n) { uint8_t t = NRF_CMD_FLUSH_TX; return cmd(n, &t, 0, 1); }
int nrf24_flush_rx(nrf24_t *n) { uint8_t t = NRF_CMD_FLUSH_RX; return cmd(n, &t, 0, 1); }
int nrf24_clear_irq(nrf24_t *n, uint8_t mask)
{
    return nrf24_write_reg8(n, NRF_REG_STATUS, (uint8_t)(mask & (NRF_ST_RX_DR | NRF_ST_TX_DS | NRF_ST_MAX_RT)));
}

#define TRY(x) do { int e_ = (x); if (e_ != QSPI_OK) return e_; } while (0)

static int wr_verify(nrf24_t *n, uint8_t reg, uint8_t v)
{
    uint8_t b;
    TRY(nrf24_write_reg8(n, reg, v));
    TRY(nrf24_read_reg8(n, reg, &b));
    return b == v ? QSPI_OK : NRF_ERR_VERIFY;
}

int nrf24_init(nrf24_t *n, const nrf24_cfg_t *c)
{
    if (!n || !c || c->channel > 125 || c->rate > NRF_RATE_250K || c->power > NRF_PWR_0 || c->crc_bytes > 2 ||
        c->addr_width < 3 || c->addr_width > 5 || c->ard > 15 || c->arc > 15) return QSPI_ERR_ARGS;
    if (c->auto_ack && c->crc_bytes == 0) return QSPI_ERR_ARGS;          /* EN_AA 가 켜지면 EN_CRC 강제 */
    if (c->dyn_payload && !c->auto_ack) return QSPI_ERR_ARGS;             /* DPL_Px 는 ENAA_Px 필요 */
    if (c->ack_payload && !c->dyn_payload) return QSPI_ERR_ARGS;          /* 각주 d */
    if (c->rate == NRF_RATE_250K && c->ard < 1) return QSPI_ERR_ARGS;      /* 각주 a: 250 kbps 는 ARD ≥ 500 µs */
    if (c->ack_payload && c->ard < 1) return QSPI_ERR_ARGS;                /* 설계: ACK 페이로드 길이를 모르므로 보수적으로 ARD ≥ 500 µs */
    ce(n, 0);
    n->aw = c->addr_width;
    uint8_t rf = 0;
    if (c->rate == NRF_RATE_250K) rf |= 1u << 5;          /* RF_DR_LOW */
    else if (c->rate == NRF_RATE_2M) rf |= 1u << 3;        /* RF_DR_HIGH */
    rf |= (uint8_t)(c->power << 1);
    uint8_t feat = (uint8_t)((c->dyn_payload ? NRF_FEAT_EN_DPL : 0) | (c->ack_payload ? NRF_FEAT_EN_ACK_PAY : 0) | (c->dyn_ack ? NRF_FEAT_EN_DYN_ACK : 0));
    TRY(wr_verify(n, NRF_REG_SETUP_AW, (uint8_t)(c->addr_width - 2)));    /* 01=3 바이트, 10=4, 11=5 */
    TRY(wr_verify(n, NRF_REG_SETUP_RETR, (uint8_t)((c->ard << 4) | c->arc)));
    TRY(wr_verify(n, NRF_REG_RF_CH, c->channel));
    TRY(wr_verify(n, NRF_REG_RF_SETUP, rf));
    TRY(wr_verify(n, NRF_REG_EN_AA, c->auto_ack ? 0x3F : 0x00));
    TRY(wr_verify(n, NRF_REG_FEATURE, feat));
    TRY(wr_verify(n, NRF_REG_DYNPD, c->dyn_payload ? 0x3F : 0x00));
    n->feature = feat;
    n->config = (uint8_t)((c->crc_bytes ? NRF_CFG_EN_CRC : 0) | (c->crc_bytes == 2 ? NRF_CFG_CRCO : 0) | NRF_CFG_PWR_UP);
    TRY(wr_verify(n, NRF_REG_CONFIG, n->config));
    us(n, NRF_T_PD2STBY_US);                                              /* 대기 모드로 올라올 때까지 CE 를 올리지 않는다 */
    TRY(nrf24_flush_tx(n));
    TRY(nrf24_flush_rx(n));
    return nrf24_clear_irq(n, NRF_ST_RX_DR | NRF_ST_TX_DS | NRF_ST_MAX_RT);
}

int nrf24_set_tx_addr(nrf24_t *n, const uint8_t *addr)
{
    if (!n || !addr || n->aw < 3) return QSPI_ERR_ARGS;
    TRY(nrf24_write_reg(n, NRF_REG_TX_ADDR, addr, n->aw));
    return nrf24_write_reg(n, NRF_REG_RX_ADDR_P0, addr, n->aw);
}

int nrf24_set_rx_pipe(nrf24_t *n, unsigned pipe, const uint8_t *addr, unsigned payload_len)
{
    if (!n || !addr || pipe > 5 || n->aw < 3) return QSPI_ERR_ARGS;
    int dyn = (n->feature & NRF_FEAT_EN_DPL) != 0;
    if (!dyn && (payload_len < 1 || payload_len > 32)) return QSPI_ERR_ARGS;
    TRY(nrf24_write_reg(n, (uint8_t)(NRF_REG_RX_ADDR_P0 + pipe), addr, pipe < 2 ? n->aw : 1));
    if (!dyn) TRY(nrf24_write_reg8(n, (uint8_t)(NRF_REG_RX_PW_P0 + pipe), (uint8_t)payload_len));
    uint8_t en;
    TRY(nrf24_read_reg8(n, NRF_REG_EN_RXADDR, &en));
    return nrf24_write_reg8(n, NRF_REG_EN_RXADDR, (uint8_t)(en | (1u << pipe)));
}

int nrf24_rx_start(nrf24_t *n)
{
    ce(n, 0);
    n->config |= NRF_CFG_PRIM_RX;
    TRY(nrf24_write_reg8(n, NRF_REG_CONFIG, n->config));
    ce(n, 1);
    us(n, NRF_T_STBY2A_US);
    return QSPI_OK;
}
int nrf24_rx_stop(nrf24_t *n) { ce(n, 0); return QSPI_OK; }

static int nrf24_write_payload(nrf24_t *n, const uint8_t *data, unsigned len, int noack)
{
    if (!data || len < 1 || len > 32) return QSPI_ERR_ARGS;
    if (noack && !(n->feature & NRF_FEAT_EN_DYN_ACK)) return QSPI_ERR_ARGS;
    uint8_t tx[33] = { noack ? NRF_CMD_W_TX_NOACK : NRF_CMD_W_TX_PAYLOAD };
    memcpy(tx + 1, data, len);
    return cmd(n, tx, 0, len + 1);
}

int nrf24_send(nrf24_t *n, const uint8_t *data, unsigned len, int noack, uint32_t timeout_us)
{
    if (!n || !data) return QSPI_ERR_ARGS;
    ce(n, 0);
    if (n->config & NRF_CFG_PRIM_RX) {
        n->config &= (uint8_t)~NRF_CFG_PRIM_RX;
        TRY(nrf24_write_reg8(n, NRF_REG_CONFIG, n->config));
    }
    uint8_t st;
    TRY(nrf24_nop(n, &st));
    if (st & NRF_ST_TX_FULL) return NRF_ERR_TXFULL;
    TRY(nrf24_write_payload(n, data, len, noack));
    ce(n, 1);
    us(n, NRF_CE_PULSE_US);          /* 최소 10 µs. 이 동안 CSN 도 건드리지 않아 Tpece2csn(4 µs) 을 만족 */
    ce(n, 0);
    uint32_t waited = 0;
    for (;;) {
        TRY(nrf24_nop(n, &st));
        if (st & NRF_ST_TX_DS) return nrf24_clear_irq(n, NRF_ST_TX_DS);
        if (st & NRF_ST_MAX_RT) {
            TRY(nrf24_clear_irq(n, NRF_ST_MAX_RT));
            TRY(nrf24_flush_tx(n));   /* MAX_RT 이면 페이로드가 FIFO 에 남는다 */
            return NRF_ERR_MAXRT;
        }
        if (waited >= timeout_us) return NRF_ERR_TIMEOUT;
        us(n, 50); waited += 50;
    }
}

int nrf24_recv(nrf24_t *n, uint8_t buf[32], unsigned *len, unsigned *pipe)
{
    if (!n || !buf || !len || !pipe) return QSPI_ERR_ARGS;
    uint8_t tx[3] = { NRF_CMD_R_REGISTER | NRF_REG_FIFO_STATUS, 0 }, rx[2];
    TRY(cmd(n, tx, rx, 2));          /* rx[0] = STATUS, rx[1] = FIFO_STATUS */
    if (rx[1] & NRF_FIFO_RX_EMPTY) return NRF_ERR_NODATA;
    unsigned p = NRF_ST_RX_P_NO(rx[0]);
    if (p > 5) return NRF_ERR_NODATA;
    uint8_t w;
    if (n->feature & NRF_FEAT_EN_DPL) {
        uint8_t t2[2] = { NRF_CMD_R_RX_PL_WID, 0 }, r2[2];
        TRY(cmd(n, t2, r2, 2)); w = r2[1];
    } else {
        TRY(nrf24_read_reg8(n, (uint8_t)(NRF_REG_RX_PW_P0 + p), &w));
    }
    if (w < 1 || w > 32) { TRY(nrf24_flush_rx(n)); TRY(nrf24_clear_irq(n, NRF_ST_RX_DR)); return NRF_ERR_BADLEN; }
    uint8_t t3[33] = { NRF_CMD_R_RX_PAYLOAD }, r3[33];
    TRY(cmd(n, t3, r3, (size_t)w + 1));
    memcpy(buf, r3 + 1, w);
    *len = w; *pipe = p;
    return nrf24_clear_irq(n, NRF_ST_RX_DR);
}

int nrf24_write_ack_payload(nrf24_t *n, unsigned pipe, const uint8_t *data, unsigned len)
{
    if (!n || !data || pipe > 5 || len < 1 || len > 32 || !(n->feature & NRF_FEAT_EN_ACK_PAY)) return QSPI_ERR_ARGS;
    uint8_t tx[33] = { (uint8_t)(NRF_CMD_W_ACK_PAYLOAD | pipe) };
    memcpy(tx + 1, data, len);
    return cmd(n, tx, 0, len + 1);
}

int nrf24_read_rpd(nrf24_t *n, int *rpd)
{
    uint8_t v;
    if (!n || !rpd) return QSPI_ERR_ARGS;
    TRY(nrf24_read_reg8(n, NRF_REG_RPD, &v));
    *rpd = v & 1;
    return QSPI_OK;
}
