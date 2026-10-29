/* MCP2515 장치 모델 시험 (DS20001801K 규칙 구현). SPI 레벨: CS 하강 → 명령 바이트 → 데이터, CS 상승에서 마무리.
 * 구현한 규칙:
 *  - 전원/RESET: 구성 모드(CANCTRL=87h, CANSTAT=80h), 나머지 0.   - CNF1~3 쓰기는 구성 모드에서만 반영.
 *  - READ(03h)/WRITE(02h) 순차 접근, BIT MODIFY(05h).   - LOAD TX(0100 0abc): 31h,36h,41h,46h,51h,56h.
 *  - RTS(1000 0nnn): TXREQ 설정. 루프백 모드에서는 TXB 를 RXB0 로 복사하고 RX0IF 설정, TXREQ 해제, TXnIF 설정.
 *  - READ RX BUFFER(1001 0nm0): 61h,66h,71h,76h 부터 순차 읽기, CS 상승 때 해당 RXnIF 해제.
 *  - READ STATUS(A0h): bit0 RX0IF, 1 RX1IF, 2 TXREQ0, 3 TX0IF, 4 TXREQ1, 5 TX1IF, 6 TXREQ2, 7 TX2IF. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../sw/mcp2515.h"

static uint8_t R[0x80]; static int cs_low, pending_rxclr = -1, mode_stuck, tx_stuck;
static void model_reset(void) { memset(R, 0, sizeof R); R[MCP_CANCTRL] = 0x87; R[MCP_CANSTAT] = 0x80; }
static void apply_ctrl(void) { if (!mode_stuck) R[MCP_CANSTAT] = (uint8_t)(R[MCP_CANSTAT] & 0x1F) | (R[MCP_CANCTRL] & 0xE0); }
static int in_config(void) { return (R[MCP_CANSTAT] & 0xE0) == 0x80; }
static void wr(uint8_t a, uint8_t v)
{
    if (a >= 0x80) return;
    if ((a == MCP_CNF1 || a == MCP_CNF2 || a == MCP_CNF3) && !in_config()) return;     /* 구성 모드에서만 */
    if (a == MCP_CANSTAT) return;                                                       /* 읽기 전용 */
    R[a] = v; if (a == MCP_CANCTRL) apply_ctrl();
}
static void do_rts(unsigned n)
{
    uint8_t base = (uint8_t)(0x30 + 0x10 * n);
    if (tx_stuck) { R[base] |= MCP_TXREQ; return; }
    if ((R[MCP_CANSTAT] & 0xE0) == MCP_MODE_LOOPBACK) {
        uint8_t dlc = R[base + 5];
        int ext = (R[base + 2] >> 3) & 1;
        R[0x61] = R[base + 1];
        R[0x62] = (uint8_t)((R[base + 2] & 0xE3) | (ext ? 0x08 : 0) | ((!ext && (dlc & 0x40)) ? 0x10 : 0));   /* SRR / IDE */
        R[0x63] = R[base + 3]; R[0x64] = R[base + 4];
        R[0x65] = (uint8_t)(ext ? (dlc & 0x4F) : (dlc & 0x0F));
        memcpy(&R[0x66], &R[base + 6], 8);
        R[MCP_CANINTF] |= MCP_RX0IF;
    }
    R[base] &= (uint8_t)~MCP_TXREQ; R[MCP_CANINTF] |= (uint8_t)(MCP_TX0IF << n);
}
void qspi_cs(qspi_t *s, unsigned i, int a)
{
    (void)s; (void)i;
    if (a) { cs_low = 1; return; }
    if (cs_low && pending_rxclr >= 0) { R[MCP_CANINTF] &= (uint8_t)~(1u << pending_rxclr); }
    pending_rxclr = -1; cs_low = 0;
}
static uint8_t status(void)
{
    uint8_t f = R[MCP_CANINTF], s = 0;
    s |= (f & 1) | (f & 2); s |= (uint8_t)(((R[0x30] >> 3) & 1) << 2); s |= (uint8_t)(((f >> 2) & 1) << 3);
    s |= (uint8_t)(((R[0x40] >> 3) & 1) << 4); s |= (uint8_t)(((f >> 3) & 1) << 5);
    s |= (uint8_t)(((R[0x50] >> 3) & 1) << 6); s |= (uint8_t)(((f >> 4) & 1) << 7);
    return s;
}
int qspi_xfer(qspi_t *s, const uint8_t *tx, uint8_t *rx, size_t n)
{
    (void)s; uint8_t op = tx[0]; if (rx) memset(rx, 0, n);
    if (op == MCP_OP_RESET) { model_reset(); return 0; }
    if (op == MCP_OP_READ_STATUS) { if (rx) for (size_t i = 1; i < n; i++) rx[i] = status(); return 0; }
    if (op == MCP_OP_READ && n >= 2) { uint8_t a = tx[1]; for (size_t i = 2; i < n; i++) { uint8_t v = (a < 0x80) ? R[a] : 0; a++; if (rx) rx[i] = v; } return 0; }
    if (op == MCP_OP_WRITE && n >= 3) { uint8_t a = tx[1]; for (size_t i = 2; i < n; i++) wr(a++, tx[i]); return 0; }
    if (op == MCP_OP_BIT_MODIFY && n == 4) { uint8_t a = tx[1]; wr(a, (uint8_t)((R[a] & ~tx[2]) | (tx[3] & tx[2]))); return 0; }
    if ((op & 0xF8) == MCP_OP_LOAD_TX) {
        static const uint8_t base[6] = { 0x31, 0x36, 0x41, 0x46, 0x51, 0x56 };
        uint8_t a = base[op & 7]; for (size_t i = 1; i < n; i++) R[a++] = tx[i]; return 0;
    }
    if ((op & 0xF8) == MCP_OP_RTS && op != 0x80) { for (unsigned k = 0; k < 3; k++) if (op & (1u << k)) do_rts(k); return 0; }
    if ((op & 0xF9) == MCP_OP_READ_RXB) {
        unsigned nn = (op >> 2) & 1, m = (op >> 1) & 1;
        uint8_t a = (uint8_t)(nn ? (m ? 0x76 : 0x71) : (m ? 0x66 : 0x61));
        for (size_t i = 1; i < n; i++) if (rx) rx[i] = R[a++];
        pending_rxclr = (int)nn; return 0;
    }
    return -9;                                         /* 모르는 명령 */
}

