/* SSD1306 128x64 OLED, I2C. axi_iic 드라이버 위에서 동작. 교재 23장(도형·이미지) 대응으로 선·원·삼각형·비트맵·5x7 글꼴을 추가했다.
 * 근거 (표: i2c/ref/ssd1306_facts.txt, 문서: i2c/ssd1306.md):
 *  - 명령 코드·인자·리셋값·제어 바이트·슬레이브 주소: SSD1306(B) 데이터시트 Rev 1.1 사본(store.comet.bg 의 PDF)을 웹 가져오기가 요약한 본문 → D1(요약 경유).
 *    요약은 이 사본이 SSD1306B 라고 밝히고 "원본 SSD1306 과 다를 수 있다"고 경고했다.
 *  - 명령 코드 값의 독립 확인: Microchip ASF ssd1306.h 의 매크로 값(요약 경유 D3) — 이번에 33개 매크로를 다시 열어 모두 일치.
 *  - 초기화 순서와 0x8D,0x14: nnarain 글(D3). 데이터시트 요약도 충전 펌프 켜는 순서를 "8Dh → 14h → AFh" 로 적어 일치.
 *  - 그리기(선·원·삼각형·비트맵)는 화면 프레임버퍼 위의 순수 계산이라 칩 근거가 필요 없다. 5x7 글꼴 표는 기억에 의존한 고전적인 글꼴(출처 문서 없음)이라
 *    렌더링으로 모양을 눈으로 확인했고 시험은 구조(7행, 중복 없음 등)와 몇 글자만 본다.
 * 미확인: I2C 최대 SCL 클럭(요약에 없음), 모듈의 승압 회로 유무, 0xD9/0xDB 는 쓰지 않는다(데이터시트 리셋값: 프리차지 2·2 DCLK, VCOMH 0.77×VCC),
 *   0x40(시작 줄)의 인자 범위, 모듈이 위아래/좌우를 어떻게 달았는지(flip 은 모듈에 따라 다르다).
 * 보드에서 실행해 보지 않았다(가짜 axi_iic_write 로 전송 바이트와 프레임버퍼만 시험). */
#ifndef SSD1306_H
#define SSD1306_H
#include <stdint.h>
#include "axi_iic.h"

#define OLED_W 128
#define OLED_H 64
#define OLED_PAGES (OLED_H / 8)
#define OLED_FB_BYTES (OLED_W * OLED_PAGES)

/* 사실표(i2c/ref/ssd1306_facts.txt)와 대조되는 상수 */
#define SSD1306_ADDR_SA0_L        0x3C   /* 0111100 */
#define SSD1306_ADDR_SA0_H        0x3D   /* 0111101 */
#define SSD1306_CTRL_CMD          0x00   /* Co=0, D/C#=0: 이후 바이트는 모두 명령 */
#define SSD1306_CTRL_DATA         0x40   /* Co=0, D/C#=1: 이후 바이트는 모두 데이터 */
#define SSD1306_CMD_MEM_MODE      0x20   /* 인자 00 = 가로 주소 지정, 01 = 세로, 10 = 페이지(리셋 값) */
#define SSD1306_MEM_HORIZONTAL    0x00
#define SSD1306_CMD_COLUMN_ADDR   0x21   /* 인자 시작, 끝 (0~127) */
#define SSD1306_CMD_PAGE_ADDR     0x22   /* 인자 시작, 끝 (0~7) */
#define SSD1306_CMD_START_LINE    0x40
#define SSD1306_CMD_CONTRAST      0x81   /* 인자 01~FF, 리셋 7F */
#define SSD1306_CMD_CHARGE_PUMP   0x8D   /* 인자 14 = 켬(내부 7.5 V) */
#define SSD1306_CHARGE_PUMP_ON    0x14
#define SSD1306_CMD_SEG_REMAP_0   0xA0   /* 열 0 = SEG0 (리셋 값) */
#define SSD1306_CMD_SEG_REMAP_127 0xA1   /* 열 127 = SEG0 */
#define SSD1306_CMD_RAM_DISPLAY   0xA4   /* 출력이 RAM 내용을 따름 (리셋 값) */
#define SSD1306_CMD_NORMAL        0xA6   /* 정상 표시 (리셋 값) */
#define SSD1306_CMD_INVERSE       0xA7   /* 반전 표시 */
#define SSD1306_CMD_MUX           0xA8   /* 인자 N-1 (15~63), 리셋 3F = 64 MUX */
#define SSD1306_CMD_DISPLAY_OFF   0xAE
#define SSD1306_CMD_DISPLAY_ON    0xAF
#define SSD1306_CMD_COM_SCAN_NORMAL 0xC0 /* 리셋 값 */
#define SSD1306_CMD_COM_SCAN_REMAP  0xC8
#define SSD1306_CMD_DISPLAY_OFFSET  0xD3 /* 인자 0~63, 리셋 0 */
#define SSD1306_CMD_CLOCK         0xD5   /* 인자: 비트 7~4 발진 주파수, 비트 3~0 분주비-1. 리셋 0x80 */
#define SSD1306_CMD_COM_PINS      0xDA   /* 인자 02 = 순차, 12 = 번갈아(리셋 값) */
#define SSD1306_COM_SEQUENTIAL    0x02
#define SSD1306_COM_ALTERNATIVE   0x12
#define SSD1306_RESET_CONTRAST    0x7F
#define SSD1306_RESET_CLOCK       0x80

