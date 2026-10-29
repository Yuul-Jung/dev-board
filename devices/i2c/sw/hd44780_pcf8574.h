/* HD44780 를 PCF8574 I2C 백팩 위에서 쓰기 위한 접착 코드. PCF8574 드라이버(io_expander.h)로 포트 바이트를 쓰고, 마이크로초 지연은 호출자가 준 함수를 쓴다.
 * 핀 배선(HD44780_PINS_PCF8574_COMMON)은 모듈 관례이며 데이터시트 근거가 없다 → 모듈의 회로를 확인하고 필요하면 pins 를 바꿀 것(설계안).
 * pcf8574_init 은 모든 비트를 1 로 쓰므로 그 순간 E·RS·RW 도 High 가 된다. 이어지는 hd44780_i2c_init 의 첫 쓰기에서 곧바로 낮아지지만, 그 사이 LCD 는 의미 없는 읽기 사이클을 볼 수 있다(전원 대기 중이라 영향은 없을 것으로 보이나 확인하지 않음).
 * 보드에서 실행해 보지 않았다. */
#ifndef HD44780_PCF8574_H
#define HD44780_PCF8574_H
#include "hd44780.h"
#include "io_expander.h"

typedef struct { hd44780_t lcd; pcf8574_t *pcf; void (*delay_us)(uint32_t us); } hd44780_i2c_t;
/* d->lcd 를 hd44780_* 함수에 넘겨 쓴다. pcf 는 pcf8574_init 을 마친 것 */
int hd44780_i2c_init(hd44780_i2c_t *d, pcf8574_t *pcf, void (*delay_us)(uint32_t us), hd44780_pins_t pins, unsigned cols, unsigned rows);
#endif
