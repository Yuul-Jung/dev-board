/* HC-SR04 드라이버 호스트 시험. 모의 소자는 us_axil_regs.v 의 레지스터 동작(머리말 주석, 1 을 쓰면 지워지는 timeout·done)을 흉내 낸다.
 * 한계: 측정 코어(노션 원본)와 에코 타이밍은 시험하지 않는다. 모델은 내가 레지스터 주석을 읽고 만든 것이다. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ultrasonic.h"

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

static struct {
    uint32_t ctrl, stat_flags, dist, bcd, irq;       /* 리터럴 비트: timeout 0x4, done 0x8 */
    uint32_t reads_until_result; int result_is_timeout; int auto_cycle;
    uint32_t stat_reads, ctrl_writes, stat_writes, polls_since_enable;
    uint32_t log[64]; int nlog;
} M;
static void fresh(void) { memset(&M, 0, sizeof M); M.reads_until_result = 3; M.dist = 100; }

uint32_t mock_us_rd(uint32_t off)
{
    switch (off) {
    case 0x00: return M.ctrl;
    case 0x04: {
        M.stat_reads++;
        if (M.ctrl && M.reads_until_result && --M.reads_until_result == 0) M.stat_flags |= M.result_is_timeout ? 0x4u : 0x8u;
        return (M.ctrl ? 0u : 0u) | M.stat_flags | 0x2u /* valid 는 상시 1 */;
    }
    case 0x08: return M.dist;
    case 0x0C: return M.bcd;
    case 0x10: return M.irq;
    default: return 0;
    }
}
void mock_us_wr(uint32_t off, uint32_t v)
{
    if (M.nlog < 64) M.log[M.nlog++] = (off << 16) | (v & 0xFFFF);
    if (off == 0x00) { M.ctrl_writes++; M.ctrl = v & 1; }
    else if (off == 0x04) { M.stat_writes++; if (v & 0x4) M.stat_flags &= ~0x4u; if (v & 0x8) M.stat_flags &= ~0x8u; }   /* 1 을 쓰면 지움; 나머지 비트는 쓰기 무시 */
    else if (off == 0x10) M.irq = v & 3;
}
#define BASE ((uintptr_t)0x44A10000u)

static void t_basic(void)
{
    int cm = -1;
    fresh(); M.dist = 123;
    us_start(BASE);
    CHECK(M.ctrl == 1 && M.nlog == 2 && M.log[0] == ((0x04u << 16) | 0xC) && M.log[1] == ((0x00u << 16) | 1));   /* 플래그를 먼저 지우고 그다음 켠다 */
    CHECK(us_read_cm(BASE, &cm) == US_OK && cm == 123);
    CHECK((M.stat_flags & 0x8) == 0);                                  /* done 을 읽고 지움 */
    /* 두 번째 측정 */
    M.reads_until_result = 2; M.dist = 7;
    CHECK(us_read_cm(BASE, &cm) == US_OK && cm == 7);
    fresh(); M.dist = 0x400 | 55;                                      /* [9:0] 만 쓴다: 상위 비트는 무시 */
    us_start(BASE); CHECK(us_read_cm(BASE, &cm) == US_OK && cm == 55);
    fresh(); us_init(BASE);
    CHECK(M.ctrl == 0 && M.nlog == 2 && M.log[0] == ((0x00u << 16) | 0) && M.log[1] == ((0x04u << 16) | 0xC));
    us_start(BASE); us_stop(BASE); CHECK(M.ctrl == 0);
    fresh(); us_start(BASE);
    CHECK(us_echo_to_cm(0) == 0 && us_echo_to_cm(57) == 0 && us_echo_to_cm(58) == 1 && us_echo_to_cm(5800) == 100 && us_echo_to_cm(23200) == 400 && us_echo_to_cm(116) == 2);
}

