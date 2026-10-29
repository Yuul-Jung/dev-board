#include "hd44780_pcf8574.h"

static int wr_cb(void *ctx, uint8_t port) { hd44780_i2c_t *d = (hd44780_i2c_t *)ctx; return pcf8574_write(d->pcf, port); }
static void delay_cb(void *ctx, uint32_t us) { hd44780_i2c_t *d = (hd44780_i2c_t *)ctx; d->delay_us(us); }

int hd44780_i2c_init(hd44780_i2c_t *d, pcf8574_t *pcf, void (*delay_us)(uint32_t us), hd44780_pins_t pins, unsigned cols, unsigned rows)
{
    if (!d || !pcf || !delay_us) return HD44780_ERR_ARGS;
    d->pcf = pcf; d->delay_us = delay_us;
    return hd44780_init(&d->lcd, wr_cb, delay_cb, d, pins, cols, rows);
}
