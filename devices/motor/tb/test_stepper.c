/* 28BYJ-48 스테퍼 드라이버 호스트 시험 (교재 27장).
 * 빌드: cc -include gpio/tb/mock_gpio.h -I gpio/sw -I motor/sw motor/tb/test_stepper.c motor/sw/stepper.c gpio/sw/axi_gpio.c -o /tmp/stepper
 *
 * 한계(중요): 모의 AXI GPIO 와 "로터 모델"은 내가 문서(Stepper.cpp 사본, CheapStepper README)를 읽고 만든 것이라, 그 해석이 틀리면 시험도 같이 틀린다.
 * 기대 코일 순서는 드라이버의 표(헤더 상수)를 쓰지 않고 이 파일에서 "A-AB-B-BC-C-CD-D-DA" 문자열(CheapStepper)과 Stepper.cpp 의 비트열(1010,0110,0101,1001)을
 * 핀 순서 (1,3,2,4) 로 바꿔 독립적으로 만든다. 로터 모델은 "상태 사이 이동이 반 스텝 단위(1·2칸)를 넘지 않는다"만 본다. 실제 모터는 시험하지 않았다. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "../sw/stepper.h"

/* ---- 모의 AXI GPIO (채널 1개) + 코일 관찰 ---- */
static uint32_t g_data, g_tri;
static int g_violation, g_writes;
static unsigned SH;                     /* 시험이 정한 IN1 비트 위치 */
static uint32_t other_expect;           /* 코일 4비트 밖의 출력 비트 기대값(건드리면 안 됨) */
static uint32_t other_mask_out;         /* 코일 밖에서 출력으로 설정한 비트 */
static int rot_known, rot_last, rot_energized;
static long rot_half;                   /* 로터가 간 반 스텝 수(부호 있음) */
static int illegal_pat, bad_jump, other_touched;
static int allowed_max = 2;             /* 허용 최대 이동(반 스텝 단위) */
static uint32_t EXP[8];                 /* 독립적으로 만든 8단계 기대 코일 (비트0 = IN1) */
static long wcount;

uint32_t mock_g_rd(uint32_t off)
{
    if (off == 0x0000) return 0;                    /* PG144: 출력으로 설정된 비트를 읽으면 0 */
    if (off == 0x0004) return g_tri;
    g_violation++; return 0;
}
static void observe(uint32_t data)
{
    wcount++;
    if ((data & ~(0xFu << SH) & other_mask_out) != other_expect) other_touched++;
    uint32_t v = (data >> SH) & 0xF;
    if (v == 0) { rot_energized = 0; return; }
    int j = -1;
    for (int k = 0; k < 8; k++) if (EXP[k] == v) j = k;
    if (j < 0) { illegal_pat++; return; }
    if (rot_known) {
        int d = (j - rot_last + 8) % 8; if (d > 4) d -= 8;       /* -3..4 */
        if (d == 4 || d > allowed_max || d < -allowed_max) bad_jump++;
        else rot_half += d;
    }
    rot_known = 1; rot_last = j; rot_energized = 1;
}
void mock_g_wr(uint32_t off, uint32_t v)
{
    if (off == 0x0000) { g_data = v; g_writes++; observe(v); }
    else if (off == 0x0004) g_tri = v;
    else g_violation++;
}

static int fails;
#define CHK(c) do { if (!(c)) { fails++; printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)

/* 독립 기대표 만들기 */
static void build_expected(void)
{
    const char *seq = "A-AB-B-BC-C-CD-D-DA";             /* CheapStepper README */
    int k = 0; uint32_t m = 0;
    for (const char *p = seq;; p++) {
        if (*p == '-' || *p == 0) { EXP[k++] = m; m = 0; if (!*p) break; }
        else m |= 1u << (*p - 'A');                       /* A..D = IN1..IN4 (설계안) */
    }
    CHK(k == 8);
}

static gpio_t gp; static stepper_t st;
static void setup(unsigned shift, uint32_t other_out)
{
    memset(&gp, 0, sizeof gp); memset(&st, 0, sizeof st);
    g_data = 0; g_tri = 0xFFFFFFFFu; g_violation = 0; g_writes = 0; wcount = 0;
    SH = shift; rot_known = 0; rot_half = 0; illegal_pat = bad_jump = other_touched = 0; allowed_max = 2;
    other_mask_out = 0; other_expect = other_out ? (0xA5A5A5A5u & other_out) : 0;
    gpio_init(&gp, 0, 1);
    for (unsigned b = 0; b < 32; b++)
        if (other_out & (1u << b)) gpio_pin_mode(&gp, 0, b, 1, (other_expect >> b) & 1);
    other_mask_out = other_out; other_touched = 0; wcount = 0; g_writes = 0; rot_known = 0;
    CHK(stepper_init(&st, &gp, 0, shift) == STEPPER_OK);
}
static int coil_now(void) { return (int)((g_data >> SH) & 0xF); }
static void rot_reset(void) { rot_half = 0; bad_jump = illegal_pat = 0; }     /* 로터의 마지막 위치는 유지 */