static void t_poll(void)
{
    int cm = -1;
    fresh(); M.reads_until_result = 4; M.dist = 50; us_start(BASE);
    CHECK(us_poll(BASE, &cm) == US_PENDING && cm == -1);
    CHECK(us_poll(BASE, &cm) == US_PENDING && us_poll(BASE, &cm) == US_PENDING && cm == -1);
    CHECK(us_poll(BASE, &cm) == US_OK && cm == 50);                    /* 4 번째 읽기에서 결과 */
    CHECK(us_poll(BASE, &cm) == US_PENDING);                           /* 플래그를 지웠으므로 다시 대기 */
    /* 보류 중에는 레지스터에 아무것도 쓰지 않는다 */
    fresh(); M.reads_until_result = 100; us_start(BASE); uint32_t w = M.stat_writes;
    for (int i = 0; i < 20; i++) us_poll(BASE, &cm);
    CHECK(M.stat_writes == w);
    CHECK(us_poll(BASE, 0) == US_ERR_ARGS && us_read_cm(BASE, 0) == US_ERR_ARGS);
    CHECK(US_OK == 0 && US_PENDING == 1 && US_ERR_TIMEOUT == -1 && US_ERR_RANGE == -2 && US_ERR_ARGS == -3 && US_ERR_BUS == -4);
}

static void t_timeout_range(void)
{
    int cm = 77;
    fresh(); M.result_is_timeout = 1; us_start(BASE);
    CHECK(us_read_cm(BASE, &cm) == US_ERR_TIMEOUT && cm == 77);
    CHECK((M.stat_flags & 0x4) == 0);                                  /* timeout 도 지움 */
    /* timeout 과 done 이 함께 보이면 timeout 이 우선(에코 없음) */
    fresh(); M.stat_flags = 0x4 | 0x8; M.dist = 100; M.reads_until_result = 0; M.ctrl = 1;
    CHECK(us_poll(BASE, &cm) == US_ERR_TIMEOUT && cm == 77 && (M.stat_flags & 0x8));    /* done 은 남겨 둔다 */
    static const struct { uint32_t d; int ok; } tb[] = {{0,0},{1,0},{2,1},{3,1},{399,1},{400,1},{401,0},{999,0},{1023,0}};
    for (unsigned i = 0; i < sizeof tb / sizeof tb[0]; i++) {
        fresh(); M.dist = tb[i].d; us_start(BASE); cm = 77;
        int r = us_read_cm(BASE, &cm);
        CHECK(tb[i].ok ? (r == US_OK && cm == (int)tb[i].d) : (r == US_ERR_RANGE && cm == 77));
        CHECK((M.stat_flags & 0x8) == 0);                              /* 범위 밖이어도 done 은 소비 */
    }
    fresh(); M.reads_until_result = 0; M.ctrl = 1; M.stat_reads = 0;   /* 결과가 영영 없음 */
    CHECK(us_read_cm(BASE, &cm) == US_ERR_BUS && M.stat_reads == 10000000);
    fresh(); M.reads_until_result = 9999999; us_start(BASE); CHECK(us_read_cm(BASE, &cm) == US_OK);   /* 상한 직전 */
    fresh(); M.reads_until_result = 10000000; us_start(BASE); CHECK(us_read_cm(BASE, &cm) == US_OK);  /* 마지막 시도 */
    fresh(); M.reads_until_result = 10000001; us_start(BASE); CHECK(us_read_cm(BASE, &cm) == US_ERR_BUS);
}

static void t_stale(void)
{
    /* 켜기 전 남아 있던 done/timeout 은 us_start 가 지운다 → 오래된 결과를 새 결과로 착각하지 않음 */
    int cm = 0;
    fresh(); M.stat_flags = 0x8 | 0x4; M.dist = 33; M.reads_until_result = 5;
    us_start(BASE);
    CHECK(us_poll(BASE, &cm) == US_PENDING);
    fresh(); M.stat_flags = 0x8; M.dist = 33; M.reads_until_result = 0;
    us_init(BASE); CHECK((M.stat_flags & 0xC) == 0 && M.ctrl == 0);
}

int main(void)
{
    t_basic(); t_poll(); t_timeout_range(); t_stale();
    printf("ultrasonic: %d 검사, 실패 %d\n", checks, fails);
    if (fails) return 1;
    printf("ALL PASS\n");
    return 0;
}
