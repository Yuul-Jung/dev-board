/* SPI 센서: MCP3008(10비트 ADC), ADXL345(가속도). axi_qspi 위에서 동작.
 * 근거: MCP3008 = Microchip 데이터시트 사본(D1), ADXL345 = Analog Devices AN-1077 Quick Start Guide(D1, 응용 노트).
 * ADXL345 데이터시트 본문은 열지 못했다 — 쓰기 명령의 R/W 비트 값(0), BW_RATE, DATA_FORMAT 의 개별 비트는 미확인.
 * 보드에서 실행해 보지 않았다. */
#ifndef SPI_SENSORS_H
#define SPI_SENSORS_H
#include "axi_qspi.h"

/* MCP3008: 모드 0,0 또는 1,1, CS 는 변환마다 High 로 올려야 함, VDD 2.7~5.5 V,
 * SCK 최대 3.6 MHz(5 V) / 1.35 MHz(2.7 V). LSB = VREF/1024, VREF 0.25 V ~ VDD.
 * sgl=1: 단일 종단 ch 0~7. sgl=0: 차동, ch 0~7 이 쌍을 고른다
 *   (0: CH0+/CH1-, 1: CH0-/CH1+, 2: CH2+/CH3-, 3: CH2-/CH3+, 4: CH4+/CH5-, 5: CH4-/CH5+, 6: CH6+/CH7-, 7: CH6-/CH7+). */
int mcp3008_read(qspi_t *s, unsigned cs, int sgl, int ch, uint16_t *code);
static inline uint32_t mcp3008_to_mv(uint16_t code, uint32_t vref_mv) { return (uint32_t)(((uint64_t)code * vref_mv) / 1024u); }

/* ADXL345: 모드 3, SCK ≤ 5 MHz, 2.0~3.6 V. 주소 바이트 = R/W(bit7) | MB(bit6) | 주소(bit5:0).
 * 읽기는 bit7=1 (AN-1077 의 데이터 읽기 명령 0xF2 = 0x80|0x40|0x32 에서 확인). */
#define ADXL_REG_DEVID       0x00
#define ADXL_REG_POWER_CTL   0x2D
#define ADXL_REG_DATA_FORMAT 0x31
#define ADXL_REG_DATAX0      0x32
#define ADXL_DEVID_VALUE     0xE5
#define ADXL_ERR_DEVID       (-30)

int adxl345_read_reg(qspi_t *s, unsigned cs, uint8_t reg, uint8_t *v);
int adxl345_write_reg(qspi_t *s, unsigned cs, uint8_t reg, uint8_t v);
/* DEVID 확인 → DATA_FORMAT=0x0B(±16 g, 13비트), POWER_CTL=0x08(측정 시작). AN-1077 의 예. 전원 투입 후 1.1 ms 대기는 호출자가 */
int adxl345_init(qspi_t *s, unsigned cs);
int adxl345_read_xyz(qspi_t *s, unsigned cs, int16_t xyz[3]);   /* 6바이트 연속 읽기(MB=1) */
static inline int32_t adxl345_to_mg(int16_t raw) { return ((int32_t)raw * 39) / 10; }   /* 13비트 모드 약 3.9 mg/LSB */

/* MCP4921 12비트 DAC (Microchip 데이터시트 사본, D1). 모드 0,0 / 1,1, SCK ≤ 20 MHz, VDD 2.7~5.5 V.
 * 16비트 명령(MSB 먼저): bit15 A/B(MCP4921 은 DAC 하나, 0 을 씀), bit14 BUF(1 = VREF 버퍼 사용),
 *   bit13 GA(**1 = 이득 1배, 0 = 이득 2배**), bit12 SHDN(0 = 셧다운·출력 고임피던스; 1 = 활성은 이 반대로 해석한 것),
 *   bit11:0 데이터. 데이터는 CS 상승 때 입력 레지스터에 래치되고, LDAC 를 Low 로 묶으면 같은 순간 출력이 갱신된다.
 *   VOUT = VREF × G × D / 4096. 전원 투입 직후 출력은 고임피던스이고 첫 유효 쓰기 뒤에 나온다. */
int mcp4921_write(qspi_t *s, unsigned cs, uint16_t code, int gain2x, int buffered);   /* code 0~4095 */
int mcp4921_shutdown(qspi_t *s, unsigned cs);
static inline uint32_t mcp4921_to_mv(uint16_t code, uint32_t vref_mv, int gain2x)
{ return (uint32_t)(((uint64_t)code * vref_mv * (gain2x ? 2u : 1u)) / 4096u); }
#endif
