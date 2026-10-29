/* 블루투스 UART 모듈(HC-06, HM-10) 시험: 공통 송수신(무음 구간 판단), 명령 만들기, 응답 해석, 모듈 시뮬레이터 위의 연산.
 * 한계: 모듈 시뮬레이터는 내가 문서 요약(HC-06: Martyn Currey 글, HM-10: 데이터시트 Itead 사본·DSD TECH V545)에서 옮긴 규칙이다.
 * 실제 모듈이 문서와 다르게 동작하면 이 시험은 잡지 못한다. 응답이 몇 ms 뒤에 오는지, 몇 조각으로 나뉘는지는 내가 가정한 값이다.
 * 빌드: cc -include uart/tb/mock_uart.h -I uart/sw uart/tb/test_bt.c uart/sw/uartlite.c uart/sw/btmod.c uart/sw/hc06.c uart/sw/hm10.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uartlite.h"
#include "hc06.h"
#include "hm10.h"

static int fails;
#define CHECK(c, msg) do { if (!(c)) { fails++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); } } while (0)

/* ---------- 모의 UART Lite ---------- */
static uint8_t rxq[4096]; static unsigned rh, rt;
static char txlog[1024]; static unsigned txn;
static uint32_t pend_err;
static void feed_bytes(const char *s, size_t n) { for (size_t i = 0; i < n; i++) rxq[rt++ % 4096] = (uint8_t)s[i]; }
uint32_t mock_ul_rd(uint32_t off)
{
    if (off == UL_STAT) { uint32_t s = pend_err | UL_ST_TXEMPTY; pend_err = 0; if (rh != rt) s |= UL_ST_RXVALID; return s; }
    if (off == UL_RX) { if (rh == rt) return 0; return rxq[rh++ % 4096]; }
    return 0;
}
void mock_ul_wr(uint32_t off, uint32_t v) { if (off == UL_TX && txn < sizeof txlog) txlog[txn++] = (char)v; }

/* ---------- 모듈 시뮬레이터 (시간은 delay 콜백이 1ms 씩 진행) ---------- */
enum { M_HC06, M_HM10 };
typedef struct { unsigned due; char d[80]; size_t n; } chunk_t;
static struct {
    int type, connected;
    int hm_variant;                  /* 0: "OK+Set:<값>", 1: 이름만 "OK+SetName:<값>", 2: "OK+Set<값>"(콜론 없음) */
    unsigned reply_after, gap, split;/* 처리 후 첫 조각까지(ms), 조각 사이(ms), 0 이면 한 조각 아니면 이 바이트 수씩 */
    unsigned now, seen_tx, proc_at;
    chunk_t ch[16]; unsigned nch;
    char name[128]; unsigned baud; char pin[128];
    int silent, garbage, err_on_deliver;
    char raw[32]; size_t rawn;       /* 0 이 아니면 모든 명령에 이 바이트열(NUL 포함 가능)로 답함 */
    unsigned delays, ncmd;
} M;

static void reply_n(const char *s, size_t n)
{
    unsigned t = M.proc_at + M.reply_after;
    M.nch = 0;
    if (!M.split) { M.ch[0].due = t; M.ch[0].n = n; memcpy(M.ch[0].d, s, n); M.nch = 1; return; }
    for (size_t i = 0; i < n && M.nch < 16; i += M.split, t += M.gap) {
        size_t c = n - i < M.split ? n - i : M.split;
        M.ch[M.nch].due = t; M.ch[M.nch].n = c; memcpy(M.ch[M.nch].d, s + i, c); M.nch++;
    }
}
static void reply(const char *s) { reply_n(s, strlen(s)); }
static int starts(const char *c, const char *p) { return strncmp(c, p, strlen(p)) == 0; }
static void process(const char *cmd)
{
    char r[256];
    M.ncmd++;
    if (M.silent) return;
    if (M.garbage) { reply("\xFF\xFE\x80"); return; }
    if (M.rawn) { reply_n(M.raw, M.rawn); return; }
    if (M.type == M_HC06) {
        if (M.connected) return;                                                   /* 연결 중에는 AT 를 받지 않음(문서) */
        if (!strcmp(cmd, "AT")) reply("OK");
        else if (!strcmp(cmd, "AT+VERSION")) reply("OKlinvorV1.8");
        else if (starts(cmd, "AT+NAME") && strlen(cmd) > 7 && strlen(cmd) <= 7 + 20) { snprintf(M.name, sizeof M.name, "%s", cmd + 7); reply("OKsetname"); }
        else if (starts(cmd, "AT+PIN") && strlen(cmd) == 10) { snprintf(M.pin, sizeof M.pin, "%s", cmd + 6); reply("OKsetPIN"); }
        else if (starts(cmd, "AT+BAUD") && strlen(cmd) == 8) {
            static const unsigned B[12] = { 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1382400 };
            char c = cmd[7]; int i = (c >= '1' && c <= '9') ? c - '1' : (c >= 'A' && c <= 'C') ? c - 'A' + 9 : -1;
            if (i >= 0) { M.baud = B[i]; snprintf(r, sizeof r, "OK%u", B[i]); reply(r); }
        }
        return;                                                                    /* 모르는 명령: 응답 없음(문서에 오류 문자열이 없음) */
    }
    /* HM-10 */
    if (!strcmp(cmd, "AT")) { reply(M.connected ? "OK+LOST" : "OK"); return; }
    if (M.connected) return;
    if (!strcmp(cmd, "AT+NAME?")) { snprintf(r, sizeof r, "OK+NAME%s", M.name[0] ? M.name : "HMSoft"); reply(r); }
    else if (!strcmp(cmd, "AT+BAUD?")) reply("OK+Get:0");
    else if (!strcmp(cmd, "AT+ADDR?")) reply("OK+ADDR:00:11:22:33:44:55");
    else if (starts(cmd, "AT+NAME") && cmd[strlen(cmd) - 1] != '?') {
        snprintf(M.name, sizeof M.name, "%s", cmd + 7);
        snprintf(r, sizeof r, M.hm_variant == 1 ? "OK+SetName:%s" : M.hm_variant == 2 ? "OK+Set%s" : "OK+Set:%s", M.name); reply(r);
    }
    else if (starts(cmd, "AT+BAUD") || starts(cmd, "AT+ROLE") || starts(cmd, "AT+MODE") || starts(cmd, "AT+TYPE") || starts(cmd, "AT+POWE") || starts(cmd, "AT+NOTI") || starts(cmd, "AT+PASS")) {
        snprintf(r, sizeof r, M.hm_variant == 2 ? "OK+Set%s" : "OK+Set:%s", cmd + 7); reply(r);
    }
    else if (!strcmp(cmd, "AT+RESET")) reply("OK+RESET");
    else if (!strcmp(cmd, "AT+RENEW")) reply("OK+RENEW");
    else if (!strcmp(cmd, "AT+SLEEP")) reply("OK+SLEEP");
    else if (!strcmp(cmd, "AT+START")) reply("OK+START");
}
static void tick(void *ctx, uint32_t us)
{
    (void)ctx;
    CHECK(us == 1000u, "delay 는 항상 1000us 단위");
    M.now++; M.delays++;
    if (txn > M.seen_tx) {                      /* 새 명령이 도착했다고 보고 처리(명령 전체가 한 번에 쓰였다고 가정) */
        char cmd[128]; unsigned n = txn - M.seen_tx; if (n > 127) n = 127;
        memcpy(cmd, txlog + M.seen_tx, n); cmd[n] = 0; M.seen_tx = txn;
        M.proc_at = M.now; process(cmd);
    }
    for (unsigned i = 0; i < M.nch; i++)
        if (M.ch[i].n && M.ch[i].due <= M.now) {
            feed_bytes(M.ch[i].d, M.ch[i].n); M.ch[i].n = 0;
            if (M.err_on_deliver) pend_err |= UL_ST_FRAME;
        }
}
static uartlite_t U;
static bt_io_t IO = { &U, tick, 0, 100, 20 };
static void reset(int type)
{
    memset(&M, 0, sizeof M); M.type = type; M.reply_after = 5; M.baud = 9600;
    rh = rt = 0; txn = 0; pend_err = 0;
    uartlite_init(&U, 0); IO.first_ms = 100; IO.quiet_ms = 20;
}
static int tx_is(const char *s) { return txn == strlen(s) && memcmp(txlog, s, txn) == 0; }
/* 정확한 크기의 힙 버퍼로 만들어 ASan 이 한 바이트 넘침도 잡도록 한다 */
static char *exact(int n) { return (char *)malloc((size_t)n + 1); }

