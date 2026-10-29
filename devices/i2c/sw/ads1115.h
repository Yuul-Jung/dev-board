/* ADS1115 16비트 I2C ADC. axi_iic 드라이버 위에서 동작.
 * 근거: TI SBAS444 데이터시트 웹 문서 (D1) — 레지스터 포인터, Config 비트 필드·리셋값 8583h,
 *   주소표(ADDR→GND/VDD/SDA/SCL = 0x48/49/4A/4B), 바이트 순서(MSB 먼저), LSB 표, 출력 코드(2의 보수),
 *   변환 시간 = 1/DR, VDD 2.0~5.5 V, 입력은 VDD+0.3 V 초과 금지, SCL 400 kHz(고속 모드 3.4 MHz).
 * 비교기/ALERT 기능은 쓰지 않는다(COMP_QUE=11 로 비활성). 보드에서 실행해 보지 않았다. */
#ifndef ADS1115_H
#define ADS1115_H
#include <stdint.h>
#include "axi_iic.h"

/* 포인터 레지스터 */
#define ADS_REG_CONV   0x00
#define ADS_REG_CONFIG 0x01
#define ADS_REG_LO     0x02
#define ADS_REG_HI     0x03

/* 입력 멀티플렉서 (Config[14:12]) */
enum { ADS_MUX_0_1 = 0, ADS_MUX_0_3, ADS_MUX_1_3, ADS_MUX_2_3,
       ADS_MUX_0_G, ADS_MUX_1_G, ADS_MUX_2_G, ADS_MUX_3_G };
/* PGA (Config[11:9]): 만점 범위 */
enum { ADS_PGA_6144 = 0, ADS_PGA_4096, ADS_PGA_2048, ADS_PGA_1024, ADS_PGA_0512, ADS_PGA_0256 };
/* 데이터 속도 (Config[7:5]) */
enum { ADS_DR_8 = 0, ADS_DR_16, ADS_DR_32, ADS_DR_64, ADS_DR_128, ADS_DR_250, ADS_DR_475, ADS_DR_860 };

typedef struct { axi_iic_t *iic; uint8_t addr7; } ads1115_t;

int ads1115_init(ads1115_t *a, axi_iic_t *iic, uint8_t addr7);   /* 0x48~0x4B, Config 읽기로 응답 확인 */
/* 단발 변환: Config 쓰기(OS=1) → OS 비트가 1 이 될 때까지 폴링 → 변환값 읽기 */
int ads1115_read_single(ads1115_t *a, int mux, int pga, int dr, int16_t *raw);
/* 연속 변환 시작/읽기 */
int ads1115_start_continuous(ads1115_t *a, int mux, int pga, int dr);
int ads1115_read_conversion(ads1115_t *a, int16_t *raw);
int ads1115_read_reg(ads1115_t *a, uint8_t reg, uint16_t *v);
int ads1115_write_reg(ads1115_t *a, uint8_t reg, uint16_t v);

/* 원시값 → µV. 단일 종단(AINx-GND)은 음수가 나오지 않는 것이 정상. 만점 ±범위 / 32768 */
int32_t ads1115_raw_to_uv(int16_t raw, int pga);
#endif
