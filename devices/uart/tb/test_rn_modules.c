/* uart/tb/test_rn_modules.c — RN-42(Pmod BT2)·RN4871(Pmod BLE) 명령 만들기·응답 확인 시험.
 *
 * IO 고수준(rn42_enter_cmd 등)은 UART 시뮬레이터 없이 시험하기 어려우므로 이 파일에는 포함하지 않는다.
 * (시험 수준: 명령 바이트 열 + 응답 파서 함수)
 *
 * 빌드: cc -I uart/sw uart/tb/test_rn_modules.c uart/sw/rn42.c uart/sw/rn4871.c
 *       (btmod.c, uartlite.c 링크 불필요 — IO 함수 미사용)
 * SAN: cc -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all \
 *         -I uart/sw uart/tb/test_rn_modules.c uart/sw/rn42.c uart/sw/rn4871.c
 *
 * 한계:
 *   - RN4871 명령 종결자('\r') 및 보레이트 코드는 D3/미확인이므로
 *     시험은 현재 설계안의 동작을 검증하는 수준이다.
 *   - 실제 모듈 응답 형식 차이는 보드 시험으로만 확인 가능.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rn42.h"
#include "rn4871.h"

static int fails;
#define CHECK(c, msg) do { if (!(c)) { fails++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); } } while(0)
/* 정확한 크기 힙 버퍼: ASan 이 한 바이트 넘침도 잡도록 */
static char *exact(int n) { return (char *)malloc((size_t)n + 1); }

/* ═══════════════════════════════ RN-42 ══════════════════════════════════ */

static void t_rn42_enter(void)
{
    printf("[rn42_cmd_enter]\n");
    char *b;
    /* "$$$" = 3글자 */
    b = exact(3); CHECK(rn42_cmd_enter(b, 4) == 3 && strcmp(b, "$$$") == 0, "enter 3글자"); free(b);
    b = exact(2); CHECK(rn42_cmd_enter(b, 3) == BT_ERR_NOSPACE, "NUL 자리 없으면 NOSPACE"); free(b);
    char t[4];
    CHECK(rn42_cmd_enter(0, 4) == BT_ERR_ARGS, "NULL 출력");
    CHECK(rn42_cmd_enter(t, 0) == BT_ERR_NOSPACE, "cap=0");
    /* CR/LF 없음 확인 */
    b = exact(3); rn42_cmd_enter(b, 4);
    CHECK(b[0] == '$' && b[1] == '$' && b[2] == '$' && b[3] == '\0', "CR/LF 없이 정확히 '$$$'"); free(b);
}