static void t_xfer(void)
{
    printf("[bt_xfer]\n");
    char r[16]; size_t n = 99;
    reset(M_HC06);
    CHECK(bt_xfer(&IO, "AT", 2, r, sizeof r, &n) == BT_OK && n == 2 && strcmp(r, "OK") == 0, "응답을 모음");
    CHECK(tx_is("AT"), "명령은 줄 끝 문자 없이 정확히 2바이트");
    CHECK(M.delays == 26, "시간: 첫 바이트 6번째 지연 + 무음 20번 = 26");
    reset(M_HC06); M.silent = 1;
    CHECK(bt_xfer(&IO, "AT", 2, r, sizeof r, &n) == BT_ERR_TIMEOUT && n == 0 && M.delays == 100, "응답이 없으면 first_ms(100)번 지연 뒤 TIMEOUT");
    reset(M_HC06); M.reply_after = 99;
    CHECK(bt_xfer(&IO, "AT", 2, r, sizeof r, &n) == BT_OK, "첫 바이트가 first_ms 번째 지연 직후에 오면 받음");
    reset(M_HC06); M.reply_after = 100;
    CHECK(bt_xfer(&IO, "AT", 2, r, sizeof r, &n) == BT_ERR_TIMEOUT, "한 틱 늦으면 TIMEOUT(경계)");
    reset(M_HC06); IO.first_ms = 0; M.reply_after = 0;
    CHECK(bt_xfer(&IO, "AT", 2, r, sizeof r, &n) == BT_ERR_TIMEOUT && M.delays == 0, "first_ms=0 이면 기다리지 않음");
    /* 조각난 응답 */
    reset(M_HC06); M.split = 3; M.gap = 10;
    CHECK(bt_xfer(&IO, "AT+VERSION", 10, r, sizeof r, &n) == BT_OK && n == 12 && strcmp(r, "OKlinvorV1.8") == 0, "10ms 간격 조각은 quiet_ms(20) 안이라 하나로 이어짐");
    reset(M_HC06); M.split = 3; M.gap = 20;
    CHECK(bt_xfer(&IO, "AT+VERSION", 10, r, sizeof r, &n) == BT_OK && n == 12, "정확히 quiet_ms 간격이면 이어짐(경계)");
    reset(M_HC06); M.split = 3; M.gap = 21;
    CHECK(bt_xfer(&IO, "AT+VERSION", 10, r, sizeof r, &n) == BT_OK && n == 3 && strcmp(r, "OKl") == 0, "21ms 간격이면 첫 조각에서 끊김(문서화된 한계)");
    for (int i = 0; i < 200; i++) tick(0, 1000);   /* 늦은 조각이 FIFO 에 도착한 상태에서 */
    CHECK(bt_xfer(&IO, "AT", 2, r, sizeof r, &n) == BT_OK && n == 2 && strcmp(r, "OK") == 0, "다음 거래 전에 도착해 있던 늦은 조각은 버려짐");
    /* 버퍼 */
    reset(M_HC06);
    { char *h = exact(2); int e = bt_xfer(&IO, "AT", 2, h, 3, &n); CHECK(e == BT_OK && n == 2 && strcmp(h, "OK") == 0, "cap 3 에 응답 2바이트 + NUL 이 정확히 들어감"); free(h); }
    reset(M_HC06);
    { char *h = exact(1); int e = bt_xfer(&IO, "AT", 2, h, 2, &n); CHECK(e == BT_ERR_NOSPACE && n == 1 && h[0] == 'O' && h[1] == 0, "cap 2 에는 1바이트만 담고 NOSPACE, NUL 종료"); free(h); }
    reset(M_HC06);
    CHECK(bt_xfer(&IO, "AT", 2, r, 1, &n) == BT_ERR_ARGS && bt_xfer(&IO, "AT", 0, r, sizeof r, &n) == BT_ERR_ARGS &&
          bt_xfer(&IO, 0, 2, r, sizeof r, &n) == BT_ERR_ARGS && bt_xfer(&IO, "AT", 2, 0, 16, &n) == BT_ERR_ARGS &&
          bt_xfer(0, "AT", 2, r, sizeof r, &n) == BT_ERR_ARGS, "인자 검사");
    CHECK(txn == 0, "인자 오류면 아무것도 보내지 않음");
    { bt_io_t b1 = { 0, tick, 0, 10, 10 }, b2 = { &U, 0, 0, 10, 10 };
      CHECK(bt_xfer(&b1, "AT", 2, r, sizeof r, &n) == BT_ERR_ARGS && bt_xfer(&b2, "AT", 2, r, sizeof r, &n) == BT_ERR_ARGS, "u·delay_us 가 NULL"); }
    CHECK(bt_xfer(&IO, "AT", 2, r, sizeof r, 0) == BT_OK, "rlen 은 NULL 허용");
    /* 묵은 수신 바이트 */
    reset(M_HC06); feed_bytes("XYZ", 3);
    CHECK(bt_xfer(&IO, "AT", 2, r, sizeof r, &n) == BT_OK && strcmp(r, "OK") == 0, "명령 전에 쌓인 묵은 바이트는 버림");
    reset(M_HC06); { char junk[512]; memset(junk, 'x', sizeof junk); feed_bytes(junk, sizeof junk); }
    CHECK(bt_xfer(&IO, "AT", 2, r, sizeof r, &n) == BT_OK && strcmp(r, "OK") == 0, "묵은 바이트 512개까지 버림");
    reset(M_HC06); { char junk[513]; memset(junk, 'x', sizeof junk); feed_bytes(junk, sizeof junk); }
    CHECK(bt_xfer(&IO, "AT", 2, r, sizeof r, &n) == BT_OK && n == 3 && strcmp(r, "xOK") == 0, "513개면 1개가 남음(상한 BT_FLUSH_MAX = 512)");
    /* UART 오류 */
    reset(M_HC06); pend_err = UL_ST_FRAME;
    CHECK(bt_xfer(&IO, "AT", 2, r, sizeof r, &n) == BT_OK, "명령 전의 묵은 오류 비트는 무시");
    reset(M_HC06); M.err_on_deliver = 1;
    CHECK(bt_xfer(&IO, "AT", 2, r, sizeof r, &n) == BT_ERR_UART && n == 2 && strcmp(r, "OK") == 0, "응답 중 프레임 오류 → UART 오류, 받은 데이터는 남김");
    reset(M_HC06); M.garbage = 1;
    CHECK(bt_xfer(&IO, "AT", 2, r, sizeof r, &n) == BT_OK && n == 3 && (unsigned char)r[0] == 0xFF && (unsigned char)r[2] == 0x80, "쓰레기 바이트도 그대로 돌려줌(해석은 호출자)");
    reset(M_HC06); memcpy(M.raw, "O\0K", 3); M.rawn = 3;
    CHECK(bt_xfer(&IO, "AT", 2, r, sizeof r, &n) == BT_OK && n == 3 && r[0] == 'O' && r[1] == 0 && r[2] == 'K', "0x00 바이트도 응답에 포함");
    reset(M_HC06); M.err_on_deliver = 1; M.reply_after = 5;
    { char *h = exact(1); int e = bt_xfer(&IO, "AT", 2, h, 2, &n); CHECK(e == BT_ERR_NOSPACE, "넘침이 UART 오류보다 우선"); free(h); }
    /* 응답이 정확히 cap-1 바이트일 때와 한 바이트 넘칠 때 */
    reset(M_HC06); M.split = 0;
    { char *h = exact(12); int e = bt_xfer(&IO, "AT+VERSION", 10, h, 13, &n); CHECK(e == BT_OK && n == 12, "응답 12바이트가 cap 13 에 꼭 맞음"); free(h); }
    reset(M_HC06);
    { char *h = exact(11); int e = bt_xfer(&IO, "AT+VERSION", 10, h, 12, &n); CHECK(e == BT_ERR_NOSPACE && n == 11 && strlen(h) == 11, "cap 12 면 11바이트 담고 NOSPACE"); free(h); }
}

