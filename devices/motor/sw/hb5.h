/* Pmod HB5 — 1채널 H 브리지 드라이버 (양방향 DC 모터, 최대 2 A).
 *
 * 핀 배열 (Pmod JA/JB 상단 6핀 기준, D1 Digilent HB5 Reference):
 *   Pin1 = DIR   — GPIO 출력: Low = 정방향(CW), High = 역방향(CCW).
 *   Pin2 = EN    — PWM 출력 (AXI Timer PWM): High = 활성화.
 *   Pin3,4 = SA, SB — 엔코더 A·B (이 드라이버는 사용하지 않음).
 *   Pin5,6 = VCC, GND.
 *
 * 안전 규칙 (D1 Digilent 권고):
 *   방향 전환 시 EN 을 먼저 0 으로 내린 뒤 DIR 을 바꿔야 한다.
 *   hb5_set_dir 또는 hb5_run 이 이 순서를 자동으로 처리한다.
 *
 * 이 드라이버가 다루는 범위:
 *   - 속도: 0~100 % duty (tpwm_set_cycles 기반).
 *   - 방향: 정방향·역방향·정지.
 *   - PWM 주기: 호출자가 period_cycles 를 넘긴다(기본 권장: FPGA 클럭 / 20000 ≈ 5 kHz).
 *
 * 한계:
 *   - 전류 감지(SA/SB 전류 임계 출력)는 이 드라이버에서 처리하지 않는다.
 *   - 실제 보드에서 시험하지 않음(호스트 모의 레지스터 시험만).
 */
#ifndef HB5_H
#define HB5_H
#include <stdint.h>
#include "axi_gpio.h"
#include "axi_timer_pwm.h"

#define HB5_OK        0
#define HB5_ERR_ARGS  (-1)
#define HB5_ERR_SPEED (-2)   /* percent > 100 */

typedef enum { HB5_FWD = 0, HB5_REV = 1 } hb5_dir_t;

typedef struct {
    gpio_t    *gpio;        /* DIR 핀에 연결된 GPIO 인스턴스 */
    unsigned   gpio_ch;     /* GPIO 채널 (0 또는 1) */
    unsigned   gpio_bit;    /* GPIO 비트 번호 (0..31) */
    uintptr_t  pwm_base;    /* AXI Timer 베이스 주소 (EN 핀) */
    uint32_t   period;      /* PWM 주기 클럭 사이클 수 */
} hb5_t;

/* 초기화: GPIO 핀을 출력으로 설정하고, EN = 0, DIR = FWD (Low) */
int hb5_init(hb5_t *h, gpio_t *g, unsigned gpio_ch, unsigned gpio_bit,
             uintptr_t pwm_base, uint32_t period_cycles);

/* 정지: EN = 0 (DIR 유지). 비상 정지 시 사용. */
int hb5_stop(hb5_t *h);

/* 방향만 변경: EN 을 먼저 내린 뒤 DIR 을 바꾼다. */
int hb5_set_dir(hb5_t *h, hb5_dir_t dir);

/* 속도 설정 (percent 0~100): EN PWM duty 를 조정. 방향은 그대로. */
int hb5_set_speed(hb5_t *h, unsigned percent);

/* 방향 + 속도 한꺼번에: EN 을 먼저 0 으로 내린 뒤 DIR, 이후 EN duty 설정. */
int hb5_run(hb5_t *h, hb5_dir_t dir, unsigned percent);

#endif /* HB5_H */
