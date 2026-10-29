/* DS1307 실시간 시계 + 24xx256 계열 I2C EEPROM(2바이트 주소). axi_iic 위에서 동작.
 * 근거(D1): DS1307 = Maxim 데이터시트 사본 2종, 24xx256 = Microchip DS21203 사본.
 * 중요: DS1307 은 VCC 4.5~5.5 V, SCL 최대 100 kHz 이고 입력 High 최소 2.2 V — 3.3 V 전용 버스에는
 *   레벨 변환이나 다른 부품(DS3231, clock.md)이 필요하다. 보드에서 실행해 보지 않았다. */
#ifndef RTC_EEPROM_H
#define RTC_EEPROM_H
#include <stdint.h>
#include "axi_iic.h"

#define DS_ERR_RANGE (-20)     /* 읽은 값이 BCD/범위 규칙에 맞지 않음(전원 인가 직후 등) 또는 쓸 값이 범위 밖 */

typedef struct {
    uint8_t sec, min, hour;    /* hour 는 항상 24시간제로 노출 */
    uint8_t wday;              /* 1~7, 요일 대응은 데이터시트에 없음(사용자 정의) */
    uint8_t date, month;
    uint8_t year;              /* 0~99 */
} ds1307_time_t;

#define DS1307_ADDR 0x68
#define DS1307_SQW_1HZ   0
#define DS1307_SQW_4KHZ  1     /* 4.096 kHz */
#define DS1307_SQW_8KHZ  2     /* 8.192 kHz */
#define DS1307_SQW_32KHZ 3     /* 32.768 kHz */
#define DS1307_SQW_OFF_LOW  4  /* 사각파 끔, OUT=0 */
#define DS1307_SQW_OFF_HIGH 5  /* 사각파 끔, OUT=1 */

int ds1307_read(axi_iic_t *d, ds1307_time_t *t, int *halted);   /* halted: CH 비트(1 = 발진 정지) */
int ds1307_set(axi_iic_t *d, const ds1307_time_t *t);           /* CH=0 으로 써서 발진 시작, 24시간제 */
int ds1307_set_sqw(axi_iic_t *d, int mode);
int ds1307_ram_write(axi_iic_t *d, unsigned off, const uint8_t *buf, unsigned len);   /* off 0..55 (08h~3Fh) */
int ds1307_ram_read(axi_iic_t *d, unsigned off, uint8_t *buf, unsigned len);

typedef struct { axi_iic_t *iic; uint8_t addr7; uint32_t size; uint16_t page; } eeprom24_t;
/* addr7: 0x50~0x57. size: 바이트 수(24xx256 = 32768), page: 쓰기 페이지(24xx256 = 64, 2의 거듭제곱 ≤128) */
int eeprom24_init(eeprom24_t *e, axi_iic_t *iic, uint8_t addr7, uint32_t size, uint16_t page);
int eeprom24_write(eeprom24_t *e, uint32_t addr, const uint8_t *buf, uint32_t len);   /* 페이지 경계로 쪼개고 ACK 폴링 */
int eeprom24_read(eeprom24_t *e, uint32_t addr, uint8_t *buf, uint32_t len);
#endif
