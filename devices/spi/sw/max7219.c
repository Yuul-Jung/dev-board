#include "max7219.h"
#include <string.h>

#define TRY(x) do { int e_ = (x); if (e_ != QSPI_OK) return e_; } while (0)

static int frame(max7219_t *m, const uint8_t *buf, size_t len)
{
    qspi_cs(m->s, m->cs, 1);
    int r = qspi_xfer(m->s, buf, 0, len);
    qspi_cs(m->s, m->cs, 0);                 /* LOAD 상승 에지 = 래치 */
    return r;
}
static int bad(const max7219_t *m) { return !m || !m->s || m->nchips < 1 || m->nchips > MAX7219_MAX_CHIPS; }

int max7219_write(max7219_t *m, unsigned chip, uint8_t reg, uint8_t val)
{
    if (bad(m) || chip >= m->nchips) return QSPI_ERR_ARGS;
    uint8_t buf[2 * MAX7219_MAX_CHIPS];
    memset(buf, 0, sizeof buf);                                   /* 모든 워드를 No-Op(0x0000)으로 */
    unsigned pos = m->nchips - 1 - chip;                          /* 먼저 들어간 워드가 가장 먼 칩에 도착한다 (p.10) */
    buf[2 * pos] = reg; buf[2 * pos + 1] = val;
    return frame(m, buf, 2u * m->nchips);
}
int max7219_write_all(max7219_t *m, uint8_t reg, uint8_t val)
{
    if (bad(m)) return QSPI_ERR_ARGS;
    uint8_t buf[2 * MAX7219_MAX_CHIPS];
    for (unsigned i = 0; i < m->nchips; i++) { buf[2 * i] = reg; buf[2 * i + 1] = val; }
    return frame(m, buf, 2u * m->nchips);
}

int max7219_init(max7219_t *m, unsigned digits, uint8_t decode_mask, uint8_t intensity)
{
    if (bad(m) || digits < 1 || digits > 8 || intensity > 15) return QSPI_ERR_ARGS;
    TRY(max7219_write_all(m, MAX7219_REG_TEST, 0));
    TRY(max7219_write_all(m, MAX7219_REG_SHUTDOWN, 0));
    TRY(max7219_write_all(m, MAX7219_REG_SCANLIMIT, (uint8_t)(digits - 1)));
    TRY(max7219_write_all(m, MAX7219_REG_DECODE, decode_mask));
    TRY(max7219_write_all(m, MAX7219_REG_INTENSITY, intensity));
    m->decode = decode_mask;
    for (unsigned d = 0; d < 8; d++)
        TRY(max7219_write_all(m, (uint8_t)(MAX7219_REG_DIGIT0 + d), (uint8_t)(((decode_mask >> d) & 1u) ? MAX7219_CODEB_BLANK : 0)));
    return max7219_write_all(m, MAX7219_REG_SHUTDOWN, 1);
}
int max7219_shutdown(max7219_t *m, int on) { return max7219_write_all(m, MAX7219_REG_SHUTDOWN, on ? 0 : 1); }
int max7219_test(max7219_t *m, int on) { return max7219_write_all(m, MAX7219_REG_TEST, on ? 1 : 0); }
int max7219_set_intensity(max7219_t *m, uint8_t level)
{
    if (level > 15) return QSPI_ERR_ARGS;
    return max7219_write_all(m, MAX7219_REG_INTENSITY, level);
}
static int is_code_b(const max7219_t *m, unsigned digit) { return (m->decode >> digit) & 1u; }

