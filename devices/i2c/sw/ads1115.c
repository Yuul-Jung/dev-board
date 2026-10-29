#include "ads1115.h"

#define CFG_OS        (1u << 15)
#define CFG_MODE_SS   (1u << 8)      /* 1 = 단발/전력 차단, 0 = 연속 */
#define CFG_COMP_OFF  0x0003u        /* COMP_QUE=11: 비교기 끔 (리셋 기본값) */

int ads1115_write_reg(ads1115_t *a, uint8_t reg, uint16_t v)
{
    uint8_t b[3] = { reg, (uint8_t)(v >> 8), (uint8_t)v };   /* MSB 먼저 */
    return axi_iic_write(a->iic, a->addr7, b, 3);
}

int ads1115_read_reg(ads1115_t *a, uint8_t reg, uint16_t *v)
{
    uint8_t r[2];
    int rc = axi_iic_write_read(a->iic, a->addr7, &reg, 1, r, 2);
    if (rc) return rc;
    *v = (uint16_t)((r[0] << 8) | r[1]);
    return IIC_OK;
}

int ads1115_init(ads1115_t *a, axi_iic_t *iic, uint8_t addr7)
{
    if (!a || !iic || addr7 < 0x48 || addr7 > 0x4B) return IIC_ERR_ARGS;
    a->iic = iic; a->addr7 = addr7;
    uint16_t c;
    return ads1115_read_reg(a, ADS_REG_CONFIG, &c);
}

static int args_ok(int mux, int pga, int dr)
{   /* PGA 코드 6,7 은 0.256 V 와 같지만 여기서는 0~5 만 허용 */
    return mux >= 0 && mux <= 7 && pga >= 0 && pga <= 5 && dr >= 0 && dr <= 7;
}

static uint16_t cfg(int mux, int pga, int dr)
{
    return (uint16_t)(((unsigned)mux << 12) | ((unsigned)pga << 9) | ((unsigned)dr << 5) | CFG_COMP_OFF);
}

int ads1115_read_single(ads1115_t *a, int mux, int pga, int dr, int16_t *raw)
{
    if (!a || !raw || !args_ok(mux, pga, dr)) return IIC_ERR_ARGS;
    int rc = ads1115_write_reg(a, ADS_REG_CONFIG, (uint16_t)(CFG_OS | CFG_MODE_SS | cfg(mux, pga, dr)));
    if (rc) return rc;
    /* 변환 시간 = 1/DR (최대 125 ms). 1 ms 간격으로 최대 200회 폴링 */
    for (int i = 0; i < 200; i++) {
        uint16_t c;
        rc = ads1115_read_reg(a, ADS_REG_CONFIG, &c);
        if (rc) return rc;
        if (c & CFG_OS) {                       /* 읽기에서 OS=1 → 변환 중 아님 */
            uint16_t v;
            rc = ads1115_read_reg(a, ADS_REG_CONV, &v);
            if (rc) return rc;
            *raw = (int16_t)v;
            return IIC_OK;
        }
        iic_delay_ms(1);
    }
    return IIC_ERR_TIMEOUT;
}

int ads1115_start_continuous(ads1115_t *a, int mux, int pga, int dr)
{
    if (!a || !args_ok(mux, pga, dr)) return IIC_ERR_ARGS;
    return ads1115_write_reg(a, ADS_REG_CONFIG, cfg(mux, pga, dr));   /* MODE=0 */
}

int ads1115_read_conversion(ads1115_t *a, int16_t *raw)
{
    uint16_t v;
    if (!a || !raw) return IIC_ERR_ARGS;
    int rc = ads1115_read_reg(a, ADS_REG_CONV, &v);
    if (rc) return rc;
    *raw = (int16_t)v;
    return IIC_OK;
}

int32_t ads1115_raw_to_uv(int16_t raw, int pga)
{
    static const int32_t fsr_uv[6] = { 6144000, 4096000, 2048000, 1024000, 512000, 256000 };
    if (pga < 0 || pga > 5) return 0;
    return (int32_t)(((int64_t)raw * fsr_uv[pga]) / 32768);
}
