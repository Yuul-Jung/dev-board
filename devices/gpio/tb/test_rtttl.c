/* RTTTL 해석기·재생기 호스트 시험 (교재 17장).
 * 빌드: cc -include gpio/tb/mock_timer.h -I gpio/sw gpio/tb/test_rtttl.c gpio/sw/rtttl.c gpio/sw/axi_timer_pwm.c -o /tmp/rtttl -lm
 *
 * 한계(중요): RTTTL 명세는 2차 자료(웹 요약) 셋이고 서로 상충하는 부분이 있다(헤더 rtttl.h, gpio/ref/rtttl_facts.txt). 이 시험의 기대값은 내가 고른 해석
 * (기본 옥타브 6, 1박 = 4분음표, 옥타브 4 의 A = 440 Hz)을 실수 계산으로 독립 구현한 것이라, 그 해석이 틀리면 시험도 같이 틀린다. 소리가 실제로 어떻게 들리는지,
 * 부저가 그 주파수를 내는지는 확인하지 못했다. 재생기는 AXI Timer 레지스터 모의(gpio/tb/mock_timer.h)로 PWM 주기·듀티를 읽어 "지금 나는 소리"를 복원해 검사한다. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "../sw/rtttl.h"

/* ---- AXI Timer 모의: 마지막으로 쓴 값만 저장하고 소리를 복원 ---- */
static uint32_t tlr0, tlr1, tcsr0, tcsr1; static long nwrites;
uint32_t mock_t_rd(uint32_t off) { (void)off; return 0; }
void mock_t_wr(uint32_t off, uint32_t v)
{
    nwrites++;
    if (off == TMR_TLR0) tlr0 = v; else if (off == TMR_TLR1) tlr1 = v;
    else if (off == TMR_TCSR0) tcsr0 = v; else if (off == TMR_TCSR1) tcsr1 = v;
}
static int fails;
#define CHK(c) do { if (!(c)) { fails++; printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
#define CLK 100000000u

/* 현재 PWM 이 내는 주파수(0 = 꺼짐). 듀티가 50 % 인지도 같이 검사 */
static unsigned sound_hz(void)
{
    if (!(tcsr1 & TCSR_ENALL)) return 0;
    uint32_t period = tlr0 + 2, high = tlr1 + 2;
    CHK(high == period / 2);
    return CLK / period;
}

/* ---- 로그 ---- */
typedef struct { uint32_t t; unsigned hz; } ev_t;
static ev_t evs[256]; static int nev; static unsigned last_hz;
static void log_sound(uint32_t t)
{
    unsigned h = sound_hz();
    if (h != last_hz) { if (nev < 256) { evs[nev].t = t; evs[nev].hz = h; nev++; } last_hz = h; }
}
static void log_reset(void) { nev = 0; last_hz = sound_hz(); }

static unsigned rs = 7;
static unsigned rnd(void) { rs = rs * 1664525u + 1013904223u; return rs >> 8; }

/* ---- 독립 기대값 ---- */
static double exact_hz(int oct, int semi) { return 440.0 * pow(2.0, ((oct + 1) * 12 + semi - 69) / 12.0); }
static double exact_ms(int bpm, int dur, int dotted) { return 240000.0 / ((double)bpm * dur) * (dotted ? 1.5 : 1.0); }