static int bad;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); bad++; } } while (0)
static int same(const mcp_frame_t *a, const mcp_frame_t *b)
{
    if (a->id != b->id || a->ext != b->ext || a->rtr != b->rtr || a->dlc != b->dlc) return 0;
    return a->rtr || !memcmp(a->data, b->data, a->dlc);
}

int main(void)
{
    qspi_t s = {0}; mcp_frame_t f, g; uint8_t v;
    model_reset();
    CHECK(mcp2515_read_regs(&s, 0, MCP_CANSTAT, &v, 1) == 0 && v == 0x80);
    CHECK(mcp2515_read_regs(&s, 0, MCP_CANCTRL, &v, 1) == 0 && v == 0x87);
    /* RESET 이후 상태 복구 */
    R[0x10] = 0x55; CHECK(mcp2515_reset(&s, 0) == 0 && R[0x10] == 0 && R[MCP_CANCTRL] == 0x87);
    /* init: 16 MHz / 500 kbit/s, 루프백. CNF 는 구성 모드에서만 쓰여야 한다 */
    CHECK(mcp2515_init(&s, 0, 16000000, 500000, MCP_MODE_LOOPBACK) == 0);
    CHECK((R[MCP_CANSTAT] & 0xE0) == MCP_MODE_LOOPBACK);
    { uint8_t c1, c2, c3; mcp2515_calc_timing(16000000, 500000, &c1, &c2, &c3); CHECK(R[MCP_CNF1] == c1 && R[MCP_CNF2] == c2 && R[MCP_CNF3] == c3 && c2 != 0); }
    CHECK(R[MCP_RXB0CTRL] == MCP_RXM_ANY && R[MCP_RXB1CTRL] == MCP_RXM_ANY && R[MCP_CANINTE] == 0);
    CHECK(R[MCP_CANCTRL] >> 5 == 2);                                               /* REQOP = 010 */
    /* 구성 모드가 아닐 때 CNF 쓰기는 무시됨(모델 규칙) → 드라이버가 구성 모드에서 썼는지 위에서 확인됨 */
    { uint8_t keep = R[MCP_CNF1], w = (uint8_t)(keep ^ 0xFF); mcp2515_write_regs(&s, 0, MCP_CNF1, &w, 1); CHECK(R[MCP_CNF1] == keep); }   /* 루프백 모드에서 CNF 쓰기는 무시 */
    /* 루프백 송수신 왕복 */
    memset(&f, 0, sizeof f); f.id = 0x123; f.dlc = 3; f.data[0] = 1; f.data[1] = 2; f.data[2] = 3;
    CHECK(mcp2515_recv(&s, 0, &g) == MCP_ERR_NODATA);
    CHECK(mcp2515_send(&s, 0, &f) == 0 && (R[MCP_CANINTF] & MCP_RX0IF));
    CHECK(mcp2515_recv(&s, 0, &g) == 0 && same(&f, &g) && !(R[MCP_CANINTF] & MCP_RX0IF));   /* 읽은 뒤 RX0IF 자동 해제 */
    CHECK(mcp2515_recv(&s, 0, &g) == MCP_ERR_NODATA);
    /* 확장 ID, RTR(표준·확장), DLC 0, DLC 8 */
    mcp_frame_t cases[] = {
        { 0x1FFFFFFF, 1, 0, 8, { 1, 2, 3, 4, 5, 6, 7, 8 } },
        { 0x12345678, 1, 0, 1, { 0xAB } },
        { 0x12345678, 1, 1, 4, { 0 } },
        { 0x7FF, 0, 1, 2, { 0 } },
        { 0x000, 0, 0, 0, { 0 } },
        { 0x7FF, 0, 0, 8, { 9, 9, 9, 9, 9, 9, 9, 0xEE } },
        { 0x00040000, 1, 0, 2, { 0xDE, 0xAD } },        /* EID 비트가 모두 0 이고 SID LSB = 1 */
        { 0x0003FFFF, 1, 0, 2, { 0xBE, 0xEF } },        /* EID[17:0] 만 1 */
    };
    for (unsigned i = 0; i < sizeof cases / sizeof *cases; i++) {
        CHECK(mcp2515_send(&s, 0, &cases[i]) == 0 && mcp2515_recv(&s, 0, &g) == 0 && same(&cases[i], &g));
    }
    /* 무작위 20000 프레임 왕복 */
    srand(11); int rt_ok = 1;
    for (int k = 0; k < 20000; k++) {
        memset(&f, 0, sizeof f); f.ext = rand() & 1; f.id = f.ext ? ((uint32_t)rand() * 7919u) & 0x1FFFFFFFu : (uint32_t)rand() & 0x7FF;
        f.rtr = rand() % 5 == 0; f.dlc = (uint8_t)(rand() % 9); for (int j = 0; j < 8; j++) f.data[j] = (uint8_t)rand();
        if (f.rtr) memset(f.data, 0, 8);
        if (mcp2515_send(&s, 0, &f) || mcp2515_recv(&s, 0, &g) || !same(&f, &g)) { rt_ok = 0; break; }
    }
    CHECK(rt_ok);
    /* 수신 버퍼 1 경로: RXB1 에 직접 메시지를 넣고 RX1IF 설정 → recv 가 RXB1 에서 읽고 RX1IF 해제 */
    R[0x71] = 0x24; R[0x72] = 0x60; R[0x75] = 2; R[0x76] = 0xCA; R[0x77] = 0xFE; R[MCP_CANINTF] |= MCP_RX1IF;
    CHECK(mcp2515_recv(&s, 0, &g) == 0 && g.id == 0x123 && !g.ext && g.dlc == 2 && g.data[0] == 0xCA && g.data[1] == 0xFE && !(R[MCP_CANINTF] & MCP_RX1IF));
    /* 두 버퍼에 모두 있으면 RXB0 먼저 */
    R[0x61] = 0x11; R[0x62] = 0; R[0x65] = 0; R[MCP_CANINTF] |= MCP_RX0IF;
    R[0x71] = 0x22; R[0x72] = 0; R[0x75] = 0; R[MCP_CANINTF] |= MCP_RX1IF;
    CHECK(mcp2515_recv(&s, 0, &g) == 0 && g.id == (0x11u << 3) && (R[MCP_CANINTF] & MCP_RX1IF) && !(R[MCP_CANINTF] & MCP_RX0IF));
    CHECK(mcp2515_recv(&s, 0, &g) == 0 && g.id == (0x22u << 3) && !(R[MCP_CANINTF] & 3));
    /* 송신 버퍼 모두 사용 중 → TXFULL */
    tx_stuck = 1; memset(&f, 0, sizeof f); f.id = 1; f.dlc = 1;
    CHECK(mcp2515_send(&s, 0, &f) == 0 && mcp2515_send(&s, 0, &f) == 0 && mcp2515_send(&s, 0, &f) == 0 && mcp2515_send(&s, 0, &f) == MCP_ERR_TXFULL);
    tx_stuck = 0; R[0x30] = R[0x40] = R[0x50] = 0;                                 /* 걸린 전송 요청 정리 */
    /* 모드 전환 실패 감지 */
    mode_stuck = 1; CHECK(mcp2515_set_mode(&s, 0, MCP_MODE_NORMAL, 3) == MCP_ERR_MODE); mode_stuck = 0;
    CHECK(mcp2515_set_mode(&s, 0, MCP_MODE_NORMAL, 3) == 0 && (R[MCP_CANSTAT] & 0xE0) == 0);
    CHECK(mcp2515_set_mode(&s, 0, MCP_MODE_CONFIG, 3) == 0 && R[MCP_CANCTRL] == (0x80 | (R[MCP_CANCTRL] & 0x1F)));
    CHECK(mcp2515_set_mode(&s, 0, 0x10, 3) == QSPI_ERR_ARGS && mcp2515_set_mode(&s, 0, 0xA0, 3) == QSPI_ERR_ARGS);
    /* 정상 모드에서는 루프백이 아니므로 수신 없음, 송신 후 TX0IF */
    mcp2515_set_mode(&s, 0, MCP_MODE_NORMAL, 3); R[MCP_CANINTF] = 0; f.id = 5;
    CHECK(mcp2515_send(&s, 0, &f) == 0 && !(R[MCP_CANINTF] & MCP_RX0IF) && (R[MCP_CANINTF] & MCP_TX0IF));
    /* 정상 모드로 동작 중인 장치에 init 을 다시 호출 → 드라이버가 먼저 구성 모드로 들어가야 CNF 가 바뀐다 */
    CHECK((R[MCP_CANSTAT] & 0xE0) == MCP_MODE_NORMAL);
    { uint8_t c1, c2, c3; CHECK(mcp2515_calc_timing(8000000, 125000, &c1, &c2, &c3) == 0);
      CHECK(mcp2515_init(&s, 0, 8000000, 125000, MCP_MODE_LISTEN) == 0 && R[MCP_CNF1] == c1 && R[MCP_CNF2] == c2 && R[MCP_CNF3] == c3 && (R[MCP_CANSTAT] & 0xE0) == MCP_MODE_LISTEN); }
    /* 해가 없는 타이밍이면 init 이 즉시 실패 */
    CHECK(mcp2515_init(&s, 0, 16000000, 83333, MCP_MODE_NORMAL) == MCP_ERR_TIMING);
    if (!bad) puts("ALL PASS");
    return bad != 0;
}
