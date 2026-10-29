/* MCP3008 / ADXL345 호스트 시험. SPI 층(qspi_cs/qspi_xfer)을 비트 단위 장치 모델로 대체.
 * MCP3008 모델은 데이터시트의 직렬 규칙(CS Low 후 DIN 첫 1 = 시작 비트, SGL/D2/D1/D0, 샘플 클럭 1개,
 * 널 비트, B9..B0)만 구현하고, 프레임 바이트 배치는 모델이 아니라 드라이버가 맞춰야 한다. */
#include <stdio.h>
#include <string.h>
#include "../sw/spi_sensors.h"

static int cs_cur = -1, cs_high_between, nack;
static uint16_t adc_in[16];            /* 모델이 반환할 값: 단일 종단 [0..7], 차동 [8..15] (인덱스 = 8 + d) */
void qspi_cs(qspi_t *s, unsigned idx, int a) { (void)s; if (a) { cs_cur = (int)idx; } else { cs_cur = -1; cs_high_between++; } }

/* ADXL345 모델 */
static uint16_t dac_word; static int dac_writes;
static uint8_t ax[64]; static uint8_t last_cmd;

static int mcp_model(const uint8_t *tx, uint8_t *rx, size_t len)
{
    int started = 0, k = -1; unsigned cmd = 0, nbits = 0; uint16_t res = 0;
    memset(rx, 0, len);
    for (size_t clk = 0; clk < len * 8; clk++) {
        int din = (tx[clk / 8] >> (7 - clk % 8)) & 1, dout = 0;
        if (!started) { if (din) { started = 1; k = 0; } }
        else {
            k++;
            if (k <= 4) { cmd = (cmd << 1) | (unsigned)din; nbits++;
                if (k == 4) { int sgl = (cmd >> 3) & 1, d = cmd & 7; res = sgl ? adc_in[d] : adc_in[8 + d]; } }
            else if (k == 5) { /* 샘플 종료용 추가 클럭, DIN 무관 */ }
            else if (k == 6) dout = 0;                                  /* 널 비트 */
            else if (k >= 7 && k <= 16) dout = (res >> (16 - k)) & 1;   /* B9..B0 */
            else dout = 0;                                              /* 이후 LSB 우선 재출력은 모델링하지 않음 */
        }
        if (dout) rx[clk / 8] |= (uint8_t)(0x80 >> (clk % 8));
    }
    (void)nbits; return 0;
}

int qspi_xfer(qspi_t *s, const uint8_t *tx, uint8_t *rx, size_t len)
{
    (void)s; if (nack) return QSPI_ERR_TIMEOUT;
    static uint8_t dummy[16]; uint8_t t[16] = {0};
    if (len > 16) return QSPI_ERR_ARGS;
    if (tx) memcpy(t, tx, len);
    if (!rx) rx = dummy;
    if (cs_cur == 5) return mcp_model(t, rx, len);
    if (cs_cur == 7) {                                   /* MCP4921: 16클럭 모두 받아야 래치 */
        if (len != 2) return QSPI_ERR_ARGS;
        dac_word = (uint16_t)((t[0] << 8) | t[1]); dac_writes++; return 0;
    }
    if (cs_cur == 6) {                                   /* ADXL345 */
        last_cmd = t[0]; unsigned reg = t[0] & 0x3F; int rd = t[0] & 0x80, mb = t[0] & 0x40;
        rx[0] = 0;
        for (size_t i = 1; i < len; i++) {
            if (rd) { rx[i] = ax[reg & 63]; if (mb) reg++; }
            else { ax[reg & 63] = t[i]; if (mb) reg++; }
        }
        return 0;
    }
    return QSPI_ERR_ARGS;
}

static int bad;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); bad++; } } while (0)

