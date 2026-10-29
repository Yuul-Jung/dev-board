/* MAX7219 체인 장치 모델 시험. 모델은 MAX7219/MAX7221 데이터시트(웹 가져오기 요약 경유)의 규칙을 구현하고 어기면 viol 에 기록한다.
 * 구현한 규칙: 체인 전체를 16N비트 시프트 레지스터로 모사(칩 0 이 DIN 쪽, 먼저 들어간 워드가 가장 먼 칩에 도착), CS(LOAD) 상승 때 각 칩이 자기 워드를 래치,
 * 프레임 길이는 정확히 16N 비트(래치가 16번째 상승 에지 뒤·다음 상승 에지 전, p.6), 주소 0x0D·0x0E 예약, 스캔 한계 0~7, No-Op 은 무시,
 * 전원 투입 상태는 셧다운·디코드 없음·밝기 최소·스캔 1자리(p.7), 시험 레지스터와 자릿수 레지스터는 값이 불명이라 최악값(시험 켜짐, 쓰레기 값)으로 시작해
 * 드라이버가 전부 직접 쓰는지 확인. 표시는 코드 B 폰트(Table 5)를 시험이 손으로 옮겨 적은 표로 해석한다. 전기·광학 특성은 모델링하지 않았다. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "../sw/max7219.h"

static int viol, fails;
#define V(msg) do { viol++; printf("  viol: %s\n", msg); } while (0)
#define CHECK(c, msg) do { if (!(c)) { fails++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); } } while (0)

#define NCH 8
static int nch = 1;
static uint16_t word[NCH];             /* 시프트 레지스터(칩별 16비트) */
static uint8_t R[NCH][16];             /* 칩별 레지스터 0..15 (자릿수 1..8) */
static int cs_low, bits_in;
static uint8_t flog[64][16]; static int flen[64], nflog;

/* 데이터시트 Table 5 코드 B 폰트를 손으로 옮긴 표: 문자와 세그먼트 A B C D E F G */
static const struct { char ch; const char *abcdefg; } FONT[16] = {
    {'0',"1111110"},{'1',"0110000"},{'2',"1101101"},{'3',"1111001"},{'4',"0110011"},{'5',"1011011"},{'6',"1011111"},{'7',"1110000"},
    {'8',"1111111"},{'9',"1111011"},{'-',"0000001"},{'E',"1001111"},{'H',"0110111"},{'L',"0001110"},{'P',"1100111"},{' ',"0000000"} };
static uint8_t segbits(const char *s) { uint8_t v = 0; for (int i = 0; i < 7; i++) if (s[i] == '1') v |= (uint8_t)(0x40 >> i); return v; }   /* A=D6 ... G=D0 (Table 6) */

static void reset_model(int n)
{
    nch = n; viol = 0; nflog = 0; cs_low = 0; bits_in = 0;
    memset(word, 0, sizeof word); memset(R, 0, sizeof R);
    for (int c = 0; c < NCH; c++) { R[c][0x0F] = 1; for (int d = 1; d <= 8; d++) R[c][d] = 0xA5; }   /* 시험 켜짐, 자릿수 쓰레기 */
}
static void latch(int c, uint16_t w)
{
    uint8_t a = (uint8_t)((w >> 8) & 0x0F), d = (uint8_t)w;
    if (a == 0x00) return;
    if (a == 0x0D || a == 0x0E) { V("예약 주소 0x0D/0x0E 에 씀"); return; }
    if (a == 0x0B && d > 7) V("스캔 한계가 0~7 을 벗어남");
    R[c][a] = d;
}
void qspi_cs(qspi_t *s, unsigned i, int a)
{
    (void)s; (void)i;
    if (a) { if (cs_low) V("CS 중첩"); cs_low = 1; bits_in = 0; nflog++; if (nflog <= 64) flen[nflog - 1] = 0; return; }
    if (!cs_low) { V("CS 해제인데 Low 가 아니었음"); return; }
    cs_low = 0;
    if (bits_in != 16 * nch) V("프레임 비트 수가 16×칩 수가 아님 (LOAD 시점 위반)");
    for (int c = 0; c < nch; c++) latch(c, word[c]);
}
int qspi_xfer(qspi_t *s, const uint8_t *tx, uint8_t *rx, size_t n)
{
    (void)s;
    if (!cs_low) V("CS 밖에서 전송");
    if (rx) V("MAX7219 는 읽기 불가인데 수신 요청");
    for (size_t i = 0; i < n; i++) {
        uint8_t b = tx ? tx[i] : 0;
        if (nflog <= 64 && flen[nflog - 1] < 16) flog[nflog - 1][flen[nflog - 1]++] = b;
        for (int k = 7; k >= 0; k--) {                       /* MSB 먼저, CLK 상승마다 한 비트 */
            uint16_t c[NCH];
            for (int j = 0; j < nch; j++) c[j] = (uint16_t)(word[j] >> 15);
            for (int j = 0; j < nch; j++) word[j] = (uint16_t)((word[j] << 1) | (j == 0 ? ((b >> k) & 1u) : c[j - 1]));
        }
        bits_in += 8;
    }
    return 0;
}

