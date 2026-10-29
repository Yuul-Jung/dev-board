/* HD44780 문자 LCD(16×2, 20×4 등) 4비트 드라이버 — 버스 방식에 의존하지 않는다: 포트 한 바이트를 쓰는 함수와 마이크로초 지연 함수만 받는다.
 * PCF8574 I2C 백팩은 hd44780_pcf8574.h 의 접착 코드로, 직접 GPIO 는 같은 함수를 GPIO 쓰기로 채우면 된다. 바쁨 플래그(BF)는 읽지 않고 고정 대기만 쓴다(RW 를 항상 0 으로 둠).
 * 근거: Hitachi HD44780U 데이터시트 (soldered.com 사본을 웹 가져오기가 요약한 본문, D1 요약 경유 — PDF 원문은 직접 열지 못함) + Wikipedia 의 명령 표(D3, 명령 코드와 시간이 서로 같음).
 *   명령 코드(Table 6), 실행 시간(fOSC 270 kHz 에서 지우기/홈 1.52 ms, 나머지 37 µs; fOSC 190~350 kHz → 최악 약 2.16 ms / 52.6 µs 는 계산),
 *   4비트 초기화(Figure 24: Vcc 4.5 V 도달 후 >15 ms, 2.7 V 도달 후 >40 ms, 0011 세 번 사이에 >4.1 ms 와 >100 µs, 그 뒤 0010 으로 4비트 전환, 이어서 기능 설정·표시 끔·지우기·입력 모드),
 *   버스 타이밍(3 V 쪽 최악: tcycE 1000 ns, PWEH 450 ns, tAS 60 ns, tDSW 195 ns, tH 10 ns, tAH 20 ns; 5 V 쪽은 500/230/40/80/10/10 ns), DDRAM(2행: 첫 행 00~27H, 둘째 행 40~67H, 1행: 00~4FH), CGRAM 64바이트·5×8 글자 8개.
 *   상수는 i2c/ref/hd44780_facts.txt 와 tools/crosscheck_pdf.py 로 대조한다.
 * 설계안/미확인: ① 20×4 의 3·4 행 시작 주소(0x14·0x54 같은 값)는 데이터시트에 없고 모듈 관례(행 3 = cols, 행 4 = 0x40 + cols) — row_addr 로 바꿀 수 있다.
 *   ② 글자 ROM 표(A00/A02)와 ASCII 의 차이(예: 0x5C 가 엔 기호, 0x7E 가 화살표인 모듈 등)는 확인하지 않았고 이 드라이버는 바이트를 그대로 보낸다. ③ 3.3 V 구동 모듈은 오실레이터가 느려 대기 시간이 길어진다는 자료가 있으나 값은 미확인(대기 상수에 여유를 둠).
 *   ④ 지연 함수의 분해능은 1 µs 로 가정(E 펄스·셋업을 1 µs 단위로 맞춤).
 * 보드·실제 모듈로 실행해 보지 않았다(호스트 모델 시험만). */
#ifndef HD44780_H
#define HD44780_H
#include <stdint.h>

/* 명령 (DB7~DB0). 모든 명령은 RS = 0, R/W = 0 */
#define HD44780_CMD_CLEAR     0x01
#define HD44780_CMD_HOME      0x02
#define HD44780_CMD_ENTRY     0x04      /* | ID(0x02) | S(0x01) */
#define HD44780_ENTRY_ID      0x02
#define HD44780_ENTRY_S       0x01
#define HD44780_CMD_DISPLAY   0x08      /* | D(0x04) | C(0x02) | B(0x01) */
#define HD44780_DISP_D        0x04
#define HD44780_DISP_C        0x02
#define HD44780_DISP_B        0x01
#define HD44780_CMD_SHIFT     0x10      /* | S/C(0x08) | R/L(0x04) */
#define HD44780_SHIFT_SC      0x08
#define HD44780_SHIFT_RL      0x04
#define HD44780_CMD_FUNCTION  0x20      /* | DL(0x10) | N(0x08) | F(0x04) */
#define HD44780_FN_DL         0x10
#define HD44780_FN_N          0x08
#define HD44780_FN_F          0x04
#define HD44780_CMD_SETCGRAM  0x40      /* | 6비트 주소 */
#define HD44780_CMD_SETDDRAM  0x80      /* | 7비트 주소 */

