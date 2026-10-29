/* 기본 입출력 예제 묶음 시험: 자유 실행 시계, 비차단 타이머, 버튼 디바운스, RGB. 자작 AXI Timer 모의에 대한 시험이며 보드에서 확인한 것이 아니다. */
#include <stdio.h>
#include <string.h>
#include "../sw/basic_io.h"

#define NB 4
static uint32_t tcsr0[NB], tcsr1[NB], tlr0[NB], tlr1[NB], tcr[NB]; static int running[NB], bad_access, nload[NB];
static int idx(uintptr_t b) { if (b < NB) return (int)b; bad_access++; return 0; }
uint32_t mock_tb_rd(uintptr_t b, uint32_t off)
{
    int i = idx(b);
    switch (off) { case TMR_TCSR0: return tcsr0[i]; case TMR_TCSR1: return tcsr1[i]; case TMR_TLR0: return tlr0[i]; case TMR_TLR1: return tlr1[i]; case 0x08: return tcr[i];           /* PG079: TCR0 = 08h (헤더 매크로가 아닌 숫자) */ default: bad_access++; return 0; }
}
void mock_tb_wr(uintptr_t b, uint32_t off, uint32_t v)
{
    int i = idx(b);
    switch (off) {
    case TMR_TLR0: tlr0[i] = v; break;
    case TMR_TLR1: tlr1[i] = v; break;
    case TMR_TCSR1: tcsr1[i] = v; break;
    case TMR_TCSR0:
        tcsr0[i] = v;
        if (v & TCSR_LOAD) { tcr[i] = tlr0[i]; running[i] = 0; nload[i]++; }       /* LOAD 는 카운터 동작을 막는다(PG079) */
        else running[i] = (v & TCSR_ENT) != 0;
        break;
    default: bad_access++;
    }
}
static void adv(int i, uint32_t n) { if (running[i]) tcr[i] += n; }
static int fails;
#define CHK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

/* PWM 출력 해석: 타이머 1 에 ENALL 이 있으면 일반 PWM, 한쪽만 ENT 이면 상수 */
static double duty(int i)
{
    if (tcsr1[i] & TCSR_ENALL) return (double)(tlr1[i] + 2) / (double)(tlr0[i] + 2);
    if ((tcsr1[i] & TCSR_ENT) && !(tcsr0[i] & TCSR_ENT)) return 0.0;
    if ((tcsr0[i] & TCSR_ENT) && !(tcsr1[i] & TCSR_ENT)) return 1.0;
    return -1.0;
}