static void t_freq_dur(void)
{
    for (int o = 4; o <= 7; o++) for (int n = 0; n < 12; n++) {
        double e = exact_hz(o, n); unsigned f = rtttl_freq_hz((unsigned)o, (unsigned)n);
        CHK(fabs(f - e) <= 0.6);
        if (o < 7) { unsigned g = rtttl_freq_hz((unsigned)o + 1, (unsigned)n); CHK(abs((int)g - 2 * (int)f) <= 1); }
        if (n > 0) CHK(f > rtttl_freq_hz((unsigned)o, (unsigned)n - 1));
    }
    CHK(rtttl_freq_hz(4, 9) == 440 && rtttl_freq_hz(5, 9) == 880 && rtttl_freq_hz(6, 9) == 1760 && rtttl_freq_hz(7, 9) == 3520);
    CHK(rtttl_freq_hz(4, 0) == 262 && rtttl_freq_hz(7, 11) == 3951);
    CHK(rtttl_freq_hz(3, 0) == 0 && rtttl_freq_hz(8, 0) == 0 && rtttl_freq_hz(5, 12) == 0 && rtttl_freq_hz(0, 0) == 0);
    static const int durs[] = { 1, 2, 4, 8, 16, 32 };
    for (int bpm = 25; bpm <= 900; bpm += (bpm < 200 ? 1 : 7)) for (int di = 0; di < 6; di++) for (int dot = 0; dot < 2; dot++) {
        unsigned ms = rtttl_dur_ms((unsigned)bpm, (unsigned)durs[di], dot);
        CHK(fabs(ms - exact_ms(bpm, durs[di], dot)) <= 0.5 + 1e-9);
        CHK(ms > 0);
    }
    CHK(rtttl_dur_ms(63, 4, 0) == 952 && rtttl_dur_ms(120, 4, 0) == 500 && rtttl_dur_ms(120, 4, 1) == 750 && rtttl_dur_ms(112, 8, 0) == 268);
    CHK(rtttl_dur_ms(120, 32, 0) == 63 && rtttl_dur_ms(900, 32, 0) == 8 && rtttl_dur_ms(25, 1, 1) == 14400);
    CHK(rtttl_dur_ms(24, 4, 0) == 0 && rtttl_dur_ms(901, 4, 0) == 0 && rtttl_dur_ms(120, 3, 0) == 0 && rtttl_dur_ms(120, 0, 0) == 0 && rtttl_dur_ms(120, 64, 0) == 0);
}

