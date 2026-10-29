/* PCF8574 / MCP23017 호스트 시험: I2C 층을 장치 모델로 대체. 모델은 확인한 규칙만 구현
 * (PCF8574: 레지스터 없음·첫 바이트만 반영·입력은 래치 1 인 핀만 외부값, MCP23017: BANK=0 지도,
 *  SEQOP=0 이면 포인터 자동 증가, IODIR 리셋 FF). */
#include <stdio.h>
#include <string.h>
#include "../sw/io_expander.h"

void iic_delay_ms(unsigned ms) { (void)ms; }
static int nack_all; static uint8_t last_addr;

/* PCF8574 모델 */
static uint8_t pcf_latch = 0xFF, pcf_ext = 0xFF;      /* ext: 외부 회로가 핀에 거는 값(0 = 끌어내림) */
/* MCP23017 모델 */
static uint8_t R[0x16], ptr; static uint16_t mcp_ext = 0xFFFF;
static void mcp_reset(void) { memset(R, 0, sizeof R); R[0] = R[1] = 0xFF; }
static uint8_t mcp_get(uint8_t r)
{
    if (r == 0x12 || r == 0x13) {   /* GPIO: 입력 핀은 외부값, 출력 핀은 래치값 */
        int b = r - 0x12; uint8_t dir = R[b], ext = (uint8_t)(mcp_ext >> (8 * b));
        return (uint8_t)((ext & dir) | (R[0x14 + b] & ~dir));
    }
    return R[r];
}
static int model_pcf;                                     /* 1 = 주소가 PCF8574 를 가리키는 시험 구간 */

int axi_iic_write(axi_iic_t *d, uint8_t a, const uint8_t *b, unsigned n)
{
    (void)d; last_addr = a; if (nack_all || n == 0) return IIC_ERR_NACK;
    if (model_pcf) {
        pcf_latch = b[0]; return IIC_OK;                      /* 첫 바이트만 반영 */
    }
    ptr = b[0]; if (ptr >= 0x16) return IIC_ERR_ARGS;
    for (unsigned i = 1; i < n; i++) {
        if (ptr > 0x15) ptr = 0;
        R[ptr] = b[i];
        if (!(R[0x0A] & MCP_IOCON_SEQOP)) ptr++;
    }
    return IIC_OK;
}
int axi_iic_read(axi_iic_t *d, uint8_t a, uint8_t *b, unsigned n)
{
    (void)d; last_addr = a; if (nack_all || n != 1) return IIC_ERR_NACK;
    b[0] = (uint8_t)(pcf_ext & pcf_latch);                    /* 래치 0 이면 0, 1 이면 외부값 */
    return IIC_OK;
}
int axi_iic_write_read(axi_iic_t *d, uint8_t a, const uint8_t *w, unsigned wl, uint8_t *r, unsigned rl)
{
    (void)d; last_addr = a; if (nack_all || wl != 1 || w[0] >= 0x16) return IIC_ERR_NACK;
    uint8_t p = w[0];
    for (unsigned i = 0; i < rl; i++) { if (p > 0x15) p = 0; r[i] = mcp_get(p); if (!(R[0x0A] & MCP_IOCON_SEQOP)) p++; }
    return IIC_OK;
}

static int bad;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); bad++; } } while (0)

int main(void)
{
    axi_iic_t iic = {0};
    /* ---- PCF8574 ---- */
    model_pcf = 1;
    pcf8574_t p;
    CHECK(pcf8574_init(&p, &iic, 0x1F) == IIC_ERR_ARGS && pcf8574_init(&p, &iic, 0x28) == IIC_ERR_ARGS);
    CHECK(pcf8574_init(&p, &iic, 0x20) == IIC_OK && pcf_latch == 0xFF && p.shadow == 0xFF);
    CHECK(pcf8574_write(&p, 0xA5) == IIC_OK && pcf_latch == 0xA5 && p.shadow == 0xA5);
    CHECK(pcf8574_set_pin(&p, 1, 1) == IIC_OK && pcf_latch == 0xA7);
    CHECK(pcf8574_set_pin(&p, 7, 0) == IIC_OK && pcf_latch == 0x27);
    CHECK(pcf8574_set_pin(&p, 8, 1) == IIC_ERR_ARGS);
    /* 입력: 래치 1 인 핀만 외부값이 보인다. 외부가 bit4 를 끌어내림 */
    pcf8574_write(&p, 0xFF); pcf_ext = 0xEF; uint8_t v;
    CHECK(pcf8574_read(&p, &v) == IIC_OK && v == 0xEF);
    pcf8574_write(&p, 0x0F); CHECK(pcf8574_read(&p, &v) == IIC_OK && v == 0x0F);   /* 래치 0 인 핀은 0 */
    CHECK(pcf8574_read(&p, 0) == IIC_ERR_ARGS);
    nack_all = 1; CHECK(pcf8574_write(&p, 0x12) == IIC_ERR_NACK && p.shadow == 0x0F); nack_all = 0;   /* 실패 시 shadow 유지 */
    /* ---- MCP23017 ---- */
    model_pcf = 0; mcp_reset(); mcp23017_t m;
    CHECK(mcp23017_init(&m, &iic, 0x28) == IIC_ERR_ARGS);
    R[0x0A] = 0xFF;                                           /* 잔여 상태: SEQOP=1 등 */
    CHECK(mcp23017_init(&m, &iic, 0x27) == IIC_OK && last_addr == 0x27 && R[0x0A] == 0);
    CHECK(mcp23017_set_dir(&m, 0xFF00) == IIC_OK && R[0] == 0x00 && R[1] == 0xFF);      /* A 출력, B 입력 */
    CHECK(mcp23017_set_pullup(&m, 0xFF00) == IIC_OK && R[0x0C] == 0 && R[0x0D] == 0xFF);
    CHECK(mcp23017_write(&m, 0x12A5) == IIC_OK && R[0x14] == 0xA5 && R[0x15] == 0x12);
    CHECK(mcp23017_set_pin(&m, 0, 0) == IIC_OK && R[0x14] == 0xA4);
    CHECK(mcp23017_set_pin(&m, 15, 1) == IIC_OK && R[0x15] == 0x92);
    CHECK(mcp23017_set_pin(&m, 16, 1) == IIC_ERR_ARGS);
    mcp_ext = 0x34FF; uint16_t w;
    CHECK(mcp23017_read(&m, &w) == IIC_OK && w == 0x34A4);   /* A: 출력 래치 A4, B: 입력 외부값 34 */
    CHECK(mcp23017_read_reg(&m, MCP_IOCON, &v) == IIC_OK && v == 0);
    CHECK(mcp23017_write_reg(&m, MCP_GPPUA, 0x5A) == IIC_OK && R[0x0C] == 0x5A);
    nack_all = 1; CHECK(mcp23017_write(&m, 0xFFFF) == IIC_ERR_NACK && m.olat == 0x92A4); nack_all = 0;
    CHECK(mcp23017_read(&m, 0) == IIC_ERR_ARGS && mcp23017_read_reg(&m, 0, 0) == IIC_ERR_ARGS);
    if (!bad) puts("ALL PASS");
    return bad != 0;
}
