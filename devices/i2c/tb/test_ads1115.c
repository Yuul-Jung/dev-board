/* ADS1115 드라이버 호스트 시험: I2C 층(axi_iic_write/write_read)을 ADS1115 동작 모델로 대체한다.
 * 모델은 데이터시트에서 확인한 규칙만 구현: 포인터 레지스터, MSB 먼저, 리셋 8583h,
 * OS=1 쓰기 → 변환 시작 → 몇 번 읽는 동안 OS=0 → 이후 OS=1. */
#include <stdio.h>
#include <string.h>
#include "../sw/ads1115.h"

static uint16_t cfg = 0x8583, conv = 0, ptr; static uint8_t expect_addr = 0x48;
static int busy_polls, busy_left, nack_all, n_cfg_reads;
static int16_t ain[8] = {0};              /* 모델 입력(MUX 코드별 원시값) */
void iic_delay_ms(unsigned ms) { (void)ms; }

int axi_iic_write(axi_iic_t *d, uint8_t a, const uint8_t *b, unsigned n)
{
    (void)d; if (nack_all || a != expect_addr) return IIC_ERR_NACK;
    if (n < 1) return IIC_ERR_ARGS;
    ptr = b[0] & 3;
    if (n == 3) {
        uint16_t v = (uint16_t)((b[1] << 8) | b[2]);
        if (ptr == 1) {
            if ((v & 0x8000) && (v & 0x0100)) { busy_left = busy_polls; conv = (uint16_t)ain[(v >> 12) & 7]; }
            else if (!(v & 0x0100)) conv = (uint16_t)ain[(v >> 12) & 7];       /* 연속 모드 */
            cfg = (uint16_t)(v & 0x7FFF); if (busy_left == 0) cfg |= 0x8000;
            if (busy_left) cfg &= 0x7FFF;
        }
    } else if (n != 1) return IIC_ERR_ARGS;
    return IIC_OK;
}
int axi_iic_write_read(axi_iic_t *d, uint8_t a, const uint8_t *w, unsigned wl, uint8_t *r, unsigned rl)
{
    int rc = axi_iic_write(d, a, w, wl); if (rc) return rc;
    if (rl != 2) return IIC_ERR_ARGS;
    uint16_t v = 0;
    if (ptr == 1) {
        n_cfg_reads++;
        if (busy_left > 0) { busy_left--; if (busy_left == 0) cfg |= 0x8000; }
        v = cfg;
    } else if (ptr == 0) v = conv;
    r[0] = (uint8_t)(v >> 8); r[1] = (uint8_t)v; return IIC_OK;
}

static int bad;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); bad++; } } while (0)

int main(void)
{
    axi_iic_t iic = {0}; ads1115_t a; int16_t raw;
    CHECK(ads1115_init(&a, &iic, 0x47) == IIC_ERR_ARGS && ads1115_init(&a, &iic, 0x4C) == IIC_ERR_ARGS);
    CHECK(ads1115_init(&a, &iic, 0x48) == IIC_OK);
    nack_all = 1; CHECK(ads1115_init(&a, &iic, 0x48) == IIC_ERR_NACK); nack_all = 0;
    expect_addr = 0x4B; CHECK(ads1115_init(&a, &iic, 0x48) == IIC_ERR_NACK);
    CHECK(ads1115_init(&a, &iic, 0x4B) == IIC_OK); expect_addr = 0x48; ads1115_init(&a, &iic, 0x48);

    /* 레지스터 왕복과 바이트 순서 */
    uint16_t v; CHECK(ads1115_read_reg(&a, ADS_REG_CONFIG, &v) == IIC_OK && v == 0x8583);
    /* Config 비트 배치: 0xC283 = OS|AIN0/GND|±4.096|연속|128SPS|비교기 끔 (openkits 예시와 동일 값) */
    busy_polls = 0; ain[4] = 1234;
    CHECK(ads1115_start_continuous(&a, ADS_MUX_0_G, ADS_PGA_4096, ADS_DR_128) == IIC_OK);
    CHECK((cfg & 0x7FFF) == (0xC283 & 0x7FFF));
    CHECK(ads1115_read_conversion(&a, &raw) == IIC_OK && raw == 1234);

    /* 단발: 변환 중에는 OS=0 으로 몇 번 읽히다 1 이 됨 */
    busy_polls = 5; ain[7] = -2000; n_cfg_reads = 0;
    CHECK(ads1115_read_single(&a, ADS_MUX_3_G, ADS_PGA_2048, ADS_DR_8, &raw) == IIC_OK && raw == -2000);
    CHECK(n_cfg_reads == 5);
    CHECK((cfg >> 12 & 7) == 7 && (cfg >> 9 & 7) == 2 && (cfg >> 5 & 7) == 0 && (cfg & 0x100));
    /* 변환이 끝나지 않으면 시간 초과 */
    busy_polls = 100000; CHECK(ads1115_read_single(&a, ADS_MUX_0_G, ADS_PGA_2048, ADS_DR_128, &raw) == IIC_ERR_TIMEOUT);
    busy_polls = 0; busy_left = 0;
    /* 인자 검사 */
    CHECK(ads1115_read_single(&a, 8, 0, 0, &raw) == IIC_ERR_ARGS && ads1115_read_single(&a, 0, 6, 0, &raw) == IIC_ERR_ARGS);
    CHECK(ads1115_read_single(&a, 0, 0, 8, &raw) == IIC_ERR_ARGS && ads1115_read_single(&a, 0, 0, 0, 0) == IIC_ERR_ARGS);
    /* 모든 mux·pga·dr 조합에서 Config 필드가 제자리에 들어감 */
    for (int m = 0; m < 8; m++) for (int p = 0; p < 6; p++) for (int r = 0; r < 8; r++) {
        busy_polls = 0; busy_left = 0;
        CHECK(ads1115_read_single(&a, m, p, r, &raw) == IIC_OK);
        CHECK((cfg >> 12 & 7) == (unsigned)m && (cfg >> 9 & 7) == (unsigned)p && (cfg >> 5 & 7) == (unsigned)r && (cfg & 3) == 3);
    }
    /* 단위 변환: 데이터시트 LSB 표와 일치 (raw=1 → LSB, 만점 7FFF → FSR 바로 아래) */
    CHECK(ads1115_raw_to_uv(1, ADS_PGA_6144) == 187);          /* 187.5 µV 를 0 쪽으로 내림 */
    CHECK(ads1115_raw_to_uv(2, ADS_PGA_6144) == 375);
    CHECK(ads1115_raw_to_uv(16, ADS_PGA_4096) == 2000);        /* 125 µV × 16 */
    CHECK(ads1115_raw_to_uv(16, ADS_PGA_2048) == 1000);        /* 62.5 µV × 16 */
    CHECK(ads1115_raw_to_uv(32, ADS_PGA_1024) == 1000);        /* 31.25 × 32 */
    CHECK(ads1115_raw_to_uv(64, ADS_PGA_0512) == 1000);        /* 15.625 × 64 */
    CHECK(ads1115_raw_to_uv(128, ADS_PGA_0256) == 1000);       /* 7.8125 × 128 */
    CHECK(ads1115_raw_to_uv(32767, ADS_PGA_4096) == 4095875);
    CHECK(ads1115_raw_to_uv(-32768, ADS_PGA_4096) == -4096000);
    CHECK(ads1115_raw_to_uv(-1, ADS_PGA_6144) == -187);
    if (!bad) puts("ALL PASS");
    return bad != 0;
}