static void t_header(void)
{
    rtttl_t r;
    CHK(rtttl_open(&r, "Test:d=8,o=5,b=112:c") == RTTTL_OK);
    CHK(r.name_len == 4 && memcmp(r.name, "Test", 4) == 0 && r.def_dur == 8 && r.def_oct == 5 && r.bpm == 112);
    CHK(rtttl_open(&r, "x::c") == RTTTL_OK && r.name_len == 1 && r.def_dur == 4 && r.def_oct == 6 && r.bpm == 63);   /* 기본값 4, 6, 63 */
    CHK(rtttl_open(&r, ":d=2:c") == RTTTL_OK && r.name_len == 0 && r.def_dur == 2 && r.def_oct == 6 && r.bpm == 63);
    CHK(rtttl_open(&r, "n:b=200:c") == RTTTL_OK && r.def_dur == 4 && r.def_oct == 6 && r.bpm == 200);
    CHK(rtttl_open(&r, "n:o=7:c") == RTTTL_OK && r.def_oct == 7);
    CHK(rtttl_open(&r, "X:D=16,O=4,B=900:C") == RTTTL_OK && r.def_dur == 16 && r.def_oct == 4 && r.bpm == 900);       /* 대문자 */
    CHK(rtttl_open(&r, "n : d = 8 , o = 5 , b = 112 : c") == RTTTL_OK && r.def_dur == 8 && r.def_oct == 5 && r.bpm == 112);   /* 공백 */
    CHK(rtttl_open(&r, "n:d=8,s=3,b=100:c") == RTTTL_OK && r.def_dur == 8 && r.bpm == 100);                         /* 모르는 키 무시 */
    CHK(rtttl_open(&r, "n:b=25:c") == RTTTL_OK && r.bpm == 25 && rtttl_open(&r, "n:b=900:c") == RTTTL_OK && r.bpm == 900);
    CHK(rtttl_open(&r, "n:d=1:c") == RTTTL_OK && r.def_dur == 1 && rtttl_open(&r, "n:d=32:c") == RTTTL_OK && r.def_dur == 32);
    CHK(rtttl_open(&r, "n:o=4:c") == RTTTL_OK && r.def_oct == 4);
    CHK(rtttl_open(&r, "n:d=4,:c") == RTTTL_OK && r.def_dur == 4);                     /* 끝 쉼표는 관대하게 허용 */
    CHK(rtttl_open(&r, "n:d=4,o=5,b=100:") == RTTTL_OK);                               /* 음표 없는 곡 */
    /* 이름 길이: 15자까지 허용, 16자는 오류 */
    CHK(rtttl_open(&r, "0123456789:d=4:c") == RTTTL_OK && r.name_len == 10);
    CHK(rtttl_open(&r, "012345678901234:d=4:c") == RTTTL_OK && r.name_len == 15);
    CHK(rtttl_open(&r, "0123456789012345:d=4:c") == RTTTL_ERR_FORMAT);
    /* 오류 */
    const char *bad[] = { "", "name", "name:d=4", "name:d=4:", "n:d=3:c", "n:d=0:c", "n:d=64:c", "n:o=3:c", "n:o=8:c", "n:b=24:c", "n:b=901:c", "n:b=99999:c",
        "n:d=:c", "n:d4:c", "n:d=4;o=5:c", "n:=4:c", "n:d=-4:c", "n:d=4 o=5:c", "n:1=4:c", "n:d==4:c", "n:d=4,,o=5:c" };
    for (unsigned i = 0; i < sizeof bad / sizeof *bad; i++) {
        int rc = rtttl_open(&r, bad[i]);
        if (i == 3) CHK(rc == RTTTL_OK);          /* "name:d=4:" 는 정상(음표 없음) */
        else if (rc != RTTTL_ERR_FORMAT) { fails++; printf("FAIL header accepted: \"%s\" -> %d\n", bad[i], rc); }
    }
    /* 공백 종류, 대문자 알 수 없는 키, 키 경계 a/z, 값 없는 알 수 없는 키 */
    CHK(rtttl_open(&r, "n:\td=8,\r\no=5\t:c") == RTTTL_OK && r.def_dur == 8 && r.def_oct == 5);
    CHK(rtttl_open(&r, "n:Z=3,d=2:c") == RTTTL_OK && r.def_dur == 2);
    CHK(rtttl_open(&r, "n:a=3:c") == RTTTL_OK && rtttl_open(&r, "n:z=3:c") == RTTTL_OK);
    CHK(rtttl_open(&r, "n:x=:c") == RTTTL_ERR_FORMAT && rtttl_open(&r, "n:x=y:c") == RTTTL_ERR_FORMAT && rtttl_open(&r, "n:x=0:c") == RTTTL_OK);
    CHK(rtttl_open(&r, "n:dx4:c") == RTTTL_ERR_FORMAT && rtttl_open(&r, "n:d:c") == RTTTL_ERR_FORMAT && rtttl_open(&r, "n:@=4:c") == RTTTL_ERR_FORMAT && rtttl_open(&r, "n:[=4:c") == RTTTL_ERR_FORMAT && rtttl_open(&r, "n:`=4:c") == RTTTL_ERR_FORMAT && rtttl_open(&r, "n:{=4:c") == RTTTL_ERR_FORMAT);
    { char *hp = malloc(8); memcpy(hp, "nonamed", 8); CHK(rtttl_open(&r, hp) == RTTTL_ERR_FORMAT); free(hp); }   /* 콜론 없는 문자열이 힙 끝에서 끝나도 넘어 읽지 않음(ASan) */
    { char *hp = malloc(10); memcpy(hp, "n:d=4,o=5", 10); CHK(rtttl_open(&r, hp) == RTTTL_ERR_FORMAT); free(hp); }
    /* 숫자 자릿수: 앞의 0 포함 4자리까지, 5자리는 오류 */
    CHK(rtttl_open(&r, "n:b=0100:c") == RTTTL_OK && r.bpm == 100 && rtttl_open(&r, "n:b=00100:c") == RTTTL_ERR_FORMAT && rtttl_open(&r, "n:x=00001:c") == RTTTL_ERR_FORMAT);
    { rtttl_note_t nn; CHK(rtttl_open(&r, "x::c") == RTTTL_OK && rtttl_next(&r, &nn) == RTTTL_NOTE && nn.freq_hz == rtttl_freq_hz(6, 0) && nn.dur_ms == rtttl_dur_ms(63, 4, 0) && rtttl_next(&r, &nn) == RTTTL_END);
      }
    CHK(rtttl_open(NULL, "a:d=4:c") == RTTTL_ERR_ARGS && rtttl_open(&r, NULL) == RTTTL_ERR_ARGS);
}

