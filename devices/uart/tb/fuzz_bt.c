/* 적대적 시험: 임의 바이트·임의 크기 버퍼를 블루투스 모듈 드라이버(HC-06, HM-10)에 넣어 메모리 오류·행·불변식 위반을 찾는다(ASan/UBSan 아래).
 * 불변식은 코드에서 독립적으로 다시 계산한 것(재조립, 길이, 허용 문자)이다. 모듈 쪽 동작은 흉내 내지 않고 임의 바이트열만 쓴다.
 * 빌드: cc -include uart/tb/mock_uart.h -I uart/sw uart/tb/fuzz_bt.c uart/sw/uartlite.c uart/sw/btmod.c uart/sw/hc06.c uart/sw/hm10.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uartlite.h"
#include "hc06.h"
#include "hm10.h"

static uint32_t rs = 4242;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs >> 8; }

/* 모의 UART: 예약된 (시각, 바이트) 목록을 시간이 지나면 FIFO 로 */
#define MAXB 300
static struct { unsigned due; uint8_t b; int done; } sch[MAXB]; static unsigned nsch, now_t;
static uint8_t q[MAXB + 600]; static unsigned qh, qt; static unsigned txcount; static uint32_t perr;
uint32_t mock_ul_rd(uint32_t off)
{
    if (off == UL_STAT) { uint32_t s = perr | UL_ST_TXEMPTY; perr = 0; if (qh != qt) s |= UL_ST_RXVALID; return s; }
    if (off == UL_RX) return qh != qt ? q[qh++] : 0;
    return 0;
}
void mock_ul_wr(uint32_t off, uint32_t v) { (void)v; if (off == UL_TX) txcount++; }
static unsigned ticks;
static void tick(void *c, uint32_t us)
{
    (void)c; if (us != 1000u) { printf("delay 단위 오류\n"); exit(1); }
    now_t++; ticks++;
    for (unsigned i = 0; i < nsch; i++) if (!sch[i].done && sch[i].due <= now_t) { sch[i].done = 1; q[qt++] = sch[i].b; if (rnd() % 50 == 0) perr |= UL_ST_FRAME; }
}

static int bad(const char *m) { printf("불변식 위반: %s\n", m); return 1; }

