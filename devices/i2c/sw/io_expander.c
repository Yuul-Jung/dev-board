#include "io_expander.h"

/* ---------------- PCF8574 ---------------- */
int pcf8574_write(pcf8574_t *p, uint8_t v)
{
    int rc = axi_iic_write(p->iic, p->addr7, &v, 1);
    if (rc == IIC_OK) p->shadow = v;
    return rc;
}

int pcf8574_init(pcf8574_t *p, axi_iic_t *iic, uint8_t addr7)
{
    if (!p || !iic || addr7 < 0x20 || addr7 > 0x27) return IIC_ERR_ARGS;
    p->iic = iic; p->addr7 = addr7; p->shadow = 0xFF;
    return pcf8574_write(p, 0xFF);
}

int pcf8574_read(pcf8574_t *p, uint8_t *v)
{
    if (!p || !v) return IIC_ERR_ARGS;
    return axi_iic_read(p->iic, p->addr7, v, 1);
}

int pcf8574_set_pin(pcf8574_t *p, unsigned pin, int level)
{
    if (!p || pin > 7) return IIC_ERR_ARGS;
    uint8_t v = level ? (uint8_t)(p->shadow | (1u << pin)) : (uint8_t)(p->shadow & ~(1u << pin));
    return pcf8574_write(p, v);
}

/* ---------------- MCP23017 ---------------- */
int mcp23017_write_reg(mcp23017_t *m, uint8_t reg, uint8_t v)
{
    uint8_t b[2] = { reg, v };
    return axi_iic_write(m->iic, m->addr7, b, 2);
}

int mcp23017_read_reg(mcp23017_t *m, uint8_t reg, uint8_t *v)
{
    if (!v) return IIC_ERR_ARGS;
    return axi_iic_write_read(m->iic, m->addr7, &reg, 1, v, 1);
}

static int write16(mcp23017_t *m, uint8_t reg_a, uint16_t v)
{
    uint8_t b[3] = { reg_a, (uint8_t)v, (uint8_t)(v >> 8) };   /* A 먼저, 포인터가 B 로 증가 */
    return axi_iic_write(m->iic, m->addr7, b, 3);
}

int mcp23017_init(mcp23017_t *m, axi_iic_t *iic, uint8_t addr7)
{
    if (!m || !iic || addr7 < 0x20 || addr7 > 0x27) return IIC_ERR_ARGS;
    m->iic = iic; m->addr7 = addr7; m->olat = 0;     /* 리셋값 OLAT=0 */
    return mcp23017_write_reg(m, MCP_IOCON, 0);       /* BANK=0, SEQOP=0, 나머지 기본 */
}

int mcp23017_set_dir(mcp23017_t *m, uint16_t input_mask) { return write16(m, MCP_IODIRA, input_mask); }
int mcp23017_set_pullup(mcp23017_t *m, uint16_t mask)    { return write16(m, MCP_GPPUA, mask); }

int mcp23017_write(mcp23017_t *m, uint16_t v)
{
    int rc = write16(m, MCP_OLATA, v);
    if (rc == IIC_OK) m->olat = v;
    return rc;
}

int mcp23017_read(mcp23017_t *m, uint16_t *v)
{
    uint8_t r[2], reg = MCP_GPIOA;
    if (!v) return IIC_ERR_ARGS;
    int rc = axi_iic_write_read(m->iic, m->addr7, &reg, 1, r, 2);
    if (rc) return rc;
    *v = (uint16_t)(r[0] | (r[1] << 8));
    return IIC_OK;
}

int mcp23017_set_pin(mcp23017_t *m, unsigned pin, int level)
{
    if (!m || pin > 15) return IIC_ERR_ARGS;
    uint16_t v = level ? (uint16_t)(m->olat | (1u << pin)) : (uint16_t)(m->olat & ~(1u << pin));
    return mcp23017_write(m, v);
}