/* ---- 시험들 ---- */
static void t_tables(void)
{
    build_expected();
    /* Stepper.cpp 4선 모드: 핀1..4 에 1010,0110,0101,1001. 관례(8,10,9,11)로 핀1=IN1, 핀2=IN3, 핀3=IN2, 핀4=IN4 */
    const char *lib[4] = { "1010", "0110", "0101", "1001" };
    const int pin_to_in[4] = { 0, 2, 1, 3 };
    for (int s = 0; s < 4; s++) {
        uint32_t m = 0;
        for (int p = 0; p < 4; p++) if (lib[s][p] == '1') m |= 1u << pin_to_in[p];
        CHK(m == EXP[2 * s + 1]);                          /* 라이브러리 case s = AB,BC,CD,DA */
    }
    for (unsigned i = 0; i < 8; i++) CHK(stepper_pattern(i) == EXP[i]);
    CHK(stepper_pattern(8) == EXP[0] && stepper_pattern(15) == EXP[7]);
    /* 인접 상태는 코일 하나만 바뀐다(풀: 두 상태 사이 한 칸씩) */
    for (int k = 0; k < 8; k++) { uint32_t x = EXP[k] ^ EXP[(k + 1) % 8]; CHK(__builtin_popcount(x) == 1); }
}

static void t_init(void)
{
    setup(4, 0x0000000Fu);
    CHK(coil_now() == 0); CHK((g_tri & (0xFu << 4)) == 0);          /* 4핀 출력 */
    CHK((g_tri & 0xFu) == 0);                                          /* 다른 출력은 그대로 */
    CHK(st.pos == 0 && st.remaining == 0 && !st.energized && st.interval_us == 0);
    CHK(st.max_hz == 600 && st.gear_micro == 64000000u && st.mode == STEPPER_FULL);
    CHK(g_violation == 0 && other_touched == 0 && illegal_pat == 0);
    gpio_t g1; stepper_t s2;
    gpio_init(&g1, 0, 1);
    CHK(stepper_init(NULL, &g1, 0, 0) == STEPPER_ERR_ARGS);
    CHK(stepper_init(&s2, NULL, 0, 0) == STEPPER_ERR_ARGS);
    CHK(stepper_init(&s2, &g1, 2, 0) == STEPPER_ERR_ARGS);
    CHK(stepper_init(&s2, &g1, 1, 0) == STEPPER_ERR_ARGS);            /* 1채널 IP 에서 채널 2 */
    CHK(stepper_init(&s2, &g1, 0, 29) == STEPPER_ERR_ARGS);
    CHK(stepper_init(&s2, &g1, 0, 28) == STEPPER_OK);                  /* 비트 28..31 */
    CHK(g_violation == 0 && (g_tri & 0xF0000000u) == 0);
}

static void t_sequences(void)
{
    for (int mode = 0; mode < 3; mode++) for (unsigned shift = 0; shift <= 28; shift += 4) {
        setup(shift, shift == 0 ? 0xF0u : 0x0Fu);
        CHK(stepper_set_mode(&st, (stepper_mode_t)mode) == STEPPER_OK);
        /* 첫 스텝은 정렬 때문에 반 칸일 수 있다 → 먼저 한 스텝 내고 이후 표와 비교 */
        CHK(stepper_step(&st, 1) == STEPPER_OK);
        int idx = -1; for (int k = 0; k < 8; k++) if (EXP[k] == (uint32_t)coil_now()) idx = k;
        CHK(idx >= 0);
        if (mode == STEPPER_WAVE) CHK(idx % 2 == 0); else if (mode == STEPPER_FULL) CHK(idx % 2 == 1);
        int inc = mode == STEPPER_HALF ? 1 : 2;
        for (int n = 0; n < 40; n++) {                                  /* 정방향 5 회전 */
            idx = (idx + inc) % 8;
            CHK(stepper_step(&st, 1) == STEPPER_OK);
            CHK(coil_now() == (int)EXP[idx]);
        }
        for (int n = 0; n < 40; n++) {                                  /* 역방향 */
            idx = (idx - inc + 8) % 8;
            CHK(stepper_step(&st, -1) == STEPPER_OK);
            CHK(coil_now() == (int)EXP[idx]);
        }
        CHK(st.pos == 1);                                               /* +1 +40 -40 */
        CHK(illegal_pat == 0 && bad_jump == 0 && other_touched == 0 && g_violation == 0);
        CHK(st.energized == 1);
    }
    /* 한 바퀴 합계: 풀 4스텝 = 모터 전기각 한 주기 = 8 반 스텝 */
    setup(0, 0);
    stepper_set_mode(&st, STEPPER_FULL); stepper_step(&st, 1); rot_reset();
    for (int n = 0; n < 4; n++) stepper_step(&st, 1);
    CHK(rot_half == 8);
    stepper_set_mode(&st, STEPPER_HALF); stepper_step(&st, 1); rot_reset();
    for (int n = 0; n < 8; n++) stepper_step(&st, -1);
    CHK(rot_half == -8);
    /* dir == 0 은 오류, 아무것도 쓰지 않는다 */
    long w = wcount; CHK(stepper_step(&st, 0) == STEPPER_ERR_ARGS && wcount == w);
    CHK(stepper_step(NULL, 1) == STEPPER_ERR_ARGS);
}

