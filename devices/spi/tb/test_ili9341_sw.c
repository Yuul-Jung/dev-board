#include <stdio.h>
#include <string.h>
#include "../sw/ili9341.h"

/* 모의 레지스터: TX 쓰기를 {dc,byte} 로 기록, FILL 기록, delay 호출 기록 */
static uint32_t ctrl, divr; static int ntx; static uint16_t tx[512];
static uint32_t fillcolor, fillcnt[8]; static int nfill, busy_polls, force_busy, force_ovr;
static unsigned delays[16]; static int ndel; static uint32_t ctrl_at_delay[16];
void lcd_delay_ms(unsigned ms) { delays[ndel] = ms; ctrl_at_delay[ndel++] = ctrl; }
uint32_t mock_l_rd(uint32_t off)
{
    if (off == LCD_CTRL) return ctrl;
    if (off == LCD_TX) { if (force_ovr) return LCD_ST_OVERRUN; if (force_busy) return LCD_ST_BUSY; return (busy_polls++ % 3 == 0) ? LCD_ST_BUSY : 0; }
    return 0;
}
void mock_l_wr(uint32_t off, uint32_t v)
{
    if (off == LCD_CTRL) ctrl = v; else if (off == LCD_DIV) divr = v;
    else if (off == LCD_TX) tx[ntx++] = (uint16_t)v;
    else if (off == LCD_FILLCOLOR) fillcolor = v;
    else if (off == LCD_FILLCNT) fillcnt[nfill++] = v;
}
static int fails;
#define CHK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
int main(void)
{
    lcd_t l; lcd_attach(&l, 0, 49);
    CHK(divr == 49 && (ctrl & LCD_CTRL_RESX) && (ctrl & LCD_CTRL_CS));
    lcd_hw_reset(&l, 7, 33);                                  /* 값은 호출자 지정: 드라이버는 숫자를 정하지 않음 */
    CHK(ndel == 2 && delays[0] == 7 && delays[1] == 33 && !(ctrl_at_delay[0] & LCD_CTRL_RESX) && (ctrl_at_delay[1] & LCD_CTRL_RESX));
    lcd_backlight(&l, 1); CHK(ctrl & LCD_CTRL_BL); lcd_backlight(&l, 0); CHK(!(ctrl & LCD_CTRL_BL));

    ndel = 0; ntx = 0;
    const lcd_step_t table[] = { { 0xAA, 2, {0x01, 0x02}, 5 }, { 0xBB, 0, {0}, 0 }, LCD_STEP_END };   /* 임의의 시험 값 */
    CHK(lcd_run_table(&l, table) == LCD_OK);
    CHK(ntx == 4 && tx[0] == 0xAA && tx[1] == 0x101 && tx[2] == 0x102 && tx[3] == 0xBB && ndel == 1 && delays[0] == 5);

    ntx = 0;
    CHK(lcd_set_window(&l, 0x0010, 0x0120, 0x00EF, 0x013F) == LCD_OK);
    uint16_t exp[] = { 0x2A, 0x100, 0x110, 0x100, 0x1EF, 0x2B, 0x101, 0x120, 0x101, 0x13F, 0x2C };
    CHK(ntx == 11 && memcmp(tx, exp, sizeof exp) == 0);
    CHK(lcd_set_window(&l, 5, 0, 4, 0) == LCD_ERR_ARGS && lcd_set_window(&l, 0, 0, 240, 0) == LCD_ERR_ARGS);

    nfill = 0; CHK(lcd_fill_rect(&l, 0, 0, 239, 319, 0xF800) == LCD_OK);   /* 전체 화면 76,800 픽셀 */
    CHK(nfill == 1 && fillcnt[0] == 76800 && fillcolor == 0xF800);

    ntx = 0; uint16_t px[2] = { 0x1234, 0xABCD };
    CHK(lcd_push_pixels(&l, px, 2) == LCD_OK && ntx == 4 && tx[0] == 0x112 && tx[1] == 0x134 && tx[2] == 0x1AB && tx[3] == 0x1CD);

    force_ovr = 1; CHK(lcd_cmd(&l, 0x11) == LCD_ERR_OVERRUN); force_ovr = 0;
    force_busy = 1; l.timeout = 5; CHK(lcd_cmd(&l, 0x11) == LCD_ERR_TIMEOUT);
    printf(fails ? "FAILED %d\n" : "ALL PASS\n", fails);
    return fails != 0;
}
