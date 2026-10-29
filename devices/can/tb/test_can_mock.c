#include <stdio.h>
#include <string.h>
#include "../sw/axi_can.h"

/* 모의: 루프백처럼 TX FIFO 4워드를 RX FIFO 큐로 넘긴다. RXNEMP 는 큐가 비어 있지 않으면 ICR 후에도 다시 세트된다고 가정(미확인 가정) */
static uint32_t reg[0x80 / 4]; static uint32_t txw[4]; static int txn;
static uint32_t rxq[64][4]; static int rq_h, rq_t; static int rxw;
static uint32_t isr;
uint32_t mock_can_rd(uint32_t off)
{
    if (off == CAN_ISR) return (rq_h != rq_t) ? (isr | CAN_IS_RXNEMP) : (isr & ~CAN_IS_RXNEMP);
    if (off >= CAN_RXFIFO && off < CAN_RXFIFO + 16) {
        uint32_t v = rxq[rq_h][(off - CAN_RXFIFO) / 4];
        if (off == CAN_RXFIFO + 12) rq_h++;
        return v;
    }
    return reg[off / 4];
}
void mock_can_wr(uint32_t off, uint32_t v)
{
    if (off >= CAN_TXFIFO && off < CAN_TXFIFO + 16) {
        txw[(off - CAN_TXFIFO) / 4] = v;
        if (++txn == 4) { memcpy(rxq[rq_t++], txw, 16); txn = 0; }
        return;
    }
    if (off == CAN_ICR) { isr &= ~v; return; }
    reg[off / 4] = v; (void)rxw;
}
static int fails;
#define CHK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main(void)
{
    can_timing_t t;
    /* PG096 예제: 24 MHz, 1 Mbps → BRPR=2, 8 tq/bit */
    CHK(can_calc_timing(24000000, 1000000, &t) == CAN_OK);
    CHK(t.brpr == 2);
    CHK((((t.btr & 0xF) + 1) + (((t.btr >> 4) & 7) + 1) + 1) == 8);
    CHK(can_calc_timing(24000000, 500000, &t) == CAN_OK && (t.brpr + 1) * 1 > 0);
    CHK(can_calc_timing(24000000, 7000000, &t) == CAN_ERR_TIMING);

    can_calc_timing(24000000, 1000000, &t);
    CHK(can_init(0, &t, 1) == CAN_OK);
    CHK(reg[CAN_BRPR / 4] == 2 && reg[CAN_MSR / 4] == CAN_MSR_LBACK && reg[CAN_SRR / 4] == CAN_SRR_CEN);

    can_msg_t m = { .id = 0x123, .ext = 0, .rtr = 0, .dlc = 3, .data = {0xDE, 0xAD, 0xBE, 0xEF} }, r;
    CHK(can_recv(0, &r) == CAN_ERR_EMPTY);
    CHK(can_send(0, &m) == CAN_OK);
    CHK(rxq[0][0] == (0x123u << 21) && rxq[0][1] == (3u << 28));
    CHK(rxq[0][2] == 0xDEADBE00u && rxq[0][3] == 0);       /* 사용 안 하는 바이트는 0 */
    CHK(can_recv(0, &r) == CAN_OK && r.id == 0x123 && !r.ext && r.dlc == 3 && r.data[2] == 0xBE);

    can_msg_t e = { .id = 0x1ABCDE01, .ext = 1, .rtr = 0, .dlc = 8, .data = {1,2,3,4,5,6,7,8} };
    CHK(can_send(0, &e) == CAN_OK && can_recv(0, &r) == CAN_OK);
    CHK(r.ext && r.id == 0x1ABCDE01 && r.dlc == 8 && r.data[7] == 8);
    CHK(((rxq[1][0] >> 19) & 3) == 3);                      /* SRR/RTR=1, IDE=1 */
    e.id = 0x20000000; CHK(can_send(0, &e) == CAN_ERR_ARGS);
    m.dlc = 9; CHK(can_send(0, &m) == CAN_ERR_ARGS);
    reg[CAN_SR / 4] = CAN_SR_TXFLL; m.dlc = 1; CHK(can_send(0, &m) == CAN_ERR_FULL);
    printf(fails ? "FAILED %d\n" : "ALL PASS\n", fails);
    return fails != 0;
}