static void t_mode_switch(void)
{
    /* 모든 시작 위치에서 모드를 바꿔도 코일은 최대 ±2 칸, 정렬 후 표대로 */
    for (int start = 0; start < 8; start++) for (int to = 0; to < 3; to++) for (int dir = -1; dir <= 1; dir += 2) {
        setup(0, 0);
        stepper_set_mode(&st, STEPPER_HALF);
        stepper_step(&st, 1);
        for (int k = 0; k < start; k++) stepper_step(&st, 1);
        rot_reset();
        int before = coil_now();
        CHK(stepper_set_mode(&st, (stepper_mode_t)to) == STEPPER_OK);
        CHK(coil_now() == before);                                       /* 모드 변경만으로는 코일이 안 바뀐다 */
        CHK(st.pos == 0 && st.interval_us == 0);
        CHK(stepper_step(&st, dir) == STEPPER_OK);
        int idx = -1; for (int k = 0; k < 8; k++) if (EXP[k] == (uint32_t)coil_now()) idx = k;
        if (to == STEPPER_WAVE) CHK(idx % 2 == 0);
        if (to == STEPPER_FULL) CHK(idx % 2 == 1);
        CHK(bad_jump == 0 && illegal_pat == 0);
        long first = rot_half;                                           /* 정렬이 필요하면 ±1, 아니면 ±(1 또는 2) */
        CHK(labs(first) >= 1 && labs(first) <= 2 && (first > 0) == (dir > 0));
        if (to != STEPPER_HALF) {
            int e1 = (start & 1) == to;                                  /* 이미 맞는 짝/홀이면 정상 2칸 */
            CHK(labs(first) == (e1 ? 2 : 1));
        }
        CHK(stepper_step(&st, dir) == STEPPER_OK);
        CHK(labs(rot_half - first) == (to == STEPPER_HALF ? 1 : 2));
    }
    setup(0, 0);
    CHK(stepper_set_mode(&st, (stepper_mode_t)3) == STEPPER_ERR_ARGS);
    CHK(stepper_set_mode(NULL, STEPPER_FULL) == STEPPER_ERR_ARGS);
    stepper_set_speed_mrpm(&st, 10000); stepper_move(&st, 5, 0);
    CHK(stepper_set_mode(&st, STEPPER_HALF) == STEPPER_ERR_BUSY);
    CHK(stepper_set_gear_micro(&st, 63683950u) == STEPPER_ERR_BUSY);
}

static uint32_t ref_interval(unsigned mode, double gear, uint32_t mrpm)
{
    double spr = (mode == STEPPER_HALF ? 64.0 : 32.0) * gear;
    return (uint32_t)floor(60e6 / (spr * mrpm / 1000.0) + 0.5);
}

