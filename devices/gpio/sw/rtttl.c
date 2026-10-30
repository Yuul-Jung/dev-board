#include "rtttl.h"

/* 옥타브 4 의 C..B, 1/64 Hz 단위 (440·2^((n-69)/12)·64 를 반올림; 계산) */
static const uint32_t OCT4_X64[12] = { 16744, 17740, 18795, 19912, 21096, 22351, 23680, 25088, 26580, 28160, 29834, 31609 };

uint16_t rtttl_freq_hz(unsigned octave, unsigned semitone)
{
    if (octave < RTTTL_OCT_MIN || octave > RTTTL_OCT_MAX || semitone > 11) return 0;
    uint32_t x64 = OCT4_X64[semitone] << (octave - RTTTL_OCT_MIN);
    return (uint16_t)((x64 + 32) >> 6);
}

uint32_t rtttl_dur_ms(unsigned bpm, unsigned dur, int dotted)
{
    if (bpm < RTTTL_BPM_MIN || bpm > RTTTL_BPM_MAX) return 0;
    if (dur != 1 && dur != 2 && dur != 4 && dur != 8 && dur != 16 && dur != 32) return 0;
    /* 온음표 = 4박, 1박 = 60000/bpm ms. 점음표는 1.5 배. 반올림 */
    uint64_t num = 60000ull * RTTTL_BEATS_PER_WHOLE * (dotted ? 3 : 2);
    uint64_t den = (uint64_t)bpm * dur * 2;
    return (uint32_t)((num + den / 2) / den);
}

