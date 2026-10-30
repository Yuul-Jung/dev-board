/* DC 모터 드라이버 (L298N / TB6612FNG / Pmod HB5). AXI4-Lite 커스텀 RTL 경유.
 * 레지스터 맵 (dcmotor_axil_regs.v):
 *   0x00 CTRL   [0]=enable [2:1]=mode(0 코스트,1 정회전,2 역회전,3 브레이크)
 *   0x04 DUTY   [15:0] 켜짐 클럭 수
 *   0x08 PERIOD [31:0] PWM 주기 클럭 수
 *   0x0C STATUS [1:0]=적용된 mode [2]=방향 전환 대기
 *
 * 한계 — Pmod HB5 (DIR/EN 2핀, 브레이크 없음):
 *   motor_set(MOT_BRAKE, ...) 를 보내면 RTL 은 IN1=IN2=1 을 출력하지만
 *   HB5 는 이를 받는 핀이 없다(DIR 1핀만 있음). HB5 에 브레이크가 필요하면
 *   대신 MOT_COAST(EN=0) 를 쓴다. IN2 핀은 HB5 쪽에서 미연결이라 무해하다.
 *   (설계안 — 실제 배선 확인 전) */
#ifndef DCMOTOR_H
#define DCMOTOR_H
#include <stdint.h>
#include <stddef.h>

#ifndef MOT_RD
#  define MOT_RD(base, off)    (*(volatile uint32_t *)((uintptr_t)(base) + (uint32_t)(off)))
#  define MOT_WR(base, off, v) (*(volatile uint32_t *)((uintptr_t)(base) + (uint32_t)(off)) = (uint32_t)(v))
#endif

#define MOT_OFF_CTRL   0x00u
#define MOT_OFF_DUTY   0x04u
#define MOT_OFF_PERIOD 0x08u
#define MOT_OFF_STATUS 0x0Cu

enum mot_mode { MOT_COAST = 0, MOT_FWD = 1, MOT_REV = 2, MOT_BRAKE = 3 };

/* percent: 0..100, 범위 밖은 클램프. period=0 이면 duty=0. */
void motor_set(uintptr_t base, int mode, int percent);
void motor_stop(uintptr_t base);

#endif /* DCMOTOR_H */
