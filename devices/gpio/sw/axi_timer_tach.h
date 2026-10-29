/* AXI Timer (PG079 v2.0) 캡처 모드로 팬 타코 주기 측정. 별도 인스턴스의 타이머 0 사용.
 * 확인(PG079): 캡처 모드(MDT=1), CAPT=1 이면 외부 capturetrig0 에서 카운터 값이 TLR0 에 저장되고 T0INT 가 세트됨
 *   (1 을 써서 지움). ARHT=1 이면 새 캡처가 이전 값을 덮어씀. 캡처 트리거 극성은 IP 파라미터(기본 active-high).
 * 설계안: 업카운터가 0 부터 자유 주행한다고 보고 연속 두 캡처 값의 차(mod 2^32)를 주기로 씀.
 * 입력은 팬 타코(12 V 풀업 오픈 컬렉터)를 3.3 V 로 변환한 신호여야 한다. */
#ifndef AXI_TIMER_TACH_H
#define AXI_TIMER_TACH_H
#include <stdint.h>
#include "axi_timer_pwm.h"     /* TMR_* 매크로와 레지스터 오프셋 공유 */

#define TCSR_MDT  (1u << 0)
#define TCSR_CAPT (1u << 3)
#define TCSR_TINT (1u << 8)

typedef struct { uintptr_t base; uint32_t prev; uint32_t period; int have_prev; } tach_t;

void tach_init(tach_t *t, uintptr_t base);
/* 캡처가 있으면 처리하고 새 주기가 계산되면 1. 첫 캡처는 기준점만 저장하므로 0 */
int  tach_poll(tach_t *t);
/* RPM = 60·clk / (period · pulses_per_rev). 주기 0 이면 0 */
uint32_t tach_rpm(const tach_t *t, uint32_t clk_hz, unsigned pulses_per_rev);
#endif