static const char *skip_ws(const char *p) { while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++; return p; }
static int is_digit(char c) { return c >= '0' && c <= '9'; }
static char lower(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

static int valid_dur(unsigned d) { return d == 1 || d == 2 || d == 4 || d == 8 || d == 16 || d == 32; }

/* 십진수 읽기(최대 4자리). 숫자가 없으면 -1 */
static int read_num(const char **pp)
{
    const char *p = *pp; int v = 0, n = 0;
    while (is_digit(*p)) { if (++n > 4) return -1; v = v * 10 + (*p - '0'); p++; }
    if (n == 0) return -1;
    *pp = p; return v;
}

int rtttl_open(rtttl_t *r, const char *s)
{
    if (!r || !s) return RTTTL_ERR_ARGS;
    r->name = s; r->name_len = 0;
    while (s[r->name_len] && s[r->name_len] != ':') {
        if (++r->name_len > RTTTL_NAME_LIMIT) return RTTTL_ERR_FORMAT;
    }
    if (s[r->name_len] != ':') return RTTTL_ERR_FORMAT;
    const char *p = s + r->name_len + 1;
    r->def_dur = RTTTL_DEFAULT_DUR; r->def_oct = RTTTL_DEFAULT_OCT; r->bpm = RTTTL_DEFAULT_BPM;
    for (;;) {                                     /* key=value 쌍, 쉼표로 구분, 콜론으로 끝 */
        p = skip_ws(p);
        if (*p == ':') { p++; break; }
        if (*p == 0) return RTTTL_ERR_FORMAT;
        char key = lower(*p); 
        if (key < 'a' || key > 'z') return RTTTL_ERR_FORMAT;
        p++; p = skip_ws(p);
        if (*p != '=') return RTTTL_ERR_FORMAT;
        p++; p = skip_ws(p);
        int v = read_num(&p);
        if (v < 0) return RTTTL_ERR_FORMAT;
        if (key == 'd') { if (!valid_dur((unsigned)v)) return RTTTL_ERR_FORMAT; r->def_dur = (uint8_t)v; }
        else if (key == 'o') { if (v < RTTTL_OCT_MIN || v > RTTTL_OCT_MAX) return RTTTL_ERR_FORMAT; r->def_oct = (uint8_t)v; }
        else if (key == 'b') { if (v < RTTTL_BPM_MIN || v > RTTTL_BPM_MAX) return RTTTL_ERR_FORMAT; r->bpm = (uint16_t)v; }
        /* 그 밖의 키는 값만 읽고 무시(명세: 모르는 이름은 무시) */
        p = skip_ws(p);
        if (*p == ',') { p++; continue; }
        if (*p == ':') { p++; break; }
        return RTTTL_ERR_FORMAT;
    }
    r->p = p;
    return RTTTL_OK;
}

/* 음 이름 → 반음 번호(C=0). 'h' 는 b. 못 쓰는 글자는 -1 */
static int semitone_of(char c)
{
    switch (c) { case 'c': return 0; case 'd': return 2; case 'e': return 4; case 'f': return 5; case 'g': return 7; case 'a': return 9; case 'b': case 'h': return 11; }
    return -1;
}

int rtttl_next(rtttl_t *r, rtttl_note_t *n)
{
    if (!r || !n || !r->p) return RTTTL_ERR_ARGS;
    const char *p = skip_ws(r->p);
    if (*p == 0) { r->p = p; return RTTTL_END; }
    int err = 0;
    int dur = r->def_dur, oct = r->def_oct, dotted = 0, rest = 0, semi = 0;
    if (is_digit(*p)) { dur = read_num(&p); if (dur < 0 || !valid_dur((unsigned)dur)) err = 1; }
    if (!err) {
        char c = lower(*p);
        if (c == 'p') { rest = 1; p++; }
        else {
            semi = semitone_of(c);
            if (semi < 0) err = 1; else p++;
            if (!err && *p == '#') {
                if (c == 'e' || c == 'b' || c == 'h') err = 1;     /* e#, b# 는 명세의 음 이름표에 없음 */
                else { semi++; p++; }
            }
        }
    }
    if (!err && *p == '.') { dotted = 1; p++; }                    /* 점이 옥타브 앞에 오는 형태 */
    if (!err && is_digit(*p)) {
        int o = read_num(&p);
        if (rest || o < RTTTL_OCT_MIN || o > RTTTL_OCT_MAX) err = 1; else oct = o;
    }
    if (!err && *p == '.') { if (dotted) err = 1; else { dotted = 1; p++; } }   /* 점이 옥타브 뒤에 오는 정식 형태 */
    if (!err) {
        p = skip_ws(p);
        if (*p == ',') p++;
        else if (*p != 0) err = 1;
    }
    if (err) { while (*p) p++; r->p = p; return RTTTL_ERR_FORMAT; }       /* 오류 뒤에는 더 읽지 않는다 */
    r->p = p;
    n->dur_ms = rtttl_dur_ms(r->bpm, (unsigned)dur, dotted);
    n->freq_hz = rest ? 0 : rtttl_freq_hz((unsigned)oct, (unsigned)semi);
    return RTTTL_NOTE;
}

/* ===================== 비차단 재생 ===================== */
static int set_tone(rtttl_player_t *p, uint16_t hz)
{
    if (hz == p->last_hz) return RTTTL_OK;
    if (buzzer_tone(p->pwm_base, p->clk_hz, hz) != TPWM_OK) return RTTTL_ERR_ARGS;
    p->last_hz = hz;
    return RTTTL_OK;
}

/* 다음 음표를 읽어 start_ms 에서 시작하는 것으로 시각을 정한다. 반환: RTTTL_NOTE / RTTTL_END / 음수 */
static int advance(rtttl_player_t *p, uint32_t start_ms)
{
    int r = rtttl_next(&p->r, &p->cur);
    if (r != RTTTL_NOTE) return r;
    p->note_start = start_ms;
    p->note_end = start_ms + p->cur.dur_ms;
    uint32_t gap = p->cur.dur_ms * p->gap_pct / 100;
    p->tone_end = start_ms + (p->cur.freq_hz ? p->cur.dur_ms - gap : 0);
    return RTTTL_NOTE;
}

static void finish(rtttl_player_t *p)
{
    p->playing = 0;
    p->last_hz = 1;                                   /* 다른 값으로 만들어 강제로 끄기 */
    (void)set_tone(p, 0);
}

int rtttl_play_start(rtttl_player_t *p, const char *s, uintptr_t pwm_base, uint32_t clk_hz, uint32_t now_ms)
{
    if (!p || !s || clk_hz == 0) return RTTTL_ERR_ARGS;
    p->pwm_base = pwm_base; p->clk_hz = clk_hz; p->gap_pct = RTTTL_GAP_PCT_DEFAULT; p->playing = 0; p->last_hz = 0;
    int r = rtttl_open(&p->r, s);
    if (r != RTTTL_OK) return r;
    r = advance(p, now_ms);
    if (r == RTTTL_END) return RTTTL_OK;              /* 음표가 하나도 없는 곡: 아무것도 안 함 */
    if (r < 0) return r;
    p->playing = 1;
    p->last_hz = 1;                                   /* 부저 상태를 모른다고 보고 첫 음을 반드시 쓴다 */
    r = set_tone(p, p->cur.freq_hz);
    if (r < 0) { p->playing = 0; return r; }
    return RTTTL_OK;
}

int rtttl_play_poll(rtttl_player_t *p, uint32_t now_ms)
{
    if (!p) return RTTTL_ERR_ARGS;
    if (!p->playing) return 0;
    while ((int32_t)(now_ms - p->note_end) >= 0) {    /* 끝난 음은 건너뛴다(늦게 호출돼도 곡이 시계와 맞음) */
        int r = advance(p, p->note_end);
        if (r == RTTTL_END) { finish(p); return 0; }
        if (r < 0) { finish(p); return r; }
    }
    int on = p->cur.freq_hz != 0 && (int32_t)(now_ms - p->tone_end) < 0;
    int r = set_tone(p, on ? p->cur.freq_hz : 0);
    if (r < 0) { p->playing = 0; return r; }
    return 1;
}

void rtttl_play_stop(rtttl_player_t *p)
{
    if (p && p->playing) finish(p);
}

void rtttl_set_gap_pct(rtttl_player_t *p, unsigned pct)
{
    if (!p) return;
    p->gap_pct = (uint8_t)(pct > 50 ? 50 : pct);
    if (p->playing) {                                 /* 지금 나는 음에도 바로 적용 */
        uint32_t gap = p->cur.dur_ms * p->gap_pct / 100;
        p->tone_end = p->note_start + (p->cur.freq_hz ? p->cur.dur_ms - gap : 0);
    }
}
