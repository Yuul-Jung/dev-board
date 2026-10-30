#include "ssd1306.h"
#include <string.h>

static int inlim(int v) { return v >= -OLED_COORD_LIM && v <= OLED_COORD_LIM; }
static int iabs(int v) { return v < 0 ? -v : v; }

#define CTRL_CMD  SSD1306_CTRL_CMD
#define CTRL_DATA SSD1306_CTRL_DATA

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
    if (!o || !iic || (addr7 != SSD1306_ADDR_SA0_L && addr7 != SSD1306_ADDR_SA0_H)) return IIC_ERR_ARGS;
    o->iic = iic; o->addr7 = addr7;
    memset(o->fb, 0, sizeof o->fb);
    const uint8_t seq[] = {
        SSD1306_CMD_DISPLAY_OFF,                          /* 표시 끔 */
        SSD1306_CMD_CLOCK, SSD1306_RESET_CLOCK,           /* 클럭 분주 1, 발진 8 (= 데이터시트 리셋 값, nnarain 값과 같음) */
        SSD1306_CMD_MUX, OLED_H - 1,                      /* 멀티플렉스 비율 = 63 (64 MUX, 리셋 값) */
        SSD1306_CMD_DISPLAY_OFFSET, 0x00,                 /* 표시 오프셋 0 */
        SSD1306_CMD_START_LINE,                           /* 시작 줄 0 */
        SSD1306_CMD_CHARGE_PUMP, SSD1306_CHARGE_PUMP_ON,  /* 차지 펌프 켬 (내장 승압 사용) */
        SSD1306_CMD_MEM_MODE, SSD1306_MEM_HORIZONTAL,     /* 가로 주소 지정 모드 */
        flip ? SSD1306_CMD_SEG_REMAP_0 : SSD1306_CMD_SEG_REMAP_127,       /* 세그먼트 리맵 */
        flip ? SSD1306_CMD_COM_SCAN_NORMAL : SSD1306_CMD_COM_SCAN_REMAP,  /* COM 스캔 방향 */
        SSD1306_CMD_COM_PINS, com_pins,
        SSD1306_CMD_CONTRAST, SSD1306_RESET_CONTRAST,     /* 대비 (리셋 값) */
        SSD1306_CMD_RAM_DISPLAY,                          /* RAM 내용 표시 */
        SSD1306_CMD_NORMAL,                               /* 정상(비반전) 표시 */
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
    uint8_t c = on ? SSD1306_CMD_DISPLAY_ON : SSD1306_CMD_DISPLAY_OFF;
    return cmds(o, &c, 1);
}

int oled_invert(oled_t *o, int on)
{
    uint8_t c = on ? SSD1306_CMD_INVERSE : SSD1306_CMD_NORMAL;
    return cmds(o, &c, 1);
}

int oled_contrast(oled_t *o, uint8_t v)
{
    uint8_t c[2] = { SSD1306_CMD_CONTRAST, v };
    return cmds(o, c, 2);
}

int oled_flush(oled_t *o)
{
    uint8_t win[] = { SSD1306_CMD_COLUMN_ADDR, 0, OLED_W - 1, SSD1306_CMD_PAGE_ADDR, 0, OLED_PAGES - 1 };
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
    if (!o || w <= 0 || y < 0 || y >= OLED_H) return;
    int64_t x0 = x, x1 = (int64_t)x + w - 1;                 /* 범위 계산은 64비트(정수 넘침 방지) */
    if (x1 < 0 || x0 >= OLED_W) return;
    if (x0 < 0) x0 = 0;
    if (x1 >= OLED_W) x1 = OLED_W - 1;
    for (int64_t i = x0; i <= x1; i++) oled_pixel(o, (int)i, y, on);
}

