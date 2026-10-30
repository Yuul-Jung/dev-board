/* 서보(SG90 등) 드라이버 — `rtl/servo_axil_*` 레지스터 위에서 동작. 교재 26장.
 * 레지스터(rtl/servo_axil_regs.v 머리말): CTRL 0x00 [0]=enable, WIDTH 0x04 [15:0] µs, PERIOD 0x08 [15:0] µs(기본 20000),
 *   MIN 0x0C 기본 1000, MAX 0x10 기본 2000, APPLIED 0x14 읽기(제한이 적용된 폭). 폭은 [MIN,MAX] 로 RTL 이 제한한다(MIN>MAX 면 서로 바꿈).
 * 근거: SG90 사양은 2차 자료뿐이고 자료마다 다르다(motor/sg90.md, D3): 주기 약 20 ms, 폭 약 1~2 ms(중앙 1.5 ms), 한 판매처는 500~2400 µs, 회전 약 180°.
 *   방향(어느 쪽이 +90°인지)은 문서 안에서 모순이라 이 드라이버는 정하지 않는다.
 * 설계안: 각도 → 폭은 MIN..MAX 를 0°..180° 에 직선으로 대응(반올림). 폭·주기는 16비트라 65535 µs 까지.
 * 보드에서 실행해 보지 않았다. 호스트 모의 레지스터 시험만(변형 검사·사실표 대조는 미실시 — 사용자 요청으로 뒤로 미룸). */
#ifndef SERVO_H
#define SERVO_H
#include <stdint.h>

#ifndef SRV_RD
#define SRV_RD(base, off)    (*(volatile uint32_t *)((uintptr_t)(base) + (uint32_t)(off)))
#define SRV_WR(base, off, v) (*(volatile uint32_t *)((uintptr_t)(base) + (uint32_t)(off)) = (uint32_t)(v))
#endif

#define SRV_REG_CTRL    0x00u
#define SRV_REG_WIDTH   0x04u
#define SRV_REG_PERIOD  0x08u
#define SRV_REG_MIN     0x0Cu
#define SRV_REG_MAX     0x10u
#define SRV_REG_APPLIED 0x14u
#define SRV_DEF_PERIOD_US 20000u
#define SRV_DEF_MIN_US    1000u
#define SRV_DEF_MAX_US    2000u
#define SRV_US_MAX        65535u   /* 16비트 레지스터 */
#define SRV_ANGLE_MAX     180

#define SRV_OK          0
#define SRV_ERR_ARGS   (-1)
#define SRV_ERR_REG    (-2)   /* 쓴 값이 그대로 읽히지 않음 */

/* 주기·하한·상한(µs)을 정하고 켠다. 0 < min <= max < period, 모두 65535 이하여야 한다. 폭은 중앙으로 시작. */
int servo_init(uintptr_t base, uint32_t period_us, uint32_t min_us, uint32_t max_us);
int servo_set_width_us(uintptr_t base, uint32_t width_us);   /* RTL 이 [MIN,MAX] 로 제한하므로 실제 폭은 servo_applied_us */
int servo_set_angle(uintptr_t base, int deg);                /* 0..180, 범위 밖은 클램프 */
uint32_t servo_angle_to_us(int deg, uint32_t min_us, uint32_t max_us);
uint32_t servo_applied_us(uintptr_t base);
void servo_enable(uintptr_t base, int on);
#endif