static void t_hc06_cmds(void)
{
    printf("[hc06 명령]\n");
    char *b;
    struct { int (*f)(char *, size_t); const char *want; } S[] = { { hc06_cmd_at, "AT" }, { hc06_cmd_version, "AT+VERSION" } };
    for (unsigned i = 0; i < 2; i++) {
        int L = (int)strlen(S[i].want);
        b = exact(L); CHECK(S[i].f(b, (size_t)L + 1) == L && strcmp(b, S[i].want) == 0, "정확한 크기 버퍼"); free(b);
        b = exact(L); CHECK(S[i].f(b, (size_t)L) == BT_ERR_NOSPACE, "NUL 자리가 없으면 NOSPACE"); free(b);
        char t[4]; CHECK(S[i].f(0, 4) == BT_ERR_ARGS && S[i].f(t, 0) == BT_ERR_NOSPACE, "NULL / cap 0");
    }
    char c[64];
    CHECK(hc06_cmd_name(c, sizeof c, "abcde") == 12 && strcmp(c, "AT+NAMEabcde") == 0, "이름 명령");
    CHECK(hc06_cmd_name(c, 12, "abcde") == BT_ERR_NOSPACE && hc06_cmd_name(c, 13, "abcde") == 12, "이름 명령 버퍼 경계");
    CHECK(hc06_cmd_name(c, sizeof c, "") == BT_ERR_VALUE, "빈 이름");
    CHECK(hc06_cmd_name(c, sizeof c, "12345678901234567890") == 27 && hc06_cmd_name(c, sizeof c, "123456789012345678901") == BT_ERR_VALUE, "이름 20자 허용·21자 거절");
    CHECK(hc06_cmd_name(c, sizeof c, "a b") == BT_ERR_VALUE && hc06_cmd_name(c, sizeof c, "a\x7f") == BT_ERR_VALUE && hc06_cmd_name(c, sizeof c, "a\x1f") == BT_ERR_VALUE && hc06_cmd_name(c, sizeof c, "\xc3\xa9") == BT_ERR_VALUE, "이름에 공백·제어·비ASCII 거절");
    CHECK(hc06_cmd_name(c, sizeof c, "!~") == 9 && strcmp(c, "AT+NAME!~") == 0, "0x21 과 0x7E 는 허용");
    CHECK(hc06_cmd_name(0, 8, "a") == BT_ERR_ARGS && hc06_cmd_name(c, sizeof c, 0) == BT_ERR_ARGS, "이름 NULL");
    CHECK(hc06_cmd_pin(c, sizeof c, "1234") == 10 && strcmp(c, "AT+PIN1234") == 0, "PIN 명령");
    CHECK(hc06_cmd_pin(c, sizeof c, "9999") == 10 && strcmp(c, "AT+PIN9999") == 0 && hc06_cmd_pin(c, sizeof c, "0000") == 10 && hc06_cmd_pin(c, sizeof c, "5890") == 10, "PIN 의 0·9 는 허용");
    CHECK(hc06_cmd_pin(c, 10, "1234") == BT_ERR_NOSPACE && hc06_cmd_pin(c, 11, "0000") == 10, "PIN 버퍼 경계");
    CHECK(hc06_cmd_pin(c, sizeof c, "123") == BT_ERR_VALUE && hc06_cmd_pin(c, sizeof c, "12345") == BT_ERR_VALUE && hc06_cmd_pin(c, sizeof c, "12a4") == BT_ERR_VALUE &&
          hc06_cmd_pin(c, sizeof c, "123/") == BT_ERR_VALUE && hc06_cmd_pin(c, sizeof c, "123:") == BT_ERR_VALUE && hc06_cmd_pin(c, sizeof c, "") == BT_ERR_VALUE, "PIN 은 숫자 4자리만");
    CHECK(hc06_cmd_pin(c, sizeof c, "09 9") == BT_ERR_VALUE, "PIN 공백");
    CHECK(hc06_cmd_pin(0, 8, "1234") == BT_ERR_ARGS && hc06_cmd_pin(c, sizeof c, 0) == BT_ERR_ARGS, "PIN NULL");
    static const unsigned BR[12] = { 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1382400 };
    static const char *CD = "123456789ABC";
    for (int i = 0; i < 12; i++) {
        char w[16]; snprintf(w, sizeof w, "AT+BAUD%c", CD[i]);
        CHECK(hc06_baud_code(BR[i]) == CD[i], "보레이트 코드");
        CHECK(hc06_cmd_baud(c, sizeof c, BR[i]) == 8 && strcmp(c, w) == 0, "보레이트 명령");
        CHECK(hc06_cmd_baud(c, 8, BR[i]) == BT_ERR_NOSPACE && hc06_cmd_baud(c, 9, BR[i]) == 8, "보레이트 버퍼 경계");
    }
    CHECK(hc06_baud_code(0) == -1 && hc06_baud_code(1201) == -1 && hc06_baud_code(9601) == -1 && hc06_baud_code(2764800) == -1 && hc06_baud_code(300) == -1, "표에 없는 보레이트");
    CHECK(hc06_cmd_baud(c, sizeof c, 14400) == BT_ERR_VALUE && hc06_cmd_baud(0, 8, 9600) == BT_ERR_ARGS, "보레이트 값·NULL");
    CHECK(HC06_BAUD_DEFAULT == 9600u && hc06_baud_code(HC06_BAUD_DEFAULT) == '4', "기본 보레이트 9600 = 코드 4");
    CHECK(hc06_cmd_parity(c, sizeof c, 'N') == 5 && strcmp(c, "AT+PN") == 0 && hc06_cmd_parity(c, sizeof c, 'E') == 5 && strcmp(c, "AT+PE") == 0 && hc06_cmd_parity(c, sizeof c, 'O') == 5 && strcmp(c, "AT+PO") == 0, "패리티");
    CHECK(hc06_cmd_parity(c, sizeof c, 'X') == BT_ERR_VALUE && hc06_cmd_parity(c, sizeof c, 'n') == BT_ERR_VALUE && hc06_cmd_parity(c, 5, 'N') == BT_ERR_NOSPACE && hc06_cmd_parity(0, 8, 'N') == BT_ERR_ARGS, "패리티 값·경계");
    CHECK(hc06_cmd_led(c, sizeof c, 1) == 7 && strcmp(c, "AT+LED1") == 0 && hc06_cmd_led(c, sizeof c, 0) == 7 && strcmp(c, "AT+LED0") == 0 && hc06_cmd_led(c, sizeof c, 5) == 7 && strcmp(c, "AT+LED1") == 0, "LED");
    CHECK(hc06_cmd_led(c, 7, 1) == BT_ERR_NOSPACE && hc06_cmd_led(0, 8, 1) == BT_ERR_ARGS, "LED 경계");
    CHECK(hc06_cmd_role(c, sizeof c, 'S') == 9 && strcmp(c, "AT+ROLE=S") == 0 && hc06_cmd_role(c, sizeof c, 'M') == 9 && strcmp(c, "AT+ROLE=M") == 0, "역할");
    CHECK(hc06_cmd_role(c, sizeof c, 'X') == BT_ERR_VALUE && hc06_cmd_role(c, 9, 'S') == BT_ERR_NOSPACE && hc06_cmd_role(0, 9, 'S') == BT_ERR_ARGS, "역할 값·경계");
}