int main(void)
{
    long ok_parse = 0, ok_x = 0, ok_cmd = 0;
    /* 1) hm10_parse: 임의 바이트열(절반은 "OK" 로 시작) */
    for (long it = 0; it < 600000; it++) {
        unsigned n = rnd() % 70;
        char *h = (char *)malloc(n ? n : 1);
        for (unsigned i = 0; i < n; i++) h[i] = (char)(rnd() & 0xFF);
        if (rnd() % 2 && n >= 3) { h[0] = 'O'; h[1] = 'K'; h[2] = (rnd() % 4) ? '+' : (char)rnd(); for (unsigned i = 3; i < n; i++) if (rnd() % 3) h[i] = (char)(0x20 + rnd() % 0x5F); }
        if (rnd() % 8 == 0 && n >= 2) { h[0] = 'O'; h[1] = 'K'; n = 2; }
        hm10_reply_t t; memset(&t, 0x5A, sizeof t);
        int e = hm10_parse(h, n, &t);
        {   /* 독립 기대값: 모양이 맞고 본문이 전부 0x20~0x7E 이면 성공(또는 길이 초과), 아니면 REPLY */
            int shape = n == 2 ? (h[0] == 'O' && h[1] == 'K') : (n >= 4 && h[0] == 'O' && h[1] == 'K' && h[2] == '+' && h[3] != ':');
            int pr = 1; for (unsigned i = 3; i < n; i++) if ((unsigned char)h[i] < 0x20 || (unsigned char)h[i] > 0x7E) pr = 0;
            if (n > 2 && n < 4) shape = 0;
            if (!shape || !pr) { if (e != BT_ERR_REPLY) return bad("모양/문자 오류인데 REPLY 아님"); }
            else if (e != 0 && e != BT_ERR_NOSPACE) return bad("정상 모양인데 거절");
        }
        if (e == 0) {
            ok_parse++;
            size_t tl = strnlen(t.tag, sizeof t.tag), vl = strnlen(t.value, sizeof t.value);
            if (tl >= sizeof t.tag || vl >= sizeof t.value) return bad("tag/value 가 NUL 로 안 끝남");
            if (t.ok_only) { if (n != 2 || tl || vl) return bad("ok_only 인데 길이"); }
            else {
                if (tl == 0) return bad("태그가 비어 있음");
                if (memchr(t.tag, ':', tl)) return bad("태그에 콜론");
                const char *colon = memchr(h + 3, ':', n - 3);
                size_t want = 3 + tl + (colon ? 1 + vl : 0);
                if (want != n) return bad("재조립 길이");
                if (memcmp(h, "OK+", 3) || memcmp(h + 3, t.tag, tl)) return bad("재조립 태그");
                if (colon && memcmp(colon + 1, t.value, vl)) return bad("재조립 값");
                if (!colon && vl) return bad("콜론 없는데 값");
            }
            const char *v = hm10_value(&t, t.tag);   /* 자기 태그로 부르면 항상 값(또는 빈 나머지)이 나와야 함 */
            if (!t.ok_only && !v) return bad("hm10_value(자기 태그) NULL");
        } else if (e != BT_ERR_REPLY && e != BT_ERR_NOSPACE) return bad("알 수 없는 오류 코드");
        free(h);
    }
    /* 2) HC-06 응답 확인: 임의 바이트열 */
    for (long it = 0; it < 600000; it++) {
        unsigned n = rnd() % 24;
        char *h = (char *)malloc(n ? n : 1);
        for (unsigned i = 0; i < n; i++) h[i] = (char)(rnd() & 0xFF);
        if (rnd() % 2 && n >= 2) { h[0] = 'O'; h[1] = 'K'; if (n > 2 && rnd() % 2) for (unsigned i = 2; i < n; i++) h[i] = (char)(0x20 + rnd() % 0x5F); }
        char out[12];
        int a = hc06_reply_ok(h, n), b = hc06_reply_name(h, n), c = hc06_reply_pin(h, n), d = hc06_reply_baud(h, n, 9600), e = hc06_reply_version(h, n, out, sizeof out);
        int ra = (n == 2 && !memcmp(h, "OK", 2)), rb = (n == 9 && !memcmp(h, "OKsetname", 9)), rc = (n == 8 && !memcmp(h, "OKsetPIN", 8)), rd = (n == 6 && !memcmp(h, "OK9600", 6));
        if ((a == 0) != ra || (b == 0) != rb || (c == 0) != rc || (d == 0) != rd) return bad("HC-06 응답 판정이 독립 계산과 다름");
        int prv = 1; for (unsigned i = 2; i < n; i++) if ((unsigned char)h[i] < 0x20 || (unsigned char)h[i] > 0x7E) prv = 0;
        int exp_e = (n < 3 || h[0] != 'O' || h[1] != 'K' || !prv) ? BT_ERR_REPLY : (n - 2 >= sizeof out ? BT_ERR_NOSPACE : 0);
        if (e != exp_e) return bad("HC-06 버전 판정");
        if (e == 0 && (strlen(out) != n - 2 || memcmp(out, h + 2, n - 2))) return bad("HC-06 버전 복사");
        free(h);
    }
    /* 3) 명령 만들기: 임의 문자열·임의 cap, 정확한 힙 버퍼 */
    for (long it = 0; it < 600000; it++) {
        unsigned sl = rnd() % 30;
        char *s = (char *)malloc(sl + 1);
        for (unsigned i = 0; i < sl; i++) s[i] = (rnd() % 3) ? (char)(0x21 + rnd() % 0x5E) : (char)(rnd() & 0xFF);
        s[sl] = 0; sl = (unsigned)strlen(s);
        size_t cap = rnd() % 40;
        char *o = (char *)malloc(cap ? cap : 1);
        int r = -1, kind = (int)(rnd() % 6);
        switch (kind) {
        case 0: r = hc06_cmd_name(o, cap, s); break;
        case 1: r = hc06_cmd_pin(o, cap, s); break;
        case 2: r = hm10_cmd_name(o, cap, s); break;
        case 3: r = hm10_cmd_pass(o, cap, s); break;
        case 4: r = hm10_cmd_query(o, cap, s); break;
        default: r = hm10_cmd_simple(o, cap, s); break;
        }
        if (r >= 0) {
            ok_cmd++;
            if ((size_t)r >= cap || strlen(o) != (size_t)r || memcmp(o, "AT+", 3)) return bad("명령 길이/접두");
            for (int i = 0; i < r; i++) if ((unsigned char)o[i] < 0x21 || (unsigned char)o[i] > 0x7E) return bad("명령에 비허용 문자");
            const char *a = o + 3;
            if (kind == 0) { if (strncmp(a, "NAME", 4) || (unsigned)(r - 7) != sl || strcmp(o + 7, s)) return bad("hc06 이름 재조립"); if (sl < 1 || sl > HC06_NAME_MAX) return bad("hc06 이름 길이"); }
            if (kind == 1) { if (sl != 4 || strcmp(o + 6, s)) return bad("hc06 pin"); for (int i = 0; i < 4; i++) if (s[i] < '0' || s[i] > '9') return bad("hc06 pin 숫자"); }
            if (kind == 2) { if (sl < 1 || sl > HM10_NAME_MAX || strchr(s, '?') || strcmp(o + 7, s)) return bad("hm10 이름"); }
            if (kind == 3) { if (sl != 6 || strcmp(o + 7, s)) return bad("hm10 pass"); for (int i = 0; i < 6; i++) if (s[i] < '0' || s[i] > '9') return bad("hm10 pass 숫자"); }
            if (kind == 4) { if (o[r - 1] != '?' || strncmp(o + 3, s, sl)) return bad("조회 재조립"); }
            if (kind == 5) { if (strcmp(o + 3, s)) return bad("단순 명령 재조립"); }
        } else if (r != BT_ERR_VALUE && r != BT_ERR_NOSPACE && r != BT_ERR_ARGS) return bad("알 수 없는 오류 코드(명령)");
        free(o); free(s);
    }
    /* 숫자 명령: 모든 값 */
    for (unsigned v = 0; v < 20; v++) {
        char *o = (char *)malloc(9);
        int r1 = hm10_cmd_role(o, 9, v), r2 = hm10_cmd_mode(o, 9, v), r3 = hm10_cmd_type(o, 9, v), r4 = hm10_cmd_power(o, 9, v);
        if ((r1 == 8) != (v <= 1) || (r2 == 8) != (v <= 2) || (r3 == 8) != (v <= 3) || (r4 == 8) != (v <= 3)) return bad("숫자 명령 범위");
        free(o);
    }
    /* 4) bt_xfer: 임의 응답 시각·바이트·버퍼 크기·무음 설정 */
    uartlite_t u; bt_io_t io = { &u, tick, 0, 0, 0 };
    for (long it = 0; it < 200000; it++) {
        nsch = rnd() % (MAXB / 2); now_t = 0; ticks = 0; qh = qt = 0; perr = (rnd() % 5 == 0) ? UL_ST_OVERRUN : 0; txcount = 0;
        unsigned t0 = rnd() % 40, gapmax = 1 + rnd() % 30, t = t0;
        for (unsigned i = 0; i < nsch; i++) { sch[i].b = (uint8_t)rnd(); sch[i].done = 0; sch[i].due = t; t += (rnd() % 4 == 0) ? rnd() % gapmax : 0; }
        unsigned stale = rnd() % 5; for (unsigned i = 0; i < stale; i++) q[qt++] = (uint8_t)rnd();
        uartlite_init(&u, 0); io.first_ms = rnd() % 60; io.quiet_ms = rnd() % 30;
        size_t cap = 2 + rnd() % 20; char *resp = (char *)malloc(cap); size_t rl = 999;
        unsigned cl = 1 + rnd() % 10; char cmd[10]; memset(cmd, 'A', sizeof cmd);
        int r = bt_xfer(&io, cmd, cl, resp, cap, &rl);
        if (txcount != cl) return bad("송신 바이트 수");
        if (r != BT_OK && r != BT_ERR_TIMEOUT && r != BT_ERR_NOSPACE && r != BT_ERR_UART) return bad("알 수 없는 오류 코드(xfer)");
        if (rl >= cap || resp[rl] != 0) return bad("resp 길이/NUL");
        if (r == BT_ERR_TIMEOUT && rl != 0) return bad("TIMEOUT 인데 데이터");
        if (r == BT_OK && rl == 0) return bad("OK 인데 데이터 없음");
        if (r == BT_ERR_NOSPACE && rl != cap - 1) return bad("NOSPACE 인데 버퍼가 안 참");
        if (r == BT_ERR_TIMEOUT && ticks != io.first_ms) return bad("TIMEOUT 시간");
        unsigned bound = io.first_ms + (unsigned)(nsch + 2) * (io.quiet_ms + 1) + t0 + 300;  /* 느슨한 상한: 행이 아님을 확인 */
        if (ticks > bound) return bad("지연 횟수가 비정상적으로 큼");
        if (r == BT_OK) ok_x++;
        free(resp);
    }
    printf("parse 성공 %ld, 명령 성공 %ld, xfer 성공 %ld\n", ok_parse, ok_cmd, ok_x);
    if (ok_parse < 1000 || ok_cmd < 1000 || ok_x < 1000) return bad("성공 사례가 너무 적음(퍼저가 한쪽으로 치우침)");
    printf("ALL PASS\n");
    return 0;
}