static void t_speed(void)
{
    setup(0, 0);
    CHK(st.interval_us == 0);
    CHK(stepper_set_speed_mrpm(&st, 10000) == STEPPER_OK);              /* 10 rpm, 풀 2048 스텝: 60e6/(2048*10) = 2929.69 */
    CHK(st.interval_us == 2930);
    stepper_set_mode(&st, STEPPER_HALF);
    CHK(st.interval_us == 0);                                            /* 모드 변경은 속도를 지움 */
    CHK(stepper_set_speed_mrpm(&st, 15000) == STEPPER_ERR_SPEED);       /* 하프 4096 × 15 rpm = 1024 Hz > 기본 한계 600 Hz */
    CHK(stepper_set_speed_mrpm(&st, 8000) == STEPPER_OK);               /* 4096 × 8 / 60 = 546 Hz: 60e6/(4096×8) = 1831.05 */
    CHK(st.interval_us == 1831);
    CHK(stepper_set_gear_micro(&st, STEPPER_GEAR_MEASURED_MICRO) == STEPPER_OK);
    CHK(st.interval_us == 0);
    CHK(stepper_set_speed_mrpm(&st, 8000) == STEPPER_OK);               /* 4075.77 × 8 */
    CHK(st.interval_us == ref_interval(STEPPER_HALF, 63.68395, 8000));
    /* 여러 조합을 독립 공식(double)과 비교 */
    int checked = 0;
    for (int mode = 0; mode < 3; mode++) for (int gi = 0; gi < 3; gi++) {
        double gears[3] = { 64.0, 63.68395, 1.0 };
        for (uint32_t mrpm = 1; mrpm <= 1000000u; mrpm = mrpm < 50 ? mrpm + 1 : mrpm * 3 / 2 + 7) {
            setup(0, 0);
            stepper_set_mode(&st, (stepper_mode_t)mode);
            stepper_set_gear_micro(&st, (uint64_t)llround(gears[gi] * 1e6));
            stepper_set_max_hz(&st, 1000);
            double spr = (mode == STEPPER_HALF ? 64.0 : 32.0) * gears[gi];
            double iv = 60e6 / (spr * mrpm / 1000.0);
            int r = stepper_set_speed_mrpm(&st, mrpm);
            uint32_t ref = iv > 4294967295.0 ? 0 : (uint32_t)floor(iv + 0.5);
            int want_ok = ref != 0 && (uint64_t)ref * 1000 >= 1000000u && iv <= 4294967295.0;
            if (ref >= 999 && ref <= 1001) continue;                      /* 한계 경계: 반올림 방식 차이로 1 µs 어긋날 수 있어 제외 */
            if (want_ok) { CHK(r == STEPPER_OK); if (r == STEPPER_OK) { int64_t d = (int64_t)st.interval_us - ref; CHK(d >= -1 && d <= 1); } }
            else CHK(r == STEPPER_ERR_SPEED);
            checked++;
        }
    }
    CHK(checked > 300);
    /* 한계 주파수 경계: 간격이 ceil(1e6/hz) 이상이면 허용 */
    for (uint32_t hz = 1; hz <= 1000; hz += (hz < 20 ? 1 : 37)) {
        for (uint32_t iv_target = 900; iv_target < 1200000; iv_target += 997) {
            setup(0, 0);
            CHK(stepper_set_max_hz(&st, hz) == STEPPER_OK);
            /* 풀 모드 2048 스텝: mrpm = 60e9/(2048*iv) */
            uint32_t mrpm = (uint32_t)llround(60e9 / (2048.0 * iv_target));
            if (mrpm == 0 || mrpm > 1000000u) continue;
            int r = stepper_set_speed_mrpm(&st, mrpm);
            uint32_t got = (uint32_t)floor(60e6 / (2048.0 * mrpm / 1000.0) + 0.5);
            int ok = (uint64_t)got * hz >= 1000000u;
            CHK((r == STEPPER_OK) == ok);
            if (r == STEPPER_OK) CHK(st.interval_us == got); else CHK(st.interval_us == 0);
        }
    }
    /* 오류 입력 */
    setup(0, 0);
    CHK(stepper_set_speed_mrpm(&st, 0) == STEPPER_ERR_SPEED);
    CHK(stepper_set_speed_mrpm(&st, 1000001u) == STEPPER_ERR_SPEED);
    CHK(stepper_set_speed_mrpm(NULL, 1) == STEPPER_ERR_ARGS);
    CHK(stepper_set_max_hz(&st, 0) == STEPPER_ERR_ARGS && stepper_set_max_hz(&st, 1001) == STEPPER_ERR_ARGS && stepper_set_max_hz(NULL, 5) == STEPPER_ERR_ARGS);
    CHK(stepper_set_gear_micro(&st, 999999u) == STEPPER_ERR_ARGS && stepper_set_gear_micro(&st, 1000000001u) == STEPPER_ERR_ARGS);
    CHK(stepper_set_gear_micro(&st, 1000000u) == STEPPER_OK && stepper_set_gear_micro(&st, 1000000000u) == STEPPER_OK);
    CHK(stepper_set_gear_micro(NULL, 64000000u) == STEPPER_ERR_ARGS);
    /* 설정된 속도가 새 한계를 넘으면 한계를 바꾸지 않는다 */
    setup(0, 0);
    stepper_set_speed_mrpm(&st, 17000);                                  /* 2048*17/60 = 580 Hz */
    CHK(st.interval_us != 0);
    CHK(stepper_set_max_hz(&st, 500) == STEPPER_ERR_SPEED && st.max_hz == 600);
    CHK(stepper_set_max_hz(&st, 700) == STEPPER_OK && st.max_hz == 700);
    /* 큰 속도·감속비 곱에서 64비트 넘침 없음(UBSan) */
    setup(0, 0);
    stepper_set_gear_micro(&st, 1000000000u); stepper_set_mode(&st, STEPPER_HALF);
    CHK(stepper_set_speed_mrpm(&st, 1000000u) == STEPPER_ERR_SPEED);
    CHK(stepper_set_speed_mrpm(&st, 1) == STEPPER_OK);
    CHK(stepper_steps_per_rev_micro(&st) == 64ull * 1000000000ull);
}