static void t_notes(void)
{
    rtttl_t r; rtttl_note_t n;
    struct { const char *tok; unsigned hz; unsigned ms; } c[] = {       /* 헤더 d=4,o=5,b=120 */
        { "c", 523, 500 }, { "8d", 587, 250 }, { "e.", 659, 750 }, { "p", 0, 500 }, { "8p.", 0, 375 }, { "16f#6", 1480, 125 },
        { "2a4", 440, 1000 }, { "1b", 988, 2000 }, { "32c7", 2093, 63 }, { "4c.5", 523, 750 }, { "4c5.", 523, 750 }, { "h", 988, 500 },
        { "C#", 554, 500 }, { "8G#4", 415, 250 }, { "a#", 932, 500 }, { "2p", 0, 1000 }, { "d#7", 2489, 500 }, { "f", 698, 500 }, { "g", 784, 500 },
        { "e4", 330, 500 }, { "b7", 3951, 500 }, { "8.c", 0, 0 /* 아래서 따로 */ },
    };
    for (unsigned i = 0; i < sizeof c / sizeof *c - 1; i++) {
        char buf[64]; snprintf(buf, sizeof buf, "t:d=4,o=5,b=120:%s", c[i].tok);
        CHK(rtttl_open(&r, buf) == RTTTL_OK);
        int rc = rtttl_next(&r, &n);
        if (rc != RTTTL_NOTE || n.freq_hz != c[i].hz || n.dur_ms != c[i].ms) { fails++; printf("FAIL note \"%s\": rc=%d %u Hz %u ms (want %u, %u)\n", c[i].tok, rc, rc == 1 ? n.freq_hz : 0, rc == 1 ? n.dur_ms : 0, c[i].hz, c[i].ms); }
        CHK(rtttl_next(&r, &n) == RTTTL_END && rtttl_next(&r, &n) == RTTTL_END);
    }
    /* 여러 음표와 공백, 줄바꿈, 끝 쉼표 */
    CHK(rtttl_open(&r, "t:d=4,o=5,b=120: c , 8d ,\n e. ,") == RTTTL_OK);
    CHK(rtttl_next(&r, &n) == RTTTL_NOTE && n.freq_hz == 523);
    CHK(rtttl_next(&r, &n) == RTTTL_NOTE && n.freq_hz == 587 && n.dur_ms == 250);
    CHK(rtttl_next(&r, &n) == RTTTL_NOTE && n.freq_hz == 659 && n.dur_ms == 750);
    CHK(rtttl_next(&r, &n) == RTTTL_END);
    /* 기본값이 적용됨: d=8 이면 길이 생략 시 8분음표 */
    CHK(rtttl_open(&r, "t:d=8,o=4,b=100:a,2a,a5") == RTTTL_OK);
    CHK(rtttl_next(&r, &n) == RTTTL_NOTE && n.freq_hz == 440 && n.dur_ms == 300);
    CHK(rtttl_next(&r, &n) == RTTTL_NOTE && n.freq_hz == 440 && n.dur_ms == 1200);
    CHK(rtttl_next(&r, &n) == RTTTL_NOTE && n.freq_hz == 880 && n.dur_ms == 300);
    /* 오류 토큰: 앞의 음표는 정상, 오류 뒤에는 END */
    const char *bad[] = { "3c", "64c", "0c", "c8", "c3", "p5", "e#", "b#", "h#", "x", "cc", "c..", "c.5.", ",", "4", "#", "c#.#", "c#5#", "-c", "c!", "9", "c4x", "1c2", "012c" };
    for (unsigned i = 0; i < sizeof bad / sizeof *bad; i++) {
        char buf[96]; snprintf(buf, sizeof buf, "t:d=4,o=5,b=120:c,%s,d", bad[i]);
        CHK(rtttl_open(&r, buf) == RTTTL_OK);
        CHK(rtttl_next(&r, &n) == RTTTL_NOTE);
        int rc = rtttl_next(&r, &n);
        if (rc != RTTTL_ERR_FORMAT) { fails++; printf("FAIL bad token accepted: \"%s\" -> %d\n", bad[i], rc); }
        CHK(rtttl_next(&r, &n) == RTTTL_END);                                       /* 그 뒤 "d" 는 읽지 않는다 */
    }
    { char *hp = malloc(24); memcpy(hp, "t:d=4:c,x,d", 12); CHK(rtttl_open(&r, hp) == RTTTL_OK && rtttl_next(&r, &n) == RTTTL_NOTE && rtttl_next(&r, &n) == RTTTL_ERR_FORMAT && rtttl_next(&r, &n) == RTTTL_END); free(hp); }
    { char *hp = malloc(8); memcpy(hp, "t:d=4:c", 8); CHK(rtttl_open(&r, hp) == RTTTL_OK && rtttl_next(&r, &n) == RTTTL_NOTE && rtttl_next(&r, &n) == RTTTL_END); free(hp); }
    /* 인자 */
    CHK(rtttl_next(NULL, &n) == RTTTL_ERR_ARGS && rtttl_next(&r, NULL) == RTTTL_ERR_ARGS);
}