static void t_hc06_replies(void)
{
    printf("[hc06 응답]\n");
    CHECK(hc06_reply_ok("OK", 2) == 0 && hc06_reply_ok("OK", 1) == BT_ERR_REPLY && hc06_reply_ok("OKx", 3) == BT_ERR_REPLY && hc06_reply_ok("ok", 2) == BT_ERR_REPLY && hc06_reply_ok("O", 1) == BT_ERR_REPLY &&
          hc06_reply_ok("", 0) == BT_ERR_REPLY && hc06_reply_ok(0, 2) == BT_ERR_REPLY && hc06_reply_ok("OK\r\n", 4) == BT_ERR_REPLY && hc06_reply_ok("OK\0", 3) == BT_ERR_REPLY, "OK 는 정확히 일치");
    CHECK(hc06_reply_name("OKsetname", 9) == 0 && hc06_reply_name("OKsetname", 8) == BT_ERR_REPLY && hc06_reply_name("OKsetnam", 8) == BT_ERR_REPLY && hc06_reply_name("OKsetNAME", 9) == BT_ERR_REPLY && hc06_reply_name("OKsetnamee", 10) == BT_ERR_REPLY && hc06_reply_name("OK", 2) == BT_ERR_REPLY, "setname");
    CHECK(hc06_reply_pin("OKsetPIN", 8) == 0 && hc06_reply_pin("OKsetPIN", 7) == BT_ERR_REPLY && hc06_reply_pin("OKsetpin", 8) == BT_ERR_REPLY && hc06_reply_pin("OKsetPINx", 9) == BT_ERR_REPLY && hc06_reply_pin("OKsetname", 9) == BT_ERR_REPLY, "setPIN");
    static const unsigned BR[12] = { 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1382400 };
    for (int i = 0; i < 12; i++) {
        char w[16]; snprintf(w, sizeof w, "OK%u", BR[i]);
        CHECK(hc06_reply_baud(w, strlen(w), BR[i]) == 0, "보레이트 응답 일치");
        CHECK(hc06_reply_baud(w, strlen(w) - 1, BR[i]) == BT_ERR_REPLY, "보레이트 응답 짧음");
        CHECK(hc06_reply_baud(w, strlen(w), BR[(i + 1) % 12]) == BT_ERR_REPLY, "다른 보레이트");
    }
    CHECK(hc06_reply_baud("OK9600", 6, 96000) == BT_ERR_REPLY && hc06_reply_baud("OK", 2, 9600) == BT_ERR_REPLY && hc06_reply_baud(0, 6, 9600) == BT_ERR_REPLY, "보레이트 응답 접두·NULL");
    char o[16];
    CHECK(hc06_reply_version("OK\xff", 3, o, sizeof o) == BT_ERR_REPLY && hc06_reply_version("OKa\x1f", 4, o, sizeof o) == BT_ERR_REPLY && hc06_reply_version("OK\x7f", 3, o, sizeof o) == BT_ERR_REPLY &&
          hc06_reply_version("OK ", 3, o, sizeof o) == 0 && hc06_reply_version("OK~", 3, o, sizeof o) == 0, "버전 본문의 제어·비ASCII 거절, 0x20·0x7E 허용");
    { char z[] = "OKa\0b"; CHECK(hc06_reply_version(z, 5, o, sizeof o) == BT_ERR_REPLY, "버전 안의 NUL 거절"); }
    CHECK(hc06_reply_version("OKlinvorV1.8", 12, o, sizeof o) == 0 && strcmp(o, "linvorV1.8") == 0, "버전");
    CHECK(hc06_reply_version("OKhc01.comV2.0", 14, o, sizeof o) == 0 && strcmp(o, "hc01.comV2.0") == 0, "버전(다른 펌웨어)");
    CHECK(hc06_reply_version("OK", 2, o, sizeof o) == BT_ERR_REPLY && hc06_reply_version("Ok1", 3, o, sizeof o) == BT_ERR_REPLY && hc06_reply_version("XKa", 3, o, sizeof o) == BT_ERR_REPLY && hc06_reply_version("O", 1, o, sizeof o) == BT_ERR_REPLY, "버전 형식");
    CHECK(hc06_reply_version("OKx", 3, o, sizeof o) == 0 && strcmp(o, "x") == 0, "버전 한 글자");
    { char *h = exact(10); CHECK(hc06_reply_version("OKlinvorV1.8", 12, h, 11) == 0 && strcmp(h, "linvorV1.8") == 0, "버전 cap 11 에 꼭 맞음"); free(h); }
    { char *h = exact(9); CHECK(hc06_reply_version("OKlinvorV1.8", 12, h, 10) == BT_ERR_NOSPACE, "버전 cap 10 은 NOSPACE"); free(h); }
    CHECK(hc06_reply_version("OKx", 3, 0, 8) == BT_ERR_ARGS && hc06_reply_version(0, 3, o, 8) == BT_ERR_ARGS && hc06_reply_version("OKx", 3, o, 0) == BT_ERR_ARGS, "버전 인자");
}

