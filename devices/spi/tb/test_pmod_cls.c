/* spi/tb/test_pmod_cls.c — Pmod CLS SPI LCD 드라이버 호스트 모의 시험.
 *
 * 빌드:
 *   cc -include spi/tb/mock_qspi.h -I spi/sw \
 *      spi/tb/test_pmod_cls.c spi/sw/pmod_cls.c spi/sw/axi_qspi.c \
 *      -o /tmp/cls
 * SAN:
 *   cc -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all \
 *      -include spi/tb/mock_qspi.h -I spi/sw \
 *      spi/tb/test_pmod_cls.c spi/sw/pmod_cls.c spi/sw/axi_qspi.c \
 *      -o /tmp/cls
 *
 * 확인 항목:
 *   1. cls_clear → ESC [ j (3바이트)
 *   2. cls_cursor(0,0) → ESC [ 0 ; 0 H (6바이트)
 *   3. cls_cursor(1,9) → ESC [ 1 ; 9 H
 *   4. cls_cursor(0,10) → ESC [ 0 ; A H (열 10 = 'A')
 *   5. cls_cursor(0,15) → ESC [ 0 ; F H (열 15 = 'F')
 *   6. cls_write: 각 문자 개별 전송
 *   7. cls_puts: cursor + write
 *   8. cls_clear_write: clear + 2행 출력
 *   9. 범위 오류: row≥2, col≥16
 *  10. NULL 인수 → CLS_ERR_ARGS
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "pmod_cls.h"

/* ── QSPI mock ─────────────────────────────────────────────────────────── */
/* axi_qspi.c 는 inhibit 해제 후 RX_EMPTY 가 해제될 때까지 기다린다.
 * inhibit 해제 시 미처리 TX 바이트를 RX 큐에 0xFF 더미로 넣어 완료를 알린다.
 * txbuf/ntx 는 리셋 전까지 누적하므로 테스트가 reset_tx() 호출 뒤 ntx 를 읽는다. */
static uint8_t txbuf[1024]; static int ntx;   /* 누적 TX 바이트 (reset_tx 전까지) */
static uint8_t rxq[512]; static int rh, rt;   /* RX 큐 */
static int pending;                            /* 현재 inhibit 구간의 미처리 DTR 바이트 수 */
static uint32_t mock_cr = QCR_INHIBIT;

uint32_t mock_q_rd(uint32_t off)
{
    if (off == QSPI_SR)  return (rt > rh) ? 0 : QSR_RX_EMPTY;
    if (off == QSPI_DRR) return (rt > rh) ? rxq[rh++] : 0u;
    return 0;
}
void mock_q_wr(uint32_t off, uint32_t v)
{
    if (off == QSPI_SSR) return;
    if (off == QSPI_CR) {
        if (v & (QCR_RST_TX | QCR_RST_RX)) { pending = 0; rh = rt = 0; }
        uint32_t prev = mock_cr; mock_cr = v;
        /* inhibit 해제 + SPE: 대기 중인 DTR 바이트마다 더미 RX(0xFF) 추가 */
        if ((prev & QCR_INHIBIT) && !(v & QCR_INHIBIT) && (v & QCR_SPE)) {
            for (int i = 0; i < pending; i++) rxq[rt++] = 0xFF;
            pending = 0;
        }
        return;
    }
    if (off == QSPI_DTR) {
        if (ntx < (int)sizeof txbuf) txbuf[ntx++] = (uint8_t)v;
        pending++;
    }
}

static void reset_tx(void) { ntx = 0; pending = 0; rh = rt = 0; mock_cr = QCR_INHIBIT; }

static int fails;
#define CHK(c, msg) do { if (!(c)) { fails++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); } } while(0)

/* ── 헬퍼: 전송된 바이트 배열이 want 와 같은지 확인 ─────────────────── */
static int eq(const uint8_t *want, int wlen)
{
    if (ntx != wlen) { printf("  ntx=%d want=%d\n", ntx, wlen); return 0; }
    for (int i = 0; i < wlen; i++) if (txbuf[i] != want[i]) {
        printf("  txbuf[%d]=0x%02X want=0x%02X\n", i, txbuf[i], want[i]);
        return 0;
    }
    return 1;
}

