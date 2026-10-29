#include "i2c_sensors.h"

/* ---------- MPU-6050 ---------- */
#define MPU_PWR_MGMT_1 0x6B
#define MPU_WHO_AM_I   0x75
#define MPU_ACCEL_XOUT_H 0x3B

int mpu6050_init(axi_iic_t *d, uint8_t addr)
{
    uint8_t reg = MPU_WHO_AM_I, id = 0;
    int r = axi_iic_write_read(d, addr, &reg, 1, &id, 1);
    if (r) return r;
    if (id != 0x68) return -10;                       /* 다른 장치 */
    uint8_t w[2] = { MPU_PWR_MGMT_1, 0x00 };          /* 리셋 0x40 의 SLEEP(bit6) 해제 */
    return axi_iic_write(d, addr, w, 2);
}

int mpu6050_read_raw(axi_iic_t *d, uint8_t addr, int16_t acc[3], int16_t *temp, int16_t gyro[3])
{
    uint8_t reg = MPU_ACCEL_XOUT_H, b[14];
    int r = axi_iic_write_read(d, addr, &reg, 1, b, 14);   /* 0x3B 가속도 6, 0x41 온도 2, 0x43 자이로 6 */
    if (r) return r;
    for (int i = 0; i < 3; i++) acc[i]  = (int16_t)((b[2 * i] << 8) | b[2 * i + 1]);
    *temp = (int16_t)((b[6] << 8) | b[7]);
    for (int i = 0; i < 3; i++) gyro[i] = (int16_t)((b[8 + 2 * i] << 8) | b[9 + 2 * i]);
    return IIC_OK;
}

/* ---------- SHT31 ---------- */
uint8_t sht3x_crc8(const uint8_t *data, unsigned len)
{
    uint8_t crc = 0xFF;
    for (unsigned i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
    }
    return crc;
}

int sht31_read(axi_iic_t *d, uint8_t addr, int32_t *temp_mc, int32_t *rh_m)
{
    const uint8_t cmd[2] = { 0x24, 0x00 };
    uint8_t b[6];
    int r = axi_iic_write(d, addr, cmd, 2);
    if (r) return r;
    iic_delay_ms(16);                                   /* High repeatability 최대 15 ms */
    r = axi_iic_read(d, addr, b, 6);
    if (r) return r;
    if (sht3x_crc8(b, 2) != b[2] || sht3x_crc8(b + 3, 2) != b[5]) return -11;
    uint32_t st = ((uint32_t)b[0] << 8) | b[1];
    uint32_t sh = ((uint32_t)b[3] << 8) | b[4];
    *temp_mc = (int32_t)(-45000 + (int64_t)175000 * st / 65535);   /* T = -45 + 175 S/(2^16-1) */
    *rh_m    = (int32_t)((int64_t)100000 * sh / 65535);            /* RH = 100 S/(2^16-1) */
    return IIC_OK;
}

/* ---------- BH1750 ---------- */
int bh1750_start(axi_iic_t *d, uint8_t addr)
{
    uint8_t on = 0x01, mode = 0x10;
    int r = axi_iic_write(d, addr, &on, 1);
    if (r) return r;
    return axi_iic_write(d, addr, &mode, 1);
}

int bh1750_read_lux10(axi_iic_t *d, uint8_t addr, uint32_t *lux_x10)
{
    uint8_t b[2];
    int r = axi_iic_read(d, addr, b, 2);                 /* 시작 후 최대 180 ms 지난 뒤 호출 */
    if (r) return r;
    uint32_t raw = ((uint32_t)b[0] << 8) | b[1];
    *lux_x10 = (raw * 10u * 10u) / 12u;                  /* raw/1.2 lx → 0.1 lx 단위 */
    return IIC_OK;
}

/* ---------- DS3231 ---------- */
int ds3231_read_raw(axi_iic_t *d, uint8_t regs[7])
{
    uint8_t reg = 0x00;
    return axi_iic_write_read(d, 0x68, &reg, 1, regs, 7);
}
