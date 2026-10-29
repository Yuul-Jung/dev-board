/* MCP2515 호스트 시험: 확인된 부분(비트 타이밍, TX 버퍼 배치, [B] 옵코드 바이트열)만 검사한다.
 * [C] 미확인 항목(WRITE/READ RX 옵코드, 모드 레지스터)은 시험하지 않는다 — 시험이 같은 추측을 반복할 뿐이므로. */
#include <stdio.h>
#include <string.h>
#include "../sw/mcp2515.h"

static uint8_t seen[32]; static size_t seen_n; static int cs_cnt, fail; static uint8_t reply[32];
void qspi_cs(qspi_t *s, unsigned i, int a) { (void)s; (void)i; if (!a) cs_cnt++; }
int qspi_xfer(qspi_t *s, const uint8_t *tx, uint8_t *rx, size_t n)
{ (void)s; if (fail) return QSPI_ERR_TIMEOUT; memcpy(seen, tx, n); seen_n = n; if (rx) memcpy(rx, reply, n); return 0; }

static int bad;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); bad++; } } while (0)

/* 독립 전수 탐색: 규칙을 만족하며 정확히 맞는 (BRP, PRSEG, PHSEG1, PHSEG2) 가 존재하는가 */
static int exists_exact(uint32_t fosc, uint32_t br)
{
    for (unsigned brp = 0; brp < 64; brp++) for (unsigned pr = 1; pr <= 8; pr++) for (unsigned p1 = 1; p1 <= 8; p1++) for (unsigned p2 = 2; p2 <= 8; p2++) {
        unsigned n = 1 + pr + p1 + p2; if (n < 8 || n > 25) continue;
        if ((uint64_t)fosc == (uint64_t)2 * (brp + 1) * br * n) return 1;
    }
    return 0;
}

