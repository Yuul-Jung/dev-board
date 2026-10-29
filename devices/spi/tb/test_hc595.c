/* 74HC595 호스트 시험: SPI 층을 비트 단위 모델로 대체. 모델 규칙(데이터시트): 클럭 상승마다 시프트(DS → Q0 쪽에서 들어와 Q7S 로 나감),
 * STCP 상승(= CS 해제)에서 시프트 레지스터를 저장 레지스터로 복사. 칩 2개 직렬 연결(Q7S → 다음 DS). */
#include <stdio.h>
#include <string.h>
#include "../sw/spi_devices.h"

#define NCHIP 2
static uint8_t sr[NCHIP], st[NCHIP]; static int stcp_rises, cs_low, fail_xfer;
void qspi_cs(qspi_t *s, unsigned idx, int a) { (void)s; (void)idx;
    if (a) cs_low = 1; else { if (cs_low) { memcpy(st, sr, sizeof st); stcp_rises++; } cs_low = 0; } }
int qspi_xfer(qspi_t *s, const uint8_t *tx, uint8_t *rx, size_t len)
{
    (void)s; (void)rx; if (fail_xfer) return QSPI_ERR_TIMEOUT;
    for (size_t i = 0; i < len; i++) for (int b = 7; b >= 0; b--) {          /* MSB 먼저, 모드 0: 상승에서 샘플 */
        int din = (tx[i] >> b) & 1;
        for (int c = 0; c < NCHIP; c++) {                                    /* DS 는 MCU 쪽 칩(0)으로, 그 Q7S 가 다음 칩 DS */
            int out7 = (sr[c] >> 7) & 1;
            sr[c] = (uint8_t)((sr[c] << 1) | din);
            din = out7;
        }
    }
    return 0;
}

static int bad;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); bad++; } } while (0)

int main(void)
{
    qspi_t s = {0};
    /* 칩 1개: 바이트 그대로 Q7..Q0 */
    uint8_t d1[1] = { 0xA5 };
    CHECK(hc595_write(&s, 0, d1, 1) == QSPI_OK && st[0] == 0xA5 && stcp_rises == 1);
    /* 칩 2개: 첫 번째 바이트가 가장 먼 칩(두 번째 칩, 인덱스 1)에 도착 */
    uint8_t d2[2] = { 0x12, 0x34 };
    CHECK(hc595_write(&s, 0, d2, 2) == QSPI_OK && st[1] == 0x12 && st[0] == 0x34 && stcp_rises == 2);
    /* 저장 레지스터는 전송이 끝나기 전에는 바뀌지 않는다(CS 해제 전) — 모델이 래치하는 시점 확인용으로 중간값 점검 */
    uint8_t before[NCHIP]; memcpy(before, st, sizeof st);
    memset(sr, 0, sizeof sr); /* 시프트 레지스터만 지움(MR) → 저장 레지스터는 유지 */
    CHECK(!memcmp(before, st, sizeof st));
    /* 모든 값 왕복 */
    int roundtrip_ok = 1;
    for (int v = 0; v < 256; v++) { uint8_t d[2] = { (uint8_t)v, (uint8_t)~v }; if (!(hc595_write(&s, 0, d, 2) == QSPI_OK && st[1] == v && st[0] == (uint8_t)~v)) roundtrip_ok = 0; }
    CHECK(roundtrip_ok);
    /* 인자·오류 */
    CHECK(hc595_write(&s, 0, d2, 0) == QSPI_ERR_ARGS && hc595_write(&s, 0, d2, 17) == QSPI_ERR_ARGS && hc595_write(&s, 0, 0, 1) == QSPI_ERR_ARGS);
    fail_xfer = 1; int before_rises = stcp_rises; CHECK(hc595_write(&s, 0, d2, 2) == QSPI_ERR_TIMEOUT && cs_low == 0); fail_xfer = 0; (void)before_rises;
    if (!bad) puts("ALL PASS");
    return bad != 0;
}
