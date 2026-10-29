#include "axi_qspi.h"

int qspi_init(qspi_t *s, uintptr_t b, unsigned mode, int lsb, unsigned nss, unsigned fifo)
{
    if (mode > 3 || nss == 0 || nss > 32) return QSPI_ERR_ARGS;
    if (fifo != 0 && fifo != 16 && fifo != 256) return QSPI_ERR_ARGS;
    s->base = b; s->timeout = 100000; s->chunk = fifo ? fifo : 1;
    s->ssr_idle = (nss == 32) ? 0xFFFFFFFFu : ((1u << nss) - 1u);   /* 모두 1 = 선택 없음 */
    QSPI_WR(b, QSPI_SRR, 0x0A);                                       /* 소프트 리셋: 0xA 만 허용 */
    uint32_t cr = QCR_MASTER | QCR_MSS | ((mode & 2) ? QCR_CPOL : 0) | ((mode & 1) ? QCR_CPHA : 0) |
                  (lsb ? QCR_LSB : 0);
    QSPI_WR(b, QSPI_CR, cr | QCR_RST_TX | QCR_RST_RX | QCR_INHIBIT);  /* FIFO 비움 (DTR 는 리셋 상태에서 채워야 함) */
    QSPI_WR(b, QSPI_DGIER, 0);
    QSPI_WR(b, QSPI_SSR, s->ssr_idle);                                /* 문서 5단계: SPISSR 전부 1 */
    s->cr = cr | QCR_SPE;
    QSPI_WR(b, QSPI_CR, s->cr | QCR_INHIBIT);                         /* 6단계: SPE + inhibit */
    return QSPI_OK;
}

void qspi_cs(qspi_t *s, unsigned idx, int assert_cs)
{
    uint32_t v = s->ssr_idle;
    if (assert_cs) v &= ~(1u << idx);                                 /* active-low one-hot */
    QSPI_WR(s->base, QSPI_SSR, v);
}

int qspi_xfer(qspi_t *s, const uint8_t *tx, uint8_t *rx, size_t len)
{
    size_t done = 0;
    while (done < len) {
        size_t n = len - done; if (n > s->chunk) n = s->chunk;
        for (size_t i = 0; i < n; i++)                                /* 4단계: inhibit 상태에서 DTR 채움 */
            QSPI_WR(s->base, QSPI_DTR, tx ? tx[done + i] : 0x00);
        QSPI_WR(s->base, QSPI_CR, s->cr);                             /* 8단계: inhibit 해제 = 전송 시작 */
        for (size_t i = 0; i < n; i++) {
            unsigned t = s->timeout;
            /* Rx_Empty 해제 = 전송 완료(FIFO 없음: "전송 성공 끝에서 지워짐", FIFO 있음: 수신 FIFO 비어 있지 않음).
             * Tx_Empty 는 FIFO 가 비면 마지막 바이트가 아직 파이프라인에 있을 수 있어 쓰지 않음 */
            while (QSPI_RD(s->base, QSPI_SR) & QSR_RX_EMPTY) if (!t--) {
                QSPI_WR(s->base, QSPI_CR, s->cr | QCR_INHIBIT);
                return QSPI_ERR_TIMEOUT;
            }
            uint8_t r = (uint8_t)(QSPI_RD(s->base, QSPI_DRR) & 0xFF);
            if (rx) rx[done + i] = r;
        }
        QSPI_WR(s->base, QSPI_CR, s->cr | QCR_INHIBIT);               /* 10단계: 완료 후 inhibit 설정 */
        done += n;
    }
    return QSPI_OK;
}
