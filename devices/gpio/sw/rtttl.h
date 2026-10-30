/* RTTTL (Ring Tone Text Transfer Language) 해석기와 비차단 재생기 (교재 17장 대응). 소리는 AXI Timer PWM 의 buzzer_tone() 으로 낸다(패시브 부저).
 *
 * 형식과 근거 등급 (표: gpio/ref/rtttl_facts.txt, 문서: gpio/rtttl.md)
 *  - 구조 "이름:기본값:음표들", 이름 ≤ 10자(콜론 없음), 기본값 d(1,2,4,8,16,32) o(4~7) b(25~900), 음표 [길이]음이름[#][옥타브][.], 'p' = 쉼표(옥타브 없음),
 *    'h' = 'b', 점음표 = 1.5배, 옥타브 4 의 A = 440 Hz(→ 옥타브 5 는 880, 6 은 1760, 7 은 3520): mobilefish 명세 페이지, 1j01/rtttl.js 문서, Wikipedia — 모두 웹 가져오기 요약 경유 D3
 *    (독립적인 원 명세 문서는 열지 못함: Nokia 원문 없음).
 *  - **상충하는 부분**(드라이버는 아래처럼 정했다, 사실표에 !val 로 기록):
 *      ① 기본 옥타브: 두 자료는 6, 한 자료(1j01 문서)는 4 로 적음 → 6 을 쓴다(RTTTL_DEFAULT_OCT).
 *      ② 박자(b)의 기준: 명세에는 박자 → 시간 식이 없다. 한 자료는 "d=8 이면 8분음표 하나가 1박"이라고 읽히는 예를 든다(예제의 초 단위 계산은 오류).
 *         이 드라이버는 가장 흔한 구현 관례를 따른다: 1박 = 4분음표, 온음표 = 4박, 음 길이 = 240000/(bpm·길이) ms. 다른 해석이 맞으면 템포가 달라진다(설계안).
 *      ③ 점의 위치: 문법은 옥타브 뒤("4c5."), 실제 파일에는 앞("4c.5")도 많음 → 둘 다 받는다(한 음표에 점 한 개만).
 *      ④ 옥타브 4 의 의미: Wikipedia 는 "A below middle C 에서 시작"이라고 적어 220 Hz 로도 읽힘 → 440 Hz 를 쓴다(두 자료 일치).
 *  - 주파수: 12평균율, A4 = 440 Hz (계산). 정수 표(1/64 Hz 단위)로 옥타브를 시프트하며, 시험은 실수 계산과 0.6 Hz 이내인지 본다.
 * 설계안(문서 근거 없음): 음 사이 무음 비율(gap_pct, 기본 10 %), 늦게 호출됐을 때 지나간 음을 건너뜀, 이름 최대 15자까지 허용(명세는 10자, 일부 앱 15자), b 는 25~900 의 모든 정수를 허용(명세는 목록).
 * 미확인: 패시브 부저의 공진 주파수와 음량(부저 사양 미확인), 부저의 가청 범위 밖 음, 보드 동작. 보드에서 실행해 보지 않았다. */
#ifndef RTTTL_H
#define RTTTL_H
#include <stdint.h>
#include "axi_timer_pwm.h"

#define RTTTL_OK          0
#define RTTTL_END         0     /* rtttl_next: 음표가 더 없음 */
#define RTTTL_NOTE        1     /* rtttl_next: 음표 하나를 돌려줌 */
#define RTTTL_ERR_ARGS    (-1)
#define RTTTL_ERR_FORMAT  (-2)  /* 형식 오류(헤더 또는 음표) */

/* 사실표와 대조되는 상수 */
#define RTTTL_NAME_MAX         10   /* 명세의 이름 길이 */
#define RTTTL_NAME_LIMIT       15   /* 받아 주는 최대 길이(설계안) */
#define RTTTL_DEFAULT_DUR      4
#define RTTTL_DEFAULT_OCT      6
#define RTTTL_DEFAULT_BPM      63
#define RTTTL_OCT_MIN          4
#define RTTTL_OCT_MAX          7
#define RTTTL_BPM_MIN          25
#define RTTTL_BPM_MAX          900
#define RTTTL_BEATS_PER_WHOLE  4
#define RTTTL_A4_HZ            440
#define RTTTL_GAP_PCT_DEFAULT  10   /* 설계안 */

typedef struct {
    const char *name; unsigned name_len;     /* 원 문자열 안의 이름 위치(NUL 로 끝나지 않음) */
    uint8_t def_dur, def_oct; uint16_t bpm;
    const char *p;                           /* 다음 음표 위치(원 문자열 안) */
} rtttl_t;

typedef struct { uint16_t freq_hz; uint32_t dur_ms; } rtttl_note_t;   /* freq_hz = 0 이면 쉼표 */

/* 헤더를 읽는다. s 는 NUL 로 끝나는 문자열이며 호출하는 동안 살아 있어야 한다. 오류면 RTTTL_ERR_FORMAT */
int rtttl_open(rtttl_t *r, const char *s);
/* 음표 하나. 반환: RTTTL_NOTE(음표를 채움) / RTTTL_END / RTTTL_ERR_FORMAT(그 뒤로는 계속 오류가 아니라 END) */
int rtttl_next(rtttl_t *r, rtttl_note_t *n);
uint16_t rtttl_freq_hz(unsigned octave, unsigned semitone);           /* 옥타브 4~7, semitone 0=C ... 11=B. 범위 밖은 0 */
uint32_t rtttl_dur_ms(unsigned bpm, unsigned dur, int dotted);        /* 반올림. 인자가 범위 밖이면 0 */

/* ---- 비차단 재생 ---- */
typedef struct {
    rtttl_t r; uintptr_t pwm_base; uint32_t clk_hz;
    uint32_t note_start, tone_end, note_end;  /* ms */
    uint8_t gap_pct, playing; uint16_t last_hz;   /* last_hz: 부저에 마지막으로 쓴 주파수(0 = 꺼짐) */
    rtttl_note_t cur;
} rtttl_player_t;

/* 재생을 시작한다. 첫 음이 now_ms 에 바로 나간다. 형식 오류면 아무것도 켜지 않고 오류 */
int rtttl_play_start(rtttl_player_t *p, const char *s, uintptr_t pwm_base, uint32_t clk_hz, uint32_t now_ms);
/* 메인 루프에서 자주 호출. 반환: 1 = 재생 중, 0 = 끝났거나 멈춤(부저 꺼짐), 음수 = 오류(부저 꺼짐). now_ms 는 mclock_millis() 같은 값(넘침은 뺄셈으로 안전) */
int rtttl_play_poll(rtttl_player_t *p, uint32_t now_ms);
void rtttl_play_stop(rtttl_player_t *p);
void rtttl_set_gap_pct(rtttl_player_t *p, unsigned pct);   /* 0~50 */
#endif