int main(void)
{
    qspi_t s;
    cls_t  d;

    /* SPI 초기화 (mode=0, 1 SS, fifo=0 → chunk=1) */
    qspi_init(&s, 0, 0, 0, 1, 0);
    CHK(cls_init(&d, &s, 0) == CLS_OK, "init OK");

    printf("[cls_clear]\n");
    reset_tx();
    CHK(cls_clear(&d) == CLS_OK, "clear OK");
    { const uint8_t w[] = { 0x1B, '[', 'j' }; CHK(eq(w, 3), "ESC[j 3바이트"); }

    printf("[cls_cursor]\n");
    reset_tx();
    CHK(cls_cursor(&d, 0, 0) == CLS_OK, "cursor(0,0)");
    { const uint8_t w[] = { 0x1B, '[', '0', ';', '0', 'H' }; CHK(eq(w, 6), "ESC[0;0H"); }

    reset_tx();
    CHK(cls_cursor(&d, 1, 9) == CLS_OK, "cursor(1,9)");
    { const uint8_t w[] = { 0x1B, '[', '1', ';', '9', 'H' }; CHK(eq(w, 6), "ESC[1;9H"); }

    reset_tx();
    CHK(cls_cursor(&d, 0, 10) == CLS_OK, "cursor(0,10)→A");
    { const uint8_t w[] = { 0x1B, '[', '0', ';', 'A', 'H' }; CHK(eq(w, 6), "ESC[0;AH"); }

    reset_tx();
    CHK(cls_cursor(&d, 0, 15) == CLS_OK, "cursor(0,15)→F");
    { const uint8_t w[] = { 0x1B, '[', '0', ';', 'F', 'H' }; CHK(eq(w, 6), "ESC[0;FH"); }

    printf("[cls_cursor 범위 오류]\n");
    CHK(cls_cursor(&d, 2,  0) == CLS_ERR_RANGE, "row=2 거절");
    CHK(cls_cursor(&d, 0, 16) == CLS_ERR_RANGE, "col=16 거절");

    printf("[cls_write]\n");
    reset_tx();
    CHK(cls_write(&d, "Hi", 0) == CLS_OK, "write 'Hi'");
    { const uint8_t w[] = { 'H', 'i' }; CHK(eq(w, 2), "H i 두 바이트"); }

    reset_tx();
    CHK(cls_write(&d, "AB", 1) == CLS_OK, "write 1바이트만");
    { const uint8_t w[] = { 'A' }; CHK(eq(w, 1), "A 한 바이트"); }

    printf("[cls_puts]\n");
    reset_tx();
    CHK(cls_puts(&d, 1, 5, "OK") == CLS_OK, "puts(1,5,'OK')");
    /* cursor(1,5) = ESC [ 1 ; 5 H (6바이트) + 'O' 'K' (2바이트) = 8바이트 */
    { const uint8_t w[] = { 0x1B,'[','1',';','5','H','O','K' }; CHK(eq(w, 8), "cursor+OK"); }

    printf("[cls_clear_write]\n");
    reset_tx();
    CHK(cls_clear_write(&d, "Hello", "World") == CLS_OK, "clear_write");
    /* ESC[j(3) + cursor(0,0)(6) + "Hello"(5) + cursor(1,0)(6) + "World"(5) = 25바이트 */
    CHK(ntx == 25, "clear_write 총 25바이트");
    CHK(txbuf[0] == 0x1B && txbuf[1] == '[' && txbuf[2] == 'j', "ESC[j 처음");
    CHK(txbuf[9]  == 'H' && txbuf[10] == 'e', "Hello 시작");
    CHK(txbuf[20] == 'W', "World 시작");

    reset_tx();
    CHK(cls_clear_write(&d, "Line1", NULL) == CLS_OK, "line2=NULL");
    /* ESC[j(3) + cursor(0,0)(6) + "Line1"(5) = 14바이트 */
    CHK(ntx == 14, "NULL line2 → 14바이트");

    reset_tx();
    CHK(cls_clear_write(&d, NULL, "L2") == CLS_OK, "line1=NULL");
    /* ESC[j(3) + cursor(1,0)(6) + "L2"(2) = 11바이트 */
    CHK(ntx == 11, "NULL line1 → 11바이트");

    printf("[NULL 인수]\n");
    CHK(cls_init(NULL, &s, 0)     == CLS_ERR_ARGS, "init NULL d");
    CHK(cls_init(&d, NULL, 0)     == CLS_ERR_ARGS, "init NULL spi");
    CHK(cls_clear(NULL)           == CLS_ERR_ARGS, "clear NULL");
    CHK(cls_home(NULL)            == CLS_ERR_ARGS, "home NULL");
    CHK(cls_cursor(NULL, 0, 0)    == CLS_ERR_ARGS, "cursor NULL");
    CHK(cls_write(NULL, "a", 0)   == CLS_ERR_ARGS, "write NULL d");
    CHK(cls_write(&d, NULL, 0)    == CLS_ERR_ARGS, "write NULL text");
    CHK(cls_puts(NULL, 0, 0, "a") == CLS_ERR_ARGS, "puts NULL d");
    CHK(cls_puts(&d, 0, 0, NULL)  == CLS_ERR_ARGS, "puts NULL text");
    CHK(cls_clear_write(NULL, "a", "b") == CLS_ERR_ARGS, "clear_write NULL");

    printf(fails ? "FAILED %d\n" : "ALL PASS\n", fails);
    return fails != 0;
}