/* 고정 대기 (µs). 데이터시트의 최소값보다 크게 잡은 값: 초기화 전원 대기(>40 ms 2.7 V 기준), >4.1 ms, >100 µs, 일반 명령(최악 52.6 µs), 지우기·홈(최악 2.16 ms) */
#define HD44780_WAIT_POWERON_US 50000
#define HD44780_WAIT_INIT1_US   5000
#define HD44780_WAIT_INIT2_US   200
#define HD44780_WAIT_CMD_US     60
#define HD44780_WAIT_CLEAR_US   2500
#define HD44780_PULSE_US        1       /* E 높음/낮음, 데이터 셋업·홀드 각각 이만큼 */

#define HD44780_OK         0
#define HD44780_ERR_ARGS   (-1)
#define HD44780_ERR_RANGE  (-80)        /* 행 끝을 넘음(보이는 부분까지만 씀) 또는 위치 범위 밖 */
#define HD44780_ERR_NOBL   (-81)        /* 백라이트 핀이 연결되지 않음 */
#define HD44780_NC         0xFF         /* 핀 미연결 표시(rw, bl 에만) */

typedef int  (*hd44780_write_fn)(void *ctx, uint8_t port);        /* 포트 한 바이트 쓰기. 음수 반환은 오류로 전파 */
typedef void (*hd44780_delay_fn)(void *ctx, uint32_t us);
/* 포트 바이트 안의 비트 위치(0~7) */
typedef struct { uint8_t rs, rw, en, bl, d4, d5, d6, d7; } hd44780_pins_t;
/* PCF8574 백팩에서 흔한 배선(P0 RS, P1 RW, P2 E, P3 백라이트, P4~P7 = D4~D7) — 모듈 관례이며 데이터시트 근거가 아니다(설계안). 모듈마다 다르다. */
#define HD44780_PINS_PCF8574_COMMON { 0, 1, 2, 3, 4, 5, 6, 7 }

typedef struct {
    hd44780_write_fn wr; hd44780_delay_fn delay; void *ctx; hd44780_pins_t pins;
    uint8_t cols, rows;             /* cols 1..40, rows 1..4 */
    uint8_t row_addr[4];            /* 각 행 시작 DDRAM 주소(초기값은 관례, 바꿔도 됨) */
    uint8_t ctrl;                   /* 표시 제어 비트 D|C|B */
    uint8_t bl;                     /* 백라이트 켜짐 */
    uint8_t col, row;               /* 소프트웨어가 추적하는 커서 위치 */
} hd44780_t;

/* 초기화(약 60 ms 걸림): 전원 대기 → 0011 ×3 → 0010 → 기능 설정(rows==1 이면 1행, 아니면 2행 모드·5×8) → 표시 끔 → 지우기 → 입력 모드(주소 증가, 이동 없음) → 표시 켬(커서·깜박임 끔). 백라이트는 켠 상태로 시작 */
int hd44780_init(hd44780_t *h, hd44780_write_fn wr, hd44780_delay_fn delay, void *ctx, hd44780_pins_t pins, unsigned cols, unsigned rows);
int hd44780_clear(hd44780_t *h);
int hd44780_home(hd44780_t *h);
int hd44780_set_cursor(hd44780_t *h, unsigned col, unsigned row);
int hd44780_putc(hd44780_t *h, uint8_t ch);                 /* 행 끝이면 HD44780_ERR_RANGE (쓰지 않음) */
int hd44780_print(hd44780_t *h, const char *s);             /* 행 끝까지만 쓰고 넘치면 HD44780_ERR_RANGE */
int hd44780_print_int(hd44780_t *h, int32_t v);
int hd44780_display(hd44780_t *h, int on);                  /* 표시 켬/끔(내용 유지) */
int hd44780_cursor(hd44780_t *h, int on);
int hd44780_blink(hd44780_t *h, int on);
int hd44780_backlight(hd44780_t *h, int on);
int hd44780_shift_display(hd44780_t *h, int right);          /* 표시 전체를 한 칸 이동 (내용은 그대로) */
/* 사용자 글자: slot 0..7(5×8). rows8[i] 의 하위 5비트가 i 번째 줄. 쓰고 나서 DDRAM 주소를 원래 커서로 되돌린다. 글자는 코드 slot(0..7)으로 출력 */
int hd44780_create_char(hd44780_t *h, unsigned slot, const uint8_t rows8[8]);
#endif
