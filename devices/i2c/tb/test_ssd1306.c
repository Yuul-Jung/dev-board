/* SSD1306 드라이버 호스트 시험: axi_iic_write 를 가짜로 바꿔 전송 바이트열을 기록하고 검사한다. */
#include <stdio.h>
#include <string.h>
#include "../sw/ssd1306.h"

static uint8_t log_[64][200]; static unsigned loglen[64], nlog, last_addr;
static int fail_after = -1;
void iic_delay_ms(unsigned ms) { (void)ms; }
int axi_iic_write(axi_iic_t *d, uint8_t a, const uint8_t *b, unsigned n)
{
    (void)d; if (fail_after == 0) return IIC_ERR_NACK; if (fail_after > 0) fail_after--;
    if (nlog < 64 && n <= 200) { memcpy(log_[nlog], b, n); loglen[nlog++] = n; }
    last_addr = a; return IIC_OK;
}

static int bad;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); bad++; } } while (0)

int main(void)
{
    static oled_t o; axi_iic_t iic = {0};
    CHECK(oled_init(&o, &iic, 0x3C, 0x12, 0) == IIC_OK);
    CHECK(last_addr == 0x3C);
    /* 초기화 명령: 모든 트랜잭션이 제어 바이트 0x00 로 시작, 평탄화하면 기대 순서와 같다 */
    uint8_t flat[64]; unsigned nf = 0;
    for (unsigned i = 0; i < 3; i++) { CHECK(log_[i][0] == 0x00); memcpy(flat + nf, log_[i] + 1, loglen[i] - 1); nf += loglen[i] - 1; }
    const uint8_t want[] = {0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0x00,0x40,0x8D,0x14,0x20,0x00,0xA1,0xC8,0xDA,0x12,0x81,0x7F,0xA4,0xA6};
    CHECK(nf == sizeof want && memcmp(flat, want, nf) == 0);
    /* 명령이 인자와 떨어져 쪼개지지 않았는지: 각 조각이 명령 경계에서 끝나야 함 */
    CHECK(loglen[0] - 1 == 7 && log_[0][7] == 0x00 /* D3 인자가 같은 조각 */);
    CHECK(log_[1][1] == 0x40 && log_[2][1] == 0xDA);   /* 두 번째·세 번째 조각의 첫 명령 */
    /* 창 설정 + 8페이지 데이터 + 표시 켜기 */
    CHECK(log_[3][0] == 0 && log_[3][1] == 0x21 && log_[3][3] == 127 && log_[3][4] == 0x22 && log_[3][6] == 7);
    for (int p = 0; p < 8; p++) CHECK(log_[4 + p][0] == 0x40 && loglen[4 + p] == 129);
    CHECK(log_[12][1] == 0xAF);
    CHECK(nlog == 13);

    /* 그리기 */
    oled_clear(&o, 0);
    oled_pixel(&o, 0, 0, 1);  CHECK(o.fb[0] == 0x01);
    oled_pixel(&o, 5, 9, 1);  CHECK(o.fb[1 * 128 + 5] == 0x02);
    oled_pixel(&o, 127, 63, 1); CHECK(o.fb[7 * 128 + 127] == 0x80);
    oled_pixel(&o, 128, 0, 1); oled_pixel(&o, -1, 0, 1); oled_pixel(&o, 0, 64, 1); oled_pixel(&o, 0, -1, 1);
    unsigned cnt = 0; for (int i = 0; i < OLED_FB_BYTES; i++) for (int b = 0; b < 8; b++) cnt += (o.fb[i] >> b) & 1;
    CHECK(cnt == 3);                                  /* 범위 밖 쓰기는 무시 */
    oled_pixel(&o, 0, 0, 0); CHECK(o.fb[0] == 0);
    oled_clear(&o, 0); oled_rect(&o, 10, 10, 5, 4, 1, 1);
    cnt = 0; for (int i = 0; i < OLED_FB_BYTES; i++) for (int b = 0; b < 8; b++) cnt += (o.fb[i] >> b) & 1;
    CHECK(cnt == 20);
    oled_clear(&o, 0); oled_rect(&o, 10, 10, 5, 4, 0, 1);
    cnt = 0; for (int i = 0; i < OLED_FB_BYTES; i++) for (int b = 0; b < 8; b++) cnt += (o.fb[i] >> b) & 1;
    CHECK(cnt == 14);                                 /* 테두리 5*2 + 양옆 2*2 */
    oled_clear(&o, 0); oled_rect(&o, -3, -3, 200, 200, 1, 1); /* 크게 벗어나도 안전 */
    CHECK(o.fb[0] == 0xFF && o.fb[OLED_FB_BYTES - 1] == 0xFF);
    /* 글꼴: '1' 의 3열 5행 모양 확인 */
    oled_clear(&o, 0); int nx = oled_text(&o, 0, 0, "10", 1, 1);
    CHECK(nx == 8);
    const char *exp[5] = {".#.", "##.", ".#.", ".#.", "###"};
    for (int r = 0; r < 5; r++) for (int c = 0; c < 3; c++)
        CHECK(((o.fb[c] >> r) & 1) == (exp[r][c] == '#'));
    /* 오류 전파 */
    nlog = 0; fail_after = 0; CHECK(oled_flush(&o) == IIC_ERR_NACK);
    fail_after = 3; CHECK(oled_flush(&o) == IIC_ERR_NACK);
    fail_after = -1;
    CHECK(oled_init(&o, &iic, 0x50, 0x12, 0) == IIC_ERR_ARGS);
    CHECK(oled_init(&o, &iic, 0x3D, 0x02, 1) == IIC_OK && last_addr == 0x3D);

    /* ---- 변형 검사에서 살아남은 전송 부분 ---- */
    /* 대비·표시·반전: 바이트와 주소 */
    nlog = 0; CHECK(oled_contrast(&o, 0x5A) == IIC_OK && nlog == 1 && loglen[0] == 3 && log_[0][0] == 0x00 && log_[0][1] == 0x81 && log_[0][2] == 0x5A);
    nlog = 0; CHECK(oled_display(&o, 0) == IIC_OK && nlog == 1 && loglen[0] == 2 && log_[0][0] == 0x00 && log_[0][1] == 0xAE);
    nlog = 0; CHECK(oled_display(&o, 1) == IIC_OK && nlog == 1 && loglen[0] == 2 && log_[0][1] == 0xAF);
    nlog = 0; CHECK(oled_invert(&o, 1) == IIC_OK && nlog == 1 && loglen[0] == 2 && log_[0][1] == 0xA7);
    nlog = 0; CHECK(oled_invert(&o, 0) == IIC_OK && nlog == 1 && loglen[0] == 2 && log_[0][1] == 0xA6);
    fail_after = 0; CHECK(oled_contrast(&o, 1) == IIC_ERR_NACK && oled_display(&o, 1) == IIC_ERR_NACK && oled_invert(&o, 1) == IIC_ERR_NACK); fail_after = -1;
    /* clear: 모든 바이트가 on 에 따라 0x00 / 0xFF */
    memset(o.fb, 0x5A, sizeof o.fb); oled_clear(&o, 0); { int ok = 1; for (int i = 0; i < OLED_FB_BYTES; i++) ok &= o.fb[i] == 0x00; CHECK(ok); }
    memset(o.fb, 0x5A, sizeof o.fb); oled_clear(&o, 1); { int ok = 1; for (int i = 0; i < OLED_FB_BYTES; i++) ok &= o.fb[i] == 0xFF; CHECK(ok); }
    /* flush: 프레임버퍼 내용이 페이지 순서 그대로 나가고, 창 설정이 정확함 */
    for (int i = 0; i < OLED_FB_BYTES; i++) o.fb[i] = (uint8_t)(i * 7 + (i >> 7) * 31 + 1);
    nlog = 0; CHECK(oled_flush(&o) == IIC_OK && nlog == 9);
    { const uint8_t win[] = { 0x00, 0x21, 0, 127, 0x22, 0, 7 }; CHECK(loglen[0] == 7 && !memcmp(log_[0], win, 7)); }
    for (int p = 0; p < 8; p++) { CHECK(loglen[1 + p] == 129 && log_[1 + p][0] == 0x40 && !memcmp(log_[1 + p] + 1, o.fb + p * 128, 128)); }
    /* 전송 중 어느 트랜잭션에서 실패해도 그 오류를 돌려주고 그 뒤를 보내지 않음 */
    for (int k = 0; k < 9; k++) { nlog = 0; fail_after = k; CHECK(oled_flush(&o) == IIC_ERR_NACK && nlog == (unsigned)k); }
    fail_after = -1;
    /* init: 인자 검사, flip, 각 단계 실패, 프레임버퍼 지움 */
    memset(o.fb, 0xFF, sizeof o.fb);
    CHECK(oled_init(NULL, &iic, 0x3C, 0x12, 0) == IIC_ERR_ARGS && oled_init(&o, NULL, 0x3C, 0x12, 0) == IIC_ERR_ARGS && oled_init(&o, &iic, 0x3B, 0x12, 0) == IIC_ERR_ARGS && oled_init(&o, &iic, 0x3E, 0x12, 0) == IIC_ERR_ARGS);
    nlog = 0; CHECK(oled_init(&o, &iic, 0x3C, 0x02, 1) == IIC_OK);
    { int ok = 1; for (int i = 0; i < OLED_FB_BYTES; i++) ok &= o.fb[i] == 0; CHECK(ok); }       /* init 은 화면을 지운 뒤 보낸다 */
    { uint8_t fl[64]; unsigned n2 = 0; for (unsigned i = 0; i < 3; i++) { memcpy(fl + n2, log_[i] + 1, loglen[i] - 1); n2 += loglen[i] - 1; }
      const uint8_t w2[] = {0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0x00,0x40,0x8D,0x14,0x20,0x00,0xA0,0xC0,0xDA,0x02,0x81,0x7F,0xA4,0xA6};
      CHECK(n2 == sizeof w2 && !memcmp(fl, w2, n2)); }                                            /* flip=1: A0/C0, com_pins 0x02 */
    for (int k = 0; k < 13; k++) { nlog = 0; fail_after = k; CHECK(oled_init(&o, &iic, 0x3C, 0x12, 0) == IIC_ERR_NACK && nlog == (unsigned)k); }
    fail_after = -1;
    nlog = 0; CHECK(oled_init(&o, &iic, 0x3C, 0x12, 0) == IIC_OK && nlog == 13);
    if (!bad) puts("ALL PASS");
    return bad != 0;
}
