/* MicroBlaze V 용 DC 모터 드라이버 예시. 컴파일과 보드 시험은 아직 하지 않았다. */
#include <stdint.h>

#define MOT_BASE   0x44A30000u   /* Vivado 주소 편집기에서 정한 값으로 바꿀 것 */
#define MOT_CTRL   (*(volatile uint32_t *)(MOT_BASE + 0x00))
#define MOT_DUTY   (*(volatile uint32_t *)(MOT_BASE + 0x04))
#define MOT_PERIOD (*(volatile uint32_t *)(MOT_BASE + 0x08))

enum { MOT_COAST = 0, MOT_FWD = 1, MOT_REV = 2, MOT_BRAKE = 3 };

/* percent: 0..100. 주기는 레지스터 값(기본 100 MHz에서 1 kHz)을 읽어 듀티를 계산한다. */
void motor_set(int mode, int percent) {
    uint32_t period = MOT_PERIOD;
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    MOT_DUTY = (period * (uint32_t)percent) / 100u;
    MOT_CTRL = 1u | ((uint32_t)mode << 1);
}
void motor_stop(void) { MOT_CTRL = 0; }
