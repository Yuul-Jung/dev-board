/* BME280 드라이버. 레지스터 맵·보정식은 Bosch 데이터시트(BST-BME280-DS002) 4.2.2~4.2.3, 3.5.1 (PDF 추출, D1).
 * 습도 식의 괄호는 PDF 추출 중 복원된 것이라 원문 대조 필요. 보드 미실행. */
#ifndef BME280_H
#define BME280_H
#include "axi_iic.h"

#define BME280_ADDR_LOW  0x76   /* SDO=GND */
#define BME280_ADDR_HIGH 0x77   /* SDO=VDDIO */
#define BME280_CHIP_ID   0x60

typedef struct {
    uint16_t T1; int16_t T2, T3;
    uint16_t P1; int16_t P2, P3, P4, P5, P6, P7, P8, P9;
    uint8_t H1; int16_t H2; uint8_t H3; int16_t H4, H5; int8_t H6;
} bme280_calib_t;

typedef struct { uint8_t addr; bme280_calib_t c; } bme280_t;

/* 순수 계산 (I/O 없음) */
void bme280_parse_calib(const uint8_t a[26] /* 0x88..0xA1 */, const uint8_t b[7] /* 0xE1..0xE7 */, bme280_calib_t *c);
int32_t  bme280_comp_T(const bme280_calib_t *c, int32_t adc_T, int32_t *t_fine);   /* 0.01 °C */
uint32_t bme280_comp_P(const bme280_calib_t *c, int32_t adc_P, int32_t t_fine);    /* Q24.8 Pa, 0 = 계산 불가 */
uint32_t bme280_comp_H(const bme280_calib_t *c, int32_t adc_H, int32_t t_fine);    /* Q22.10 %RH */
/* 0xF7..0xFE 8바이트 → 원시값 */
void bme280_split(const uint8_t d[8], int32_t *adc_P, int32_t *adc_T, int32_t *adc_H);

/* I/O */
int bme280_init(bme280_t *s, axi_iic_t *d, uint8_t addr);  /* id 확인, 보정 계수 읽기 */
/* 강제 모드 1회 측정(T,P,H ×1, 필터 끔: 데이터시트 날씨 모니터링 설정). 결과: 온도 0.01 °C, 압력 Q24.8 Pa, 습도 Q22.10 %RH (데이터시트 출력 형식 그대로) */
int bme280_measure(bme280_t *s, axi_iic_t *d, int32_t *temp_cdeg, uint32_t *press_pa_q8, uint32_t *hum_q10);
#endif
