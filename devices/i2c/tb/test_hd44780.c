/* HD44780 드라이버 시험 — 데이터시트 요약(i2c/ref/hd44780_facts.txt)을 옮겨 만든 모델(자작)에 대해서만 확인한다. 실제 LCD 모듈이나 보드에서 확인한 것이 아니다.
 * 모델이 강제하는 것: ① 전원 후 첫 E 하강 ≥ 40 ms, 0011 세 번 사이 간격 4.1 ms / 100 µs, 그 뒤 0010 으로 4비트 전환 ② 4비트에서 높은 니블 다음 낮은 니블 ③ 명령 실행 중(바쁨)에 오는 E 하강은 위반
 *   (지우기·홈 2.16 ms, 나머지 52.6 µs: fOSC 190 kHz 최악) ④ E 타이밍(3 V 쪽 최악): E 하이 ≥ 450 ns, E 주기 ≥ 1000 ns, 데이터·RS 셋업 ≥ 195 ns, E 하강 뒤 데이터 홀드 ≥ 20 ns
 *   ⑤ E 가 높은 동안 데이터 변경 금지 ⑥ RW 는 항상 0 ⑦ DDRAM 주소 유효 범위(2행 00~27, 40~67 / 1행 00~4F)와 CGRAM 쓰기. 보이는 화면은 모듈 관례(행 시작 주소)를 시험이 독립적으로 가정해 읽는다. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../sw/hd44780.h"
#include "../sw/hd44780_pcf8574.h"

typedef struct {
    hd44780_pins_t pins; uint64_t bus_cost_ns, now;
    uint8_t port, e, rs, rw, data;                      /* 현재 선 상태 */
    uint64_t t_data_change, t_e_rise, t_e_fall, t_prev_rise; int had_rise, had_fall;
    uint64_t busy_until; int four_bit, pend_hi, hi, init_pulses; uint64_t t_pulse[4]; uint64_t t_last_init;
    uint8_t ddram[0x80], cgram[0x40]; int cg_hi_bits; uint8_t ac; int mode_cg, id, s, d, c, b, n_lines, fn_set, shift_net, bl;
    int v_timing, v_busy, v_init, v_data, v_rw, v_addr, v_seq, nwrites, npulses, fail_after;
} model_t;
static model_t M;
static int fails;
#define CHK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static void model_reset(hd44780_pins_t p, uint64_t cost_ns)
{
    memset(&M, 0, sizeof M); M.pins = p; M.bus_cost_ns = cost_ns; memset(M.ddram, 0x20, sizeof M.ddram); M.fail_after = -1;
    M.t_data_change = 0;
}
static int valid_dd(uint8_t a) { return M.n_lines ? (a <= 0x27 || (a >= 0x40 && a <= 0x67)) : a <= 0x4F; }
static void exec_byte(uint8_t b, int rs)
{
    if (rs) {
        if (M.mode_cg) { if (b & 0xE0) M.cg_hi_bits++; M.cgram[M.ac & 0x3F] = b & 0x1F; M.ac = (uint8_t)((M.ac + (M.id ? 1 : -1)) & 0x3F); }
        else { if (valid_dd(M.ac)) M.ddram[M.ac] = b; else M.v_addr++; M.ac = (uint8_t)((M.ac + (M.id ? 1 : -1)) & 0x7F); }
        M.busy_until = M.now + 53000; return;
    }
    uint64_t busy = 53000;                                    /* 37 µs × 270/190 = 52.6 µs */
    if (b == 0x01) { memset(M.ddram, 0x20, sizeof M.ddram); M.ac = 0; M.mode_cg = 0; M.shift_net = 0; busy = 2160000; }   /* 1.52 ms × 270/190 = 2.16 ms */
    else if ((b & 0xFE) == 0x02) { M.ac = 0; M.mode_cg = 0; M.shift_net = 0; busy = 2160000; }
    else if ((b & 0xFC) == 0x04) { M.id = !!(b & 2); M.s = b & 1; }
    else if ((b & 0xF8) == 0x08) { M.d = !!(b & 4); M.c = !!(b & 2); M.b = b & 1; }
    else if ((b & 0xF0) == 0x10) { if (b & 8) M.shift_net += (b & 4) ? 1 : -1; }
    else if ((b & 0xE0) == 0x20) { if (M.fn_set && b != M.fn_set) M.v_seq++; M.fn_set = b; M.n_lines = !!(b & 8); if (b & 0x10 || b & 4) M.v_seq++; }
    else if ((b & 0xC0) == 0x40) { M.mode_cg = 1; M.ac = b & 0x3F; }
    else { M.mode_cg = 0; M.ac = b & 0x7F; }
    M.busy_until = M.now + busy;
}
static void on_e_fall(void)
{
    M.npulses++;
    uint8_t nib = M.data;
    if (M.now < M.busy_until && M.four_bit) { M.v_busy++; return; }
    if (!M.four_bit) {
        if (M.rs) { M.v_init++; return; }
        if (M.init_pulses < 3) {
            if (nib != 0x3) { M.v_init++; return; }
            uint64_t min_gap = M.init_pulses == 0 ? 40000000ull : M.init_pulses == 1 ? 4100000ull : 100000ull;
            uint64_t ref = M.init_pulses == 0 ? 0 : M.t_last_init;
            if (M.now < ref + min_gap) M.v_init++;
            M.t_last_init = M.now; M.init_pulses++;
        } else {
            if (nib != 0x2 || M.now < M.t_last_init + 53000) M.v_init++;
            M.four_bit = 1; M.busy_until = M.now + 53000;
        }
        return;
    }
    if (!M.pend_hi) { M.hi = nib; M.pend_hi = 1; M.rs = M.rs; return; }
    M.pend_hi = 0; exec_byte((uint8_t)((M.hi << 4) | nib), M.rs);
}
static int mget(uint8_t port, uint8_t pos) { return pos == HD44780_NC ? 0 : (port >> pos) & 1; }
static int model_write(uint8_t port)
{
    if (M.fail_after >= 0 && M.nwrites++ >= M.fail_after) return -5;
    if (M.fail_after < 0) M.nwrites++;
    const hd44780_pins_t *p = &M.pins;
    uint8_t nib = (uint8_t)(mget(port, p->d4) | mget(port, p->d5) << 1 | mget(port, p->d6) << 2 | mget(port, p->d7) << 3);
    int e = mget(port, p->en), rs = mget(port, p->rs), rw = mget(port, p->rw);
    M.bl = mget(port, p->bl);
    if (rw) M.v_rw++;
    int dchg = (nib != M.data) || (rs != M.rs) || (rw != M.rw);
    if (dchg) {
        if (M.e) M.v_data++;                                                    /* E 가 높은 동안 변경 */
        else if (M.had_fall && M.now - M.t_e_fall < 20) M.v_timing++;           /* tAH/tH */
        M.t_data_change = M.now;
    }
    if (e && !M.e) {                                                            /* 상승 */
        if (M.now - M.t_data_change < 195 && M.nwrites > 1) M.v_timing++;       /* tDSW(195) ≥ tAS(60) */
        if (M.had_rise && M.now - M.t_prev_rise < 1000) M.v_timing++;           /* tcycE */
        M.t_prev_rise = M.now; M.t_e_rise = M.now; M.had_rise = 1;
    }
    M.data = nib; M.rs = (uint8_t)rs; M.rw = (uint8_t)rw;
    if (!e && M.e) {                                                            /* 하강 */
        if (M.now - M.t_e_rise < 450) M.v_timing++;                             /* PWEH */
        M.t_e_fall = M.now; M.had_fall = 1; M.e = 0;
        on_e_fall();
    }
    M.e = (uint8_t)e;
    M.port = port;
    M.now += M.bus_cost_ns;
    return 0;
}
static int halve;                                                               /* 음성 대조: 지연을 절반으로 */
static void model_delay(uint32_t us) { M.now += (uint64_t)(halve ? us / 2 : us) * 1000u; }
static int wr_cb(void *ctx, uint8_t port) { (void)ctx; return model_write(port); }
static void dl_cb(void *ctx, uint32_t us) { (void)ctx; model_delay(us); }