int main(void)
{
    /* ---- 시계 ---- */
    {
        mclock_t m;
        CHK(mclock_start(&m, 0, 999) == BIO_ERR_ARGS && mclock_start(0, 0, 1000000) == BIO_ERR_ARGS);
        tlr0[0] = 0xDEAD; tcr[0] = 12345;                           /* 이전에 쓰이던 타이머 */
        CHK(mclock_start(&m, 0, 100000000) == BIO_OK && nload[0] == 1 && tcr[0] == 0);
        CHK((tcsr0[0] & (TCSR_ARHT | TCSR_ENT)) == (TCSR_ARHT | TCSR_ENT) && !(tcsr0[0] & (TCSR_LOAD | TCSR_UDT | 1u)) && running[0] && tlr0[0] == 0);
        CHK(mclock_cycles(&m) == 0);
        adv(0, 100000); CHK(mclock_millis(&m) == 1 && mclock_micros(&m) == 1000);
        adv(0, 99900000); CHK(mclock_millis(&m) == 1000 && mclock_micros(&m) == 1000000);
        adv(0, 0xFFFFFF00u); CHK(mclock_cycles(&m) == 100000000ull + 0xFFFFFF00ull);     /* 한 바퀴 직전 */
        adv(0, 0x200);       CHK(mclock_cycles(&m) == 100000000ull + 0xFFFFFF00ull + 0x200);   /* 넘침 건너감 */
        unsigned __int128 c = (unsigned __int128)(100000000ull + 0xFFFFFF00ull + 0x200);
        CHK(mclock_millis(&m) == (uint32_t)(c * 1000 / 100000000u) && mclock_micros(&m) == (uint32_t)(c * 1000000 / 100000000u));
        /* 1 MHz 아닌 값: 33.333333 MHz */
        mclock_t n; memset(tcr, 0, sizeof tcr);
        CHK(mclock_start(&n, 1, 33333333) == BIO_OK);
        adv(1, 33333333); CHK(mclock_millis(&n) == 1000);
        adv(1, 16666666); CHK(mclock_millis(&n) == 1500 - 0 || mclock_millis(&n) == 1499);   /* (33333333+16666666)/33333333 s = 1.49999998 s */
        CHK(mclock_millis(&n) == 1499 && mclock_micros(&n) == 1499999);
        /* 큰 값에서 32비트 되돌아감 */
        mclock_t w; memset(tcr, 0, sizeof tcr); mclock_start(&w, 2, 1000);        /* 1 kHz → 1 클럭 = 1 ms */
        adv(2, 0x7FFFFFFFu); mclock_cycles(&w); adv(2, 0x7FFFFFFFu); mclock_cycles(&w); adv(2, 5);   /* 2^32 + 3 클럭, 바퀴 안에서 호출 */
        CHK(mclock_millis(&w) == 3u && w.cycles == 0xFFFFFFFEull + 5);            /* 1 kHz: 클럭 수 = ms, 32비트로는 3 */
        adv(2, 0x7FFFFFFFu); adv(2, 0x7FFFFFFFu); adv(2, 5);                      /* 한 바퀴 넘게 호출 안 하면 한 바퀴를 잃음(문서화된 한계) */
        CHK(mclock_cycles(&w) == 0xFFFFFFFEull + 5 + 3);
    }
    /* ---- 비차단 타이머 ---- */
    {
        ticker_t t;
        CHK(ticker_init(&t, 0, 0) == BIO_ERR_ARGS && ticker_init(0, 5, 0) == BIO_ERR_ARGS);
        CHK(ticker_init(&t, 10, 0) == BIO_OK);
        CHK(!ticker_due(&t, 0) && !ticker_due(&t, 9) && ticker_due(&t, 10) && !ticker_due(&t, 10) && !ticker_due(&t, 19) && ticker_due(&t, 20));
        CHK(ticker_due(&t, 35) && t.last == 30);                   /* 1.5 주기 늦음: 기준을 한 주기만 전진(오차 누적 없음) */
        CHK(!ticker_due(&t, 39) && ticker_due(&t, 40) && t.last == 40);
        CHK(ticker_due(&t, 65) && t.last == 65);                   /* 2.5 주기 늦음 → 밀린 것을 버리고 지금으로 재정렬 */
        CHK(!ticker_due(&t, 74) && ticker_due(&t, 75) && t.last == 75);
        CHK(ticker_due(&t, 85) && t.last == 85 && ticker_due(&t, 95));
        CHK(ticker_due(&t, 115) && t.last == 115);                 /* 정확히 2 주기 늦음도 재정렬 */
        ticker_t u; ticker_init(&u, 0x20, 0xFFFFFFF0u);
        CHK(!ticker_due(&u, 0x0000000Fu) && ticker_due(&u, 0x10) && u.last == 0x10 && !ticker_due(&u, 0x2F) && ticker_due(&u, 0x30));
        ticker_t one; ticker_init(&one, 1, 100);
        CHK(!ticker_due(&one, 100) && ticker_due(&one, 101) && ticker_due(&one, 102) && ticker_due(&one, 110));
    }
    /* ---- 버튼 ---- */
    {
        button_t b;
        CHK(button_init(&b, 20, 1) == BIO_OK && button_init(0, 1, 1) == BIO_ERR_ARGS);
        CHK(button_update(&b, 1, 0) == BTN_EV_NONE && !button_down(&b));                /* 풀업이라 1 = 안 눌림, 첫 호출은 기준 */
        /* 채터링: 0/1 번갈아 → 안정될 때까지 이벤트 없음 */
        int ev = 0; uint32_t t = 1;
        int bounce[] = { 0, 1, 0, 1, 0, 0, 1, 0 };
        for (unsigned i = 0; i < sizeof bounce / sizeof *bounce; i++, t += 3) ev |= button_update(&b, bounce[i], t);
        CHK(ev == 0 && !button_down(&b) && b.presses == 0);
        /* 마지막 샘플부터 0 이 안정 → 20 ms 뒤 눌림 */
        uint32_t t0 = t - 3;                                                              /* 마지막 변화(값 0) 시각: 이 시점부터 후보가 유지됨 */
        CHK(button_update(&b, 0, t0 + 19) == BTN_EV_NONE);
        CHK(button_update(&b, 0, t0 + 20) == BTN_EV_PRESS && button_down(&b) && b.presses == 1);
        CHK(button_update(&b, 0, t0 + 21) == BTN_EV_NONE && b.presses == 1);              /* 같은 눌림에서 다시 세지 않음 */
        CHK(button_held_ms(&b, t0 + 120) == 100);
        /* 짧은 글리치(값 1 이 10 ms)는 놓은 것으로 안 봄 */
        CHK(button_update(&b, 1, t0 + 130) == BTN_EV_NONE && button_down(&b) && button_update(&b, 1, t0 + 139) == BTN_EV_NONE && button_update(&b, 0, t0 + 140) == BTN_EV_NONE && button_down(&b));
        CHK(button_update(&b, 0, t0 + 200) == BTN_EV_NONE && b.releases == 0);
        /* 놓음 */
        CHK(button_update(&b, 1, t0 + 300) == BTN_EV_NONE && button_update(&b, 1, t0 + 319) == BTN_EV_NONE);
        CHK(button_update(&b, 1, t0 + 320) == BTN_EV_RELEASE && !button_down(&b) && b.releases == 1 && b.presses == 1 && button_held_ms(&b, t0 + 400) == 0);
        /* 두 번째 눌림 */
        button_update(&b, 0, t0 + 400); CHK(button_update(&b, 0, t0 + 420) == BTN_EV_PRESS && b.presses == 2);
        /* 극성: active-high, debounce 0 */
        button_t h; button_init(&h, 0, 0);
        CHK(button_update(&h, 0, 5) == BTN_EV_NONE);
        CHK(button_update(&h, 1, 6) == BTN_EV_PRESS && button_update(&h, 0, 7) == BTN_EV_RELEASE && h.presses == 1 && h.releases == 1);
        CHK(button_update(&h, 2, 8) == BTN_EV_PRESS);                                       /* 0 이 아닌 값은 모두 참 */
        /* 시간 되돌아감 */
        button_t r; button_init(&r, 20, 0); button_update(&r, 0, 0xFFFFFFF0u); button_update(&r, 1, 0xFFFFFFF8u);
        CHK(button_update(&r, 1, 0xFFFFFFF8u + 19) == BTN_EV_NONE && button_update(&r, 1, 0xFFFFFFF8u + 20) == BTN_EV_PRESS);
        CHK(button_held_ms(&r, 0xFFFFFFF8u + 20 + 50) == 50);
        /* 시작 시 이미 눌려 있으면 이벤트 없이 눌린 상태 */
        button_t s; button_init(&s, 20, 1); CHK(button_update(&s, 0, 100) == BTN_EV_NONE && button_down(&s) && s.presses == 0);
    }
    /* ---- RGB ---- */
    {
        rgb_t r;
        CHK(rgb_init(&r, 1, 2, 3, 255000, 1000, 0) == BIO_ERR_ARGS && rgb_init(&r, 1, 2, 3, 100000000, 0, 0) == BIO_ERR_ARGS && rgb_init(&r, 1, 2, 3, 100000, 1000, 0) == BIO_ERR_ARGS && rgb_init(0, 1, 2, 3, 100000000, 1000, 0) == BIO_ERR_ARGS);
        CHK(rgb_init(&r, 1, 2, 3, 100000000, 1000, 0) == BIO_OK);
        CHK(rgb_set(&r, 255, 0, 128) == BIO_OK);
        CHK(duty(1) == 1.0 && duty(2) == 0.0);
        CHK(tlr0[3] == 100000 - 2 && tcsr1[3] & TCSR_ENALL);
        CHK(((long)(tlr1[3] + 2) * 255 - 100000L * 128 <= 127) && ((long)(tlr1[3] + 2) * 255 - 100000L * 128 >= -127));
        CHK(rgb_set(&r, 1, 254, 0) == BIO_OK);                                        /* 1/255 → 약 392 클럭 */
        CHK(tlr1[1] + 2 > 380 && tlr1[1] + 2 < 400 && tlr0[1] == 100000 - 2);
        CHK(tlr1[2] + 2 > 99600 && tlr1[2] + 2 < 99620 && duty(3) == 0.0);
        CHK(rgb_set(&r, 127, 127, 127) == BIO_OK && ((long)(tlr1[1] + 2) * 255 - 100000L * 127 <= 127) && ((long)(tlr1[1] + 2) * 255 - 100000L * 127 >= -127));   /* 반올림: 49803.9 → 49804 */
        /* 공통 애노드: 반대 */
        rgb_t a; rgb_init(&a, 1, 2, 3, 100000000, 1000, 1);
        CHK(rgb_set(&a, 255, 0, 128) == BIO_OK && duty(1) == 0.0 && duty(2) == 1.0);
        CHK(((long)(tlr1[3] + 2) * 255 - 100000L * 127 <= 127) && ((long)(tlr1[3] + 2) * 255 - 100000L * 127 >= -127));    /* 100000 - 51373 ≈ 48627 → 127/255 */
        /* 아주 짧은 하이 시간 보정 */
        rgb_t s; rgb_init(&s, 1, 2, 3, 256 * 1000, 1000, 0);                           /* 주기 256 클럭 */
        CHK(rgb_set(&s, 1, 255, 254) == BIO_OK);                                       /* 256/255 ≈ 1 → 2 로 올림, 254 → 255 클럭(=period-1) → period-2 로 */
        CHK(tlr1[1] + 2 == 2 && tlr1[3] + 2 <= 254 && tlr1[3] + 2 >= 253);
        CHK(bad_access == 0);
        /* HSV */
        uint8_t o[3];
        rgb_from_hsv(0, 255, 255, o);   CHK(o[0] == 255 && o[1] == 0 && o[2] == 0);
        rgb_from_hsv(60, 255, 255, o);  CHK(o[0] == 255 && o[1] == 255 && o[2] == 0);
        rgb_from_hsv(120, 255, 255, o); CHK(o[0] == 0 && o[1] == 255 && o[2] == 0);
        rgb_from_hsv(180, 255, 255, o); CHK(o[0] == 0 && o[1] == 255 && o[2] == 255);
        rgb_from_hsv(240, 255, 255, o); CHK(o[0] == 0 && o[1] == 0 && o[2] == 255);
        rgb_from_hsv(300, 255, 255, o); CHK(o[0] == 255 && o[1] == 0 && o[2] == 255);
        rgb_from_hsv(359, 255, 255, o); CHK(o[0] == 255 && o[1] == 0 && o[2] == 5);
        rgb_from_hsv(720 + 120, 255, 255, o); CHK(o[0] == 0 && o[1] == 255 && o[2] == 0);   /* 360 으로 나머지 */
        rgb_from_hsv(90, 0, 200, o);    CHK(o[0] == 200 && o[1] == 200 && o[2] == 200);     /* 채도 0 = 회색 */
        rgb_from_hsv(30, 255, 100, o);  CHK(o[0] == 100 && o[1] == 49 && o[2] == 0);        /* 어두운 주황: 100·(255-128)/255 = 49.8 → 버림 49 */
        rgb_from_hsv(200, 128, 255, o); CHK(o[1] > o[0] && o[2] == 255 && o[0] == 127);     /* 채도 절반: 최소 채널 = v·(255-s)/255 */
        rgb_from_hsv(0, 255, 0, o);     CHK(o[0] == 0 && o[1] == 0 && o[2] == 0);
        /* 전 범위: 독립적인 실수 계산과 ±2 이내 (정수 내림 오차) */
        int worst = 0;
        static const unsigned sv[][2] = { {255, 255}, {128, 255}, {255, 100}, {64, 200}, {0, 90} };
        for (unsigned h = 0; h < 360; h++) for (unsigned k = 0; k < 5; k++) {
            double S = sv[k][0] / 255.0, V = sv[k][1] / 255.0, hh = h / 60.0; int i = (int)hh; double f = hh - i;
            double p = V * (1 - S), q = V * (1 - S * f), t = V * (1 - S * (1 - f)), rr, gg, bb;
            switch (i) { case 0: rr = V; gg = t; bb = p; break; case 1: rr = q; gg = V; bb = p; break; case 2: rr = p; gg = V; bb = t; break;
                         case 3: rr = p; gg = q; bb = V; break; case 4: rr = t; gg = p; bb = V; break; default: rr = V; gg = p; bb = q; break; }
            rgb_from_hsv(h, (uint8_t)sv[k][0], (uint8_t)sv[k][1], o);
            double e[3] = { rr * 255, gg * 255, bb * 255 };
            for (int c = 0; c < 3; c++) { int d = (int)o[c] - (int)(e[c] + 0.5); if (d < 0) d = -d; if (d > worst) worst = d; }
        }
        CHK(worst <= 2);
    }
    printf(fails ? "FAILED %d\n" : "ALL PASS (basic_io model)\n", fails);
    return fails != 0;
}
