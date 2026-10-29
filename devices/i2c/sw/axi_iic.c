#include "axi_iic.h"

#define RD(d, o)    AXI_IIC_RD((d)->base, o)
#define WR(d, o, v) AXI_IIC_WR((d)->base, o, v)

static int wait_sr(axi_iic_t *d, uint32_t mask, uint32_t want)
{
    for (uint32_t i = 0; i < d->timeout; i++)
        if ((RD(d, IIC_SR) & mask) == want) return IIC_OK;
    return IIC_ERR_TIMEOUT;
}

/* 송신 오류 복구: ISR 비트 토글 지우기 + TX FIFO 리셋 (PG090: 송신 오류 시 TX FIFO 리셋 필요) */
static void clear_tx_error(axi_iic_t *d)
{
    WR(d, IIC_ISR, ISR_TXERR);                 /* 1을 써서 토글(지우기) */
    WR(d, IIC_CR, CR_EN | CR_TX_FIFO_RST);
    WR(d, IIC_CR, CR_EN);
}

int axi_iic_init(axi_iic_t *d, uintptr_t base)
{
    d->base = base;
    d->timeout = 1000000u;
    /* PG090 초기화 순서: RX FIFO 최대 깊이, TX FIFO 리셋, 활성화 + 리셋 해제 + 일반 호출 끔 */
    WR(d, IIC_RX_PIRQ, 0x0F);
    WR(d, IIC_CR, CR_TX_FIFO_RST);
    WR(d, IIC_CR, CR_EN);
    return IIC_OK;
}

void axi_iic_reset(axi_iic_t *d)
{
    WR(d, IIC_SOFTR, 0xA);
}

static int push(axi_iic_t *d, uint32_t word)
{
    int r = wait_sr(d, SR_TX_FULL, 0);
    if (r) return r;
    WR(d, IIC_TX_FIFO, word);
    return IIC_OK;
}

static int idle_check(axi_iic_t *d)
{
    uint32_t sr = RD(d, IIC_SR);
    if (!(sr & SR_TX_EMPTY) || !(sr & SR_RX_EMPTY) || (sr & SR_BB)) return IIC_ERR_BUSY;
    return IIC_OK;
}

static int write_phase(axi_iic_t *d, uint8_t addr7, const uint8_t *buf, unsigned len, int stop)
{
    int r = push(d, TXF_START | ((uint32_t)addr7 << 1));
    if (r) goto fail;
    for (unsigned i = 0; i < len; i++) {
        uint32_t w = buf[i];
        if (stop && i == len - 1) w |= TXF_STOP;
        r = push(d, w);
        if (r) goto fail;
    }
    r = wait_sr(d, SR_TX_EMPTY, SR_TX_EMPTY);
    if (r) goto fail;
    if (stop) { r = wait_sr(d, SR_BB, 0); if (r) goto fail; }
    if (RD(d, IIC_ISR) & ISR_TXERR) { clear_tx_error(d); return IIC_ERR_NACK; }
    return IIC_OK;
fail:
    if (RD(d, IIC_ISR) & ISR_TXERR) { clear_tx_error(d); return IIC_ERR_NACK; }
    return r;
}

static int read_phase(axi_iic_t *d, uint8_t addr7, uint8_t *buf, unsigned len, int stop)
{
    int r = push(d, TXF_START | ((uint32_t)addr7 << 1) | 1u);
    if (r) return r;
    r = push(d, (stop ? TXF_STOP : 0u) | (uint32_t)len);   /* 수신 바이트 수 */
    if (r) return r;
    for (unsigned i = 0; i < len; i++) {
        r = wait_sr(d, SR_RX_EMPTY, 0);
        if (r) {   /* 주소 NACK 등: FIFO와 컨트롤러 복구 */
            clear_tx_error(d);
            return IIC_ERR_TIMEOUT;
        }
        buf[i] = (uint8_t)(RD(d, IIC_RX_FIFO) & 0xFF);
    }
    if (stop) { r = wait_sr(d, SR_BB, 0); if (r) return r; }
    if (RD(d, IIC_ISR) & ISR_TXERR) WR(d, IIC_ISR, ISR_TXERR);   /* 수신 완료 표시 지우기 */
    return IIC_OK;
}

int axi_iic_write(axi_iic_t *d, uint8_t addr7, const uint8_t *buf, unsigned len)
{
    if (!buf || len == 0 || addr7 > 0x7F) return IIC_ERR_ARGS;
    int r = idle_check(d);
    if (r) return r;
    return write_phase(d, addr7, buf, len, 1);
}

int axi_iic_read(axi_iic_t *d, uint8_t addr7, uint8_t *buf, unsigned len)
{
    if (!buf || len == 0 || len > 255 || addr7 > 0x7F) return IIC_ERR_ARGS;
    int r = idle_check(d);
    if (r) return r;
    return read_phase(d, addr7, buf, len, 1);
}

int axi_iic_write_read(axi_iic_t *d, uint8_t addr7, const uint8_t *wbuf, unsigned wlen,
                       uint8_t *rbuf, unsigned rlen)
{
    if (!wbuf || !rbuf || wlen == 0 || rlen == 0 || rlen > 255 || addr7 > 0x7F) return IIC_ERR_ARGS;
    int r = idle_check(d);
    if (r) return r;
    r = write_phase(d, addr7, wbuf, wlen, 0);          /* STOP 없이 */
    if (r) return r;
    return read_phase(d, addr7, rbuf, rlen, 1);        /* MSMS 가 켜져 있으므로 반복 시작이 생성됨 */
}