static void t_rn42_cmds(void)
{
    printf("[rn42 명령]\n");
    char c[64]; char *b;

    /* exit: "---\r\n" = 5글자 */
    b = exact(5); CHECK(rn42_cmd_exit(b, 6) == 5 && strcmp(b, "---\r\n") == 0, "exit"); free(b);
    b = exact(4); CHECK(rn42_cmd_exit(b, 5) == BT_ERR_NOSPACE, "exit 경계"); free(b);
    CHECK(rn42_cmd_exit(0, 6) == BT_ERR_ARGS, "exit NULL");

    /* AT\r\n = 4글자 */
    b = exact(4); CHECK(rn42_cmd_at(b, 5) == 4 && strcmp(b, "AT\r\n") == 0, "AT"); free(b);
    b = exact(3); CHECK(rn42_cmd_at(b, 4) == BT_ERR_NOSPACE, "AT 경계"); free(b);
    CHECK(rn42_cmd_at(0, 5) == BT_ERR_ARGS, "AT NULL");

    /* factory: "SF,1\r\n" = 6글자 */
    b = exact(6); CHECK(rn42_cmd_factory(b, 7) == 6 && strcmp(b, "SF,1\r\n") == 0, "factory"); free(b);
    CHECK(rn42_cmd_factory(0, 7) == BT_ERR_ARGS, "factory NULL");

    /* reboot: "R,1\r\n" = 5글자 */
    b = exact(5); CHECK(rn42_cmd_reboot(b, 6) == 5 && strcmp(b, "R,1\r\n") == 0, "reboot"); free(b);
    CHECK(rn42_cmd_reboot(0, 6) == BT_ERR_ARGS, "reboot NULL");

    /* 이름: "SN,<name>\r\n" → "SN," 3 + name + "\r\n" 2 */
    CHECK(rn42_cmd_name(c, sizeof c, "Basys3") == 11 && strcmp(c, "SN,Basys3\r\n") == 0, "이름");
    CHECK(rn42_cmd_name(c, 11, "Basys3") == BT_ERR_NOSPACE && rn42_cmd_name(c, 12, "Basys3") == 11, "이름 경계");
    CHECK(rn42_cmd_name(c, sizeof c, "") == BT_ERR_VALUE, "빈 이름");
    CHECK(rn42_cmd_name(c, sizeof c, "12345678901234567890") == 25, "이름 20자 허용");
    CHECK(rn42_cmd_name(c, sizeof c, "123456789012345678901") == BT_ERR_VALUE, "이름 21자 거절");
    CHECK(rn42_cmd_name(c, sizeof c, "a b") == BT_ERR_VALUE, "공백 거절");
    CHECK(rn42_cmd_name(c, sizeof c, "a\x7f") == BT_ERR_VALUE, "0x7F 거절");
    CHECK(rn42_cmd_name(c, sizeof c, "\xc3\xa9") == BT_ERR_VALUE, "비ASCII 거절");
    CHECK(rn42_cmd_name(0, 16, "a") == BT_ERR_ARGS && rn42_cmd_name(c, sizeof c, 0) == BT_ERR_ARGS, "이름 NULL");

    /* 친근 이름: "S-,<name>\r\n" → "S-," 3 + name + "\r\n" 2 */
    CHECK(rn42_cmd_fname(c, sizeof c, "Base") == 9 && strcmp(c, "S-,Base\r\n") == 0, "친근 이름");
    CHECK(rn42_cmd_fname(c, sizeof c, "123456789012345") == 20, "친근 이름 15자 허용");
    CHECK(rn42_cmd_fname(c, sizeof c, "1234567890123456") == BT_ERR_VALUE, "친근 이름 16자 거절");
    CHECK(rn42_cmd_fname(0, 16, "a") == BT_ERR_ARGS && rn42_cmd_fname(c, sizeof c, 0) == BT_ERR_ARGS, "친근 이름 NULL");
}

static void t_rn42_baud(void)
{
    printf("[rn42 보레이트]\n");
    char c[64];
    static const unsigned BR[] = {1200,2400,4800,9600,19200,38400,57600,115200,230400,460800,921600};
    static const char *CD[] = {"12","24","48","96","19","38","57","11","23","46","92"};
    unsigned n = (unsigned)(sizeof BR / sizeof BR[0]);
    for (unsigned i = 0; i < n; i++) {
        CHECK(rn42_baud_code(BR[i]) != NULL && strcmp(rn42_baud_code(BR[i]), CD[i]) == 0, "보레이트 코드");
        char want[16]; snprintf(want, sizeof want, "SU,%s\r\n", CD[i]);
        int wl = (int)strlen(want);
        CHECK(rn42_cmd_baud(c, sizeof c, BR[i]) == wl && strcmp(c, want) == 0, "보레이트 명령");
    }
    CHECK(rn42_baud_code(0) == NULL && rn42_baud_code(9601) == NULL && rn42_baud_code(1382400) == NULL, "미지원 보레이트 NULL");
    CHECK(rn42_cmd_baud(c, sizeof c, 14400) == BT_ERR_VALUE, "미지원 보레이트 명령");
    CHECK(rn42_cmd_baud(0, 16, 115200) == BT_ERR_ARGS, "보레이트 NULL");
    /* 기본 보레이트 */
    CHECK(RN42_BAUD_DEFAULT == 115200u && rn42_baud_code(RN42_BAUD_DEFAULT) != NULL && strcmp(rn42_baud_code(RN42_BAUD_DEFAULT), "11") == 0, "기본 보레이트 코드");
}