/* 시계 모의: 1 µs 단위로 poll 하며 각 스텝 시각을 기록 */
static uint32_t stamps[5000]; static int nst;
static void run_poll(uint32_t start, uint32_t span, uint32_t gran)
{
    nst = 0;
    for (uint32_t t = 0; t <= span; t += gran) {
        int before = (int)wcount;
        int r = stepper_poll(&st, start + t);
        CHK(r == 0 || r == 1);
        CHK(((int)wcount - before) == r);                                /* 한 번의 poll 은 DATA 쓰기 r 번 */
        if (r && nst < 5000) stamps[nst++] = start + t;
    }
}
static void t_motion(void)
{
    static const uint32_t starts[] = { 0u, 1000u, 0xFFFFF000u, 0xFFFFFFFFu, 0x7FFFFFF0u };
    for (unsigned si = 0; si < sizeof starts / sizeof *starts; si++) {
        uint32_t t0 = starts[si];
        setup(0, 0);
        stepper_set_speed_mrpm(&st, 10000);                              /* 간격 2930 µs */
        uint32_t iv = st.interval_us;
        CHK(stepper_move(&st, 100, t0) == STEPPER_OK);
        CHK(stepper_remaining(&st) == 100);
        run_poll(t0, iv * 120u, 1);
        CHK(nst == 100);
        CHK(stamps[0] == t0);                                            /* 첫 스텝은 즉시 */
        for (int k = 1; k < nst; k++) CHK(stamps[k] - stamps[k - 1] == iv);   /* 1 µs 해상도에서 정확히 간격 */
        CHK(stepper_remaining(&st) == 0 && st.pos == 100);
        CHK(stepper_poll(&st, t0 + iv * 1000u) == 0);
        /* 역방향 */
        CHK(stepper_move(&st, -30, t0 + iv * 1000u) == STEPPER_OK);
        run_poll(t0 + iv * 1000u, iv * 40u, 1);
        CHK(nst == 30 && st.pos == 70);
        CHK(illegal_pat == 0 && bad_jump == 0);
    }
    /* 거친 poll 간격(간격의 1/3 ~ 지터): 한 번에 한 스텝, 장기 평균 속도 유지(누적 오차 없음) */
    setup(0, 0);
    stepper_set_speed_mrpm(&st, 10000);
    uint32_t iv = st.interval_us;
    stepper_move(&st, 500, 0);
    uint32_t t = 0; unsigned seed = 12345; int steps = 0; uint32_t last_stamp = 0;
    while (stepper_remaining(&st) != 0 && t < iv * 1000u) {
        seed = seed * 1103515245u + 12345u;
        t += 1 + (seed >> 16) % (iv / 2);                                  /* 지터가 간격 절반 미만 */
        int r = stepper_poll(&st, t);
        steps += r;
        if (r) { CHK(t >= (uint32_t)(steps - 1) * iv); last_stamp = t; }
    }
    CHK(steps == 500);
    CHK(last_stamp < 499u * iv + iv / 2);                                  /* 마지막 스텝이 이상 시각(499*iv)에서 지터(간격 절반) 이상 늦지 않음 */
    /* 오래 멈췄다 poll: 몰아치지 않음 */
    setup(0, 0);
    stepper_set_speed_mrpm(&st, 10000); iv = st.interval_us;
    stepper_move(&st, 50, 0);
    CHK(stepper_poll(&st, 0) == 1);
    CHK(stepper_poll(&st, iv * 20u) == 1);                                  /* 20 간격 늦음 → 한 스텝 */
    CHK(stepper_poll(&st, iv * 20u) == 0);                                  /* 같은 시각에 또 호출해도 안 나감 */
    CHK(stepper_poll(&st, iv * 20u + iv - 1) == 0);
    CHK(stepper_poll(&st, iv * 21u) == 1);                                  /* 지금부터 한 간격 뒤 */
    CHK(stepper_remaining(&st) == 47);
    /* 약간(간격 미만) 늦은 경우는 기준을 유지 */
    setup(0, 0);
    stepper_set_speed_mrpm(&st, 10000); iv = st.interval_us;
    stepper_move(&st, 10, 0);
    stepper_poll(&st, 0); CHK(stepper_poll(&st, iv + 100) == 1);
    CHK(stepper_poll(&st, 2 * iv - 1) == 0 && stepper_poll(&st, 2 * iv) == 1);   /* 기준 시각 2*iv 유지 */
}

