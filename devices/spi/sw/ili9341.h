/* ILI9341 드라이버 (ili9341_axil 전용 RTL 위). 레지스터 맵은 rtl/README.md 와 같다.
 * 확인된 것: 명령 코드(01h 11h 29h 2Ah 2Bh 2Ch 36h 3Ah), 4선 SPI 핀, 해상도 240x320.
 * 미확인(데이터시트 본문을 열지 못함): 초기화 순서, COLMOD/MADCTL 파라미터 값, 리셋 폭과 대기 시간, SCL 최소 주기.
 * 그래서 이 드라이버는 초기화 내용을 코드에 넣지 않고, 호출자가 데이터시트 값으로 채운 표(lcd_step_t)를 실행한다.
 * 호스트 모의 테스트만 수행, 보드·시뮬레이션 미검증. */
#ifndef ILI9341_H
#define ILI9341_H
#include <stdint.h>
#include <stddef.h>

#ifndef LCD_RD
#define LCD_RD(base, off)    (*(volatile uint32_t *)((uintptr_t)(base) + (off)))
#define LCD_WR(base, off, v) (*(volatile uint32_t *)((uintptr_t)(base) + (off)) = (v))
#endif
#define LCD_CTRL      0x00   /* [0] RESX 레벨(1=해제) [1] CSX 활성 [2] 백라이트 */
#define LCD_DIV       0x04   /* SCL = clk / (2 (DIV+1)) */
#define LCD_TX        0x08   /* 쓰기 [7:0] 바이트 [8] D/CX ; 읽기 [0] busy [1] overrun(읽으면 지워짐) */
#define LCD_FILLCOLOR 0x0C
#define LCD_FILLCNT   0x10
#define LCD_CTRL_RESX (1u << 0)
#define LCD_CTRL_CS   (1u << 1)
#define LCD_CTRL_BL   (1u << 2)
#define LCD_ST_BUSY    1u
#define LCD_ST_OVERRUN 2u
#define LCD_FILL_MAX   0xFFFFFu   /* 한 번에 최대 2^20-1 픽셀 */

#define LCD_OK 0
#define LCD_ERR_TIMEOUT -1
#define LCD_ERR_OVERRUN -2
#define LCD_ERR_ARGS -3

#define LCD_W 240
#define LCD_H 320

/* 초기화 표의 한 단계: 명령, 파라미터 n개, 이후 대기(ms). 끝은 cmd = 0xFF && n = 0xFF 로 표시 */
typedef struct { uint8_t cmd; uint8_t n; uint8_t p[16]; uint16_t delay_ms; } lcd_step_t;
#define LCD_STEP_END { 0xFF, 0xFF, {0}, 0 }

typedef struct { uintptr_t base; unsigned timeout; } lcd_t;

void lcd_delay_ms(unsigned ms);       /* 호출자가 구현 */

void lcd_attach(lcd_t *l, uintptr_t base, uint32_t div);                 /* DIV 설정, CS 활성, RESX 해제 상태 유지 */
void lcd_hw_reset(lcd_t *l, unsigned low_ms, unsigned after_ms);         /* RESX Low low_ms → High → after_ms 대기. 값은 데이터시트 기준으로 호출자가 정함 */
void lcd_backlight(lcd_t *l, int on);
int  lcd_run_table(lcd_t *l, const lcd_step_t *steps);
int  lcd_cmd(lcd_t *l, uint8_t c);
int  lcd_data(lcd_t *l, uint8_t d);
int  lcd_set_window(lcd_t *l, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);   /* 2Ah, 2Bh, 2Ch */
int  lcd_fill_rect(lcd_t *l, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t rgb565);
int  lcd_push_pixels(lcd_t *l, const uint16_t *rgb565, size_t n);        /* 2Ch 이후 상위 바이트 먼저(16비트 색 가정) */
#endif