static void t_rn42_replies(void)
{
    printf("[rn42 응답 확인]\n");
    /* rn42_reply_ok: "AOK" 포함 */
    CHECK(rn42_reply_ok("AOK\r\n", 5) == 0, "AOK 포함");
    CHECK(rn42_reply_ok("AOK", 3) == 0, "AOK 정확히");
    CHECK(rn42_reply_ok("ERR\r\n", 5) == BT_ERR_REPLY, "ERR 응답");
    CHECK(rn42_reply_ok("", 0) == BT_ERR_REPLY, "빈 응답");
    CHECK(rn42_reply_ok("aok", 3) == BT_ERR_REPLY, "소문자 거절");
    CHECK(rn42_reply_ok("OK", 2) == BT_ERR_REPLY, "OK 는 AOK 아님");
    CHECK(rn42_reply_ok("XAOK", 4) == 0, "AOK 가 내부에 있어도 통과");
    CHECK(rn42_reply_ok(NULL, 3) == BT_ERR_REPLY, "NULL");

    /* rn42_reply_cmd: "CMD" 포함 */
    CHECK(rn42_reply_cmd("CMD\r\n", 5) == 0, "CMD 포함");
    CHECK(rn42_reply_cmd("CMD", 3) == 0, "CMD 정확히");
    CHECK(rn42_reply_cmd("cmd", 3) == BT_ERR_REPLY, "소문자 거절");
    CHECK(rn42_reply_cmd("AOK\r\n", 5) == BT_ERR_REPLY, "AOK 는 CMD 아님");
    CHECK(rn42_reply_cmd(NULL, 3) == BT_ERR_REPLY, "NULL");

    /* rn42_reply_end: "END" 포함 */
    CHECK(rn42_reply_end("END\r\n", 5) == 0, "END 포함");
    CHECK(rn42_reply_end("END", 3) == 0, "END 정확히");
    CHECK(rn42_reply_end("AOK\r\n", 5) == BT_ERR_REPLY, "AOK 는 END 아님");
    CHECK(rn42_reply_end(NULL, 3) == BT_ERR_REPLY, "NULL");
}

/* ═══════════════════════════════ RN4871 ══════════════════════════════════ */

static void t_rn4871_cmds(void)
{
    printf("[rn4871 명령]\n");
    char c[64]; char *b;

    /* enter: "$$$" 3글자 */
    b = exact(3); CHECK(rn4871_cmd_enter(b, 4) == 3 && strcmp(b, "$$$") == 0, "enter"); free(b);
    CHECK(rn4871_cmd_enter(0, 4) == BT_ERR_ARGS, "enter NULL");

    /* exit: "---\r" = 4글자 (RN4871 은 \r 만) */
    b = exact(4); CHECK(rn4871_cmd_exit(b, 5) == 4 && strcmp(b, "---\r") == 0, "exit(\\r 만)"); free(b);
    b = exact(3); CHECK(rn4871_cmd_exit(b, 4) == BT_ERR_NOSPACE, "exit 경계"); free(b);
    CHECK(rn4871_cmd_exit(0, 5) == BT_ERR_ARGS, "exit NULL");

    /* AT 핑: "+\r" = 2글자 */
    b = exact(2); CHECK(rn4871_cmd_at(b, 3) == 2 && strcmp(b, "+\r") == 0, "AT(+\\r)"); free(b);
    CHECK(rn4871_cmd_at(0, 3) == BT_ERR_ARGS, "AT NULL");

    /* 이름: "SN,<name>\r" → "SN," 3 + name + "\r" 1 */
    CHECK(rn4871_cmd_name(c, sizeof c, "BLEDev") == 10 && strcmp(c, "SN,BLEDev\r") == 0, "이름");
    CHECK(rn4871_cmd_name(c, sizeof c, "12345678901234567890") == 24, "이름 20자 허용");
    CHECK(rn4871_cmd_name(c, sizeof c, "123456789012345678901") == BT_ERR_VALUE, "이름 21자 거절");
    CHECK(rn4871_cmd_name(c, sizeof c, "") == BT_ERR_VALUE, "빈 이름");
    CHECK(rn4871_cmd_name(0, 16, "a") == BT_ERR_ARGS, "이름 NULL 출력");
    CHECK(rn4871_cmd_name(c, sizeof c, 0) == BT_ERR_ARGS, "이름 NULL 입력");

    /* factory: "SF,1\r" = 5글자 */
    b = exact(5); CHECK(rn4871_cmd_factory(b, 6) == 5 && strcmp(b, "SF,1\r") == 0, "factory"); free(b);
    CHECK(rn4871_cmd_factory(0, 6) == BT_ERR_ARGS, "factory NULL");

    /* reboot: "R,1\r" = 4글자 */
    b = exact(4); CHECK(rn4871_cmd_reboot(b, 5) == 4 && strcmp(b, "R,1\r") == 0, "reboot"); free(b);
    CHECK(rn4871_cmd_reboot(0, 5) == BT_ERR_ARGS, "reboot NULL");
}