int main(void)
{
    /* ---- 비트 타이밍 ---- */
    static const uint32_t fo[] = { 8000000, 16000000, 20000000 };
    static const uint32_t br[] = { 10000, 20000, 50000, 83333, 100000, 125000, 250000, 500000, 800000, 1000000 };
    int solved = 0, unsolved = 0;
    for (unsigned a = 0; a < 3; a++) for (unsigned b = 0; b < 10; b++) {
        uint8_t c1, c2, c3; int r = mcp2515_calc_timing(fo[a], br[b], &c1, &c2, &c3);
        int ex = exists_exact(fo[a], br[b]);
        CHECK((r == QSPI_OK) == ex);                            /* 해 존재 여부가 독립 탐색과 일치 */
        if (r != QSPI_OK) { CHECK(r == MCP_ERR_TIMING); unsolved++; continue; }
        solved++;
        unsigned sjw = (c1 >> 6) + 1, brp = c1 & 63, ph1 = ((c2 >> 3) & 7) + 1, pr = (c2 & 7) + 1, ph2 = (c3 & 7) + 1;
        unsigned n = 1 + pr + ph1 + ph2;
        CHECK((uint64_t)fo[a] == (uint64_t)2 * (brp + 1) * br[b] * n);           /* 비트레이트 정확 */
        CHECK(ph2 >= 2 && n >= 8 && n <= 25 && sjw >= 1 && sjw <= 4 && sjw <= ph2);
        CHECK((c2 & 0x80) && !(c2 & 0x40) && !(c3 & 0xF8));     /* BTLMODE=1, SAM=0, CNF3 상위 비트 0 */
        unsigned sp = (1 + pr + ph1) * 1000 / n;
        CHECK(sp >= 700 && sp <= 925);
    }
    CHECK(solved > 0 && unsolved > 0);
    /* 대표값: 16 MHz, 500 kbit/s → BRP=0 이면 16 Tq. 표본점 87.5% = 14/16 */
    { uint8_t c1, c2, c3; CHECK(mcp2515_calc_timing(16000000, 500000, &c1, &c2, &c3) == QSPI_OK);
      unsigned brp = c1 & 63, n = 1 + ((c2 & 7) + 1) + (((c2 >> 3) & 7) + 1) + ((c3 & 7) + 1); CHECK(2 * (brp + 1) * n * 500000u == 16000000u); }
    { uint8_t c1, c2, c3; CHECK(mcp2515_calc_timing(0, 500000, &c1, &c2, &c3) == QSPI_ERR_ARGS && mcp2515_calc_timing(8000000, 0, &c1, &c2, &c3) == QSPI_ERR_ARGS); }

    /* ---- TX 버퍼 배치: 손계산 벡터 (SIDH, SIDL, EID8, EID0) ---- */
    mcp_frame_t f; uint8_t o[13]; memset(&f, 0, sizeof f);
    f.id = 0x123; f.dlc = 2; f.data[0] = 0xAA; f.data[1] = 0x55;
    CHECK(mcp2515_pack_tx(&f, o) == 7 && o[0] == 0x24 && o[1] == 0x60 && o[2] == 0 && o[3] == 0 && o[4] == 2 && o[5] == 0xAA && o[6] == 0x55);
    f.ext = 1; f.id = 0x12345678; f.dlc = 1; f.data[0] = 7;
    CHECK(mcp2515_pack_tx(&f, o) == 6 && o[0] == 0x91 && o[1] == 0xA8 && o[2] == 0x56 && o[3] == 0x78 && o[4] == 1 && o[5] == 7);
    f.id = 0x1FFFFFFF; f.dlc = 8; memset(f.data, 0xFF, 8);
    CHECK(mcp2515_pack_tx(&f, o) == 13 && o[0] == 0xFF && o[1] == 0xEB && o[2] == 0xFF && o[3] == 0xFF && o[4] == 8);
    f.rtr = 1; f.dlc = 4; CHECK(mcp2515_pack_tx(&f, o) == 5 && o[4] == 0x44);       /* RTR: 데이터 없음 */
    f.rtr = 0; f.ext = 0; f.id = 0x800; CHECK(mcp2515_pack_tx(&f, o) == QSPI_ERR_ARGS);   /* 표준 ID 11비트 초과 */
    f.ext = 1; f.id = 0x20000000; CHECK(mcp2515_pack_tx(&f, o) == QSPI_ERR_ARGS);
    f.id = 1; f.dlc = 9; CHECK(mcp2515_pack_tx(&f, o) == QSPI_ERR_ARGS);

    /* ---- [B] 옵코드 바이트열 ---- */
    qspi_t s = {0};
    CHECK(mcp2515_reset(&s, 0) == QSPI_OK && seen_n == 1 && seen[0] == 0xC0);
    reply[2] = 0xAB; reply[3] = 0xCD; uint8_t b[2];
    CHECK(mcp2515_read_regs(&s, 0, 0x2A, b, 2) == QSPI_OK && seen_n == 4 && seen[0] == 0x03 && seen[1] == 0x2A && b[0] == 0xAB && b[1] == 0xCD);
    CHECK(mcp2515_read_regs(&s, 0, 0, b, 0) == QSPI_ERR_ARGS && mcp2515_read_regs(&s, 0, 0, b, 17) == QSPI_ERR_ARGS);
    CHECK(mcp2515_bit_modify(&s, 0, 0x60, 0x60, 0x60) == QSPI_OK && seen_n == 4 && seen[0] == 0x05 && seen[1] == 0x60 && seen[2] == 0x60 && seen[3] == 0x60);
    CHECK(mcp2515_rts(&s, 0, 0) == QSPI_OK && seen[0] == 0x81 && mcp2515_rts(&s, 0, 2) == QSPI_OK && seen[0] == 0x84 && mcp2515_rts(&s, 0, 3) == QSPI_ERR_ARGS);
    reply[1] = 0x5A; uint8_t st; CHECK(mcp2515_read_status(&s, 0, &st) == QSPI_OK && seen[0] == 0xA0 && st == 0x5A);
    f.ext = 0; f.id = 0x123; f.dlc = 1; f.rtr = 0; f.data[0] = 9;
    CHECK(mcp2515_load_tx(&s, 0, 1, &f) == QSPI_OK && seen_n == 7 && seen[0] == 0x42 && seen[1] == 0x24 && seen[2] == 0x60 && seen[5] == 1 && seen[6] == 9);
    CHECK(mcp2515_load_tx(&s, 0, 3, &f) == QSPI_ERR_ARGS);
    CHECK(MCP_TXB_CTRL(0) == 0x30 && MCP_TXB_CTRL(1) == 0x40 && MCP_TXB_CTRL(2) == 0x50 && MCP_TXB_SIDH(2) == 0x51);
    fail = 1; int c0 = cs_cnt; CHECK(mcp2515_reset(&s, 0) == QSPI_ERR_TIMEOUT && cs_cnt == c0 + 1); fail = 0;   /* 실패해도 CS 해제 */
    if (!bad) puts("ALL PASS");
    return bad != 0;
}
