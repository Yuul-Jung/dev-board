#include "axi_gpio.h"

static uint32_t off_data(unsigned ch) { return ch ? GPIO_REG_DATA2 : GPIO_REG_DATA; }
static uint32_t off_tri(unsigned ch)  { return ch ? GPIO_REG_TRI2 : GPIO_REG_TRI; }
static int bad(const gpio_t *g, unsigned ch) { return !g || g->channels < 1 || g->channels > 2 || ch >= g->channels; }

int gpio_init(gpio_t *g, uintptr_t base, unsigned channels)
{
    if (!g || channels < 1 || channels > 2) return GPIO_ERR_ARGS;
    g->base = base; g->channels = channels; g->ier = 0;
    for (unsigned ch = 0; ch < channels; ch++) {
        g->out[ch] = 0; g->in_mask[ch] = 0xFFFFFFFFu;
        GPIO_WR(base, off_tri(ch), 0xFFFFFFFFu);
        GPIO_WR(base, off_data(ch), 0);
    }
    return GPIO_OK;
}

int gpio_pin_mode(gpio_t *g, unsigned ch, unsigned bit, int out, int level)
{
    if (bad(g, ch) || bit > 31) return GPIO_ERR_ARGS;
    uint32_t m = 1u << bit;
    if (out) {
        if (level) g->out[ch] |= m; else g->out[ch] &= ~m;
        GPIO_WR(g->base, off_data(ch), g->out[ch]);              /* 입력 상태에서의 쓰기 효과는 미확인 → 방향을 바꾼 뒤 다시 쓴다 */
        g->in_mask[ch] &= ~m;
        GPIO_WR(g->base, off_tri(ch), g->in_mask[ch]);
        GPIO_WR(g->base, off_data(ch), g->out[ch]);
    } else {
        g->in_mask[ch] |= m;
        GPIO_WR(g->base, off_tri(ch), g->in_mask[ch]);
    }
    return GPIO_OK;
}

int gpio_write(gpio_t *g, unsigned ch, unsigned bit, int level)
{
    if (bad(g, ch) || bit > 31) return GPIO_ERR_ARGS;
    uint32_t m = 1u << bit;
    if (g->in_mask[ch] & m) return GPIO_ERR_DIR;
    if (level) g->out[ch] |= m; else g->out[ch] &= ~m;
    GPIO_WR(g->base, off_data(ch), g->out[ch]);
    return GPIO_OK;
}

int gpio_toggle(gpio_t *g, unsigned ch, unsigned bit)
{
    if (bad(g, ch) || bit > 31) return GPIO_ERR_ARGS;
    return gpio_write(g, ch, bit, !((g->out[ch] >> bit) & 1u));
}

int gpio_write_mask(gpio_t *g, unsigned ch, uint32_t mask, uint32_t value)
{
    if (bad(g, ch)) return GPIO_ERR_ARGS;
    if (mask & g->in_mask[ch]) return GPIO_ERR_DIR;
    g->out[ch] = (g->out[ch] & ~mask) | (value & mask);
    GPIO_WR(g->base, off_data(ch), g->out[ch]);
    return GPIO_OK;
}

int gpio_read_port(gpio_t *g, unsigned ch, uint32_t *value)
{
    if (bad(g, ch) || !value) return GPIO_ERR_ARGS;
    uint32_t hw = GPIO_RD(g->base, off_data(ch));
    *value = (hw & g->in_mask[ch]) | (g->out[ch] & ~g->in_mask[ch]);
    return GPIO_OK;
}

int gpio_read(gpio_t *g, unsigned ch, unsigned bit)
{
    uint32_t v;
    if (bit > 31) return GPIO_ERR_ARGS;
    int r = gpio_read_port(g, ch, &v);
    return r < 0 ? r : (int)((v >> bit) & 1u);
}

unsigned gpio_irq_pending(gpio_t *g)
{
    return g ? (GPIO_RD(g->base, GPIO_REG_ISR) & 3u) : 0;
}

int gpio_irq_ack(gpio_t *g, unsigned irq_mask)
{
    if (!g || (irq_mask & ~3u)) return GPIO_ERR_ARGS;
    uint32_t p = GPIO_RD(g->base, GPIO_REG_ISR) & irq_mask;
    if (p) GPIO_WR(g->base, GPIO_REG_ISR, p);
    return GPIO_OK;
}

int gpio_irq_enable(gpio_t *g, unsigned irq_mask)
{
    if (!g || !irq_mask || (irq_mask & ~3u) || (irq_mask >> g->channels)) return GPIO_ERR_ARGS;
    gpio_irq_ack(g, irq_mask);
    g->ier |= irq_mask;
    GPIO_WR(g->base, GPIO_REG_IER, g->ier);
    GPIO_WR(g->base, GPIO_REG_GIER, GPIO_GIER_ENABLE);
    return GPIO_OK;
}

int gpio_irq_disable(gpio_t *g, unsigned irq_mask)
{
    if (!g || (irq_mask & ~3u)) return GPIO_ERR_ARGS;
    g->ier &= ~irq_mask;
    GPIO_WR(g->base, GPIO_REG_IER, g->ier);
    if (!g->ier) GPIO_WR(g->base, GPIO_REG_GIER, 0);
    return GPIO_OK;
}