int max7219_set_raw(max7219_t *m, unsigned chip, unsigned digit, uint8_t segs)
{
    if (!m || digit > 7) return QSPI_ERR_ARGS;
    if (is_code_b(m, digit)) return MAX7219_ERR_MODE;
    return max7219_write(m, chip, (uint8_t)(MAX7219_REG_DIGIT0 + digit), segs);
}
int max7219_set_code(max7219_t *m, unsigned chip, unsigned digit, uint8_t code, int dp)
{
    if (!m || digit > 7 || code > 0x0F) return QSPI_ERR_ARGS;
    if (!is_code_b(m, digit)) return MAX7219_ERR_MODE;
    return max7219_write(m, chip, (uint8_t)(MAX7219_REG_DIGIT0 + digit), (uint8_t)(code | (dp ? MAX7219_DP : 0)));
}
int max7219_clear(max7219_t *m, unsigned chip, unsigned digits)
{
    if (!m || digits < 1 || digits > 8) return QSPI_ERR_ARGS;
    for (unsigned d = 0; d < digits; d++)
        TRY(max7219_write(m, chip, (uint8_t)(MAX7219_REG_DIGIT0 + d), (uint8_t)(is_code_b(m, d) ? MAX7219_CODEB_BLANK : 0)));
    return QSPI_OK;
}

int max7219_print_int(max7219_t *m, unsigned chip, unsigned ndigits, int32_t value, int dp_digit)
{
    if (!m || ndigits < 1 || ndigits > 8 || dp_digit < -1 || dp_digit >= (int)ndigits) return QSPI_ERR_ARGS;
    uint8_t code[8];
    int neg = value < 0;
    uint32_t u = neg ? (uint32_t)0 - (uint32_t)value : (uint32_t)value;
    unsigned n = 0;
    do { if (n >= ndigits) return MAX7219_ERR_RANGE; code[n++] = (uint8_t)(u % 10u); u /= 10u; } while (u);
    if (dp_digit >= 0)                                          /* 소수점 자리까지 0 으로 채움: 5, dp 2 → 0.05 */
        while (n <= (unsigned)dp_digit) code[n++] = 0;
    if (neg) { if (n >= ndigits) return MAX7219_ERR_RANGE; code[n++] = MAX7219_CODEB_DASH; }
    for (unsigned d = 0; d < ndigits; d++)
        if (!is_code_b(m, d)) return MAX7219_ERR_MODE;
    for (unsigned d = 0; d < ndigits; d++) {
        TRY(max7219_set_code(m, chip, d, d < n ? code[d] : MAX7219_CODEB_BLANK, dp_digit == (int)d));
    }
    return QSPI_OK;
}

static int char_code(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    switch (c) {
    case '-': return MAX7219_CODEB_DASH;
    case 'E': case 'e': return MAX7219_CODEB_E;
    case 'H': case 'h': return MAX7219_CODEB_H;
    case 'L': case 'l': return MAX7219_CODEB_L;
    case 'P': case 'p': return MAX7219_CODEB_P;
    case ' ': return MAX7219_CODEB_BLANK;
    default: return -1;
    }
}
int max7219_print_str(max7219_t *m, unsigned chip, unsigned ndigits, const char *s)
{
    if (!m || !s || ndigits < 1 || ndigits > 8) return QSPI_ERR_ARGS;
    uint8_t code[8], dp[8];
    unsigned n = 0;
    for (const char *p = s; *p; p++) {
        if (*p == '.') { if (n == 0) return MAX7219_ERR_CHAR; dp[n - 1] = 1; continue; }
        int c = char_code(*p);
        if (c < 0) return MAX7219_ERR_CHAR;
        if (n >= ndigits) return MAX7219_ERR_RANGE;
        code[n] = (uint8_t)c; dp[n] = 0; n++;
    }
    for (unsigned d = 0; d < ndigits; d++)
        if (!is_code_b(m, d)) return MAX7219_ERR_MODE;
    for (unsigned i = 0; i < ndigits; i++) {                        /* 문자열 왼쪽 끝 = 가장 높은 번호 자릿수 */
        unsigned digit = ndigits - 1 - i;
        TRY(max7219_set_code(m, chip, digit, i < n ? code[i] : MAX7219_CODEB_BLANK, i < n && dp[i]));
    }
    return QSPI_OK;
}

int max7219_matrix(max7219_t *m, unsigned chip, const uint8_t rows[8])
{
    if (!m || !rows) return QSPI_ERR_ARGS;
    if (m->decode != 0) return MAX7219_ERR_MODE;
    for (unsigned r = 0; r < 8; r++) TRY(max7219_set_raw(m, chip, r, rows[r]));
    return QSPI_OK;
}
