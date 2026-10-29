/* AXI Timer (PG079 v2.0) PWM — 팬(25 kHz)과 패시브 부저(주파수)에 사용.
 * 확인(PG079 PDF): 레지스터 TCSR0 0x00 / TLR0 0x04 / TCR0 0x08, TCSR1 0x10 / TLR1 0x14 / TCR1 0x18.
 *   PWM 은 타이머 0(주기)과 타이머 1(하이 시간) 한 쌍을 쓴다. 두 타이머 모두 MDT=0(generate), GENT=1, PWMA=1,
 *   GenerateOut 은 active-high. 카운트다운(UDT=1)일 때 주기 = (TLR0+2)·T_clk, 하이 = (TLR1+2)·T_clk.
 *   LOAD 비트는 카운터 동작을 막으므로 ENT 를 켤 때 같이 지워야 함. ENALL 에 1 을 쓰면 ENALL·ENT0·ENT1 이 세트됨.
 * 설계안/미확인: ARHT(자동 재적재) 사용은 PG079 PWM 절에 명시가 없고 Zephyr 드라이버를 따름. 0 %/100 % 상수 출력은
 *   PG079 에 설명이 없어 Zephyr 요약대로 추정. 카운터 폭 파라미터는 32비트로 가정. 이 PWM 은 타이머 0·1 을 모두 쓰므로
 *   캡처(타코)용으로는 별도 AXI Timer 인스턴스가 필요하다. */
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