#define OLED_TRANSPARENT (-1)     /* 비트맵·글자의 bg 인자: 배경 픽셀을 건드리지 않음 */
#define OLED_COORD_LIM   4096     /* 선·원·삼각형 좌표 한계. 넘으면 그리지 않는다(반복 횟수 한도) */

typedef struct {
    axi_iic_t *iic;
    uint8_t addr7;            /* 0x3C (SA0=L) 또는 0x3D (SA0=H) */
    uint8_t fb[OLED_FB_BYTES];/* [page*128 + x], 비트0 = 페이지 맨 위 행 */
} oled_t;

/* com_pins: 0xDA 다음 바이트. 데이터시트 요약: 02 = 순차, 12 = 번갈아(리셋 값). 128x64 모듈 대부분이 12 라는 것은 2차 자료.
 * 모듈에서 화면이 한 줄씩 건너뛰거나 위아래가 섞이면 0x02 와 0x12 를 바꿔 본다. */
int oled_init(oled_t *o, axi_iic_t *iic, uint8_t addr7, uint8_t com_pins, int flip);
int oled_flush(oled_t *o);                 /* 프레임버퍼 전체를 전송 */
int oled_display(oled_t *o, int on);       /* 0xAE / 0xAF */
int oled_contrast(oled_t *o, uint8_t c);   /* 0x81 c */
int oled_invert(oled_t *o, int on);        /* 0xA7(반전) / 0xA6(정상). 프레임버퍼는 그대로 */

void oled_clear(oled_t *o, int on);
void oled_pixel(oled_t *o, int x, int y, int on);       /* 범위 밖은 무시 */
void oled_hline(oled_t *o, int x, int y, int w, int on);
void oled_rect(oled_t *o, int x, int y, int w, int h, int fill, int on);
/* 3x5 숫자 글꼴(자체 제작). 문자는 '0'~'9', '-', '.', ':', ' ' 만. 반환: 다음 x */
int oled_text(oled_t *o, int x, int y, const char *s, int scale, int on);

/* ---- 도형 (모두 화면 밖은 잘라 낸다. 좌표 절댓값이 OLED_COORD_LIM 을 넘으면 아무것도 그리지 않는다) ---- */
void oled_line(oled_t *o, int x0, int y0, int x1, int y1, int on);        /* Bresenham, 양끝 포함 */
void oled_circle(oled_t *o, int cx, int cy, int r, int on);               /* 중점 원 알고리즘 외곽선. r < 0 이면 아무것도 안 함, r = 0 이면 점 */
void oled_fill_circle(oled_t *o, int cx, int cy, int r, int on);          /* 채운 원: dx²+dy² ≤ r²+r 인 점(외곽선을 포함) */
void oled_triangle(oled_t *o, int x0, int y0, int x1, int y1, int x2, int y2, int fill, int on);   /* 채움은 각 행에서 세 변 픽셀의 최소~최대 x 구간 */
/* 비트맵: 행 우선, 한 행은 (w+7)/8 바이트, 각 바이트의 최상위 비트가 왼쪽(Adafruit drawBitmap 과 같은 형식으로 알려짐 — 책이 쓰는 형식은 미확인).
 * 비트 1 은 fg, 비트 0 은 bg (bg = OLED_TRANSPARENT 이면 그대로). fg, bg 는 0 또는 1 */
void oled_bitmap(oled_t *o, int x, int y, const uint8_t *bmp, int w, int h, int fg, int bg);

/* ---- 5x7 글꼴 (ASCII 0x20~0x7E, 글자 칸은 6x8: 오른쪽 한 열과 아래 한 행은 간격) ---- */
#define OLED_FONT_W 5
#define OLED_FONT_H 7
#define OLED_CELL_W 6
#define OLED_CELL_H 8
/* scale 1~8 (범위 밖은 1~8 로 맞춤). '\n' 은 x 를 처음 값으로, y 를 8*scale 만큼 내림. 범위 밖 문자는 '?'. 반환: 마지막 글자 다음 x */
int oled_print(oled_t *o, int x, int y, const char *s, int scale, int fg, int bg);
int oled_text_width(const char *s, int scale);     /* 첫 줄의 폭(마지막 간격 열 제외). 빈 문자열은 0 */
/* 글꼴 열 데이터: ch 의 5열(비트0 = 맨 위 행). 범위 밖은 '?' */
const uint8_t *oled_font5x7(int ch);
#endif
