#include "axi_can.h"

int can_calc_timing(uint32_t clk, uint32_t bps, can_timing_t *t)
{
    if (!clk || !bps || !t) return CAN_ERR_ARGS;
    int found = 0; int64_t best = 1 << 30;
    for (uint32_t brp = 1; brp <= 256; brp++) {
        for (uint32_t ntq = 4; ntq <= 25; ntq++) {
            if ((uint64_t)brp * ntq * bps != clk) continue;
            /* 1(동기) + TS1(1~16) + TS2(1~8) = ntq, SJW 1~4 이면서 TS2 이하로 둠 */
            for (uint32_t ts2 = 1; ts2 <= 8; ts2++) {
                if (ntq < 2 + ts2) continue;
                uint32_t ts1 = ntq - 1 - ts2;
                if (ts1 < 1 || ts1 > 16) continue;
                int64_t sp_x1000 = (int64_t)(1 + ts1) * 1000 / ntq;
                int64_t d = sp_x1000 - 875; if (d < 0) d = -d;
                if (d < best) {
                    best = d; found = 1;
                    uint32_t sjw = ts2 < 4 ? ts2 : 4;
                    t->brpr = (uint8_t)(brp - 1);
                    t->btr = (uint16_t)(((sjw - 1) << 7) | ((ts2 - 1) << 4) | (ts1 - 1));
                }
            }
        }
    }
    return found ? CAN_OK : CAN_ERR_TIMING;
}

int can_init(uintptr_t b, const can_timing_t *t, int loopback)
{
    if (!t) return CAN_ERR_ARGS;
    CAN_WR(b, CAN_SRR, 0);                                 /* CEN=0: 설정 모드 */
    CAN_WR(b, CAN_BRPR, t->brpr);
    CAN_WR(b, CAN_BTR, t->btr);
    CAN_WR(b, CAN_MSR, loopback ? CAN_MSR_LBACK : 0);
    CAN_WR(b, CAN_SRR, CAN_SRR_CEN);
    return CAN_OK;  /* CONFIG 해제는 버스 11 연속 열성 비트 후라서 호출자가 CAN_SR 로 확인 */
}

static int put(uintptr_t b, uint32_t base_off, const can_msg_t *m)
{
    if (!m || m->dlc > 8) return CAN_ERR_ARGS;
    uint32_t id;
    if (m->ext) {
        if (m->id > 0x1FFFFFFFu) return CAN_ERR_ARGS;
        id = ((m->id >> 18) << 21) | (1u << 20) | (1u << 19) | (((m->id & 0x3FFFFu)) << 1) | (m->rtr ? 1u : 0u);
    } else {
        if (m->id > 0x7FFu) return CAN_ERR_ARGS;
        id = (m->id << 21) | (m->rtr ? (1u << 20) : 0u);
    }
    uint32_t dw1 = 0, dw2 = 0;
    for (int i = 0; i < 4; i++) { dw1 |= (uint32_t)m->data[i] << (24 - 8 * i); dw2 |= (uint32_t)m->data[i + 4] << (24 - 8 * i); }
    if (m->dlc < 8) { /* 사용하지 않는 데이터 필드는 0 */
        for (int i = m->dlc; i < 8; i++) { if (i < 4) dw1 &= ~(0xFFu << (24 - 8 * i)); else dw2 &= ~(0xFFu << (24 - 8 * (i - 4))); }
    }
    CAN_WR(b, base_off + 0, id);
    CAN_WR(b, base_off + 4, (uint32_t)m->dlc << 28);
    CAN_WR(b, base_off + 8, dw1);
    CAN_WR(b, base_off + 12, dw2);              /* 4워드 모두 써야 함 */
    return CAN_OK;
}

int can_send(uintptr_t b, const can_msg_t *m)
{
    if (CAN_RD(b, CAN_SR) & CAN_SR_TXFLL) return CAN_ERR_FULL;
    return put(b, CAN_TXFIFO, m);
}
int can_send_hp(uintptr_t b, const can_msg_t *m)
{
    if (CAN_RD(b, CAN_SR) & CAN_SR_TXBFLL) return CAN_ERR_FULL;
    return put(b, CAN_TXHPB, m);
}

int can_recv(uintptr_t b, can_msg_t *m)
{
    if (!(CAN_RD(b, CAN_ISR) & CAN_IS_RXNEMP)) return CAN_ERR_EMPTY;
    uint32_t id = CAN_RD(b, CAN_RXFIFO + 0), dlc = CAN_RD(b, CAN_RXFIFO + 4);
    uint32_t dw1 = CAN_RD(b, CAN_RXFIFO + 8), dw2 = CAN_RD(b, CAN_RXFIFO + 12);
    CAN_WR(b, CAN_ICR, CAN_IS_RXNEMP);     /* RXNEMP 는 ICR 쓰기로만 지워짐 */
    m->ext = (id >> 19) & 1;
    if (m->ext) {
        m->id = ((id >> 21) << 18) | ((id >> 1) & 0x3FFFFu);
        m->rtr = id & 1;
    } else {
        m->id = id >> 21;
        m->rtr = (id >> 20) & 1;
    }
    m->dlc = (uint8_t)(dlc >> 28);
    for (int i = 0; i < 4; i++) { m->data[i] = (uint8_t)(dw1 >> (24 - 8 * i)); m->data[i + 4] = (uint8_t)(dw2 >> (24 - 8 * i)); }
    return CAN_OK;
}
