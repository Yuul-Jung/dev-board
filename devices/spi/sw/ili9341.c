/* ILI9341 드라이버 스케치 (설계안, 미실행). 주소는 예시.
 * 초기화 순서와 대기 시간은 데이터시트 확인 전이므로 이 파일에 넣지 않았다.
 * 사용할 수 있는 명령 코드(확인): 01h SWRESET, 11h SLPOUT, 29h DISPON,
 * 2Ah 열 주소, 2Bh 페이지 주소, 2Ch 메모리 쓰기, 36h MADCTL, 3Ah COLMOD. */
#include <stdint.h>
#define LCD_BASE   0x44A10000u
#define REG(o)     (*(volatile uint32_t *)(LCD_BASE + (o)))
#define CTRL 0x00
#define DIV  0x04
#define TX   0x08   /* 쓰기: 송신, 읽기: STATUS */
#define FILLCOLOR 0x0C
#define FILLCNT   0x10
#define ST_BUSY 1u
#define ST_OVERRUN 2u

static void lcd_wait(void){ while (REG(TX) & ST_BUSY) ; }
void lcd_cmd(uint8_t c){ lcd_wait(); REG(TX) = c; }
void lcd_data(uint8_t d){ lcd_wait(); REG(TX) = 0x100u | d; }
void lcd_set_window(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1){
    lcd_cmd(0x2A); lcd_data(x0>>8); lcd_data(x0); lcd_data(x1>>8); lcd_data(x1);
    lcd_cmd(0x2B); lcd_data(y0>>8); lcd_data(y0); lcd_data(y1>>8); lcd_data(y1);
    lcd_cmd(0x2C);
}
void lcd_fill_rect(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t rgb565){
    lcd_set_window(x0,y0,x1,y1);
    lcd_wait();
    REG(FILLCOLOR) = rgb565;
    REG(FILLCNT) = (uint32_t)(x1-x0+1) * (y1-y0+1);
    lcd_wait();
}