static void t_hc06_ops(void)
{
    printf("[hc06 연산]\n");
    char v[16];
    reset(M_HC06);
    CHECK(hc06_probe(&IO) == 0 && tx_is("AT"), "probe");
    reset(M_HC06);
    CHECK(hc06_get_version(&IO, v, sizeof v) == 0 && strcmp(v, "linvorV1.8") == 0 && tx_is("AT+VERSION"), "버전 읽기");
    reset(M_HC06);
    CHECK(hc06_get_version(&IO, v, 5) == BT_ERR_NOSPACE, "버전 출력 버퍼 작음");
    reset(M_HC06);
    { char *h = exact(10); CHECK(hc06_get_version(&IO, h, 11) == 0 && strcmp(h, "linvorV1.8") == 0, "연산: 버전 cap 11 에 꼭 맞음"); free(h); }
    reset(M_HC06);
    { char *h = exact(9); CHECK(hc06_get_version(&IO, h, 10) == BT_ERR_NOSPACE, "연산: 버전 cap 10 은 NOSPACE"); free(h); }
    reset(M_HC06);
    CHECK(hc06_set_name(&IO, "Basys3") == 0 && tx_is("AT+NAMEBasys3") && strcmp(M.name, "Basys3") == 0, "이름 설정");
    reset(M_HC06);
    CHECK(hc06_set_pin(&IO, "4321") == 0 && tx_is("AT+PIN4321") && strcmp(M.pin, "4321") == 0, "PIN 설정");
    reset(M_HC06);
    CHECK(hc06_set_baud(&IO, 115200) == 0 && tx_is("AT+BAUD8") && M.baud == 115200, "보레이트 설정");
    reset(M_HC06);
    CHECK(hc06_set_baud(&IO, 1382400) == 0 && tx_is("AT+BAUDC") && M.baud == 1382400, "보레이트 코드 C");
    /* 잘못된 값은 아무것도 보내지 않는다 */
    reset(M_HC06);
    CHECK(hc06_set_name(&IO, "") == BT_ERR_VALUE && hc06_set_name(&IO, "a b") == BT_ERR_VALUE && hc06_set_pin(&IO, "12") == BT_ERR_VALUE && hc06_set_baud(&IO, 5) == BT_ERR_VALUE && txn == 0 && M.delays == 0, "값 오류는 송신 전에 걸러냄");
    CHECK(hc06_set_name(&IO, 0) == BT_ERR_ARGS && hc06_set_pin(&IO, 0) == BT_ERR_ARGS && txn == 0, "NULL");
    /* 모듈 이상 */
    reset(M_HC06); M.silent = 1;
    CHECK(hc06_probe(&IO) == BT_ERR_TIMEOUT && hc06_get_version(&IO, v, sizeof v) == BT_ERR_TIMEOUT && hc06_set_baud(&IO, 9600) == BT_ERR_TIMEOUT, "응답 없음");
    reset(M_HC06); M.connected = 1;
    CHECK(hc06_probe(&IO) == BT_ERR_TIMEOUT && M.ncmd == 1, "연결 중에는 AT 를 받지 않음 → TIMEOUT");
    reset(M_HC06); M.garbage = 1;
    CHECK(hc06_probe(&IO) == BT_ERR_REPLY && hc06_set_name(&IO, "x") == BT_ERR_REPLY && hc06_set_pin(&IO, "1234") == BT_ERR_REPLY && hc06_set_baud(&IO, 9600) == BT_ERR_REPLY, "쓰레기 응답 → REPLY");
    reset(M_HC06); M.err_on_deliver = 1;
    CHECK(hc06_probe(&IO) == BT_ERR_UART, "보레이트 불일치 흉내(프레임 오류) → UART 오류");
    /* 이름 변경 응답이 보레이트 응답으로 오면 거절 */
    reset(M_HC06); feed_bytes("OK", 2);
    CHECK(hc06_set_name(&IO, "n") == 0, "묵은 'OK' 는 버려지고 새 응답으로 판단");
    /* 느린 응답 / 경계 */
    reset(M_HC06); IO.first_ms = 6; M.reply_after = 5;
    CHECK(hc06_probe(&IO) == 0, "first_ms 6 이고 응답 6번째에 오면 성공");
    reset(M_HC06); IO.first_ms = 5; M.reply_after = 5;
    CHECK(hc06_probe(&IO) == BT_ERR_TIMEOUT, "first_ms 5 면 실패");
    /* 거래가 연달아 일어난다 */
    reset(M_HC06);
    CHECK(hc06_probe(&IO) == 0 && hc06_set_name(&IO, "A") == 0 && hc06_probe(&IO) == 0 && M.ncmd == 3, "연속 거래");
}