static int viol(void) { return M.v_timing + M.v_busy + M.v_init + M.v_data + M.v_rw + M.v_addr + M.v_seq; }
static void screen(unsigned row, const unsigned *row_start, unsigned cols, char *out)
{
    for (unsigned c = 0; c < cols; c++) out[c] = (char)M.ddram[(row_start[row] + c) & 0x7F];
    out[cols] = 0;
}
static const unsigned RS16x2[4] = { 0x00, 0x40, 0, 0 };
static const unsigned RS20x4[4] = { 0x00, 0x40, 0x14, 0x54 };
static const unsigned RS16x4[4] = { 0x00, 0x40, 0x10, 0x50 };
static const hd44780_pins_t PCF = HD44780_PINS_PCF8574_COMMON;
static hd44780_pins_t pcf(void) { return PCF; }

static int setup(hd44780_t *h, hd44780_pins_t pins, uint64_t cost, unsigned cols, unsigned rows)
{
    model_reset(pins, cost);
    return hd44780_init(h, wr_cb, dl_cb, 0, pins, cols, rows);
}

/* 접착 코드 시험용: pcf8574_write 를 모델로 연결 */
int pcf8574_write(pcf8574_t *p, uint8_t v) { p->shadow = v; return model_write(v); }
static void us_cb(uint32_t us) { model_delay(us); }