int main(void)
{
    qspi_t s = {0}; uint16_t code;
    /* ---- MCP3008 ---- */
    for (int ch = 0; ch < 8; ch++) adc_in[ch] = (uint16_t)(100 * ch + 7), adc_in[8 + ch] = (uint16_t)(1000 - 111 * ch);
    for (int ch = 0; ch < 8; ch++) {
        CHECK(mcp3008_read(&s, 5, 1, ch, &code) == QSPI_OK && code == 100 * ch + 7);
        CHECK(mcp3008_read(&s, 5, 0, ch, &code) == QSPI_OK && code == (uint16_t)(1000 - 111 * ch));
    }
    uint16_t edge[] = { 0, 1, 2, 3, 0x155, 0x2AA, 0x1FF, 0x200, 1022, 1023 };
    for (unsigned i = 0; i < sizeof edge / sizeof *edge; i++) {
        adc_in[3] = edge[i]; CHECK(mcp3008_read(&s, 5, 1, 3, &code) == QSPI_OK && code == edge[i]);
    }
    for (unsigned v = 0; v < 1024; v++) { adc_in[0] = (uint16_t)v; CHECK(mcp3008_read(&s, 5, 1, 0, &code) == QSPI_OK && code == v); }
    cs_high_between = 0; mcp3008_read(&s, 5, 1, 0, &code); CHECK(cs_high_between == 1 && cs_cur == -1);   /* 변환마다 CS 상승 */
    CHECK(mcp3008_read(&s, 5, 1, 8, &code) == QSPI_ERR_ARGS && mcp3008_read(&s, 5, 2, 0, &code) == QSPI_ERR_ARGS);
    CHECK(mcp3008_read(&s, 5, 1, -1, &code) == QSPI_ERR_ARGS && mcp3008_read(&s, 5, 1, 0, 0) == QSPI_ERR_ARGS);
    nack = 1; CHECK(mcp3008_read(&s, 5, 1, 0, &code) == QSPI_ERR_TIMEOUT && cs_cur == -1); nack = 0;
    CHECK(mcp3008_to_mv(1023, 3300) == 3296 && mcp3008_to_mv(512, 3300) == 1650 && mcp3008_to_mv(0, 3300) == 0);
    /* ---- ADXL345 ---- */
    uint8_t v; memset(ax, 0, sizeof ax); ax[0] = 0x00;
    CHECK(adxl345_init(&s, 6) == ADXL_ERR_DEVID);                              /* 응답 없음/잘못된 ID */
    ax[0] = 0xE5;
    CHECK(adxl345_init(&s, 6) == QSPI_OK && ax[0x31] == 0x0B && ax[0x2D] == 0x08 && ax[0] == 0xE5);   /* DEVID 는 쓰기로 안 바뀜(모델은 쓰기 허용하나 드라이버가 쓰지 않음) */
    CHECK(adxl345_read_reg(&s, 6, 0x00, &v) == QSPI_OK && v == 0xE5 && last_cmd == 0x80);
    CHECK(adxl345_write_reg(&s, 6, 0x2D, 0x08) == QSPI_OK && last_cmd == 0x2D);
    CHECK(adxl345_read_reg(&s, 6, 0x40, &v) == QSPI_ERR_ARGS && adxl345_write_reg(&s, 6, 0x40, 0) == QSPI_ERR_ARGS);
    int16_t want[3] = { 256, -1, -4096 }, xyz[3];
    for (int i = 0; i < 3; i++) { ax[0x32 + 2 * i] = (uint8_t)(want[i] & 0xFF); ax[0x33 + 2 * i] = (uint8_t)((uint16_t)want[i] >> 8); }
    CHECK(adxl345_read_xyz(&s, 6, xyz) == QSPI_OK && last_cmd == 0xF2);       /* AN-1077 의 읽기 명령 */
    CHECK(xyz[0] == 256 && xyz[1] == -1 && xyz[2] == -4096);
    CHECK(adxl345_to_mg(256) == 998 && adxl345_to_mg(-1) == -3 && adxl345_to_mg(-4096) == -15974);
    nack = 1; CHECK(adxl345_read_xyz(&s, 6, xyz) == QSPI_ERR_TIMEOUT); nack = 0;
    /* ---- MCP4921 ---- */
    CHECK(mcp4921_write(&s, 7, 0, 1, 0) == QSPI_OK && dac_word == 0x1000);   /* gain2x=1 → GA=0, 버퍼 없음, 활성 */
    CHECK(mcp4921_write(&s, 7, 4095, 0, 0) == QSPI_OK && dac_word == 0x3FFF);   /* gain2x=0 → GA=1 (1배) */
    CHECK(mcp4921_write(&s, 7, 0x0ABC, 1, 1) == QSPI_OK && dac_word == 0x5ABC);  /* BUF=1, GA=0, SHDN=1 */
    CHECK(mcp4921_write(&s, 7, 0x0ABC, 0, 1) == QSPI_OK && dac_word == 0x7ABC);
    CHECK(mcp4921_write(&s, 7, 4096, 1, 0) == QSPI_ERR_ARGS);
    dac_writes = 0; CHECK(mcp4921_shutdown(&s, 7) == QSPI_OK && (dac_word & 0x1000) == 0 && dac_writes == 1);
    CHECK((dac_word & 0xF000) == 0 && cs_cur == -1);
    CHECK(mcp4921_to_mv(4095, 2048, 0) == 2047 && mcp4921_to_mv(4095, 2048, 1) == 4095 && mcp4921_to_mv(2048, 2048, 1) == 2048 && mcp4921_to_mv(0, 2048, 1) == 0);
    nack = 1; CHECK(mcp4921_write(&s, 7, 1, 1, 0) == QSPI_ERR_TIMEOUT && cs_cur == -1); nack = 0;
    if (!bad) puts("ALL PASS");
    return bad != 0;
}