static void t_hm10_cmds(void)
{
    printf("[hm10 명령]\n");
    char c[64]; char *b;
    CHECK(hm10_cmd_at(c, sizeof c) == 2 && strcmp(c, "AT") == 0 && hm10_cmd_at(c, 2) == BT_ERR_NOSPACE && hm10_cmd_at(0, 8) == BT_ERR_ARGS, "AT");
    static const char *Q[] = { "NAME", "ADDR", "VERS", "BAUD", "ROLE", "MODE", "PASS", "TYPE", "POWE", "NOTI" };
    for (unsigned i = 0; i < 10; i++) {
        char w[16]; snprintf(w, sizeof w, "AT+%s?", Q[i]);
        b = exact(8); CHECK(hm10_cmd_query(b, 9, Q[i]) == 8 && strcmp(b, w) == 0, "조회 명령"); free(b);
        b = exact(7); CHECK(hm10_cmd_query(b, 8, Q[i]) == BT_ERR_NOSPACE, "조회 명령 경계"); free(b);
    }
    CHECK(hm10_cmd_query(c, sizeof c, "name") == BT_ERR_VALUE && hm10_cmd_query(c, sizeof c, "NAME?") == BT_ERR_VALUE && hm10_cmd_query(c, sizeof c, "") == BT_ERR_VALUE &&
          hm10_cmd_query(c, sizeof c, "RESET") == BT_ERR_VALUE && hm10_cmd_query(c, sizeof c, "NAM") == BT_ERR_VALUE && hm10_cmd_query(c, sizeof c, "NAMEX") == BT_ERR_VALUE, "조회는 허용 목록만");
    CHECK(hm10_cmd_query(0, 8, "NAME") == BT_ERR_ARGS && hm10_cmd_query(c, sizeof c, 0) == BT_ERR_ARGS, "조회 NULL");
    static const char *S[] = { "RESET", "RENEW", "SLEEP", "START" };
    for (unsigned i = 0; i < 4; i++) {
        char w[16]; snprintf(w, sizeof w, "AT+%s", S[i]);
        CHECK(hm10_cmd_simple(c, sizeof c, S[i]) == 8 && strcmp(c, w) == 0 && hm10_cmd_simple(c, 8, S[i]) == BT_ERR_NOSPACE, "단순 명령");
    }
    CHECK(hm10_cmd_simple(c, sizeof c, "NAME") == BT_ERR_VALUE && hm10_cmd_simple(c, sizeof c, "reset") == BT_ERR_VALUE && hm10_cmd_simple(c, sizeof c, "") == BT_ERR_VALUE && hm10_cmd_simple(0, 8, "RESET") == BT_ERR_ARGS && hm10_cmd_simple(c, sizeof c, 0) == BT_ERR_ARGS, "단순 명령 거절");
    CHECK(hm10_cmd_name(c, sizeof c, "BLE-1") == 12 && strcmp(c, "AT+NAMEBLE-1") == 0, "이름 명령");
    CHECK(hm10_cmd_name(c, 12, "BLE-1") == BT_ERR_NOSPACE && hm10_cmd_name(c, 13, "BLE-1") == 12, "이름 경계");
    CHECK(hm10_cmd_name(c, sizeof c, "123456789012") == 19 && hm10_cmd_name(c, sizeof c, "1234567890123") == BT_ERR_VALUE && hm10_cmd_name(c, sizeof c, "") == BT_ERR_VALUE, "이름 12자 허용·13자 거절");
    CHECK(hm10_cmd_name(c, sizeof c, "a?") == BT_ERR_VALUE && hm10_cmd_name(c, sizeof c, "?") == BT_ERR_VALUE && hm10_cmd_name(c, sizeof c, "a b") == BT_ERR_VALUE && hm10_cmd_name(c, sizeof c, "a\x7f") == BT_ERR_VALUE && hm10_cmd_name(c, sizeof c, "a\n") == BT_ERR_VALUE, "이름에 ?·공백·제어 거절");
    CHECK(hm10_cmd_name(0, 8, "a") == BT_ERR_ARGS && hm10_cmd_name(c, sizeof c, 0) == BT_ERR_ARGS, "이름 NULL");
    static const unsigned BR[9] = { 9600, 19200, 38400, 57600, 115200, 4800, 2400, 1200, 230400 };
    for (int i = 0; i < 9; i++) {
        char w[32]; snprintf(w, sizeof w, "AT+BAUD%d", i);
        int ok4 = (i <= 4), ok8 = (i != 7);
        CHECK(hm10_baud_code(BR[i], 0) == (ok4 ? i : -1), "코드(기본 표)");
        CHECK(hm10_baud_code(BR[i], 1) == (ok8 ? i : -1), "코드(V545 표)");
        CHECK(hm10_cmd_baud(c, sizeof c, BR[i], 0) == (ok4 ? 8 : BT_ERR_VALUE) && (!ok4 || strcmp(c, w) == 0), "명령(기본 표)");
        CHECK(hm10_cmd_baud(c, sizeof c, BR[i], 1) == (ok8 ? 8 : BT_ERR_VALUE) && (!ok8 || strcmp(c, w) == 0), "명령(V545 표)");
    }
    CHECK(hm10_baud_code(0, 1) == -1 && hm10_baud_code(9601, 1) == -1 && hm10_cmd_baud(c, 8, 9600, 0) == BT_ERR_NOSPACE && hm10_cmd_baud(0, 8, 9600, 0) == BT_ERR_ARGS, "보레이트 값·경계");
    CHECK(HM10_BAUD_DEFAULT == 9600u && hm10_baud_code(HM10_BAUD_DEFAULT, 0) == 0, "기본 보레이트 9600 = 코드 0");
    CHECK(hm10_cmd_role(c, sizeof c, 0) == 8 && strcmp(c, "AT+ROLE0") == 0 && hm10_cmd_role(c, sizeof c, 1) == 8 && strcmp(c, "AT+ROLE1") == 0 && hm10_cmd_role(c, sizeof c, 2) == BT_ERR_VALUE, "역할");
    CHECK(hm10_cmd_mode(c, sizeof c, 2) == 8 && strcmp(c, "AT+MODE2") == 0 && hm10_cmd_mode(c, sizeof c, 3) == BT_ERR_VALUE && hm10_cmd_mode(c, sizeof c, 0) == 8, "모드");
    CHECK(hm10_cmd_type(c, sizeof c, 3) == 8 && strcmp(c, "AT+TYPE3") == 0 && hm10_cmd_type(c, sizeof c, 4) == BT_ERR_VALUE, "유형");
    CHECK(hm10_cmd_power(c, sizeof c, 3) == 8 && strcmp(c, "AT+POWE3") == 0 && hm10_cmd_power(c, sizeof c, 4) == BT_ERR_VALUE, "송신 세기");
    CHECK(hm10_cmd_notify(c, sizeof c, 1) == 8 && strcmp(c, "AT+NOTI1") == 0 && hm10_cmd_notify(c, sizeof c, 0) == 8 && strcmp(c, "AT+NOTI0") == 0 && hm10_cmd_notify(c, sizeof c, 7) == 8 && strcmp(c, "AT+NOTI1") == 0, "알림");
    CHECK(hm10_cmd_role(c, 8, 1) == BT_ERR_NOSPACE && hm10_cmd_role(0, 8, 1) == BT_ERR_ARGS && hm10_cmd_role(c, 9, 0xFFFFFFFFu) == BT_ERR_VALUE, "숫자 명령 경계·큰 값");
    CHECK(hm10_cmd_pass(c, sizeof c, "000000") == 13 && strcmp(c, "AT+PASS000000") == 0 && hm10_cmd_pass(c, sizeof c, "999999") == 13, "암호");
    CHECK(hm10_cmd_pass(c, 13, "123456") == BT_ERR_NOSPACE && hm10_cmd_pass(c, 14, "123456") == 13, "암호 경계");
    CHECK(hm10_cmd_pass(c, sizeof c, "12345") == BT_ERR_VALUE && hm10_cmd_pass(c, sizeof c, "1234567") == BT_ERR_VALUE && hm10_cmd_pass(c, sizeof c, "12345a") == BT_ERR_VALUE && hm10_cmd_pass(c, sizeof c, "12345/") == BT_ERR_VALUE && hm10_cmd_pass(c, sizeof c, "12345:") == BT_ERR_VALUE && hm10_cmd_pass(c, sizeof c, "") == BT_ERR_VALUE, "암호는 숫자 6자리");
    CHECK(hm10_cmd_pass(0, 8, "123456") == BT_ERR_ARGS && hm10_cmd_pass(c, sizeof c, 0) == BT_ERR_ARGS, "암호 NULL");
}