/* 무작위 곡을 만들어 왕복: 문자열 → 해석 → 실수 계산과 비교 */
static void t_roundtrip(void)
{
    static const char NOTES[] = "cdefgab"; static const int SEMI[] = { 0, 2, 4, 5, 7, 9, 11 };
    static const int DURS[] = { 1, 2, 4, 8, 16, 32 };
    for (int tune = 0; tune < 3000; tune++) {
        int dd = DURS[rnd() % 6], oo = 4 + (int)(rnd() % 4), bb = 25 + (int)(rnd() % 876);
        char buf[1200]; int len = snprintf(buf, sizeof buf, "rt:d=%d,o=%d,b=%d:", dd, oo, bb);
        struct { unsigned hz; double ms; } want[40]; int nn = (int)(rnd() % 30) + 1;
        for (int k = 0; k < nn; k++) {
            int hasd = rnd() & 1, d = hasd ? DURS[rnd() % 6] : dd, dot = (int)(rnd() % 3 == 0), rest = (int)(rnd() % 6 == 0);
            int ni = (int)(rnd() % 7), sharp = 0, hasoct = (int)(rnd() & 1), o = hasoct ? 4 + (int)(rnd() % 4) : oo, dotfirst = (int)(rnd() & 1);
            if (ni != 2 && ni != 6 && rnd() % 4 == 0) sharp = 1;                    /* e#, b# 는 없음 */
            int up = (int)(rnd() & 1);
            if (hasd) len += snprintf(buf + len, sizeof buf - (size_t)len, "%d", d);
            if (rest) { len += snprintf(buf + len, sizeof buf - (size_t)len, "%c%s", up ? 'P' : 'p', dot ? "." : ""); want[k].hz = 0; }
            else {
                len += snprintf(buf + len, sizeof buf - (size_t)len, "%c%s", up ? NOTES[ni] - 32 : NOTES[ni], sharp ? "#" : "");
                if (dot && dotfirst) len += snprintf(buf + len, sizeof buf - (size_t)len, ".");
                if (hasoct) len += snprintf(buf + len, sizeof buf - (size_t)len, "%d", o);
                if (dot && !dotfirst) len += snprintf(buf + len, sizeof buf - (size_t)len, ".");
                want[k].hz = (unsigned)floor(exact_hz(o, SEMI[ni] + sharp) + 0.5);
            }
            if (rest && dot) {}                                                    /* 위에서 이미 점 출력 */
            want[k].ms = exact_ms(bb, d, dot);
            len += snprintf(buf + len, sizeof buf - (size_t)len, k + 1 < nn ? "," : "");
        }
        rtttl_t r; rtttl_note_t n; CHK(rtttl_open(&r, buf) == RTTTL_OK);
        for (int k = 0; k < nn; k++) {
            int rc = rtttl_next(&r, &n);
            if (rc != RTTTL_NOTE) { fails++; printf("FAIL roundtrip rc=%d at %d of \"%s\"\n", rc, k, buf); break; }
            if (abs((int)n.freq_hz - (int)want[k].hz) > 1 || fabs(n.dur_ms - want[k].ms) > 0.5 + 1e-9) { fails++; printf("FAIL roundtrip note %d of \"%s\": %u Hz %u ms want %u Hz %.2f ms\n", k, buf, n.freq_hz, n.dur_ms, want[k].hz, want[k].ms); break; }
        }
        CHK(rtttl_next(&r, &n) == RTTTL_END);
    }
}

