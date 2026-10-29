/* SSD1306 128x64 OLED, I2C. axi_iic 드라이버 위에서 동작.
 * 근거(모두 2차 자료 D3, SSD1306 데이터시트 원문은 열지 못함):
 *  - 명령 코드 값: Microchip ASF ssd1306.h
 *  - 주소 0x3C, 제어 바이트(명령 0x00 / 데이터 0x40), 초기화 순서와 0x8D,0x14: nnarain 글.
 *    주소 0x3C/0x3D(SA0)와 Co/DC# 비트 위치는 형제 칩 SH1106 데이터시트(D1)와 일치.
 * 미확인: COM 핀 설정값(0xDA 인자, 모듈마다 다름 — 호출자가 지정), 프리차지·VCOMH 값(쓰지 않음),
 *         I2C 최대 클럭, 모듈별 승압 회로 유무. 보드에서 실행해 보지 않았다. */
#ifndef SSD1306_H
#define SSD1306_H
#include <stdint.h>
#include "axi_iic.h"

#define OLED_W 128
#define OLED_H 64
#define OLED_PAGES (OLED_H / 8)
#define OLED_FB_BYTES (OLED_W * OLED_PAGES)

typedef struct {
    axi_iic_t *iic;
    uint8_t addr7;            /* 0x3C (SA0=L) 또는 0x3D (SA0=H) */
    uint8_t fb[OLED_FB_BYTES];/* [page*128 + x], 비트0 = 페이지 맨 위 행 */
} oled_t;

/* com_pins: 0xDA 다음 바이트. 128x64 모듈 대부분은 0x12 라고 알려졌으나 이 저장소에서는 미확인.
 * 모듈에서 화면이 한 줄씩 건너뛰거나 위아래가 섞이면 0x02 와 0x12 를 바꿔 본다. */
int oled_init(oled_t *o, axi_iic_t *iic, uint8_t addr7, uint8_t com_pins, int flip);
int oled_flush(oled_t *o);                 /* 프레임버퍼 전체를 전송 */
int oled_display(oled_t *o, int on);       /* 0xAE / 0xAF */
int oled_contrast(oled_t *o, uint8_t c);   /* 0x81 c */

void oled_clear(oled_t *o, int on);
void oled_pixel(oled_t *o, int x, int y, int on);       /* 범위 밖은 무시 */
void oled_hline(oled_t *o, int x, int y, int w, int on);
void oled_rect(oled_t *o, int x, int y, int w, int h, int fill, int on);
/* 3x5 숫자 글꼴(자체 제작). 문자는 '0'~'9', '-', '.', ':', ' ' 만. 반환: 다음 x */
int oled_text(oled_t *o, int x, int y, const char *s, int scale, int on);
#endif