static void t_hm10_parse(void)
{
    printf("[hm10 응답]\n");
    hm10_reply_t t; const char *v;
#define P(s, rc) hm10_parse((s), strlen(s), &t) == (rc)
#define L(s) (void)hm10_parse((s), strlen(s), &t)
    CHECK(P("OK", 0) && t.ok_only == 1 && t.tag[0] == 0 && t.value[0] == 0, "OK");
    CHECK(P("OK+LOST", 0) && !t.ok_only && strcmp(t.tag, "LOST") == 0 && t.value[0] == 0, "OK+LOST");
    CHECK(P("OK+Set:1", 0) && strcmp(t.tag, "Set") == 0 && strcmp(t.value, "1") == 0 && !t.ok_only, "OK+Set:1");
    CHECK(P("OK+Get:0", 0) && strcmp(t.tag, "Get") == 0 && strcmp(t.value, "0") == 0, "OK+Get:0");
    CHECK(P("OK+NAMEHMSoft", 0) && strcmp(t.tag, "NAMEHMSoft") == 0 && t.value[0] == 0, "콜론 없는 이름 응답");
    CHECK(P("OK+ADDR:00:11:22:33:44:55", 0) && strcmp(t.tag, "ADDR") == 0 && strcmp(t.value, "00:11:22:33:44:55") == 0, "첫 콜론에서만 나눔");
    CHECK(P("OK+X:", 0) && strcmp(t.tag, "X") == 0 && t.value[0] == 0, "빈 값");
    CHECK(P("OK+SetName:abc", 0) && strcmp(t.tag, "SetName") == 0 && strcmp(t.value, "abc") == 0, "SetName 변형");
    CHECK(P("OK+123456789012345", 0) && strlen(t.tag) == 15, "태그 15자");
    CHECK(P("OK+1234567890123456", BT_ERR_NOSPACE), "태그 16자 → NOSPACE");
    CHECK(P("OK+T:1234567890123456789012345678901", 0) && strlen(t.value) == 31, "값 31자");
    CHECK(P("OK+T:12345678901234567890123456789012", BT_ERR_NOSPACE), "값 32자 → NOSPACE");
    CHECK(P("OK+1234567890123456:1", BT_ERR_NOSPACE), "태그만 넘쳐도 NOSPACE");
    CHECK(P("", BT_ERR_REPLY) && P("O", BT_ERR_REPLY) && P("ok", BT_ERR_REPLY) && P("OKx", BT_ERR_REPLY) && P("OK ", BT_ERR_REPLY) && P("OK+", BT_ERR_REPLY) && P("OK+:x", BT_ERR_REPLY) &&
          P("XK+LOST", BT_ERR_REPLY) && P("Ok+LOST", BT_ERR_REPLY) && P("+OK", BT_ERR_REPLY) && P("OK-LOST", BT_ERR_REPLY), "형식 오류");
    { char z[] = "OK+AB\0CD"; CHECK(hm10_parse(z, 8, &t) == BT_ERR_REPLY, "태그 안의 NUL 거절"); }
    { char z[] = "OK+T:a\0b"; CHECK(hm10_parse(z, 8, &t) == BT_ERR_REPLY, "값 안의 NUL 거절"); }
    CHECK(P("OK+T:\x7f", BT_ERR_REPLY) && P("OK+T:\xff\xfe", BT_ERR_REPLY) && P("OK+T:\x1f", BT_ERR_REPLY) && P("OK+T:\n", BT_ERR_REPLY) && P("OK+LOST\r\n", BT_ERR_REPLY), "제어·비ASCII 바이트 거절(설계안)");
    CHECK(P("OK+Get:HMSoft V540", 0) && strcmp(t.value, "HMSoft V540") == 0 && P("OK+T:~", 0) && P("OK+T: ", 0), "공백(0x20)과 ~(0x7E)는 허용");
    CHECK(hm10_parse("OK", 2, 0) == BT_ERR_ARGS && hm10_parse(0, 2, &t) == BT_ERR_ARGS, "NULL");
    CHECK(hm10_parse("OK+LOST", 2, &t) == 0 && t.ok_only, "길이 n 만 본다(뒤는 무시)");
    CHECK(hm10_parse("OK+LOSTX", 7, &t) == 0 && strcmp(t.tag, "LOST") == 0, "길이 n 으로 자름");
    { char *h = malloc(1); h[0] = 'O'; CHECK(hm10_parse(h, 1, &t) == BT_ERR_REPLY, "한 글자 'O' 는 뒤를 읽지 않고 거절"); free(h); }
    { char *h = malloc(2); memcpy(h, "OK", 2); CHECK(hm10_parse(h, 2, &t) == 0 && t.ok_only, "정확히 2바이트 OK"); free(h); }
    { char *h = malloc(8); memcpy(h, "OK+Set:1", 8); CHECK(hm10_parse(h, 8, &t) == 0 && strcmp(t.value, "1") == 0, "경계 버퍼"); free(h); }
    /* hm10_value */
    L("OK+Set:1"); v = hm10_value(&t, "Set"); CHECK(v && strcmp(v, "1") == 0, "value Set");
    CHECK(hm10_value(&t, "Get") == 0 && hm10_value(&t, "SetX") == 0 && hm10_value(&t, "") == 0 && hm10_value(&t, 0) == 0 && hm10_value(0, "Set") == 0, "value 불일치");
    L("OK+SetName:abc");
    CHECK(hm10_value(&t, "Set") == 0, "tag 가 Set 보다 길고 값이 있으면 정확히 같아야 함");
    v = hm10_value(&t, "SetName"); CHECK(v && strcmp(v, "abc") == 0, "value SetName");
    L("OK+NAMEHMSoft"); v = hm10_value(&t, "NAME"); CHECK(v && strcmp(v, "HMSoft") == 0, "콜론 없는 변형");
    CHECK(hm10_value(&t, "ADDR") == 0, "태그 접두 불일치");
    L("OK+Setabc"); v = hm10_value(&t, "Set"); CHECK(v && strcmp(v, "abc") == 0, "Set 변형(콜론 없음)");
    L("OK+NAMEHMSoft"); CHECK(hm10_value(&t, "") == 0, "빈 want 는 NULL(값 없는 응답)");
    L("OK"); CHECK(hm10_value(&t, "") == 0, "OK 만 + 빈 want 는 NULL");
    L("OK+NAME"); v = hm10_value(&t, "NAME"); CHECK(v && *v == 0, "빈 나머지");
    L("OK"); CHECK(hm10_value(&t, "Set") == 0, "OK 만 → 값 없음");
    L("OK+X:"); v = hm10_value(&t, "X"); CHECK(v && *v == 0, "빈 값은 빈 문자열");
#undef P
}

