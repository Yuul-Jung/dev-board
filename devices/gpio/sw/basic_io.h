/* 기본 입출력 예제 묶음 (교재 6·7·9·10장 대응): 비차단 타이머(millis 식), 버튼 디바운스·누른 횟수, RGB LED(PWM), 자유 실행 시계.
 * 근거: 시계와 RGB 는 PG079 AXI Timer v2.0(PDF 텍스트를 직접 확인): TCR0 0x08 = 현재 카운터, 올림 카운트 UDT=0 일 때 한 바퀴 = (MAX_COUNT - TLR + 2) 클럭,
 *   ARHT=0 이면 넘침(전부 1 → 0)에서 멈추므로 자유 실행하려면 ARHT=1(생성 값을 다시 적재하고 계속). LOAD 비트는 카운터 동작을 막으므로 ENT 를 켤 때 지운다.
 * 설계안(문서 근거 없음): 디바운스 시간(호출자가 정함), RGB PWM 주파수(호출자가 정함, LED 용으로 수 백 Hz~수 kHz 가 흔하다는 일반 상식뿐), 늦은 주기의 처리 방식.
 * 시계 한계: 한 바퀴가 2^32 + 1 클럭이라 넘침마다 1 클럭 어긋난다(계산: 100 MHz 에서 43 초마다 10 ns). 넘침 한 바퀴 안에 mclock_* 를 한 번은 호출해야 한다
 *   (100 MHz 에서 42.9 초). 호출이 한 바퀴 이상 뜸하면 시간이 줄어든다.
 * 보드에서 실행해 보지 않았다. */
#ifndef BASIC_IO_H
#define BASIC_IO_H
#include <stdint.h>
#include "axi_timer_pwm.h"

#ifndef TMR_TCR0
#define TMR_TCR0 0x08
#endif
#define BIO_OK       0
#define BIO_ERR_ARGS (-1)

/* ---- 자유 실행 시계 (AXI Timer 0 을 올림 카운터로) ---- */
typedef struct { uintptr_t base; uint32_t clk_hz; uint32_t last; uint64_t cycles; } mclock_t;
int mclock_start(mclock_t *m, uintptr_t base, uint32_t clk_hz);   /* clk_hz >= 1000 */
uint64_t mclock_cycles(mclock_t *m);                               /* 시작 이후 클럭 수 */
uint32_t mclock_millis(mclock_t *m);                               /* 시작 이후 ms (32비트로 49.7 일마다 되돌아감, 뺄셈으로 비교하면 안전) */
uint32_t mclock_micros(mclock_t *m);                               /* 시작 이후 µs (71.6 분마다 되돌아감) */

/* ---- 비차단 주기 타이머: if (ticker_due(&t, millis)) { ... } ---- */
typedef struct { uint32_t period, last; } ticker_t;
int ticker_init(ticker_t *t, uint32_t period, uint32_t now);       /* period 0 은 오류 */
/* 한 주기가 지났으면 1. 기준을 한 주기씩 전진(오차 누적 없음). 두 주기 이상 늦었으면 밀린 만큼을 버리고 지금으로 다시 맞춤(몰아서 실행하지 않음) */
int ticker_due(ticker_t *t, uint32_t now);

/* ---- 버튼: 안정된 상태가 debounce_ms 이상 유지될 때만 바뀐 것으로 본다 ---- */
#define BTN_EV_NONE    0
#define BTN_EV_PRESS   1
#define BTN_EV_RELEASE 2
typedef struct {
    uint32_t debounce_ms, t_cand, t_stable, presses, releases;
    uint8_t active_low, primed, cand, stable;
} button_t;
int button_init(button_t *b, uint32_t debounce_ms, int active_low);
int button_update(button_t *b, int raw, uint32_t now_ms);          /* 샘플마다 호출. 첫 호출은 기준만 잡고 이벤트 없음 */
int button_down(const button_t *b);
uint32_t button_held_ms(const button_t *b, uint32_t now_ms);        /* 눌린 채 유지된 시간, 안 눌렸으면 0 */

/* ---- RGB LED: AXI Timer PWM 3개(타이머 인스턴스 3개: 인스턴스마다 PWM 출력은 1개) ---- */
typedef struct { uintptr_t base[3]; uint32_t clk_hz, freq_hz; int common_anode; } rgb_t;
int rgb_init(rgb_t *r, uintptr_t base_r, uintptr_t base_g, uintptr_t base_b, uint32_t clk_hz, uint32_t freq_hz, int common_anode);
int rgb_set(rgb_t *r, uint8_t red, uint8_t green, uint8_t blue);    /* 0..255 선형. 공통 애노드는 핀 극성을 반대로 */
void rgb_from_hsv(unsigned hue_deg, uint8_t sat, uint8_t val, uint8_t out[3]);   /* hue 0..359 (넘으면 나머지), 정수 연산 */
#endif