void oled_rect(oled_t *o, int x, int y, int w, int h, int fill, int on)
{
    if (!o || w <= 0 || h <= 0) return;
    int64_t top = y, bot = (int64_t)y + h - 1;
    int64_t r0 = top < 0 ? 0 : top, r1 = bot >= OLED_H ? OLED_H - 1 : bot;      /* 화면 안의 행만 */
    int64_t xr = (int64_t)x + w - 1;
    for (int64_t j = r0; j <= r1; j++) {
        if (fill || j == top || j == bot) oled_hline(o, x, (int)j, w, on);
        else {
            if (x >= 0 && x < OLED_W) oled_pixel(o, x, (int)j, on);
            if (xr >= 0 && xr < OLED_W) oled_pixel(o, (int)xr, (int)j, on);
        }
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
    if (!o || !s || !inlim(x) || !inlim(y)) return x;
    if (scale < 1) scale = 1;
    if (scale > 8) scale = 8;
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

/* ===================== 도형 ===================== */

void oled_line(oled_t *o, int x0, int y0, int x1, int y1, int on)
{
    if (!inlim(x0) || !inlim(y0) || !inlim(x1) || !inlim(y1)) return;
    int dx = iabs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -iabs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        oled_pixel(o, x0, y0, on);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void oled_circle(oled_t *o, int cx, int cy, int r, int on)
{
    if (r < 0 || r > OLED_COORD_LIM || !inlim(cx) || !inlim(cy)) return;
    int x = 0, y = r, d = 1 - r;
    while (x <= y) {
        oled_pixel(o, cx + x, cy + y, on); oled_pixel(o, cx - x, cy + y, on);
        oled_pixel(o, cx + x, cy - y, on); oled_pixel(o, cx - x, cy - y, on);
        oled_pixel(o, cx + y, cy + x, on); oled_pixel(o, cx - y, cy + x, on);
        oled_pixel(o, cx + y, cy - x, on); oled_pixel(o, cx - y, cy - x, on);
        x++;
        if (d < 0) d += 2 * x + 1;
        else { y--; d += 2 * (x - y) + 1; }
    }
}

void oled_fill_circle(oled_t *o, int cx, int cy, int r, int on)
{
    if (r < 0 || r > OLED_COORD_LIM || !inlim(cx) || !inlim(cy)) return;
    long hw = r;                                             /* dy = 0 일 때 반폭: r² ≤ r²+r < (r+1)² */
    for (int dy = 0; dy <= r; dy++) {
        long lim = (long)r * r + r - (long)dy * dy;          /* 이 행에서 dx² ≤ lim */
        while (hw * hw > lim) hw--;
        oled_hline(o, cx - (int)hw, cy + dy, 2 * (int)hw + 1, on);
        if (dy) oled_hline(o, cx - (int)hw, cy - dy, 2 * (int)hw + 1, on);
    }
}

/* 삼각형 채우기용: 한 변을 Bresenham 으로 따라가며 화면 행 0..63 마다 x 의 최소·최대를 기록 */
static void edge_span(int x0, int y0, int x1, int y1, int *lo, int *hi)
{
    int dx = iabs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -iabs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        if (y0 >= 0 && y0 < OLED_H) {
            if (x0 < lo[y0]) lo[y0] = x0;
            if (x0 > hi[y0]) hi[y0] = x0;
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void oled_triangle(oled_t *o, int x0, int y0, int x1, int y1, int x2, int y2, int fill, int on)
{
    if (!inlim(x0) || !inlim(y0) || !inlim(x1) || !inlim(y1) || !inlim(x2) || !inlim(y2)) return;
    if (!fill) {
        oled_line(o, x0, y0, x1, y1, on); oled_line(o, x1, y1, x2, y2, on); oled_line(o, x2, y2, x0, y0, on);
        return;
    }
    int lo[OLED_H], hi[OLED_H];
    for (int y = 0; y < OLED_H; y++) { lo[y] = OLED_COORD_LIM + 1; hi[y] = -OLED_COORD_LIM - 1; }
    edge_span(x0, y0, x1, y1, lo, hi); edge_span(x1, y1, x2, y2, lo, hi); edge_span(x2, y2, x0, y0, lo, hi);
    for (int y = 0; y < OLED_H; y++)
        if (lo[y] <= hi[y]) oled_hline(o, lo[y], y, hi[y] - lo[y] + 1, on);
}

void oled_bitmap(oled_t *o, int x, int y, const uint8_t *bmp, int w, int h, int fg, int bg)
{
    if (!o || !bmp || w <= 0 || h <= 0 || w > OLED_COORD_LIM || h > OLED_COORD_LIM || !inlim(x) || !inlim(y)) return;
    int stride = (w + 7) / 8;
    for (int j = 0; j < h; j++) {
        if (y + j < 0 || y + j >= OLED_H) continue;
        for (int i = 0; i < w; i++) {
            if (x + i < 0 || x + i >= OLED_W) continue;
            int bit = (bmp[j * stride + (i >> 3)] >> (7 - (i & 7))) & 1;
            if (bit) oled_pixel(o, x + i, y + j, fg);
            else if (bg >= 0) oled_pixel(o, x + i, y + j, bg);
        }
    }
}

/* ===================== 5x7 글꼴 ===================== */
/* ASCII 0x20~0x7E, 글자마다 5열, 열마다 비트0 = 맨 위 행. 출처 문서 없음(기억에 의존한 고전적인 5x7 글꼴), 따옴표(')와 물결(~)은 직접 그림. */
static const uint8_t FONT[95][5] = {
    {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00}, {0x00,0x07,0x00,0x07,0x00}, {0x14,0x7F,0x14,0x7F,0x14},   /*  ! " # */
    {0x24,0x2A,0x7F,0x2A,0x12}, {0x23,0x13,0x08,0x64,0x62}, {0x36,0x49,0x55,0x22,0x50}, {0x00,0x00,0x07,0x00,0x00},   /* $ % & ' */
    {0x00,0x1C,0x22,0x41,0x00}, {0x00,0x41,0x22,0x1C,0x00}, {0x14,0x08,0x3E,0x08,0x14}, {0x08,0x08,0x3E,0x08,0x08},   /* ( ) * + */
    {0x00,0x50,0x30,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08}, {0x00,0x60,0x60,0x00,0x00}, {0x20,0x10,0x08,0x04,0x02},   /* , - . / */
    {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00}, {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},   /* 0 1 2 3 */
    {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39}, {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},   /* 4 5 6 7 */
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}, {0x00,0x36,0x36,0x00,0x00}, {0x00,0x56,0x36,0x00,0x00},   /* 8 9 : ; */
    {0x08,0x14,0x22,0x41,0x00}, {0x14,0x14,0x14,0x14,0x14}, {0x00,0x41,0x22,0x14,0x08}, {0x02,0x01,0x51,0x09,0x06},   /* < = > ? */
    {0x32,0x49,0x79,0x41,0x3E}, {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22},   /* @ A B C */
    {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01}, {0x3E,0x41,0x49,0x49,0x7A},   /* D E F G */
    {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41},   /* H I J K */
    {0x7F,0x40,0x40,0x40,0x40}, {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E},   /* L M N O */
    {0x7F,0x09,0x09,0x09,0x06}, {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46}, {0x46,0x49,0x49,0x49,0x31},   /* P Q R S */
    {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F}, {0x3F,0x40,0x38,0x40,0x3F},   /* T U V W */
    {0x63,0x14,0x08,0x14,0x63}, {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}, {0x00,0x7F,0x41,0x41,0x00},   /* X Y Z [ */
    {0x02,0x04,0x08,0x10,0x20}, {0x00,0x41,0x41,0x7F,0x00}, {0x04,0x02,0x01,0x02,0x04}, {0x40,0x40,0x40,0x40,0x40},   /* \ ] ^ _ */
    {0x00,0x01,0x02,0x04,0x00}, {0x20,0x54,0x54,0x54,0x78}, {0x7F,0x48,0x44,0x44,0x38}, {0x38,0x44,0x44,0x44,0x20},   /* ` a b c */
    {0x38,0x44,0x44,0x48,0x7F}, {0x38,0x54,0x54,0x54,0x18}, {0x08,0x7E,0x09,0x01,0x02}, {0x0C,0x52,0x52,0x52,0x3E},   /* d e f g */
    {0x7F,0x08,0x04,0x04,0x78}, {0x00,0x44,0x7D,0x40,0x00}, {0x20,0x40,0x44,0x3D,0x00}, {0x7F,0x10,0x28,0x44,0x00},   /* h i j k */
    {0x00,0x41,0x7F,0x40,0x00}, {0x7C,0x04,0x18,0x04,0x78}, {0x7C,0x08,0x04,0x04,0x78}, {0x38,0x44,0x44,0x44,0x38},   /* l m n o */
    {0x7C,0x14,0x14,0x14,0x08}, {0x08,0x14,0x14,0x18,0x7C}, {0x7C,0x08,0x04,0x04,0x08}, {0x48,0x54,0x54,0x54,0x20},   /* p q r s */
    {0x04,0x3F,0x44,0x40,0x20}, {0x3C,0x40,0x40,0x20,0x7C}, {0x1C,0x20,0x40,0x20,0x1C}, {0x3C,0x40,0x30,0x40,0x3C},   /* t u v w */
    {0x44,0x28,0x10,0x28,0x44}, {0x0C,0x50,0x50,0x50,0x3C}, {0x44,0x64,0x54,0x4C,0x44}, {0x00,0x08,0x36,0x41,0x00},   /* x y z { */
    {0x00,0x00,0x7F,0x00,0x00}, {0x00,0x41,0x36,0x08,0x00}, {0x04,0x02,0x04,0x08,0x04},                               /* | } ~ */
};

const uint8_t *oled_font5x7(int ch)
{
    if (ch < 0x20 || ch > 0x7E) ch = '?';
    return FONT[ch - 0x20];
}

int oled_text_width(const char *s, int scale)
{
    if (!s) return 0;
    if (scale < 1) scale = 1;
    if (scale > 8) scale = 8;
    int n = 0;
    while (s[n] && s[n] != '\n') n++;
    return n ? n * OLED_CELL_W * scale - scale : 0;
}

int oled_print(oled_t *o, int x, int y, const char *s, int scale, int fg, int bg)
{
    if (!o || !s || !inlim(x) || !inlim(y)) return x;
    if (scale < 1) scale = 1;
    if (scale > 8) scale = 8;
    int64_t cx = x, cy = y;                                  /* 긴 문자열·많은 줄바꿈에서도 넘치지 않도록 64비트 */
    for (; *s; s++) {
        if (*s == '\n') { cx = x; cy += OLED_CELL_H * scale; continue; }
        if (cx < OLED_W && cx + OLED_CELL_W * scale > 0 && cy < OLED_H && cy + OLED_CELL_H * scale > 0) {   /* 보이는 글자만 그린다 */
            const uint8_t *g = oled_font5x7((unsigned char)*s);
            for (int c = 0; c < OLED_CELL_W; c++) {
                for (int r = 0; r < OLED_CELL_H; r++) {
                    int bit = (c < OLED_FONT_W && r < OLED_FONT_H) ? (g[c] >> r) & 1 : 0;
                    int v = bit ? fg : bg;
                    if (v >= 0) oled_rect(o, (int)(cx + c * scale), (int)(cy + r * scale), scale, scale, 1, v);
                }
            }
        }
        cx += OLED_CELL_W * scale;
    }
    return cx > INT32_MAX ? INT32_MAX : (int)cx;
}