/* 표시 해석: ndig 자리를 왼쪽(높은 번호)부터. 소수점은 문자 뒤에 '.' */
static void render(int c, int ndig, char *out)
{
    char *p = out;
    int test = R[c][0x0F] & 1, on = R[c][0x0C] & 1, scan = R[c][0x0B] & 7;
    for (int d = ndig - 1; d >= 0; d--) {
        uint8_t v = R[c][1 + d]; char ch = ' '; int dp = 0;
        if (test) { ch = '8'; dp = 1; }
        else if (!on || d > scan) ch = ' ';
        else if ((R[c][9] >> d) & 1) { ch = FONT[v & 0x0F].ch; dp = (v & 0x80) != 0; }
        else {
            dp = (v & 0x80) != 0; ch = '?';
            for (int k = 0; k < 16; k++) if (segbits(FONT[k].abcdefg) == (v & 0x7F)) { ch = FONT[k].ch; break; }
            if ((v & 0x7F) == 0) ch = ' ';
        }
        *p++ = ch; if (dp) *p++ = '.';
    }
    *p = 0;
}

static qspi_t Q; static max7219_t M;
static void setup(int n, unsigned digits, uint8_t decode, uint8_t inten)
{
    reset_model(n); memset(&M, 0, sizeof M); M.s = &Q; M.nchips = (unsigned)n;
    int e = max7219_init(&M, digits, decode, inten); CHECK(e == 0, "init");
}
static int shows(int c, int nd, const char *want) { char b[40]; render(c, nd, b); if (strcmp(b, want)) printf("  표시 '%s' (기대 '%s')\n", b, want); return strcmp(b, want) == 0; }