static void t_move_errors(void)
{
    setup(0, 0);
    CHK(stepper_move(&st, 10, 0) == STEPPER_ERR_SPEED);                    /* 속도 미설정 */
    stepper_set_speed_mrpm(&st, 5000);
    CHK(stepper_move(&st, 0, 0) == STEPPER_OK && stepper_remaining(&st) == 0);
    CHK(stepper_move(&st, 10, 0) == STEPPER_OK);
    CHK(stepper_move(&st, 5, 0) == STEPPER_ERR_BUSY && stepper_remaining(&st) == 10);
    stepper_poll(&st, 0);
    stepper_stop(&st);
    CHK(stepper_remaining(&st) == 0 && st.energized == 1);                  /* 정지해도 코일 유지 */
    CHK(stepper_poll(&st, 100000) == 0);
    CHK(stepper_move(&st, -7, 0) == STEPPER_OK);
    CHK(stepper_move(NULL, 1, 0) == STEPPER_ERR_ARGS);
    CHK(stepper_remaining(NULL) == 0);
    stepper_stop(NULL); stepper_set_release_on_done(NULL, 1);
    CHK(stepper_poll(NULL, 0) == 0);
    /* INT32_MIN 은 시작만 확인(끝까지 돌지 않음) */
    stepper_stop(&st);
    CHK(stepper_move(&st, INT32_MIN, 0) == STEPPER_OK);
    CHK(stepper_poll(&st, 0) == 1 && stepper_remaining(&st) == INT32_MIN + 1);
    stepper_stop(&st);
    CHK(stepper_move(&st, INT32_MAX, 0) == STEPPER_OK);
    CHK(stepper_poll(&st, 0) == 1 && stepper_remaining(&st) == INT32_MAX - 1);
    /* pos 넘침(부호 있는 정수 넘침 UB 없이 되돌아감) */
    stepper_stop(&st); st.pos = INT32_MAX;
    CHK(stepper_step(&st, 1) == STEPPER_OK && st.pos == INT32_MIN);
    CHK(stepper_step(&st, -1) == STEPPER_OK && st.pos == INT32_MAX);
}

static void t_degrees(void)
{
    struct { int mode; int32_t d10; int32_t steps; } c[] = {
        { STEPPER_FULL, 3600, 2048 }, { STEPPER_FULL, 1800, 1024 }, { STEPPER_FULL, 900, 512 }, { STEPPER_FULL, -900, -512 },
        { STEPPER_HALF, 3600, 4096 }, { STEPPER_HALF, 900, 1024 }, { STEPPER_FULL, 0, 0 },
        { STEPPER_FULL, 1, 1 },          /* 0.1° = 0.569 스텝 → 반올림 1 */
        { STEPPER_FULL, 2, 1 },          /* 1.138 → 1 */
        { STEPPER_FULL, 3, 2 },          /* 1.707 → 2 */
        { STEPPER_FULL, -3, -2 },
        { STEPPER_HALF, 1, 1 },          /* 1.138 → 1 */
        { STEPPER_WAVE, 3600, 2048 },
    };
    for (unsigned i = 0; i < sizeof c / sizeof *c; i++) {
        setup(0, 0);
        stepper_set_mode(&st, (stepper_mode_t)c[i].mode);
        stepper_set_speed_mrpm(&st, 1000);
        CHK(stepper_move_deg10(&st, c[i].d10, 0) == STEPPER_OK);
        CHK(stepper_remaining(&st) == c[i].steps);
    }
    /* 각도 정확도: 로터 모델의 반 스텝 수로 출력축 각도 계산 */
    int32_t tests[] = { 900, 3600, 450, 1234, -2700, 1 };
    for (unsigned i = 0; i < sizeof tests / sizeof *tests; i++) for (int mode = 0; mode < 3; mode++) {
        setup(0, 0);
        stepper_set_mode(&st, (stepper_mode_t)mode); stepper_step(&st, 1); stepper_step(&st, 1); rot_reset();
        stepper_set_speed_mrpm(&st, 1000);
        CHK(stepper_move_deg10(&st, tests[i], 0) == STEPPER_OK);
        for (uint32_t t = 0; stepper_remaining(&st) != 0; t += 1000) stepper_poll(&st, t);
        double out_deg = (double)rot_half * (360.0 / 64.0) / 64.0;        /* 반 스텝 = 모터 5.625° → 출력축은 64:1 감속 */
        double step_deg = (mode == STEPPER_HALF ? 1 : 2) * (360.0 / 64.0) / 64.0;
        CHK(fabs(out_deg - tests[i] / 10.0) <= step_deg / 2 + 1e-9);        /* 반올림 → 오차 ≤ 반 스텝 */
    }
    setup(0, 0);
    stepper_set_speed_mrpm(&st, 1000);
    CHK(stepper_move_deg10(&st, 3600001, 0) == STEPPER_ERR_ARGS && stepper_move_deg10(&st, -3600001, 0) == STEPPER_ERR_ARGS);
    CHK(stepper_move_deg10(&st, 3600000, 0) == STEPPER_OK && stepper_remaining(&st) == 2048000);
    stepper_stop(&st);
    stepper_set_gear_micro(&st, 1000000000u); stepper_set_mode(&st, STEPPER_HALF); stepper_set_speed_mrpm(&st, 1);
    CHK(stepper_move_deg10(&st, 3600000, 0) == STEPPER_OK && stepper_remaining(&st) == 64000000);
    CHK(stepper_move_deg10(NULL, 5, 0) == STEPPER_ERR_ARGS);
    /* 속도 미설정 */
    setup(0, 0);
    CHK(stepper_move_deg10(&st, 900, 0) == STEPPER_ERR_SPEED);
    /* 한 스텝보다 작은 각도는 0 스텝 */
    stepper_set_speed_mrpm(&st, 1000);
    CHK(stepper_move_deg10(&st, 0, 0) == STEPPER_OK && stepper_remaining(&st) == 0);
}

