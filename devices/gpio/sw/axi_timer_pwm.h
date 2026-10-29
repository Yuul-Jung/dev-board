/* AXI Timer(PG079)로 PWM 만들기 — 팬(25 kHz)과 패시브 부저(주파수)에 사용.
 * 근거는 PG079 가 아니라 Zephyr xlnx,xps-timer-1.00.a PWM 드라이버(D3, 2차 자료)이다. PG079 원문은 열지 못했다.
 * 레지스터: TCSR0 0x00 TLR0 0x04, TCSR1 0x10 TLR1 0x14. 타이머0 = 주기, 타이머1 = 하이 시간. */
#ifndef AXI_TIMER_PWM_H
#define AXI_TIMER_PWM_H
#include <stdint.h>

#ifndef TMR_RD
#define TMR_RD(base, off)    (*(volatile uint32_t *)((uintptr_t)(base) + (off)))
#define TMR_WR(base, off, v) (*(volatile uint32_t *)((uintptr_t)(base) + (off)) = (v))
#endif
#define TMR_TCSR0 0x00
#define TMR_TLR0  0x04
#define TMR_TCSR1 0x10
#define TMR_TLR1  0x14
#define TCSR_UDT   (1u << 1)
#define TCSR_GENT  (1u << 2)
#define TCSR_ARHT  (1u << 4)
#define TCSR_LOAD  (1u << 5)
#define TCSR_ENT   (1u << 7)
#define TCSR_PWMA  (1u << 9)
#define TCSR_ENALL (1u << 10)
#define TCSR_PWM   (TCSR_UDT | TCSR_GENT | TCSR_ARHT | TCSR_PWMA)

#define TPWM_OK 0
#define TPWM_ERR_ARGS -1

/* 클럭 사이클 단위. 0 < high < period 이면 일반 PWM(period>=2, high>=2 필요). high==0 / high==period 는 상수 출력 */
int tpwm_set_cycles(uintptr_t base, uint32_t period, uint32_t high);
/* 팬: Intel 4선 팬 규격의 25 kHz (허용 21~28 kHz), duty 0~100 % */
int fan_set_percent(uintptr_t base, uint32_t clk_hz, unsigned percent);
/* 패시브 부저: 50 % duty 의 freq_hz. 0 이면 끔 */
int buzzer_tone(uintptr_t base, uint32_t clk_hz, uint32_t freq_hz);
#endif