static void t_init(void)
{
    printf("[init 바이트열]\n");
    reset_model(1); memset(&M, 0, sizeof M); M.s = &Q; M.nchips = 1;
    CHECK(max7219_init(&M, 4, 0x0F, 8) == 0, "init 4자리");
    /* 손으로 계산한 워드 순서: 시험 끔, 셧다운, 스캔 한계 3, 디코드 0x0F, 밝기 8, DIG0~3 코드 B 공백(0x0F), DIG4~7 은 0, 정상 */
    static const uint8_t E[][2] = { {0x0F,0x00},{0x0C,0x00},{0x0B,0x03},{0x09,0x0F},{0x0A,0x08},{0x01,0x0F},{0x02,0x0F},{0x03,0x0F},{0x04,0x0F},{0x05,0x00},{0x06,0x00},{0x07,0x00},{0x08,0x00},{0x0C,0x01} };
    int ne = (int)(sizeof E / sizeof E[0]);
    CHECK(nflog == ne, "프레임 수 14");
    for (int i = 0; i < ne && i < nflog; i++) CHECK(flen[i] == 2 && flog[i][0] == E[i][0] && flog[i][1] == E[i][1], "init 프레임이 손 계산과 같음");
    CHECK(viol == 0, "위반 없음");
    CHECK(shows(0, 4, "    "), "초기 표시는 공백(쓰레기·시험 모드 제거)");
    CHECK(R[0][0x0A] == 8 && R[0][0x0B] == 3 && R[0][0x09] == 0x0F && (R[0][0x0C] & 1) && !(R[0][0x0F] & 1), "레지스터 값");

    setup(4, 4, 0x0F, 3);
    CHECK(viol == 0, "4체인 init 위반 없음");
    CHECK(flen[0] == 8 && flog[0][0] == 0x0F && flog[0][1] == 0 && flog[0][2] == 0x0F && flog[0][6] == 0x0F, "4체인: 같은 워드가 4번");
    for (int c = 0; c < 4; c++) CHECK(shows(c, 4, "    ") && R[c][0x0A] == 3 && R[c][0x0B] == 3, "칩마다 초기화됨");
}
static void t_numbers(void)
{
    printf("[숫자]\n");
    setup(1, 4, 0x0F, 8);
    CHECK(max7219_print_int(&M, 0, 4, 0, -1) == 0 && shows(0, 4, "   0"), "0");
    CHECK(max7219_print_int(&M, 0, 4, 42, -1) == 0 && shows(0, 4, "  42"), "42");
    CHECK(max7219_print_int(&M, 0, 4, 9999, -1) == 0 && shows(0, 4, "9999"), "9999");
    CHECK(max7219_print_int(&M, 0, 4, 1234, -1) == 0 && shows(0, 4, "1234") && R[0][1] == 4 && R[0][4] == 1, "1234: DIG0 = 4(가장 낮은 자리), DIG3 = 1");
    CHECK(max7219_print_int(&M, 0, 4, -123, -1) == 0 && shows(0, 4, "-123"), "-123");
    CHECK(max7219_print_int(&M, 0, 4, -1, -1) == 0 && shows(0, 4, "  -1"), "-1");
    CHECK(max7219_print_int(&M, 0, 4, -999, -1) == 0 && shows(0, 4, "-999"), "-999");
    CHECK(max7219_print_int(&M, 0, 4, -1000, -1) == MAX7219_ERR_RANGE, "-1000 은 안 들어감");
    CHECK(max7219_print_int(&M, 0, 4, 10000, -1) == MAX7219_ERR_RANGE, "10000 은 안 들어감");
    CHECK(max7219_print_int(&M, 0, 4, INT32_MIN, -1) == MAX7219_ERR_RANGE, "INT32_MIN");
    CHECK(max7219_print_int(&M, 0, 4, 1234, 2) == 0 && shows(0, 4, "12.34"), "소수점 2: 12.34");
    CHECK(max7219_print_int(&M, 0, 4, 5, 1) == 0 && shows(0, 4, "  0.5"), "5, dp 1 → 0.5");
    CHECK(max7219_print_int(&M, 0, 4, 5, 3) == 0 && shows(0, 4, "0.005"), "5, dp 3 → 0.005");
    CHECK(max7219_print_int(&M, 0, 4, -5, 1) == 0 && shows(0, 4, " -0.5"), "-5, dp 1 → -0.5");
    CHECK(max7219_print_int(&M, 0, 4, -5, 2) == 0 && shows(0, 4, "-0.05"), "-5, dp 2 → -0.05");
    CHECK(max7219_print_int(&M, 0, 4, -5, 3) == MAX7219_ERR_RANGE, "-5, dp 3 은 부호 자리가 없음");
    CHECK(max7219_print_int(&M, 0, 4, 1, 4) == QSPI_ERR_ARGS && max7219_print_int(&M, 0, 4, 1, -2) == QSPI_ERR_ARGS, "dp 범위");
    CHECK(max7219_print_int(&M, 0, 0, 1, -1) == QSPI_ERR_ARGS && max7219_print_int(&M, 0, 9, 1, -1) == QSPI_ERR_ARGS, "자릿수 범위");
    CHECK(max7219_print_int(&M, 0, 5, 1, -1) == MAX7219_ERR_MODE, "코드 B 가 아닌 자릿수가 섞이면 MODE");
    CHECK(viol == 0, "위반 없음");

    setup(1, 8, 0xFF, 15);
    CHECK(max7219_print_int(&M, 0, 8, 12345678, -1) == 0 && shows(0, 8, "12345678"), "8자리");
    CHECK(max7219_print_int(&M, 0, 8, -1234567, -1) == 0 && shows(0, 8, "-1234567"), "8자리 음수");
    CHECK(max7219_print_int(&M, 0, 8, 100000000, -1) == MAX7219_ERR_RANGE, "9자리는 안 들어감");
    CHECK(viol == 0, "위반 없음");
}
static void t_text(void)
{
    printf("[문자열·원시 세그먼트·매트릭스]\n");
    setup(1, 4, 0x0F, 8);
    CHECK(max7219_print_str(&M, 0, 4, "HELP") == 0 && shows(0, 4, "HELP"), "HELP");
    CHECK(max7219_print_str(&M, 0, 4, "h-lp") == 0 && shows(0, 4, "H-LP") && R[0][4] == MAX7219_CODEB_H, "소문자와 왼쪽 끝 = DIG3");
    CHECK(max7219_print_str(&M, 0, 4, "E-1.5") == 0 && shows(0, 4, "E-1.5"), "소수점");
    CHECK(max7219_print_str(&M, 0, 4, "12") == 0 && shows(0, 4, "12  "), "왼쪽 정렬");
    CHECK(max7219_print_str(&M, 0, 4, "1 2 ") == 0 && shows(0, 4, "1 2 "), "문자열 안의 공백");
    CHECK(max7219_print_str(&M, 0, 4, "") == 0 && shows(0, 4, "    "), "빈 문자열");
    CHECK(max7219_print_str(&M, 0, 4, "AB") == MAX7219_ERR_CHAR && max7219_print_str(&M, 0, 4, ".5") == MAX7219_ERR_CHAR, "못 나타내는 문자·앞쪽 소수점");
    CHECK(max7219_print_str(&M, 0, 4, "12345") == MAX7219_ERR_RANGE, "자릿수 초과");
    CHECK(max7219_set_code(&M, 0, 0, 16, 0) == QSPI_ERR_ARGS && max7219_set_code(&M, 0, 8, 1, 0) == QSPI_ERR_ARGS, "코드·자릿수 범위");
    CHECK(max7219_set_code(&M, 0, 5, 1, 0) == MAX7219_ERR_MODE, "디코드 안 하는 자릿수에 코드 쓰기");
    CHECK(max7219_set_raw(&M, 0, 0, 0x7E) == MAX7219_ERR_MODE, "디코드 자릿수에 raw 쓰기");
    CHECK(max7219_set_raw(&M, 0, 5, MAX7219_SEG_A | MAX7219_SEG_B | MAX7219_SEG_C | MAX7219_SEG_D | MAX7219_SEG_E | MAX7219_SEG_F) == 0 && R[0][6] == 0x7E, "raw: 0 모양 = 0x7E");
    CHECK(MAX7219_SEG_A == 0x40 && MAX7219_SEG_B == 0x20 && MAX7219_SEG_C == 0x10 && MAX7219_SEG_D == 0x08 && MAX7219_SEG_E == 0x04 && MAX7219_SEG_F == 0x02 && MAX7219_SEG_G == 0x01 && MAX7219_DP == 0x80, "세그먼트 비트 배치 (Table 6)");
    CHECK(max7219_clear(&M, 0, 4) == 0 && shows(0, 4, "    "), "clear");

    setup(1, 8, 0x00, 4);
    uint8_t rows[8] = { 0x18, 0x24, 0x42, 0x81, 0x81, 0x42, 0x24, 0x18 };
    CHECK(max7219_matrix(&M, 0, rows) == 0, "매트릭스");
    int ok = 1; for (int r = 0; r < 8; r++) ok &= R[0][1 + r] == rows[r];
    CHECK(ok, "DIGn = 행 n");
    CHECK(max7219_set_raw(&M, 0, 8, 0) == QSPI_ERR_ARGS && max7219_matrix(&M, 0, 0) == QSPI_ERR_ARGS, "인자 검사");
    setup(1, 8, 0x00, 4);
    CHECK(max7219_set_raw(&M, 0, 3, 0xFF) == 0 && shows(0, 4, "8.   "), "raw 전부 켬(DP 포함) → 8.");
    setup(1, 4, 0x0F, 4);
    CHECK(max7219_matrix(&M, 0, rows) == MAX7219_ERR_MODE, "디코드가 켜져 있으면 매트릭스 거절");
    setup(1, 8, 0x80, 4); int nf = nflog;
    CHECK(max7219_matrix(&M, 0, rows) == MAX7219_ERR_MODE && nflog == nf, "디코드가 일부만 켜져 있어도 거절하고 아무것도 쓰지 않음");
    CHECK(viol == 0, "위반 없음");
}
static void t_control(void)
{
    printf("[제어]\n");
    setup(1, 4, 0x0F, 8);
    max7219_print_int(&M, 0, 4, 1234, -1);
    CHECK(max7219_shutdown(&M, 1) == 0 && shows(0, 4, "    ") && R[0][4] == 1, "셧다운: 꺼지지만 데이터는 유지");
    CHECK(max7219_shutdown(&M, 0) == 0 && shows(0, 4, "1234"), "복귀");
    CHECK(max7219_test(&M, 1) == 0 && shows(0, 4, "8.8.8.8."), "시험 모드: 전체 점등");
    CHECK(max7219_test(&M, 0) == 0 && shows(0, 4, "1234"), "시험 모드 해제 후 표시 복귀");
    int ok = 1;
    for (int l = 0; l <= 15; l++) ok &= max7219_set_intensity(&M, (uint8_t)l) == 0 && R[0][0x0A] == l;
    CHECK(ok, "밝기 0~15");
    CHECK(max7219_set_intensity(&M, 16) == QSPI_ERR_ARGS, "밝기 16 거절");
    CHECK(viol == 0, "위반 없음");
}
static void t_chain(void)
{
    printf("[체인]\n");
    setup(4, 4, 0x0F, 8);
    for (int c = 0; c < 4; c++) CHECK(max7219_print_int(&M, (unsigned)c, 4, 1111 * (c + 1), -1) == 0, "칩마다 다른 값");
    CHECK(shows(0, 4, "1111") && shows(1, 4, "2222") && shows(2, 4, "3333") && shows(3, 4, "4444"), "각 칩에 자기 값이 들어감(순서 보존)");
    CHECK(max7219_print_int(&M, 2, 4, 7, -1) == 0, "칩 2 만 변경");
    CHECK(shows(0, 4, "1111") && shows(1, 4, "2222") && shows(2, 4, "   7") && shows(3, 4, "4444"), "다른 칩은 그대로 (No-Op)");
    CHECK(max7219_set_intensity(&M, 5) == 0, "밝기는 전체");
    int ok = 1; for (int c = 0; c < 4; c++) ok &= R[c][0x0A] == 5;
    CHECK(ok, "모든 칩의 밝기");
    CHECK(max7219_write(&M, 3, MAX7219_REG_INTENSITY, 9) == 0 && R[3][0x0A] == 9 && R[0][0x0A] == 5 && R[1][0x0A] == 5 && R[2][0x0A] == 5, "가장 먼 칩에만 쓰기");
    CHECK(max7219_write(&M, 0, MAX7219_REG_INTENSITY, 2) == 0 && R[0][0x0A] == 2 && R[3][0x0A] == 9, "가장 가까운 칩에만 쓰기");
    CHECK(flen[nflog - 1] == 8 && flog[nflog - 1][0] == 0 && flog[nflog - 1][1] == 0 && flog[nflog - 1][6] == MAX7219_REG_INTENSITY && flog[nflog - 1][7] == 2, "칩 0 대상 워드는 마지막에, 앞은 No-Op");
    CHECK(max7219_write(&M, 4, 1, 1) == QSPI_ERR_ARGS, "칩 번호 범위");
    CHECK(viol == 0, "위반 없음");
    /* 8체인 */
    setup(8, 8, 0xFF, 8);
    for (int c = 0; c < 8; c++) max7219_print_int(&M, (unsigned)c, 8, 10000000 + c, -1);
    ok = 1; for (int c = 0; c < 8; c++) { char w[16]; snprintf(w, sizeof w, "%d", 10000000 + c); ok &= shows(c, 8, w); }
    CHECK(ok, "8체인 × 8자리");
    CHECK(viol == 0, "위반 없음");
}
static void t_args(void)
{
    printf("[인자]\n");
    reset_model(1); memset(&M, 0, sizeof M); M.s = &Q;
    M.nchips = 0; CHECK(max7219_init(&M, 4, 0, 0) == QSPI_ERR_ARGS && nflog == 0, "칩 0개");
    M.nchips = 9; CHECK(max7219_init(&M, 4, 0, 0) == QSPI_ERR_ARGS && nflog == 0, "칩 9개");
    M.nchips = 1;
    CHECK(max7219_init(&M, 0, 0, 0) == QSPI_ERR_ARGS && max7219_init(&M, 9, 0, 0) == QSPI_ERR_ARGS && max7219_init(&M, 4, 0, 16) == QSPI_ERR_ARGS && nflog == 0, "자릿수·밝기 범위, SPI 통신 없음");
    CHECK(max7219_init(0, 4, 0, 0) == QSPI_ERR_ARGS && max7219_write_all(0, 1, 1) == QSPI_ERR_ARGS, "NULL");
    CHECK(viol == 0, "위반 없음");
}

int main(void)
{
    t_init(); t_numbers(); t_text(); t_control(); t_chain(); t_args();
    if (fails) { printf("FAIL: %d\n", fails); return 1; }
    printf("ALL PASS (max7219 model)\n");
    return 0;
}
