#include <stdio.h>
#include <string.h>
#include "../sw/spi_devices.h"

/* 모의 IP + 소자: CS(SSR 비트 0)가 눌린 동안 바이트마다 소자 응답. 응답은 명령 바이트 뒤에 오는 스크립트 */
static uint32_t ssr = 0xFFFF, cr = QCR_INHIBIT; static int cs_idx = -1, pos;
static uint8_t cmd; static int never_ready;
static uint8_t txq[512], rxq[512]; static int nt, rh, rt, dtr_when_not_inhibited, max_batch;
static uint8_t jedec[3] = { 0xEF, 0x40, 0x17 }; static uint8_t status1 = 0x01;
static uint8_t tc[4] = { 0, 0, 0, 0 };
static int dev_kind;                                  /* 1 = W25Q, 2 = MAX31855, 3 = 에코(바이트+1) */
static int sent_before_cs;
static uint8_t device(uint8_t v)
{
    uint8_t out = 0xFF;
    if (dev_kind == 1) {
        if (pos == 0) cmd = v;
        else if (cmd == 0x9F && pos <= 3) out = jedec[pos - 1];
        else if (cmd == 0x05 && pos == 1) out = status1;
    } else if (dev_kind == 2) out = tc[pos];
    else if (dev_kind == 3) out = (uint8_t)(v + 1);
    pos++; return out;
}
uint32_t mock_q_rd(uint32_t off)
{
    if (off == QSPI_SR) return (rt > rh && !never_ready) ? 0 : QSR_RX_EMPTY;
    if (off == QSPI_DRR) return rt > rh ? rxq[rh++] : 0;
    return 0;
}
void mock_q_wr(uint32_t off, uint32_t v)
{
    if (off == QSPI_SSR) {
        ssr = v; int c = -1;
        for (int i = 0; i < 4; i++) if (!(v & (1u << i))) c = i;
        if (c != cs_idx) pos = 0;
        cs_idx = c; return;
    }
    if (off == QSPI_CR) {
        if (v & (QCR_RST_TX | QCR_RST_RX)) { nt = 0; rh = rt = 0; }
        int was_inh = (cr & QCR_INHIBIT) != 0; cr = v;
        if (was_inh && !(v & QCR_INHIBIT) && (v & QCR_SPE)) {      /* 전송 시작: 쌓인 DTR 를 모두 처리 */
            if (nt > max_batch) max_batch = nt;
            if (cs_idx < 0) sent_before_cs += nt;
            for (int i = 0; i < nt; i++) rxq[rt++] = device(txq[i]);
            nt = 0;
        }
        return;
    }
    if (off == QSPI_DTR) { if (!(cr & QCR_INHIBIT)) dtr_when_not_inhibited++; txq[nt++] = (uint8_t)v; }
}
static int fails;
#define CHK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main(void)
{
    qspi_t s;
    CHK(qspi_init(&s, 0, 3, 0, 4, 0) == QSPI_OK);
    CHK(cr == (QCR_MASTER | QCR_MSS | QCR_CPOL | QCR_CPHA | QCR_SPE | QCR_INHIBIT));   /* 0x19E: 초기엔 inhibit */
    CHK(ssr == 0xF);
    CHK(qspi_init(&s, 0, 4, 0, 4, 0) == QSPI_ERR_ARGS);
    qspi_init(&s, 0, 0, 0, 4, 0);
    CHK(cr == (QCR_MASTER | QCR_MSS | QCR_SPE | QCR_INHIBIT));                                   /* 0x186 */

    dev_kind = 1; uint8_t id[3]; int busy = 0;
    CHK(w25q_read_jedec(&s, 2, id) == QSPI_OK && id[0] == 0xEF && id[1] == 0x40 && id[2] == 0x17);
    CHK(ssr == 0xF && sent_before_cs == 0);                                                       /* CS 해제 */
    CHK(w25q_is_busy(&s, 2, &busy) == QSPI_OK && busy == 1);
    status1 = 0; CHK(w25q_is_busy(&s, 2, &busy) == QSPI_OK && busy == 0);

    /* MAX31855: 열전대 +25.00 °C → 100 카운트 << 18 ; 내부 +25.0625 → 401 << 4 ; 오류 없음 */
    uint32_t raw = (100u << 18) | (401u << 4);
    tc[0] = raw >> 24; tc[1] = raw >> 16; tc[2] = raw >> 8; tc[3] = raw;
    dev_kind = 2; max31855_t m;
    CHK(max31855_read(&s, 1, &m) == QSPI_OK && m.tc_mc == 25000 && m.cj_mc == 25062 && !m.fault);
    max31855_decode(0xFFFFFFFFu & ~1u, &m);            /* 음수: 모두 1 → tc=-1 카운트 = -250 m°C */
    CHK(m.tc_mc == -250 && m.cj_mc == (-1 * 625) / 10 && m.fault && !m.oc);
    max31855_decode((1u << 16) | 1u, &m);
    CHK(m.fault && m.oc && !m.scg);

    CHK(dtr_when_not_inhibited == 0 && (cr & QCR_INHIBIT));     /* DTR 는 항상 inhibit 상태에서 채우고, 끝나면 다시 inhibit */

    /* FIFO 16: 40바이트 → 16+16+8 로 나눠 보내고 순서대로 돌려받음 */
    qspi_init(&s, 0, 0, 0, 4, 16); dev_kind = 3; max_batch = 0;
    uint8_t out[40], in[40]; for (int i = 0; i < 40; i++) out[i] = (uint8_t)(i * 3);
    qspi_cs(&s, 0, 1);
    CHK(qspi_xfer(&s, out, in, 40) == QSPI_OK);
    qspi_cs(&s, 0, 0);
    int okv = 1; for (int i = 0; i < 40; i++) if (in[i] != (uint8_t)(out[i] + 1)) okv = 0;
    CHK(okv && max_batch == 16);
    CHK(qspi_init(&s, 0, 0, 0, 4, 8) == QSPI_ERR_ARGS);

    qspi_init(&s, 0, 0, 0, 4, 0);
    never_ready = 1; s.timeout = 10;
    uint8_t b = 0; CHK(qspi_xfer(&s, &b, &b, 1) == QSPI_ERR_TIMEOUT && (cr & QCR_INHIBIT));
    printf(fails ? "FAILED %d\n" : "ALL PASS\n", fails);
    return fails != 0;
}
