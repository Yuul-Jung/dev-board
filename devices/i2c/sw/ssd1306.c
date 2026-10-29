#include "ssd1306.h"
#include <string.h>

#define CTRL_CMD  0x00
#define CTRL_DATA 0x40

static int cmds(oled_t *o, const uint8_t *c, unsigned n)
{
    uint8_t buf[16];
    if (n + 1 > sizeof buf) return IIC_ERR_ARGS;
    buf[0] = CTRL_CMD;                 /* Co=0: 이후 바이트는 모두 명령 바이트 */
    memcpy(buf + 1, c, n);
    return axi_iic_write(o->iic, o->addr7, buf, n + 1);
}

int oled_init(oled_t *o, axi_iic_t *iic, uint8_t addr7, uint8_t com_pins, int flip)
{
    if (!o || !iic || (addr7 != 0x3C && addr7 != 0x3D)) return IIC_ERR_ARGS;
    o->iic = iic; o->addr7 = addr7;
    memset(o->fb, 0, sizeof o->fb);
    const uint8_t seq[] = {
        0xAE,                    /* 표시 끔 */
        0xD5, 0x80,              /* 클럭 분주/발진 (nnarain 값) */
        0xA8, OLED_H - 1,        /* 멀티플렉스 비율 = 63 (0x3F, nnarain) */
        0xD3, 0x00,              /* 표시 오프셋 0 */
        0x40,                    /* 시작 줄 0 */
        0x8D, 0x14,              /* 차지 펌프 켬 (내장 승압 사용) */
        0x20, 0x00,              /* 수평 주소 지정 모드 */
        flip ? 0xA0 : 0xA1,      /* 세그먼트 리맵 */
        flip ? 0xC0 : 0xC8,      /* COM 스캔 방향 */
        0xDA, com_pins,
        0x81, 0x7F,              /* 대비 */
        0xA4,                    /* RAM 내용 표시 */
        0xA6,                    /* 정상(비반전) 표시 */
    };
    /* 16바이트 한도에 맞춰 나눠 보낸다. 명령 하나(인자 포함)가 쪼개지지 않도록 경계를 직접 지정. */
    static const uint8_t cut[] = { 7, 7, 6 };   /* 명령 경계. 합계 = sizeof seq (20) */
    unsigned pos = 0;
    for (unsigned i = 0; i < sizeof cut; i++) {
        if (pos + cut[i] > sizeof seq) return IIC_ERR_ARGS;
        int r = cmds(o, seq + pos, cut[i]);
        if (r) return r;
        pos += cut[i];
    }
    if (pos != sizeof seq) return IIC_ERR_ARGS;
    return oled_flush(o) ? IIC_ERR_NACK : oled_display(o, 1);
}

int oled_display(oled_t *o, int on)
{
    uint8_t c = on ? 0xAF : 0xAE;
    return cmds(o, &c, 1);
}

int oled_contrast(oled_t *o, uint8_t v)
{
    uint8_t c[2] = { 0x81, v };
    return cmds(o, c, 2);
}

int oled_flush(oled_t *o)
{
    uint8_t win[] = { 0x21, 0, OLED_W - 1, 0x22, 0, OLED_PAGES - 1 };
    int r = cmds(o, win, sizeof win);
    if (r) return r;
    uint8_t buf[1 + OLED_W];
    buf[0] = CTRL_DATA;                /* Co=0, D/C#=1: 이후 전부 데이터 */
    for (int p = 0; p < OLED_PAGES; p++) {
        memcpy(buf + 1, o->fb + p * OLED_W, OLED_W);
        r = axi_iic_write(o->iic, o->addr7, buf, sizeof buf);
        if (r) return r;
    }
    return IIC_OK;
}

void oled_clear(oled_t *o, int on) { memset(o->fb, on ? 0xFF : 0x00, sizeof o->fb); }

void oled_pixel(oled_t *o, int x, int y, int on)
{
    if (x < 0 || x >= OLED_W || y < 0 || y >= OLED_H) return;
    uint8_t *b = &o->fb[(y >> 3) * OLED_W + x];
    if (on) *b |= (uint8_t)(1u << (y & 7)); else *b &= (uint8_t)~(1u << (y & 7));
}

void oled_hline(oled_t *o, int x, int y, int w, int on)
{
    for (int i = 0; i < w; i++) oled_pixel(o, x + i, y, on);
}

void oled_rect(oled_t *o, int x, int y, int w, int h, int fill, int on)
{
    if (w <= 0 || h <= 0) return;
    for (int j = 0; j < h; j++) {
        if (fill || j == 0 || j == h - 1) oled_hline(o, x, y + j, w, on);
        else { oled_pixel(o, x, y + j, on); oled_pixel(o, x + w - 1, y + j, on); }
    }
}

/* 3x5 글꼴: 5행, 행마다 하위 3비트 (비트2 = 왼쪽) */
static const uint8_t *glyph(char c)
{
    static const uint8_t g[][5] = {
        {7,5,5,5,7}, {2,6,2,2,7}, {7,1,7,4,7}, {7,1,7,1,7}, {5,5,7,1,1},
        {7,4,7,1,7}, {7,4,7,5,7}, {7,1,1,1,1}, {7,5,7,5,7}, {7,5,7,1,7},
        {0,0,7,0,0},   /* '-' */
        {0,0,0,0,2},   /* '.' */
        {0,2,0,2,0},   /* ':' */
        {0,0,0,0,0},   /* ' ' */
    };
    if (c >= '0' && c <= '9') return g[c - '0'];
    switch (c) { case '-': return g[10]; case '.': return g[11]; case ':': return g[12]; case ' ': return g[13]; }
    return 0;
}

int oled_text(oled_t *o, int x, int y, const char *s, int scale, int on)
{
    if (scale < 1) scale = 1;
    for (; *s; s++) {
        const uint8_t *g = glyph(*s);
        if (g) {
            for (int r = 0; r < 5; r++)
                for (int c = 0; c < 3; c++)
                    if (g[r] & (4 >> c))
                        oled_rect(o, x + c * scale, y + r * scale, scale, scale, 1, on);
        }
        x += 4 * scale;
    }
    return x;
}