int main(void)
{
    hd44780_t h; char row[48];
    for (int k = 0; k < 2; k++) {                                              /* 버스 비용 0(GPIO 같음) 과 25 µs(I2C 같음) */
        uint64_t cost = k ? 25000 : 0;
        CHK(setup(&h, pcf(), cost, 16, 2) == HD44780_OK);
        CHK(viol() == 0 && M.four_bit && M.init_pulses == 3 && M.n_lines == 1 && M.fn_set == 0x28 && M.d == 1 && M.c == 0 && M.b == 0 && M.id == 1 && M.s == 0 && M.bl == 1);
        screen(0, RS16x2, 16, row); CHK(strcmp(row, "                ") == 0);
        CHK(M.now >= 40000000ull + 4100000ull);                               /* 전체 시간은 대기 합 이상 */
        CHK(hd44780_print(&h, "Hello") == HD44780_OK);
        screen(0, RS16x2, 16, row); CHK(strcmp(row, "Hello           ") == 0);
        CHK(hd44780_set_cursor(&h, 3, 1) == HD44780_OK && hd44780_print(&h, "World") == HD44780_OK);
        screen(1, RS16x2, 16, row); CHK(strcmp(row, "   World        ") == 0);
        CHK(hd44780_set_cursor(&h, 0, 1) == HD44780_OK && hd44780_print_int(&h, -123) == HD44780_OK);
        screen(1, RS16x2, 16, row); CHK(strncmp(row, "-123orld", 8) == 0);                  /* 앞 4칸만 덮어씀 */
        CHK(hd44780_set_cursor(&h, 0, 1) == HD44780_OK && hd44780_print_int(&h, INT32_MIN) == HD44780_OK);
        screen(1, RS16x2, 16, row); CHK(strncmp(row, "-2147483648", 11) == 0);
        CHK(hd44780_set_cursor(&h, 0, 1) == HD44780_OK && hd44780_print_int(&h, 0) == HD44780_OK);
        screen(1, RS16x2, 16, row); CHK(row[0] == '0' && row[1] == '2');                       /* 이전 내용은 지우지 않음(첫 칸만 0 으로 바뀜) */
        CHK(h.col == 1 && h.row == 1);
        CHK(viol() == 0);
    }
    /* 행 끝 처리 */
    CHK(setup(&h, pcf(), 0, 16, 2) == HD44780_OK);
    CHK(hd44780_print(&h, "0123456789ABCDEFGHIJ") == HD44780_ERR_RANGE);
    screen(0, RS16x2, 16, row); CHK(strcmp(row, "0123456789ABCDEF") == 0);
    CHK(M.ddram[16] == 0x20 && M.ddram[17] == 0x20 && M.ddram[0x28] == 0x20);                 /* 숨은 RAM 에 쓰지 않음 */
    CHK(hd44780_putc(&h, 'x') == HD44780_ERR_RANGE && h.col == 16);
    CHK(hd44780_set_cursor(&h, 14, 0) == HD44780_OK && hd44780_print_int(&h, 123) == HD44780_ERR_RANGE);   /* 3자리가 2칸에 안 들어가면 아무것도 안 씀 */
    screen(0, RS16x2, 16, row); CHK(strcmp(row, "0123456789ABCDEF") == 0 && h.col == 14);
    CHK(hd44780_set_cursor(&h, 16, 0) == HD44780_ERR_RANGE && hd44780_set_cursor(&h, 0, 2) == HD44780_ERR_RANGE && viol() == 0);
    /* clear / home */
    CHK(hd44780_set_cursor(&h, 5, 1) == HD44780_OK && hd44780_clear(&h) == HD44780_OK && h.col == 0 && h.row == 0);
    screen(0, RS16x2, 16, row); CHK(strcmp(row, "                ") == 0 && M.ac == 0 && M.id == 1);
    CHK(hd44780_print(&h, "ab") == HD44780_OK && hd44780_home(&h) == HD44780_OK && M.ac == 0 && h.col == 0);
    screen(0, RS16x2, 16, row); CHK(strncmp(row, "ab", 2) == 0);                               /* 홈은 지우지 않음 */
    CHK(hd44780_print(&h, "Z") == HD44780_OK); screen(0, RS16x2, 16, row); CHK(strncmp(row, "Zb", 2) == 0);
    /* 표시 제어 */
    CHK(hd44780_cursor(&h, 1) == HD44780_OK && M.c == 1 && M.d == 1 && M.b == 0);
    CHK(hd44780_blink(&h, 1) == HD44780_OK && M.b == 1 && M.c == 1);
    CHK(hd44780_display(&h, 0) == HD44780_OK && M.d == 0 && M.c == 1 && M.b == 1);
    CHK(hd44780_display(&h, 1) == HD44780_OK && hd44780_cursor(&h, 0) == HD44780_OK && hd44780_blink(&h, 0) == HD44780_OK && M.d == 1 && M.c == 0 && M.b == 0);
    CHK(hd44780_shift_display(&h, 1) == HD44780_OK && M.shift_net == 1 && hd44780_shift_display(&h, 0) == HD44780_OK && hd44780_shift_display(&h, 0) == HD44780_OK && M.shift_net == -1);
    /* 백라이트: 이후 모든 포트 바이트에 반영 */
    CHK(hd44780_backlight(&h, 0) == HD44780_OK && M.bl == 0 && !(M.port & 8));
    CHK(hd44780_print(&h, "q") == HD44780_OK && M.bl == 0 && !(M.port & 8));
    CHK(hd44780_backlight(&h, 1) == HD44780_OK && M.bl == 1 && (M.port & 8));
    CHK(viol() == 0);
    /* 사용자 글자 */
    CHK(setup(&h, pcf(), 0, 16, 2) == HD44780_OK && hd44780_set_cursor(&h, 4, 1) == HD44780_OK);
    static const uint8_t heart[8] = { 0x00, 0x0A, 0x1F, 0x1F, 0x0E, 0x04, 0x00, 0x00 };
    static const uint8_t dirty[8] = { 0xFF, 0xE1, 0x00, 0x1F, 0x80, 0x01, 0xF0, 0x0F };
    CHK(hd44780_create_char(&h, 0, heart) == HD44780_OK && memcmp(M.cgram, heart, 8) == 0);
    CHK(M.mode_cg == 0 && M.ac == 0x44);                                                       /* DDRAM(행 2, 열 4)로 복귀 */
    CHK(hd44780_create_char(&h, 7, dirty) == HD44780_OK && M.cgram[56] == 0x1F && M.cgram[57] == 0x01 && M.cgram[58] == 0 && M.cgram[59] == 0x1F && M.cgram[60] == 0 && M.cgram[61] == 1 && M.cgram[62] == 0x10 && M.cgram[63] == 0x0F && M.cg_hi_bits == 0);
    CHK(M.cgram[8] == 0 && M.cgram[48] == 0);                                                  /* 다른 슬롯은 안 건드림 */
    CHK(hd44780_putc(&h, 0) == HD44780_OK && M.ddram[0x44] == 0 && hd44780_putc(&h, 7) == HD44780_OK && M.ddram[0x45] == 7 && h.col == 6);
    CHK(hd44780_create_char(&h, 8, heart) == HD44780_ERR_ARGS && hd44780_create_char(&h, 0, 0) == HD44780_ERR_ARGS && viol() == 0);
    /* 20×4 / 16×4 / 1행 */
    CHK(setup(&h, pcf(), 0, 20, 4) == HD44780_OK && M.n_lines == 1);
    CHK(hd44780_set_cursor(&h, 18, 2) == HD44780_OK && hd44780_print(&h, "AB") == HD44780_OK && hd44780_putc(&h, 'C') == HD44780_ERR_RANGE);
    CHK(hd44780_set_cursor(&h, 0, 3) == HD44780_OK && hd44780_print(&h, "row4") == HD44780_OK && hd44780_set_cursor(&h, 0, 0) == HD44780_OK && hd44780_print(&h, "row1") == HD44780_OK);
    screen(2, RS20x4, 20, row); CHK(strcmp(row, "                  AB") == 0);
    screen(3, RS20x4, 20, row); CHK(strncmp(row, "row4", 4) == 0);
    screen(0, RS20x4, 20, row); CHK(strncmp(row, "row1", 4) == 0 && viol() == 0);
    CHK(setup(&h, pcf(), 0, 16, 4) == HD44780_OK && hd44780_set_cursor(&h, 0, 2) == HD44780_OK && hd44780_print(&h, "r3") == HD44780_OK && hd44780_set_cursor(&h, 0, 3) == HD44780_OK && hd44780_print(&h, "r4") == HD44780_OK);
    screen(2, RS16x4, 16, row); CHK(strncmp(row, "r3", 2) == 0); screen(3, RS16x4, 16, row); CHK(strncmp(row, "r4", 2) == 0);
    CHK(setup(&h, pcf(), 0, 16, 1) == HD44780_OK && M.n_lines == 0 && M.fn_set == 0x20 && hd44780_print(&h, "0123456789ABCDEF") == HD44780_OK && M.ddram[0x0F] == 'F' && viol() == 0);
    CHK(setup(&h, pcf(), 0, 40, 2) == HD44780_OK && hd44780_set_cursor(&h, 39, 0) == HD44780_OK && hd44780_putc(&h, 'x') == HD44780_OK && M.ddram[0x27] == 'x' && hd44780_putc(&h, 'y') == HD44780_ERR_RANGE && viol() == 0);
    /* 다른 핀 배선: D4~D7 = 비트 0~3, RS = 4, E = 5, RW·백라이트 미연결 */
    {
        hd44780_pins_t q = { 4, HD44780_NC, 5, HD44780_NC, 0, 1, 2, 3 };
        CHK(setup(&h, q, 0, 16, 2) == HD44780_OK && viol() == 0 && M.four_bit);
        CHK(hd44780_print(&h, "Hi!~") == HD44780_OK);
        screen(0, RS16x2, 16, row); CHK(strncmp(row, "Hi!~", 4) == 0);
        CHK(hd44780_backlight(&h, 0) == HD44780_ERR_NOBL && h.bl == 1);
        hd44780_pins_t r = { 7, 6, 5, 4, 3, 2, 1, 0 };                                         /* 역순 */
        CHK(setup(&h, r, 0, 16, 2) == HD44780_OK && viol() == 0 && hd44780_print(&h, "\x5A\xA5") == HD44780_OK && M.ddram[0] == 0x5A && M.ddram[1] == 0xA5);
        CHK(hd44780_backlight(&h, 0) == HD44780_OK && !(M.port & 0x10) && hd44780_backlight(&h, 1) == HD44780_OK && (M.port & 0x10));
    }
    /* 오류 처리 */
    {
        hd44780_pins_t dup = { 0, 1, 2, 3, 4, 5, 6, 6 }, big = { 0, 1, 2, 3, 4, 5, 6, 8 }, ncrs = { HD44780_NC, 1, 2, 3, 4, 5, 6, 7 }, ncd = { 0, 1, 2, 3, 4, 5, 6, HD44780_NC };
        model_reset(pcf(), 0);
        CHK(hd44780_init(&h, wr_cb, dl_cb, 0, dup, 16, 2) == HD44780_ERR_ARGS && hd44780_init(&h, wr_cb, dl_cb, 0, big, 16, 2) == HD44780_ERR_ARGS);
        CHK(hd44780_init(&h, wr_cb, dl_cb, 0, ncrs, 16, 2) == HD44780_ERR_ARGS && hd44780_init(&h, wr_cb, dl_cb, 0, ncd, 16, 2) == HD44780_ERR_ARGS);
        CHK(hd44780_init(&h, wr_cb, dl_cb, 0, pcf(), 0, 2) == HD44780_ERR_ARGS && hd44780_init(&h, wr_cb, dl_cb, 0, pcf(), 41, 2) == HD44780_ERR_ARGS);
        CHK(hd44780_init(&h, wr_cb, dl_cb, 0, pcf(), 16, 0) == HD44780_ERR_ARGS && hd44780_init(&h, wr_cb, dl_cb, 0, pcf(), 16, 5) == HD44780_ERR_ARGS);
        CHK(hd44780_init(&h, 0, dl_cb, 0, pcf(), 16, 2) == HD44780_ERR_ARGS && hd44780_init(&h, wr_cb, 0, 0, pcf(), 16, 2) == HD44780_ERR_ARGS && hd44780_init(0, wr_cb, dl_cb, 0, pcf(), 16, 2) == HD44780_ERR_ARGS);
        CHK(M.nwrites == 0);                                                                   /* 인자 오류 때는 아무것도 쓰지 않음 */
        CHK(hd44780_clear(0) == HD44780_ERR_ARGS && hd44780_print(0, "x") == HD44780_ERR_ARGS && hd44780_print_int(0, 1) == HD44780_ERR_ARGS && hd44780_set_cursor(0, 0, 0) == HD44780_ERR_ARGS);
        CHK(setup(&h, pcf(), 0, 16, 2) == HD44780_OK && hd44780_print(&h, 0) == HD44780_ERR_ARGS && hd44780_backlight(0, 1) == HD44780_ERR_ARGS && hd44780_home(0) == HD44780_ERR_ARGS);
        CHK(hd44780_display(0, 1) == HD44780_ERR_ARGS && hd44780_shift_display(0, 1) == HD44780_ERR_ARGS && hd44780_putc(0, 'a') == HD44780_ERR_ARGS);
        /* 쓰기 함수 실패 전파: 어디서 실패하든 같은 음수 */
        int ok_all = 1;
        for (int n = 0; n < 40; n++) {
            model_reset(pcf(), 0); M.fail_after = n;
            if (hd44780_init(&h, wr_cb, dl_cb, 0, pcf(), 16, 2) != -5) ok_all = 0;
        }
        CHK(ok_all);
        CHK(setup(&h, pcf(), 0, 16, 2) == HD44780_OK); M.fail_after = 0; M.nwrites = 0;
        CHK(hd44780_putc(&h, 'a') == -5 && h.col == 0);                                       /* 실패하면 커서 추적을 올리지 않음 */
        M.fail_after = -1;
    }
    /* 음성 대조: 지연을 절반으로 줄이면 모델이 위반을 잡아야 한다(모델이 실제로 시간을 검사한다는 증거) */
    halve = 1; CHK(setup(&h, pcf(), 0, 16, 2) == HD44780_OK); CHK(viol() > 0 && (M.v_init > 0)); halve = 0;
    /* PCF8574 접착 코드 */
    {
        hd44780_i2c_t d; pcf8574_t pc; memset(&pc, 0, sizeof pc); model_reset(pcf(), 25000);
        CHK(hd44780_i2c_init(&d, &pc, us_cb, pcf(), 16, 2) == HD44780_OK && viol() == 0 && M.four_bit);
        CHK(hd44780_print(&d.lcd, "I2C") == HD44780_OK); screen(0, RS16x2, 16, row); CHK(strncmp(row, "I2C", 3) == 0 && pc.shadow == M.port);
        CHK(hd44780_i2c_init(0, &pc, us_cb, pcf(), 16, 2) == HD44780_ERR_ARGS && hd44780_i2c_init(&d, 0, us_cb, pcf(), 16, 2) == HD44780_ERR_ARGS && hd44780_i2c_init(&d, &pc, 0, pcf(), 16, 2) == HD44780_ERR_ARGS);
    }
    printf(fails ? "FAILED %d\n" : "ALL PASS (hd44780 model)\n", fails);
    return fails != 0;
}
