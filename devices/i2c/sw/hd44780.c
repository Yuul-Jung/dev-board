#include "hd44780.h"

#define TRY(x) do { int e_ = (x); if (e_ < 0) return e_; } while (0)

static uint8_t bit(uint8_t pos) { return pos == HD44780_NC ? 0 : (uint8_t)(1u << pos); }

static uint8_t compose(const hd44780_t *h, uint8_t nib, int rs, int en)
{
    const hd44780_pins_t *p = &h->pins;
    uint8_t b = 0;
    if (nib & 1) b |= bit(p->d4);
    if (nib & 2) b |= bit(p->d5);
    if (nib & 4) b |= bit(p->d6);
    if (nib & 8) b |= bit(p->d7);
    if (rs) b |= bit(p->rs);
    if (en) b |= bit(p->en);
    if (h->bl) b |= bit(p->bl);                                  /* RW 는 항상 0 */
    return b;
}

static int nibble(hd44780_t *h, uint8_t nib, int rs)
{
    uint8_t lo = compose(h, nib, rs, 0), hi = compose(h, nib, rs, 1);
    TRY(h->wr(h->ctx, lo));                  /* 데이터·RS 를 먼저 확정 */
    h->delay(h->ctx, HD44780_PULSE_US);
    TRY(h->wr(h->ctx, hi));                  /* E 상승 */
    h->delay(h->ctx, HD44780_PULSE_US);
    TRY(h->wr(h->ctx, lo));                  /* E 하강에서 래치 */
    h->delay(h->ctx, HD44780_PULSE_US);
    return HD44780_OK;
}

static int send(hd44780_t *h, uint8_t byte, int rs, uint32_t wait_us)
{
    TRY(nibble(h, (uint8_t)(byte >> 4), rs));
    TRY(nibble(h, (uint8_t)(byte & 0x0F), rs));
    h->delay(h->ctx, wait_us);
    return HD44780_OK;
}
static int cmd(hd44780_t *h, uint8_t c) { return send(h, c, 0, HD44780_WAIT_CMD_US); }

static int pins_ok(const hd44780_pins_t *p)
{
    uint8_t v[8] = { p->rs, p->rw, p->en, p->bl, p->d4, p->d5, p->d6, p->d7 };
    uint8_t used = 0;
    for (int i = 0; i < 8; i++) {
        if (v[i] == HD44780_NC) { if (i == 1 || i == 3) continue; return 0; }      /* rw, bl 만 미연결 허용 */
        if (v[i] > 7 || (used & (1u << v[i]))) return 0;
        used |= (uint8_t)(1u << v[i]);
    }
    return 1;
}

int hd44780_init(hd44780_t *h, hd44780_write_fn wr, hd44780_delay_fn delay, void *ctx, hd44780_pins_t pins, unsigned cols, unsigned rows)
{
    if (!h || !wr || !delay || cols < 1 || cols > 40 || rows < 1 || rows > 4 || !pins_ok(&pins)) return HD44780_ERR_ARGS;
    h->wr = wr; h->delay = delay; h->ctx = ctx; h->pins = pins; h->cols = (uint8_t)cols; h->rows = (uint8_t)rows;
    h->ctrl = HD44780_DISP_D; h->bl = 1; h->col = h->row = 0;
    h->row_addr[0] = 0x00; h->row_addr[1] = 0x40; h->row_addr[2] = (uint8_t)cols; h->row_addr[3] = (uint8_t)(0x40 + cols);
    TRY(h->wr(h->ctx, compose(h, 0, 0, 0)));                        /* 백라이트만 켜고 선을 낮춤 */
    h->delay(h->ctx, HD44780_WAIT_POWERON_US);
    TRY(nibble(h, 0x3, 0)); h->delay(h->ctx, HD44780_WAIT_INIT1_US);   /* 기능 설정(8비트) */
    TRY(nibble(h, 0x3, 0)); h->delay(h->ctx, HD44780_WAIT_INIT2_US);
    TRY(nibble(h, 0x3, 0)); h->delay(h->ctx, HD44780_WAIT_CMD_US);
    TRY(nibble(h, 0x2, 0)); h->delay(h->ctx, HD44780_WAIT_CMD_US);     /* 4비트 전환 */
    TRY(cmd(h, (uint8_t)(HD44780_CMD_FUNCTION | (rows > 1 ? HD44780_FN_N : 0))));   /* DL=0, F=0 (5×8) */
    TRY(cmd(h, HD44780_CMD_DISPLAY));                               /* 표시 끔 */
    TRY(send(h, HD44780_CMD_CLEAR, 0, HD44780_WAIT_CLEAR_US));
    TRY(cmd(h, HD44780_CMD_ENTRY | HD44780_ENTRY_ID));              /* 증가, 이동 없음 */
    return cmd(h, (uint8_t)(HD44780_CMD_DISPLAY | h->ctrl));
}