static void t_rn4871_baud(void)
{
    printf("[rn4871 보레이트]\n");
    char c[64];
    static const unsigned BR[] = {9600,115200,230400};
    static const char *CD[] = {"96","11","23"};
    unsigned n = (unsigned)(sizeof BR / sizeof BR[0]);
    for (unsigned i = 0; i < n; i++) {
        CHECK(rn4871_baud_code(BR[i]) != NULL && strcmp(rn4871_baud_code(BR[i]), CD[i]) == 0, "보레이트 코드");
        char want[16]; snprintf(want, sizeof want, "SB,%s\r", CD[i]);
        int wl = (int)strlen(want);
        CHECK(rn4871_cmd_baud(c, sizeof c, BR[i]) == wl && strcmp(c, want) == 0, "보레이트 명령");
    }
    CHECK(rn4871_baud_code(0) == NULL && rn4871_baud_code(1382400) == NULL, "미지원 보레이트 NULL");
    CHECK(rn4871_cmd_baud(c, sizeof c, 300) == BT_ERR_VALUE, "미지원 보레이트 명령");
    CHECK(rn4871_cmd_baud(0, 16, 115200) == BT_ERR_ARGS, "보레이트 NULL");
}

static void t_rn4871_replies(void)
{
    printf("[rn4871 응답 확인]\n");
    /* reply_ok: "AOK" 포함 */
    CHECK(rn4871_reply_ok("AOK\r\n", 5) == 0, "AOK");
    CHECK(rn4871_reply_ok("Err\r\n", 5) == BT_ERR_REPLY, "Err");
    CHECK(rn4871_reply_ok("", 0) == BT_ERR_REPLY, "빈");
    CHECK(rn4871_reply_ok(NULL, 3) == BT_ERR_REPLY, "NULL");

    /* reply_cmd: "CMD>" 포함 — RN4871 은 "CMD>" (꺽쇠 포함) */
    CHECK(rn4871_reply_cmd("CMD>", 4) == 0, "CMD>");
    CHECK(rn4871_reply_cmd("CMD\r\n", 5) == BT_ERR_REPLY, "CMD 는 CMD> 아님 — RN-42 형식 거절");
    CHECK(rn4871_reply_cmd("AOK", 3) == BT_ERR_REPLY, "AOK");
    CHECK(rn4871_reply_cmd(NULL, 3) == BT_ERR_REPLY, "NULL");

    /* reply_end: "END" 포함 */
    CHECK(rn4871_reply_end("END\r\n", 5) == 0, "END");
    CHECK(rn4871_reply_end("AOK", 3) == BT_ERR_REPLY, "AOK");

    /* reply_err: "Err" 포함 — RN4871 은 대소문자 혼합 */
    CHECK(rn4871_reply_err("Err\r\n", 5) == 0, "Err");
    CHECK(rn4871_reply_err("ERR\r\n", 5) == BT_ERR_REPLY, "ERR 는 Err 아님");
    CHECK(rn4871_reply_err("AOK", 3) == BT_ERR_REPLY, "AOK");

    /* RN-42 와 RN4871 의 명령 모드 응답 차이 교차 확인 */
    CHECK(rn4871_reply_cmd("CMD\r\n", 5) == BT_ERR_REPLY, "RN-42 CMD 응답은 RN4871 에서 실패");
    /* 의도: RN4871 은 "CMD>" 를 요구하므로 "CMD\r\n" 은 거절해야 함 */
}

int main(void)
{
    t_rn42_enter();
    t_rn42_cmds();
    t_rn42_baud();
    t_rn42_replies();
    t_rn4871_cmds();
    t_rn4871_baud();
    t_rn4871_replies();
    printf(fails ? "FAILED %d\n" : "ALL PASS\n", fails);
    return fails != 0;
}
