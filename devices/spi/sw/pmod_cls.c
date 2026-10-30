/* Pmod CLS 드라이버 구현. */
#include "pmod_cls.h"
#include <string.h>

/* 1바이트 전송 helper */
static int send1(cls_t *c, uint8_t b)
{
    qspi_cs(c->spi, c->cs_idx, 1);
    int e = qspi_xfer(c->spi, &b, NULL, 1);
    qspi_cs(c->spi, c->cs_idx, 0);
    return e ? CLS_ERR_SPI : CLS_OK;
}

/* n 바이트 배열 전송 (각 바이트를 개별 1바이트 전송) */
static int sendn(cls_t *c, const uint8_t *buf, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        int e = send1(c, buf[i]);
        if (e) return e;
    }
    return CLS_OK;
}

int cls_init(cls_t *c, qspi_t *spi, unsigned cs_idx)
{
    if (!c || !spi) return CLS_ERR_ARGS;
    c->spi    = spi;
    c->cs_idx = cs_idx;
    return CLS_OK;
}

int cls_clear(cls_t *c)
{
    if (!c || !c->spi) return CLS_ERR_ARGS;
    /* ESC [ j = 0x1B 0x5B 0x6A */
    const uint8_t seq[] = { 0x1Bu, '[', 'j' };
    return sendn(c, seq, sizeof seq);
}

int cls_home(cls_t *c)
{
    if (!c || !c->spi) return CLS_ERR_ARGS;
    return cls_cursor(c, 0, 0);
}

int cls_cursor(cls_t *c, unsigned row, unsigned col)
{
    if (!c || !c->spi) return CLS_ERR_ARGS;
    if (row >= CLS_ROWS || col >= CLS_COLS) return CLS_ERR_RANGE;
    /* ESC [ <r> ; <cc> H
     * Pmod CLS Reference 예제: \x1B[1;2H → 1행 2열.
     * 열은 0~9, A~F (16진 소문자 없음 — 0~9 '0'..'9', 10~15 'A'..'F') (D3/미확인).
     * 행: '0' 또는 '1'. */
    char col_ch;
    if (col < 10u)  col_ch = (char)('0' + col);
    else            col_ch = (char)('A' + (col - 10u));
    const uint8_t seq[] = {
        0x1Bu, '[',
        (uint8_t)('0' + row), ';',
        (uint8_t)col_ch, 'H'
    };
    return sendn(c, seq, sizeof seq);
}

int cls_write(cls_t *c, const char *text, size_t len)
{
    if (!c || !c->spi || !text) return CLS_ERR_ARGS;
    if (len == 0) len = strlen(text);
    for (size_t i = 0; i < len; i++) {
        int e = send1(c, (uint8_t)text[i]);
        if (e) return e;
    }
    return CLS_OK;
}

int cls_puts(cls_t *c, unsigned row, unsigned col, const char *text)
{
    if (!c || !c->spi || !text) return CLS_ERR_ARGS;
    int e = cls_cursor(c, row, col);
    if (e) return e;
    return cls_write(c, text, 0);
}

int cls_clear_write(cls_t *c, const char *line1, const char *line2)
{
    if (!c || !c->spi) return CLS_ERR_ARGS;
    int e;
    e = cls_clear(c); if (e) return e;
    if (line1) { e = cls_puts(c, 0, 0, line1); if (e) return e; }
    if (line2) { e = cls_puts(c, 1, 0, line2); if (e) return e; }
    return CLS_OK;
}
