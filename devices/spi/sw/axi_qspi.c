#include "axi_qspi.h"

int qspi_init(qspi_t *s, uintptr_t b, unsigned mode, int lsb, unsigned nss)
{
    if (mode > 3 || nss == 0 || nss > 32) return QSPI_ERR_ARGS;
    s->base = b; s->timeout = 100000;
    s->ssr_idle = (nss == 32) ? 0xFFFFFFFFu : ((1u << nss) - 1u);   /* 모두 1 = 선택 없음 */
    QSPI_WR(b, QSPI_SRR, 0x0A);                                       /* 소프트 리셋: 0xA 만 허용 */
    uint32_t cr = QCR_MASTER | QCR_MSS | ((mode & 2) ? QCR_CPOL : 0) | ((mode & 1) ? QCR_CPHA : 0) |
                  (lsb ? QCR_LSB : 0);
    QSPI_WR(b, QSPI_CR, cr | QCR_RST_TX | QCR_RST_RX);                /* FIFO 비움 */
    QSPI_WR(b, QSPI_DGIER, 0);
    QSPI_WR(b, QSPI_SSR, s->ssr_idle);
    s->cr = cr | QCR_SPE;
    QSPI_WR(b, QSPI_CR, s->cr);                                       /* inhibit 0 → 바로 전송 */
    return QSPI_OK;
}

void qspi_cs(qspi_t *s, unsigned idx, int assert_cs)
{
    uint32_t v = s->ssr_idle;
    if (assert_cs) v &= ~(1u << idx);                                 /* active-low */
    QSPI_WR(s->base, QSPI_SSR, v);
}

int qspi_xfer(qspi_t *s, const uint8_t *tx, uint8_t *rx, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        QSPI_WR(s->base, QSPI_DTR, tx ? tx[i] : 0x00);
        unsigned t = s->timeout;
        /* FreeBSD 는 TX_EMPTY 만 기다렸다. 마지막 비트가 아직 나가는 중일 수 있어 여기서는 Rx_Empty 해제(수신 완료)를 기다린다 — 설계안 */
        while (QSPI_RD(s->base, QSPI_SR) & QSR_RX_EMPTY) if (!t--) return QSPI_ERR_TIMEOUT;
        uint8_t r = (uint8_t)(QSPI_RD(s->base, QSPI_DRR) & 0xFF);
        if (rx) rx[i] = r;
    }
    return QSPI_OK;
}
