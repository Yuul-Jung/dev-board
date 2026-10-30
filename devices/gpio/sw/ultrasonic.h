/* HC-SR04 거리계 드라이버 — `rtl/us_axil_*`(노션 원본 코어를 감싼 AXI4-Lite) 레지스터 위에서 동작.
 * 근거: 거리[cm] = Echo High 시간[µs] / 58, 측정 범위 2 cm~4 m, 측정 주기 60 ms 초과 권장 (gpio/hc-sr04.md, D1 제조사 데이터시트 사본).
 *   레지스터는 `rtl/us_axil_regs.v` 머리말: CTRL 0x00 [0]=enable, STATUS 0x04 [0]busy [1]valid [2]timeout(1 쓰면 지움) [3]done(1 쓰면 지움),
 *   DIST 0x08 [9:0] cm, BCD 0x0C, IRQ_EN 0x10. 코어는 enable 이 켜져 있는 동안 60 ms 간격으로 계속 측정한다(rtl/README.md).
 *   done 은 측정이 끝나고 valid 일 때만 켜지고, 에코가 없으면 timeout 이 켜진다.
 * 설계안: 폴링 상한, 비차단 us_poll, 범위(2~400 cm) 밖 값 거부. 코어는 최대 999 cm 까지 낼 수 있어 400 초과는 반사 오류 의심.
 * 미확인: enable 을 껐다 켜면 측정이 즉시 새로 시작하는지(코어 소스를 읽지 못함). 이 드라이버는 켜 두고 done/timeout 플래그만 기다린다.
 * 보드에서 실행해 보지 않았다. 호스트 모의 레지스터 시험만. */
#ifndef ULTRASONIC_H
#define ULTRASONIC_H
#include <stdint.h>

#ifndef US_RD
#define US_RD(base, off)    (*(volatile uint32_t *)((uintptr_t)(base) + (off)))
#define US_WR(base, off, v) (*(volatile uint32_t *)((uintptr_t)(base) + (off)) = (v))
#endif

#define US_REG_CTRL   0x00
#define US_REG_STAT   0x04
#define US_REG_DIST   0x08
#define US_REG_BCD    0x0C
#define US_REG_IRQEN  0x10
#define US_ST_BUSY    (1u << 0)
#define US_ST_VALID   (1u << 1)
#define US_ST_TIMEOUT (1u << 2)
#define US_ST_DONE    (1u << 3)
#define US_DIST_MASK  0x3FFu
#define US_US_PER_CM  58          /* 거리[cm] = echo[µs] / 58 */
#define US_CM_MIN     2
#define US_CM_MAX     400
#define US_SPINS      10000000u

#define US_OK           0
#define US_PENDING      1         /* us_poll: 아직 결과 없음 */
#define US_ERR_TIMEOUT (-1)       /* 에코 없음 */
#define US_ERR_RANGE   (-2)       /* 2~400 cm 밖 */
#define US_ERR_ARGS    (-3)
#define US_ERR_BUS     (-4)       /* us_read_cm 이 상한까지 기다려도 done/timeout 이 안 옴 */

void us_init(uintptr_t base);                         /* 끄고 남은 플래그를 지움 */
void us_start(uintptr_t base);                        /* 플래그를 지우고 켠다 */
void us_stop(uintptr_t base);
int us_poll(uintptr_t base, int *cm);                 /* 비차단. US_OK / US_PENDING / 음수 */
int us_read_cm(uintptr_t base, int *cm);              /* 결과가 올 때까지(US_SPINS 상한) us_poll */
uint32_t us_echo_to_cm(uint32_t echo_us);             /* echo_us / 58 */
#endif
