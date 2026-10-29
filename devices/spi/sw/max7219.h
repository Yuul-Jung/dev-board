/* MAX7219 8자리 7세그/8×8 LED 드라이버 (체인 최대 8개). axi_qspi 위에서 동작. LOAD 를 CS 로 쓴다(MAX7221 도 같은 방식).
 * 근거: Maxim/ADI MAX7219-MAX7221 데이터시트(웹 가져오기가 돌려준 본문 요약, D1 요약 경유) — Table 2 레지스터 지도, Table 3~10, 코드 B 폰트(Table 5),
 *   직렬 인터페이스·타이밍(p.3~6), 전원 투입 상태(p.7), 체인 연결(p.10~11). 상수는 spi/ref/max7219_facts.txt 와 tools/crosscheck_pdf.py 로 대조한다.
 * SPI: DIN 은 CLK 상승 에지에서 시프트, 16비트마다 LOAD 상승 에지로 래치, CLK 최대 10 MHz. CLK 휴지 레벨은 데이터시트 요약에서 확인 못 함 → 모드 0 을 쓰는 것은 설계 선택(상승 에지 샘플링 조건은 모드 0·3 모두 충족).
 * 전원 V+ 4.0~5.5 V, 입력 VIH 최소 3.5 V 이므로 FPGA 3.3 V 신호를 직접 연결하면 안 된다 → 레벨 변환기 가정(확인).
 * 자릿수 번호(DIG0~7)가 모듈에서 왼쪽/오른쪽 어느 쪽에 배선됐는지, 8×8 매트릭스의 행·열 방향은 모듈마다 달라 데이터시트에 없다 → 이 드라이버는 DIG0 을 오른쪽 끝 자릿수로 간주(통상 4자리 모듈, 설계안)하고 매트릭스는 DIGn 을 행 n 으로만 다룬다.
 * 보드에서 실행해 보지 않았다. */
#ifndef MAX7219_H
#define MAX7219_H
#include "axi_qspi.h"

/* 레지스터 주소 (Table 2, p.7). D15~D12 는 don't care */
#define MAX7219_REG_NOOP      0x00
#define MAX7219_REG_DIGIT0    0x01      /* DIGIT n = 0x01 + n (n = 0..7) */
#define MAX7219_REG_DECODE    0x09
#define MAX7219_REG_INTENSITY 0x0A
#define MAX7219_REG_SCANLIMIT 0x0B
#define MAX7219_REG_SHUTDOWN  0x0C      /* D0 = 0 셧다운, 1 정상 (Table 3) */
#define MAX7219_REG_TEST      0x0F      /* D0 = 1 전체 점등 (Table 10) */

/* 코드 B 폰트 (Table 5, p.8): D3~D0 코드. D7 = 소수점 */
#define MAX7219_CODEB_DASH    0x0A
#define MAX7219_CODEB_E       0x0B
#define MAX7219_CODEB_H       0x0C
#define MAX7219_CODEB_L       0x0D
#define MAX7219_CODEB_P       0x0E
#define MAX7219_CODEB_BLANK   0x0F
#define MAX7219_DP            0x80
/* 디코드 안 할 때 세그먼트 비트 (Table 6, p.8): D7 DP, D6 A, D5 B, D4 C, D3 D, D2 E, D1 F, D0 G */
#define MAX7219_SEG_A 0x40
#define MAX7219_SEG_B 0x20
#define MAX7219_SEG_C 0x10
#define MAX7219_SEG_D 0x08
#define MAX7219_SEG_E 0x04
#define MAX7219_SEG_F 0x02
#define MAX7219_SEG_G 0x01

#define MAX7219_MAX_CHIPS 8
#define MAX7219_ERR_RANGE (-70)   /* 값이 자릿수에 안 들어감 */
#define MAX7219_ERR_CHAR  (-71)   /* 코드 B 로 못 나타내는 문자 */
#define MAX7219_ERR_MODE  (-72)   /* 그 자릿수의 디코드 설정과 맞지 않는 호출(코드 B 자릿수에 raw 쓰기 또는 반대) */

typedef struct { qspi_t *s; unsigned cs; unsigned nchips; uint8_t decode; } max7219_t;   /* nchips 1..8, chip 0 = DIN 에 가장 가까운 칩. decode 는 init 이 기억(모든 칩 공통) */

int max7219_write(max7219_t *m, unsigned chip, uint8_t reg, uint8_t val);   /* 그 칩에만 쓰고 나머지에는 No-Op (p.10) */
int max7219_write_all(max7219_t *m, uint8_t reg, uint8_t val);              /* 모든 칩에 같은 값 */
/* 초기화: 시험 모드 끔 → 셧다운 → 스캔 한계·디코드·밝기 → 자릿수 지움 → 정상 동작. digits 1..8 은 모든 칩 공통(밝기 일치를 위해 스캔 한계를 같게, p.11).
 * decode_mask 는 자릿수별 코드 B 사용 비트(Table 4), intensity 0..15. 전원 투입 직후 레지스터 값은 자릿수 레지스터와 시험 레지스터가 명시돼 있지 않아 전부 직접 쓴다 */
int max7219_init(max7219_t *m, unsigned digits, uint8_t decode_mask, uint8_t intensity);
int max7219_shutdown(max7219_t *m, int on);                  /* on=1 이면 표시 끔(자릿수 데이터는 유지, p.6) */
int max7219_test(max7219_t *m, int on);
int max7219_set_intensity(max7219_t *m, uint8_t level);      /* 0..15 (MAX7219 듀티 1/32~31/32) */
int max7219_clear(max7219_t *m, unsigned chip, unsigned digits);   /* 자릿수 0..digits-1 을 끔: 디코드 자릿수는 코드 B 공백, 아니면 0 */
int max7219_set_raw(max7219_t *m, unsigned chip, unsigned digit, uint8_t segs);   /* digit 0..7. 디코드 안 하는 자릿수만(아니면 MAX7219_ERR_MODE) */
int max7219_set_code(max7219_t *m, unsigned chip, unsigned digit, uint8_t code, int dp);   /* 코드 B. 디코드 자릿수만(아니면 MAX7219_ERR_MODE) */
/* 4자리 등 숫자 표시: digit 0 = 가장 낮은 자리. 부호는 최상위 쪽에 '-' . dp_digit 은 소수점을 켤 자릿수(-1 이면 없음) */
int max7219_print_int(max7219_t *m, unsigned chip, unsigned ndigits, int32_t value, int dp_digit);
/* 문자열(왼쪽부터): 0-9 - E H L P 공백(대소문자 무관), '.' 은 앞 문자의 소수점. 다른 문자는 MAX7219_ERR_CHAR */
int max7219_print_str(max7219_t *m, unsigned chip, unsigned ndigits, const char *s);
/* 8×8 매트릭스: DIGn 을 행 n 으로. 디코드 마스크가 0 일 때만 */
int max7219_matrix(max7219_t *m, unsigned chip, const uint8_t rows[8]);
#endif