static void t_release_hold(void)
{
    setup(2, 0);
    stepper_set_speed_mrpm(&st, 10000);
    stepper_step(&st, 1); stepper_step(&st, 1);
    int c = coil_now(); CHK(c != 0);
    CHK(stepper_release(&st) == STEPPER_OK);
    CHK(coil_now() == 0 && !st.energized);
    CHK(stepper_hold(&st) == STEPPER_OK);
    CHK(coil_now() == c && st.energized);                                   /* 같은 상태로 복귀 */
    rot_reset();
    stepper_step(&st, 1);                                                   /* 이어서 한 칸(정상 2칸) */
    CHK(bad_jump == 0 && illegal_pat == 0);
    /* release_on_done */
    stepper_set_release_on_done(&st, 1);
    stepper_move(&st, 3, 0);
    for (uint32_t t = 0; t < 20000; t += 100) stepper_poll(&st, t);
    CHK(coil_now() == 0 && !st.energized && stepper_remaining(&st) == 0 && st.pos == 6);
    stepper_set_release_on_done(&st, 0);
    stepper_move(&st, 2, 0);
    for (uint32_t t = 0; t < 20000; t += 100) stepper_poll(&st, t);
    CHK(coil_now() != 0 && st.energized);
    /* release 는 이동도 취소 */
    stepper_move(&st, 9, 0); CHK(stepper_release(&st) == STEPPER_OK && stepper_remaining(&st) == 0);
    CHK(stepper_release(NULL) == STEPPER_ERR_ARGS && stepper_hold(NULL) == STEPPER_ERR_ARGS);
    CHK(g_violation == 0 && other_touched == 0);
}

/* 무작위 호출 시퀀스: 어떤 순서로 불러도 불법 코일 상태·큰 점프·다른 핀 변경이 없어야 한다 */
static void t_random(void)
{
    unsigned seed = 2026;
#define RND() (seed = seed * 1664525u + 1013904223u, seed >> 8)
    setup(8, 0x000000FFu);
    uint32_t now = 0; long ops = 0;
    for (int n = 0; n < 200000; n++) {
        unsigned r = RND() % 16;
        switch (r) {
        case 0: stepper_set_mode(&st, (stepper_mode_t)(RND() % 3)); break;
        case 1: stepper_set_speed_mrpm(&st, 1 + RND() % 30000); break;
        case 2: stepper_move(&st, (int32_t)(RND() % 200) - 100, now); break;
        case 3: stepper_move_deg10(&st, (int32_t)(RND() % 7200) - 3600, now); break;
        case 4: stepper_stop(&st); break;
        case 5: stepper_release(&st); break;
        case 6: stepper_hold(&st); break;
        case 7: stepper_step(&st, (RND() & 1) ? 1 : -1); break;
        case 8: stepper_set_max_hz(&st, 1 + RND() % 1000); break;
        case 9: stepper_set_gear_micro(&st, (RND() & 1) ? 64000000u : 63683950u); break;
        case 10: stepper_set_release_on_done(&st, RND() & 1); break;
        default: now += RND() % 3000; stepper_poll(&st, now); break;
        }
        ops++;
        if (st.remaining == 0 && st.interval_us && (RND() & 31) == 0) stepper_move(&st, (int32_t)(RND() % 50) - 25, now);
    }
    CHK(ops == 200000);
    CHK(illegal_pat == 0 && bad_jump == 0 && other_touched == 0 && g_violation == 0);
    CHK((g_tri & (0xFu << 8)) == 0 && (g_tri & 0xFFu) == 0);
}

