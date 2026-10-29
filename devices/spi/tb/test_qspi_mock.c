#include <stdio.h>
#include <string.h>
#include "../sw/spi_devices.h"

/* 모의 IP + 소자: CS(SSR 비트 0)가 눌린 동안 바이트마다 소자 응답. 응답은 명령 바이트 뒤에 오는 스크립트 */
static uint32_t ssr = 0xFFFF, cr, rxreg; static int rx_full;
static int cs_idx = -1, pos; static uint8_t cmd; static int never_ready;
static uint8_t jedec[3] = { 0xEF, 0x40, 0x17 }; static uint8_t status1 = 0x01;
static uint8_t tc[4] = { 0x01, 0x90, 0x19, 0x00 };   /* 아래에서 값 설정 */
static int dev_kind;                                  /* 1 = W25Q, 2 = MAX31855 */
static int sent_before_cs;
uint32_t mock_q_rd(uint32_t off)
{
    if (off == QSPI_SR) return (rx_full && !never_ready) ? 0 : QSR_RX_EMPTY;
    if (off == QSPI_DRR) { rx_full = 0; return rxreg; }
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
    if (off == QSPI_CR) { cr = v; return; }
    if (off == QSPI_DTR) {
        if (cs_idx < 0) { sent_before_cs++; return; }
        uint8_t out = 0xFF;
        if (dev_kind == 1) {
            if (pos == 0) cmd = (uint8_t)v;
            else if (cmd == 0x9F && pos <= 3) out = jedec[pos - 1];
            else if (cmd == 0x05 && pos == 1) out = status1;
        } else if (dev_kind == 2) out = tc[pos];
        pos++; rxreg = out; rx_full = 1;
    }
}
static int fails;
#define CHK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main(void)
{
    qspi_t s;
    CHK(qspi_init(&s, 0, 3, 0, 4) == QSPI_OK);
    CHK(cr == (QCR_MASTER | QCR_MSS | QCR_CPOL | QCR_CPHA | QCR_SPE) && !(cr & QCR_INHIBIT));   /* 0x9E */
    CHK(ssr == 0xF);
    CHK(qspi_init(&s, 0, 4, 0, 4) == QSPI_ERR_ARGS);
    qspi_init(&s, 0, 0, 0, 4);
    CHK(cr == (QCR_MASTER | QCR_MSS | QCR_SPE));                                                  /* 0x86 */

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

    never_ready = 1; s.timeout = 10;
    uint8_t b = 0; CHK(qspi_xfer(&s, &b, &b, 1) == QSPI_ERR_TIMEOUT);
    printf(fails ? "FAILED %d\n" : "ALL PASS\n", fails);
    return fails != 0;
}