int hd44780_clear(hd44780_t *h)
{
    if (!h) return HD44780_ERR_ARGS;
    TRY(send(h, HD44780_CMD_CLEAR, 0, HD44780_WAIT_CLEAR_US));
    h->col = h->row = 0;
    return HD44780_OK;
}
int hd44780_home(hd44780_t *h)
{
    if (!h) return HD44780_ERR_ARGS;
    TRY(send(h, HD44780_CMD_HOME, 0, HD44780_WAIT_CLEAR_US));
    h->col = h->row = 0;
    return HD44780_OK;
}
int hd44780_set_cursor(hd44780_t *h, unsigned col, unsigned row)
{
    if (!h) return HD44780_ERR_ARGS;
    if (col >= h->cols || row >= h->rows) return HD44780_ERR_RANGE;
    TRY(cmd(h, (uint8_t)(HD44780_CMD_SETDDRAM | (h->row_addr[row] + col))));
    h->col = (uint8_t)col; h->row = (uint8_t)row;
    return HD44780_OK;
}
int hd44780_putc(hd44780_t *h, uint8_t ch)
{
    if (!h) return HD44780_ERR_ARGS;
    if (h->col >= h->cols) return HD44780_ERR_RANGE;
    TRY(send(h, ch, 1, HD44780_WAIT_CMD_US));
    h->col++;
    return HD44780_OK;
}
int hd44780_print(hd44780_t *h, const char *s)
{
    if (!h || !s) return HD44780_ERR_ARGS;
    for (; *s; s++) TRY(hd44780_putc(h, (uint8_t)*s));
    return HD44780_OK;
}
int hd44780_print_int(hd44780_t *h, int32_t v)
{
    char buf[12]; unsigned n = 0;
    uint32_t u = v < 0 ? (uint32_t)0 - (uint32_t)v : (uint32_t)v;
    do { buf[n++] = (char)('0' + u % 10u); u /= 10u; } while (u);
    if (v < 0) buf[n++] = '-';
    if (!h) return HD44780_ERR_ARGS;
    if ((unsigned)(h->cols - h->col) < n) return HD44780_ERR_RANGE;        /* 일부만 찍히지 않도록 미리 검사 */
    while (n) TRY(hd44780_putc(h, (uint8_t)buf[--n]));
    return HD44780_OK;
}

static int set_ctrl(hd44780_t *h, uint8_t mask, int on)
{
    if (!h) return HD44780_ERR_ARGS;
    if (on) h->ctrl |= mask; else h->ctrl &= (uint8_t)~mask;
    return cmd(h, (uint8_t)(HD44780_CMD_DISPLAY | h->ctrl));
}
int hd44780_display(hd44780_t *h, int on) { return set_ctrl(h, HD44780_DISP_D, on); }
int hd44780_cursor(hd44780_t *h, int on)  { return set_ctrl(h, HD44780_DISP_C, on); }
int hd44780_blink(hd44780_t *h, int on)   { return set_ctrl(h, HD44780_DISP_B, on); }

int hd44780_backlight(hd44780_t *h, int on)
{
    if (!h) return HD44780_ERR_ARGS;
    if (h->pins.bl == HD44780_NC) return HD44780_ERR_NOBL;
    h->bl = on ? 1 : 0;
    return h->wr(h->ctx, compose(h, 0, 0, 0));
}

int hd44780_shift_display(hd44780_t *h, int right)
{
    if (!h) return HD44780_ERR_ARGS;
    return cmd(h, (uint8_t)(HD44780_CMD_SHIFT | HD44780_SHIFT_SC | (right ? HD44780_SHIFT_RL : 0)));
}

int hd44780_create_char(hd44780_t *h, unsigned slot, const uint8_t rows8[8])
{
    if (!h || !rows8 || slot > 7) return HD44780_ERR_ARGS;
    TRY(cmd(h, (uint8_t)(HD44780_CMD_SETCGRAM | (slot << 3))));
    for (unsigned i = 0; i < 8; i++) TRY(send(h, (uint8_t)(rows8[i] & 0x1F), 1, HD44780_WAIT_CMD_US));
    return cmd(h, (uint8_t)(HD44780_CMD_SETDDRAM | (h->row_addr[h->row] + h->col)));   /* 주소 카운터를 CGRAM 에서 DDRAM 으로 되돌림 */
}
