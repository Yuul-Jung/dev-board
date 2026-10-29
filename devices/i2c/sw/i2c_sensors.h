/* I2C 소자 코드: MPU-6050, SHT31, BH1750, DS3231.
 * 사양 출처는 devices/i2c/sensors.md, clock.md (데이터시트 확인분). 보드 미실행. */
#ifndef I2C_SENSORS_H
#define I2C_SENSORS_H
#include "axi_iic.h"

/* MPU-6050: addr 0x68 (AD0=Low) 또는 0x69 (AD0=High) */
int mpu6050_init(axi_iic_t *d, uint8_t addr);            /* WHO_AM_I 0x68 확인 후 sleep 해제 */
int mpu6050_read_raw(axi_iic_t *d, uint8_t addr, int16_t acc[3], int16_t *temp, int16_t gyro[3]);
/* 기본 범위 ±2 g (16384 LSB/g), ±250 °/s (131 LSB/(°/s)) 기준 변환 */
static inline int32_t mpu6050_acc_mg(int16_t raw)  { return ((int32_t)raw * 1000) / 16384; }
static inline int32_t mpu6050_gyro_dps10(int16_t raw) { return ((int32_t)raw * 100) / 131; }  /* 0.1 °/s 단위 */
/* 온도: 35 + (raw + 521)/340 °C (스펙 표의 340 LSB/°C, 35 °C에서 -521 LSB 오프셋. 오프셋 정확도는 규격 확인 필요) */
static inline int32_t mpu6050_temp_mc(int16_t raw) { return 35000 + (((int32_t)raw + 521) * 1000) / 340; }

/* SHT31: addr 0x44 또는 0x45. 단일 측정, 클럭 스트레칭 끔, High repeatability(0x2400) */
uint8_t sht3x_crc8(const uint8_t *data, unsigned len);   /* 다항식 0x31, 초기값 0xFF */
int sht31_read(axi_iic_t *d, uint8_t addr, int32_t *temp_mc, int32_t *rh_m);  /* m°C, m%RH */

/* BH1750: addr 0x23 또는 0x5C. 연속 고분해능 모드(0x10) */
int bh1750_start(axi_iic_t *d, uint8_t addr);
int bh1750_read_lux10(axi_iic_t *d, uint8_t addr, uint32_t *lux_x10);   /* lux = raw / 1.2, 0.1 lx 단위 */

/* DS3231: addr 0x68. 00h~06h 시간 레지스터 7바이트(BCD) 원시 읽기 */
int ds3231_read_raw(axi_iic_t *d, uint8_t regs[7]);
static inline uint8_t bcd2bin(uint8_t b) { return (uint8_t)((b >> 4) * 10 + (b & 0x0F)); }
#endif