static void t_hm10_ops(void)
{
    printf("[hm10 연산]\n");
    hm10_reply_t t; const char *v;
    reset(M_HM10);
    CHECK(hm10_probe(&IO) == 0 && tx_is("AT"), "probe");
    reset(M_HM10); M.connected = 1;
    CHECK(hm10_probe(&IO) == BT_ERR_REPLY, "연결 중 AT → OK+LOST 라서 probe 실패");
    CHECK(hm10_query(&IO, "NAME", &t) == BT_ERR_TIMEOUT, "연결 중 명령은 응답 없음");
    for (int var = 0; var < 3; var++) {
        reset(M_HM10); M.hm_variant = var;
        CHECK(hm10_set_name(&IO, "Basys3") == 0 && tx_is("AT+NAMEBasys3") && strcmp(M.name, "Basys3") == 0, "이름 설정(응답 변형)");
        reset(M_HM10); M.hm_variant = var;
        CHECK(hm10_set_baud(&IO, 115200, 0) == 0 && tx_is("AT+BAUD4"), "보레이트 설정(응답 변형)");
    }
    reset(M_HM10);
    CHECK(hm10_set_baud(&IO, 230400, 0) == BT_ERR_VALUE && txn == 0, "기본 표에 없는 보레이트는 보내지 않음");
    CHECK(hm10_set_baud(&IO, 230400, 1) == 0 && tx_is("AT+BAUD8"), "V545 표");
    reset(M_HM10);
    CHECK(hm10_set_baud(&IO, 1200, 1) == BT_ERR_VALUE && txn == 0, "1200 은 보내지 않음");
    CHECK(hm10_set_name(&IO, "a?") == BT_ERR_VALUE && hm10_set_name(&IO, 0) == BT_ERR_ARGS && txn == 0, "이름 오류는 송신 전");
    reset(M_HM10);
    CHECK(hm10_query(&IO, "NAME", &t) == 0 && tx_is("AT+NAME?") && (v = hm10_value(&t, "NAME")) && strcmp(v, "HMSoft") == 0, "이름 조회(기본)");
    reset(M_HM10); strcpy(M.name, "Zed");
    CHECK(hm10_query(&IO, "NAME", &t) == 0 && (v = hm10_value(&t, "NAME")) && strcmp(v, "Zed") == 0, "이름 조회(설정 뒤)");
    reset(M_HM10);
    CHECK(hm10_query(&IO, "ADDR", &t) == 0 && (v = hm10_value(&t, "ADDR")) && strcmp(v, "00:11:22:33:44:55") == 0, "주소 조회");
    reset(M_HM10);
    CHECK(hm10_query(&IO, "BAUD", &t) == 0 && (v = hm10_value(&t, "Get")) && strcmp(v, "0") == 0, "보레이트 조회");
    reset(M_HM10);
    CHECK(hm10_query(&IO, "name", &t) == BT_ERR_VALUE && hm10_query(&IO, 0, &t) == BT_ERR_ARGS && txn == 0, "조회 값 오류");
    reset(M_HM10); M.silent = 1;
    CHECK(hm10_probe(&IO) == BT_ERR_TIMEOUT && hm10_set_name(&IO, "x") == BT_ERR_TIMEOUT && hm10_query(&IO, "NAME", &t) == BT_ERR_TIMEOUT, "응답 없음");
    reset(M_HM10); M.garbage = 1;
    CHECK(hm10_probe(&IO) == BT_ERR_REPLY && hm10_set_name(&IO, "x") == BT_ERR_REPLY && hm10_query(&IO, "NAME", &t) == BT_ERR_REPLY, "쓰레기 응답");
    reset(M_HM10); M.err_on_deliver = 1;
    CHECK(hm10_probe(&IO) == BT_ERR_UART, "프레임 오류");
    /* 쓰기 명령에 Set 이 아닌 응답이 오면 실패해야 함 */
    reset(M_HM10); memcpy(M.raw, "OK+Get:1", 8); M.rawn = 8;
    CHECK(hm10_set_name(&IO, "x") == BT_ERR_REPLY && hm10_set_baud(&IO, 9600, 0) == BT_ERR_REPLY, "Get 응답은 설정 성공이 아님");
    reset(M_HM10); memcpy(M.raw, "OK+Sex:1", 8); M.rawn = 8;
    CHECK(hm10_set_name(&IO, "x") == BT_ERR_REPLY && hm10_set_baud(&IO, 9600, 0) == BT_ERR_REPLY, "Se 로 시작해도 Set 이 아니면 실패");
    reset(M_HM10); memcpy(M.raw, "OK", 2); M.rawn = 2;
    CHECK(hm10_set_name(&IO, "x") == BT_ERR_REPLY && hm10_set_baud(&IO, 9600, 0) == BT_ERR_REPLY, "OK 만 오면 설정 성공이 아님");
    reset(M_HM10); memcpy(M.raw, "OK+Set:1", 8); M.rawn = 8;
    CHECK(hm10_set_name(&IO, "x") == 0 && hm10_set_baud(&IO, 9600, 0) == 0, "Set 응답이면 성공");
    reset(M_HM10); memcpy(M.raw, "OK+LOST", 7); M.rawn = 7;
    CHECK(hm10_probe(&IO) == BT_ERR_REPLY, "probe: OK+LOST 는 실패");
    /* 쓰기 명령에 조회형 응답이 오면 실패해야 함 */
    reset(M_HM10); M.split = 4; M.gap = 5;
    CHECK(hm10_set_name(&IO, "Basys3") == 0, "5ms 간격 조각도 이어짐");
    reset(M_HM10); M.split = 4; M.gap = 25;
    CHECK(hm10_set_name(&IO, "Basys3") != 0, "25ms 간격 조각은 끊겨서 실패(문서화된 한계)");
    /* 단순 명령 응답을 조회로 해석 */
    reset(M_HM10);
    { char c[16], r[32]; size_t n; int cl = hm10_cmd_simple(c, sizeof c, "RESET");
      CHECK(bt_xfer(&IO, c, (size_t)cl, r, sizeof r, &n) == 0 && strcmp(r, "OK+RESET") == 0 && hm10_parse(r, n, &t) == 0 && strcmp(t.tag, "RESET") == 0, "RESET 응답"); }
    /* set_* 는 Set 태그가 아닌 응답을 거절 */
    reset(M_HM10); feed_bytes("OK+Get:0", 8);
    CHECK(hm10_probe(&IO) == 0, "묵은 응답은 버림");
}

int main(void)
{
    t_xfer();
    t_hc06_cmds();
    t_hc06_replies();
    t_hc06_ops();
    t_hm10_cmds();
    t_hm10_parse();
    t_hm10_ops();
    printf(fails ? "FAILED %d\n" : "ALL PASS\n", fails);
    return fails != 0;
}
