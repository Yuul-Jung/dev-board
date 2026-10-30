/* 서보 드라이버 호스트 시험. 모의 레지스터는 servo_axil_regs.v 머리말(리터럴 오프셋)과 servo_engine.v 의 제한 동작(MIN>MAX 면 교환, 클램프)을 흉내 낸다.
 * 한계: RTL 과 연결되지 않았다. 변형 검사·사실표 대조는 아직 하지 않았다. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "servo.h"

static uint32_t en, wd, per = 20000, lo = 1000, hi = 2000, stuck_mask = 0xFFFF;
uint32_t mock_srv_rd(uint32_t off)
{
    switch (off) {
    case 0x00: return en; case 0x04: return wd; case 0x08: return per; case 0x0C: return lo; case 0x10: return hi;
    case 0x14: { uint32_t a = lo > hi ? hi : lo, b = lo > hi ? lo : hi; return wd < a ? a : wd > b ? b : wd; }
    default: return 0;
    }
}
void mock_srv_wr(uint32_t off, uint32_t v)
{
    v &= stuck_mask;
    switch (off) { case 0x00: en = v & 1; break; case 0x04: wd = v; break; case 0x08: per = v; break; case 0x0C: lo = v; break; case 0x10: hi = v; break; default: break; }
}
static int fails, checks;
#define CHK(c) do { checks++; if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
static void reset(void) { en = 0; wd = 0; per = 20000; lo = 1000; hi = 2000; stuck_mask = 0xFFFF; }

int main(void)
{
    CHK(servo_angle_to_us(0, 1000, 2000) == 1000 && servo_angle_to_us(180, 1000, 2000) == 2000 && servo_angle_to_us(90, 1000, 2000) == 1500);
    CHK(servo_angle_to_us(-5, 1000, 2000) == 1000 && servo_angle_to_us(999, 1000, 2000) == 2000);
    CHK(servo_angle_to_us(1, 1000, 2000) == 1006 && servo_angle_to_us(179, 1000, 2000) == 1994);      /* 5.56 → 6, 994.4 → 994 */
    CHK(servo_angle_to_us(90, 500, 2400) == 1450 && servo_angle_to_us(45, 500, 2400) == 975);
    CHK(servo_angle_to_us(90, 2000, 1000) == 1500);                                                     /* 뒤집힌 한계도 같은 결과 */
    CHK(servo_angle_to_us(180, 0, 65535) == 65535 && servo_angle_to_us(90, 0, 65535) == 32768);

    reset();
    CHK(servo_init(0, 20000, 1000, 2000) == SRV_OK && en == 1 && per == 20000 && lo == 1000 && hi == 2000 && wd == 1500);
    CHK(servo_set_angle(0, 0) == SRV_OK && servo_applied_us(0) == 1000);
    CHK(servo_set_angle(0, 180) == SRV_OK && servo_applied_us(0) == 2000);
    CHK(servo_set_angle(0, 45) == SRV_OK && wd == 1250 && servo_applied_us(0) == 1250);
    CHK(servo_set_width_us(0, 3000) == SRV_OK && wd == 3000 && servo_applied_us(0) == 2000);          /* RTL 이 제한 */
    CHK(servo_set_width_us(0, 10) == SRV_OK && servo_applied_us(0) == 1000);
    CHK(servo_set_width_us(0, 65536) == SRV_ERR_ARGS && wd == 10);
    servo_enable(0, 0); CHK(en == 0); servo_enable(0, 5); CHK(en == 1); servo_enable(0, 0); CHK(en == 0);

    reset(); CHK(servo_init(0, 20000, 500, 2400) == SRV_OK && wd == 1450);
    CHK(servo_set_angle(0, 90) == SRV_OK && wd == 1450 && servo_applied_us(0) == 1450);              /* 현재 MIN·MAX 레지스터 기준 */
    CHK(servo_set_angle(0, 180) == SRV_OK && wd == 2400);

    /* 인자 검사: 아무것도 쓰지 않는다 */
    reset(); en = 7;
    CHK(servo_init(0, 20000, 0, 2000) == SRV_ERR_ARGS && servo_init(0, 20000, 2001, 2000) == SRV_ERR_ARGS);
    CHK(servo_init(0, 2000, 1000, 2000) == SRV_ERR_ARGS && servo_init(0, 2001, 1000, 2000) == SRV_OK);
    reset(); en = 7;
    CHK(servo_init(0, 65536, 1000, 2000) == SRV_ERR_ARGS && en == 7);
    CHK(servo_init(0, 65535, 1000, 2000) == SRV_OK && per == 65535);

    /* 레지스터가 값을 못 받으면 오류 */
    reset(); stuck_mask = 0x00FF; CHK(servo_init(0, 20000, 1000, 2000) == SRV_ERR_REG && en == 0);
    reset(); servo_init(0, 20000, 1000, 2000); stuck_mask = 0x00FF; CHK(servo_set_width_us(0, 1500) == SRV_ERR_REG);

    /* 속성: 각도 단조 증가, 양 끝 정확, 결과는 [min,max] 안 */
    srand(2);
    for (int n = 0; n < 5000; n++) {
        uint32_t a = (uint32_t)rand() % 3000 + 1, b = a + (uint32_t)rand() % 3000; int prev = -1;
        for (int d = 0; d <= 180; d++) { int w = (int)servo_angle_to_us(d, a, b); CHK(w >= (int)a && w <= (int)b && w >= prev); prev = w; }
        CHK(servo_angle_to_us(0, a, b) == a && servo_angle_to_us(180, a, b) == b);
    }
    printf("servo: %d 검사, 실패 %d\n", checks, fails);
    if (fails) return 1;
    printf("ALL PASS\n");
    return 0;
}