static void t_fuzz(void)
{
    static const char ALPHA[] = "abcdefgphABCDEFGPH#.,:=0123456789 dob\n";
    for (long it = 0; it < 300000; it++) {
        char buf[80]; unsigned n = rnd() % 60;
        for (unsigned i = 0; i < n; i++) buf[i] = (rnd() % 8) ? ALPHA[rnd() % (sizeof ALPHA - 1)] : (char)(rnd() % 255 + 1);
        buf[n] = 0;
        if (it & 1) { const char *h = "f:d=8,o=5,b=100:"; size_t hl = strlen(h); memmove(buf + hl, buf, n + 1 > 60 ? 60 : n + 1); memcpy(buf, h, hl); buf[hl + (n > 60 ? 60 : n)] = 0; }
        rtttl_t r; rtttl_note_t nt;
        if (rtttl_open(&r, buf) != RTTTL_OK) continue;
        size_t cap = strlen(buf) + 2; int ended = 0;
        for (size_t k = 0; k < cap; k++) {
            int rc = rtttl_next(&r, &nt);
            if (rc == RTTTL_NOTE) {
                CHK(nt.freq_hz == 0 || (nt.freq_hz >= 262 && nt.freq_hz <= 3951));
                CHK(nt.dur_ms >= 8 && nt.dur_ms <= 14400);
                continue;
            }
            CHK(rc == RTTTL_END || rc == RTTTL_ERR_FORMAT);
            if (rc == RTTTL_ERR_FORMAT) CHK(rtttl_next(&r, &nt) == RTTTL_END);
            ended = 1; break;
        }
        CHK(ended);                                                                /* 문자열 길이 안에 반드시 끝남 */
    }
}