/* 변형 검사에서 살아남았던 경계들을 직접 겨냥 */
static void t_edges(void)
{
    /* 다시 초기화하면 이동·위치·간격이 모두 지워진다 */
    setup(0, 0);
    stepper_set_speed_mrpm(&st, 5000); stepper_move(&st, 50, 0); stepper_poll(&st, 0); stepper_poll(&st, 100000);
    CHK(st.pos != 0 && st.remaining != 0 && st.interval_us != 0);
    st.next_us = 12345; st.mode = STEPPER_HALF; st.energized = 1;
    CHK(stepper_init(&st, &gp, 0, 0) == STEPPER_OK);
    CHK(st.pos == 0 && st.remaining == 0 && st.interval_us == 0 && st.next_us == 0 && st.mode == STEPPER_FULL && !st.energized);
    CHK(stepper_move(&st, 5, 0) == STEPPER_ERR_SPEED);
    /* 속도 한계 경계: 간격 × 한계 주파수 ≥ 1e6 일 때만 허용 (독립 실수 계산으로 대조) */
    static const uint32_t hz_list[] = { 1, 3, 7, 100, 250, 500, 600, 999, 1000 };
    long acc = 0, rej = 0;
    for (unsigned hi = 0; hi < sizeof hz_list / sizeof *hz_list; hi++) {
        uint32_t hz = hz_list[hi];
        for (int mode = STEPPER_WAVE; mode <= STEPPER_HALF; mode += (mode == STEPPER_WAVE ? 1 : 1)) {
            setup(0, 0); CHK(stepper_set_mode(&st, (stepper_mode_t)mode) == STEPPER_OK); CHK(stepper_set_max_hz(&st, hz) == STEPPER_OK);
            double spr = (mode == STEPPER_HALF ? 64.0 : 32.0) * 64.0;
            double mrpm_b = 6e10 / ((1e6 / hz) * spr);                    /* 경계에서의 milli-rpm (실수) */
            for (long d = -3; d <= 3; d++) {
                long mrpm = (long)mrpm_b + d; if (mrpm < 1) continue;
                double iv_exact = 6e10 / (spr * (double)mrpm);
                uint64_t iv = (uint64_t)(iv_exact + 0.5);
                int want = iv != 0 && (double)iv * hz >= 1e6;
                int rc = stepper_set_speed_mrpm(&st, (uint32_t)mrpm);
                if ((rc == STEPPER_OK) != want) { fails++; printf("FAIL rate boundary hz=%u mode=%d mrpm=%ld iv=%llu rc=%d want=%d\n", hz, mode, mrpm, (unsigned long long)iv, rc, want); }
                if (rc == STEPPER_OK) { acc++; CHK(st.interval_us == iv); } else rej++;
            }
        }
    }
    CHK(acc > 20 && rej > 20);
    /* 도 단위 이동 경계 */
    setup(0, 0); stepper_set_speed_mrpm(&st, 5000);
    CHK(stepper_move_deg10(&st, 3600000, 0) == STEPPER_OK && stepper_remaining(&st) == 2048000);
    stepper_stop(&st);
    CHK(stepper_move_deg10(&st, -3600000, 0) == STEPPER_OK && stepper_remaining(&st) == -2048000);
    stepper_stop(&st);
    CHK(stepper_move_deg10(&st, 3600001, 0) == STEPPER_ERR_ARGS && stepper_move_deg10(&st, -3600001, 0) == STEPPER_ERR_ARGS && stepper_remaining(&st) == 0);
    /* 정확히 한 간격 늦은 poll: 다시 맞춤(몰아치지 않는다) */
    setup(0, 0); stepper_set_speed_mrpm(&st, 5000);
    uint32_t iv = st.interval_us; CHK(iv > 0);
    CHK(stepper_move(&st, 5, 1000) == STEPPER_OK && stepper_poll(&st, 1000) == 1);
    CHK(stepper_poll(&st, 1000 + 2 * iv) == 1);                       /* 한 간격 늦음 */
    CHK(stepper_poll(&st, 1000 + 2 * iv) == 0);                       /* 같은 순간에 또 한 스텝은 없다 */
    CHK(stepper_poll(&st, 1000 + 3 * iv - 1) == 0 && stepper_poll(&st, 1000 + 3 * iv) == 1);
    /* 정확히 제때: 공칭 시각 그대로 이어짐 */
    setup(0, 0); stepper_set_speed_mrpm(&st, 5000);
    CHK(stepper_move(&st, 5, 0) == STEPPER_OK && stepper_poll(&st, 0) == 1 && stepper_poll(&st, iv - 1) == 0 && stepper_poll(&st, iv) == 1 && stepper_poll(&st, 2 * iv - 1) == 0 && stepper_poll(&st, 2 * iv) == 1);
}

int main(void)
{
    t_tables(); t_init(); t_sequences(); t_mode_switch(); t_speed(); t_motion(); t_move_errors(); t_degrees(); t_release_hold(); t_random(); t_edges();
    if (fails) { printf("%d FAILED\n", fails); return 1; }
    printf("ALL PASS\n");
    return 0;
}
