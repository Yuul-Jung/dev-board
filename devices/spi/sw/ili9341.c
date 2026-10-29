#include "ili9341.h"

static int wait_idle(lcd_t *l)
{
    unsigned t = l->timeout;
    for (;;) {
        uint32_t s = LCD_RD(l->base, LCD_TX);
        if (s & LCD_ST_OVERRUN) return LCD_ERR_OVERRUN;
        if (!(s & LCD_ST_BUSY)) return LCD_OK;
        if (!t--) return LCD_ERR_TIMEOUT;
    }
}

void lcd_attach(lcd_t *l, uintptr_t base, uint32_t div)
{
    l->base = base; l->timeout = 1000000;
    LCD_WR(base, LCD_DIV, div);
    LCD_WR(base, LCD_CTRL, LCD_CTRL_RESX | LCD_CTRL_CS);
}

void lcd_hw_reset(lcd_t *l, unsigned low_ms, unsigned after_ms)
{
    uint32_t c = LCD_RD(l->base, LCD_CTRL) & ~LCD_CTRL_RESX;
    LCD_WR(l->base, LCD_CTRL, c);
    lcd_delay_ms(low_ms);
    LCD_WR(l->base, LCD_CTRL, c | LCD_CTRL_RESX);
    lcd_delay_ms(after_ms);
}

void lcd_backlight(lcd_t *l, int on)
{
    uint32_t c = LCD_RD(l->base, LCD_CTRL) & ~LCD_CTRL_BL;
    LCD_WR(l->base, LCD_CTRL, on ? (c | LCD_CTRL_BL) : c);
}

int lcd_cmd(lcd_t *l, uint8_t c)
{
    int r = wait_idle(l); if (r) return r;
    LCD_WR(l->base, LCD_TX, c);
    return LCD_OK;
}
int lcd_data(lcd_t *l, uint8_t d)
{
    int r = wait_idle(l); if (r) return r;
    LCD_WR(l->base, LCD_TX, 0x100u | d);
    return LCD_OK;
}

int lcd_run_table(lcd_t *l, const lcd_step_t *s)
{
    for (; !(s->cmd == 0xFF && s->n == 0xFF); s++) {
        if (s->n > 16) return LCD_ERR_ARGS;
        int r = lcd_cmd(l, s->cmd); if (r) return r;
        for (unsigned i = 0; i < s->n; i++) { r = lcd_data(l, s->p[i]); if (r) return r; }
        if (s->delay_ms) { r = wait_idle(l); if (r) return r; lcd_delay_ms(s->delay_ms); }
    }
    return wait_idle(l);
}

int lcd_set_window(lcd_t *l, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    if (x0 > x1 || y0 > y1 || x1 >= LCD_W || y1 >= LCD_H) return LCD_ERR_ARGS;
    const uint8_t cx[4] = { (uint8_t)(x0 >> 8), (uint8_t)x0, (uint8_t)(x1 >> 8), (uint8_t)x1 };
    const uint8_t cy[4] = { (uint8_t)(y0 >> 8), (uint8_t)y0, (uint8_t)(y1 >> 8), (uint8_t)y1 };
    int r;
    if ((r = lcd_cmd(l, 0x2A))) return r;
    for (int i = 0; i < 4; i++) if ((r = lcd_data(l, cx[i]))) return r;
    if ((r = lcd_cmd(l, 0x2B))) return r;
    for (int i = 0; i < 4; i++) if ((r = lcd_data(l, cy[i]))) return r;
    return lcd_cmd(l, 0x2C);
}

int lcd_fill_rect(lcd_t *l, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t c)
{
    int r = lcd_set_window(l, x0, y0, x1, y1); if (r) return r;
    uint32_t n = (uint32_t)(x1 - x0 + 1) * (uint32_t)(y1 - y0 + 1);
    if ((r = wait_idle(l))) return r;
    LCD_WR(l->base, LCD_FILLCOLOR, c);
    while (n) {
        uint32_t k = n > LCD_FILL_MAX ? LCD_FILL_MAX : n;
        LCD_WR(l->base, LCD_FILLCNT, k);
        if ((r = wait_idle(l))) return r;
        n -= k;
    }
    return LCD_OK;
}

int lcd_push_pixels(lcd_t *l, const uint16_t *px, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        int r = lcd_data(l, (uint8_t)(px[i] >> 8)); if (r) return r;
        if ((r = lcd_data(l, (uint8_t)px[i]))) return r;
    }
    return LCD_OK;
}