/* ---- 재생기 ---- */
static rtttl_player_t pl;
static void run_to(uint32_t t0, uint32_t from, uint32_t to)       /* t0 기준 상대 ms 를 1 ms 씩 poll */
{
    for (uint32_t t = from; t <= to; t++) { (void)rtttl_play_poll(&pl, t0 + t); log_sound(t0 + t); }
}
static void t_player(void)
{
    static const uint32_t starts[] = { 0u, 1000u, 0xFFFFFF00u, 0xFFFFFFFFu };
    for (unsigned si = 0; si < sizeof starts / sizeof *starts; si++) {
        uint32_t t0 = starts[si];
        tcsr0 = tcsr1 = tlr0 = tlr1 = 0; log_reset();
        CHK(rtttl_play_start(&pl, "t:d=4,o=5,b=120:c,8d,p,2a4", 0, CLK, t0) == RTTTL_OK);
        log_sound(t0);
        run_to(t0, 1, 2400);
        /* 기대: 0 ms C5(523) / 450 꺼짐 / 500 D5(587) / 725 꺼짐 / 750~1250 쉼표 / 1250 A4(440) / 2150 꺼짐 */
        struct { uint32_t t; unsigned hz; } want[] = { { 0, 523 }, { 450, 0 }, { 500, 587 }, { 725, 0 }, { 1250, 440 }, { 2150, 0 } };
        CHK(nev == 6);
        for (int i = 0; i < nev && i < 6; i++) {
            if (evs[i].t - t0 != want[i].t || evs[i].hz != want[i].hz) { fails++; printf("FAIL event %d: t=%u hz=%u want t=%u hz=%u (start %u)\n", i, evs[i].t - t0, evs[i].hz, want[i].t, want[i].hz, t0); }
        }
        CHK(rtttl_play_poll(&pl, t0 + 2500) == 0 && sound_hz() == 0);               /* 끝난 뒤 */
        long w = nwrites; CHK(rtttl_play_poll(&pl, t0 + 3000) == 0 && nwrites == w);   /* 더는 쓰지 않음 */
    }
    /* 반환값: 재생 중 1, 끝나면 0 */
    tcsr0 = tcsr1 = 0; rtttl_play_start(&pl, "t:d=4,o=5,b=120:c,d", 0, CLK, 100);
    CHK(rtttl_play_poll(&pl, 100) == 1 && rtttl_play_poll(&pl, 599) == 1 && rtttl_play_poll(&pl, 600) == 1 && rtttl_play_poll(&pl, 1099) == 1 && rtttl_play_poll(&pl, 1100) == 0);
    /* 간격 0 % : 같은 음이 이어지면 소리가 끊기지 않음 */
    log_reset(); tcsr0 = tcsr1 = 0;
    CHK(rtttl_play_start(&pl, "t:d=4,o=5,b=120:c,c,d", 0, CLK, 0) == RTTTL_OK);
    rtttl_set_gap_pct(&pl, 0); log_sound(0); run_to(0, 1, 1600);
    CHK(nev == 3 && evs[0].hz == 523 && evs[0].t == 0 && evs[1].hz == 587 && evs[1].t == 1000 && evs[2].hz == 0 && evs[2].t == 1500);
    /* 간격 10 % 기본: 같은 음이 이어지면 사이에 꺼짐 */
    log_reset(); tcsr0 = tcsr1 = 0;
    CHK(rtttl_play_start(&pl, "t:d=4,o=5,b=120:c,c", 0, CLK, 0) == RTTTL_OK); log_sound(0); run_to(0, 1, 1100);
    CHK(nev == 4 && evs[1].hz == 0 && evs[1].t == 450 && evs[2].hz == 523 && evs[2].t == 500 && evs[3].hz == 0 && evs[3].t == 950);
    /* 간격 상한 50 % */
    log_reset(); tcsr0 = tcsr1 = 0;
    rtttl_play_start(&pl, "t:d=4,o=5,b=120:c", 0, CLK, 0); rtttl_set_gap_pct(&pl, 100); log_sound(0); run_to(0, 1, 600);
    CHK(nev == 2 && evs[1].t == 250 && evs[1].hz == 0);
    /* 늦게 호출: 지나간 음은 건너뜀, 곡은 시계와 맞음 */
    log_reset(); tcsr0 = tcsr1 = 0;
    CHK(rtttl_play_start(&pl, "t:d=4,o=5,b=120:c,8d,p,2a4", 0, CLK, 1000) == RTTTL_OK); log_sound(1000);
    CHK(nev == 1 && evs[0].hz == 523);
    CHK(rtttl_play_poll(&pl, 1000 + 1300) == 1); log_sound(2300);
    CHK(nev == 2 && evs[1].hz == 440 && evs[1].t == 2300);                           /* C, D, 쉼표는 소리 없이 건너뜀 */
    CHK(rtttl_play_poll(&pl, 1000 + 5000) == 0 && sound_hz() == 0);                  /* 한참 늦게 와도 안전하게 끝 */
    /* 멈춤 */
    tcsr0 = tcsr1 = 0; rtttl_play_start(&pl, "t:d=4,o=5,b=120:c,d", 0, CLK, 0); CHK(sound_hz() == 523);
    rtttl_play_stop(&pl); CHK(sound_hz() == 0 && rtttl_play_poll(&pl, 10) == 0);
    long w = nwrites; rtttl_play_stop(&pl); rtttl_play_stop(NULL); CHK(nwrites == w);  /* 이미 멈춘 것을 또 멈춰도 쓰지 않음 */
    /* 다시 시작 */
    CHK(rtttl_play_start(&pl, "t:d=4,o=6,b=120:a", 0, CLK, 50) == RTTTL_OK && sound_hz() == 1760);
    /* 같은 소리가 이어지는 동안은 PWM 레지스터를 다시 쓰지 않는다 */
    tcsr0 = tcsr1 = 0; rtttl_play_start(&pl, "t:d=4,o=5,b=120:c", 0, CLK, 0);
    (void)rtttl_play_poll(&pl, 10); w = nwrites; for (uint32_t t = 11; t < 440; t++) (void)rtttl_play_poll(&pl, t); CHK(nwrites == w);
    (void)rtttl_play_poll(&pl, 460); CHK(nwrites > w); w = nwrites; for (uint32_t t = 461; t < 499; t++) (void)rtttl_play_poll(&pl, t); CHK(nwrites == w);
    /* 쉼표로 시작해도 부저가 켜져 있던 상태라면 끈다(부저의 이전 상태를 모른다고 본다) */
    tcsr1 = TCSR_ENALL; tlr0 = 998; tlr1 = 498; CHK(sound_hz() != 0);
    CHK(rtttl_play_start(&pl, "t:d=4,o=5,b=120:p,c", 0, CLK, 0) == RTTTL_OK && sound_hz() == 0);
    /* 곡 도중에 지금 노트의 간격을 바꾸면 그 음에도 바로 적용(두 번째 음, 시작 시각이 0 이 아님) */
    log_reset(); tcsr0 = tcsr1 = 0; rtttl_play_start(&pl, "t:d=4,o=5,b=120:c,c", 0, CLK, 100); log_sound(100); run_to(100, 1, 520);
    rtttl_set_gap_pct(&pl, 50); run_to(100, 521, 900); rtttl_set_gap_pct(&pl, 0); run_to(100, 901, 1100);
    { int ok = 0; for (int i = 0; i < nev; i++) if (evs[i].hz == 0 && evs[i].t - 100 == 750) ok = 1; CHK(ok); }
    log_reset(); rtttl_play_start(&pl, "t:d=4,o=5,b=120:c,c", 0, CLK, 0); rtttl_set_gap_pct(&pl, 51); run_to(0, 1, 249); CHK(sound_hz() == 523); run_to(0, 250, 260); CHK(sound_hz() == 0);
    log_reset(); rtttl_play_start(&pl, "t:d=4,o=5,b=120:c", 0, CLK, 0); rtttl_set_gap_pct(&pl, 49); run_to(0, 1, 254); CHK(sound_hz() == 523); run_to(0, 255, 255); CHK(sound_hz() == 0);
    log_reset(); tcsr0 = tcsr1 = 0; rtttl_play_start(&pl, "t:d=4,o=5,b=120:c", 0, CLK, 0); rtttl_set_gap_pct(&pl, 50); run_to(0, 1, 249); CHK(sound_hz() == 523); run_to(0, 250, 250); CHK(sound_hz() == 0);
    /* 곡 도중 PWM 설정이 실패하면(낮은 클럭에서 높은 음) 오류를 돌려주고 멈춘다 */
    tcsr0 = tcsr1 = 0; CHK(rtttl_play_start(&pl, "t:d=4,o=4,b=120:c,c7", 0, 8000, 0) == RTTTL_OK);
    CHK(rtttl_play_poll(&pl, 500) < 0 && !pl.playing && rtttl_play_poll(&pl, 501) == 0);
    /* 오류 */
    tcsr0 = tcsr1 = 0; w = nwrites;
    CHK(rtttl_play_start(&pl, "nocolon", 0, CLK, 0) == RTTTL_ERR_FORMAT && nwrites == w && !pl.playing);
    CHK(rtttl_play_start(&pl, "t:d=4:3c", 0, CLK, 0) == RTTTL_ERR_FORMAT && nwrites == w && !pl.playing);     /* 첫 음표가 오류: 켜지 않음 */
    CHK(rtttl_play_start(&pl, "t:d=4:", 0, CLK, 0) == RTTTL_OK && nwrites == w && !pl.playing && rtttl_play_poll(&pl, 1) == 0);   /* 음표 없음 */
    CHK(rtttl_play_start(NULL, "t:d=4:c", 0, CLK, 0) == RTTTL_ERR_ARGS && rtttl_play_start(&pl, NULL, 0, CLK, 0) == RTTTL_ERR_ARGS && rtttl_play_start(&pl, "t:d=4:c", 0, 0, 0) == RTTTL_ERR_ARGS);
    CHK(rtttl_play_poll(NULL, 0) == RTTTL_ERR_ARGS);
    rtttl_set_gap_pct(NULL, 5);
    /* 곡 중간의 오류: 오류 지점에서 poll 이 오류를 돌려주고 소리를 끈다 */
    tcsr0 = tcsr1 = 0; log_reset();
    CHK(rtttl_play_start(&pl, "t:d=4,o=5,b=120:c,3d,e", 0, CLK, 0) == RTTTL_OK); log_sound(0);
    CHK(rtttl_play_poll(&pl, 499) == 1 && rtttl_play_poll(&pl, 500) == RTTTL_ERR_FORMAT && sound_hz() == 0 && !pl.playing && rtttl_play_poll(&pl, 501) == 0);
    /* 클럭이 너무 낮아 PWM 주기를 만들 수 없는 경우: 시작이 실패하고 재생 중이 아님 */
    tcsr0 = tcsr1 = 0;
    CHK(rtttl_play_start(&pl, "t:d=4,o=7,b=120:c", 0, 8000, 0) == RTTTL_ERR_ARGS && !pl.playing);
    /* 쉼표로 시작: 소리를 켜지 않음 */
    tcsr0 = tcsr1 = 0; CHK(rtttl_play_start(&pl, "t:d=4,o=5,b=120:p,c", 0, CLK, 0) == RTTTL_OK && sound_hz() == 0 && rtttl_play_poll(&pl, 499) == 1 && sound_hz() == 0 && rtttl_play_poll(&pl, 500) == 1 && sound_hz() == 523);
    /* 같은 곡을 낮은 클럭(1 MHz)에서: 주기 = clk / 주파수 */
    tcsr0 = tcsr1 = 0; CHK(rtttl_play_start(&pl, "t:d=4,o=4,b=120:a", 0, 1000000u, 0) == RTTTL_OK && tlr0 + 2 == 1000000u / 440);
}

int main(void)
{
    t_freq_dur(); t_header(); t_notes(); t_roundtrip(); t_fuzz(); t_player();
    if (fails) { printf("%d FAILED\n", fails); return 1; }
    printf("ALL PASS\n");
    return 0;
}
