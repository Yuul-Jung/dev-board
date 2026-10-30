/* SSD1306 그래픽 확장(선·원·삼각형·비트맵·5x7 글꼴) 호스트 시험 (교재 23장).
 * 빌드: cc -I i2c/sw i2c/tb/test_oled_gfx.c i2c/sw/ssd1306.c -o /tmp/oled_gfx -lm
 *
 * 한계(중요): 프레임버퍼 위의 그리기는 순수 계산이라 칩 없이 시험할 수 있지만, "화면에 보기 좋게 나오는가"는 여기서 확인되지 않는다.
 * 기대값은 드라이버 코드를 복사하지 않고 성질로 만들었다: 선 = 주축마다 정확히 한 점이고 이상적인 직선에서 0.5 칸 이내, 원 = 8방향 대칭이고 반지름과의 차이가 한계 이내,
 * 채운 원 = dx²+dy² ≤ r²+r, 삼각형 = 기하학적 거리·내부 판정, 모든 도형 = 평행 이동 불변(화면 밖으로 잘려도 겹치는 부분은 같음).
 * 5x7 글꼴 표는 출처 문서가 없고 사람이 눈으로 확인한 것이라 시험은 구조와 몇 글자 모양만 본다. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "../sw/ssd1306.h"

/* 가짜 I2C: 전송 바이트 기록 */
static uint8_t txlog[16][200]; static unsigned logn[16], nlog; static int fail_next;
void iic_delay_ms(unsigned ms) { (void)ms; }
int axi_iic_write(axi_iic_t *d, uint8_t a, const uint8_t *b, unsigned n)
{
    (void)d; (void)a;
    if (fail_next) return IIC_ERR_NACK;
    if (nlog < 16 && n <= 200) { memcpy(txlog[nlog], b, n); logn[nlog++] = n; }
    return IIC_OK;
}

static int fails;
#define CHK(c) do { if (!(c)) { fails++; printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)

static oled_t O, P;       /* P: 비교용 두 번째 화면 */
static int px(const oled_t *o, int x, int y) { return (o->fb[(y >> 3) * OLED_W + x] >> (y & 7)) & 1; }
static int count(const oled_t *o) { int n = 0; for (int y = 0; y < OLED_H; y++) for (int x = 0; x < OLED_W; x++) n += px(o, x, y); return n; }
static unsigned rs = 99;
static unsigned rnd(void) { rs = rs * 1664525u + 1013904223u; return rs >> 8; }
static int rr(int lo, int hi) { return lo + (int)(rnd() % (unsigned)(hi - lo + 1)); }

/* P 를 (sx,sy) 만큼 옮긴 O 와 비교: O 의 모든 점에 대해 P(x+sx, y+sy) == O(x,y), 단 P 화면 안일 때. P 밖으로 나간 O 의 점은 허용 안 함(원본이 화면 안에 완전히 들어온다는 전제) */
static int translate_equal(const oled_t *full, const oled_t *moved, int sx, int sy)
{
    for (int y = 0; y < OLED_H; y++) for (int x = 0; x < OLED_W; x++) {
        int fx = x - sx, fy = y - sy;
        int want = (fx >= 0 && fx < OLED_W && fy >= 0 && fy < OLED_H) ? px(full, fx, fy) : 0;
        if (px(moved, x, y) != want) return 0;
    }
    return 1;
}

static void t_invert_and_consts(void)
{
    axi_iic_t iic = {0};
    CHK(oled_init(&O, &iic, SSD1306_ADDR_SA0_L, SSD1306_COM_ALTERNATIVE, 0) == IIC_OK);
    nlog = 0;
    CHK(oled_invert(&O, 1) == IIC_OK && nlog == 1 && logn[0] == 2 && txlog[0][0] == SSD1306_CTRL_CMD && txlog[0][1] == 0xA7);
    CHK(oled_invert(&O, 0) == IIC_OK && nlog == 2 && txlog[1][1] == 0xA6);
    CHK(oled_invert(&O, 5) == IIC_OK && txlog[2][1] == 0xA7);            /* 0 이 아니면 켬 */
    fail_next = 1; CHK(oled_invert(&O, 1) == IIC_ERR_NACK); fail_next = 0;
}

/* ---- 선 ---- */
static void line_check(int x0, int y0, int x1, int y1)
{
    oled_clear(&O, 0);
    oled_line(&O, x0, y0, x1, y1, 1);
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int expect_cnt = 0, bad = 0;
    if (dx >= dy) {
        int lo = x0 < x1 ? x0 : x1, hi = x0 < x1 ? x1 : x0;
        for (int x = lo; x <= hi; x++) {
            double iy = dx == 0 ? y0 : y0 + (double)(x - x0) * (y1 - y0) / (x1 - x0);
            int n = 0, yy = -1;
            if (x >= 0 && x < OLED_W) for (int y = 0; y < OLED_H; y++) if (px(&O, x, y)) { n++; yy = y; }
            if (n > 1) bad++;
            if (n == 1) { if (fabs(yy - iy) > 0.5 + 1e-9) bad++; expect_cnt++; }
            if (n == 0 && x >= 0 && x < OLED_W) {                         /* 비었다면 이상적인 점이 화면 밖이어야 한다 */
                if (iy > -0.5 + 1e-9 && iy < OLED_H - 0.5 - 1e-9) bad++;
            }
        }
        for (int x = 0; x < OLED_W; x++) if ((x < lo || x > hi)) for (int y = 0; y < OLED_H; y++) if (px(&O, x, y)) bad++;
    } else {
        int lo = y0 < y1 ? y0 : y1, hi = y0 < y1 ? y1 : y0;
        for (int y = lo; y <= hi; y++) {
            double ix = y0 == y1 ? x0 : x0 + (double)(y - y0) * (x1 - x0) / (y1 - y0);
            int n = 0, xx = -1;
            if (y >= 0 && y < OLED_H) for (int x = 0; x < OLED_W; x++) if (px(&O, x, y)) { n++; xx = x; }
            if (n > 1) bad++;
            if (n == 1) { if (fabs(xx - ix) > 0.5 + 1e-9) bad++; expect_cnt++; }
            if (n == 0 && y >= 0 && y < OLED_H) { if (ix > -0.5 + 1e-9 && ix < OLED_W - 0.5 - 1e-9) bad++; }
        }
        for (int y = 0; y < OLED_H; y++) if (y < lo || y > hi) for (int x = 0; x < OLED_W; x++) if (px(&O, x, y)) bad++;
    }
    if (count(&O) != expect_cnt) bad++;
    if (bad) { fails++; printf("FAIL line (%d,%d)-(%d,%d): bad=%d count=%d expect=%d\n", x0, y0, x1, y1, bad, count(&O), expect_cnt); }
}

static void t_line(void)
{
    oled_clear(&O, 0); oled_line(&O, 5, 5, 5, 5, 1); CHK(count(&O) == 1 && px(&O, 5, 5));          /* 점 */
    oled_clear(&O, 0); oled_line(&O, 2, 3, 9, 3, 1); CHK(count(&O) == 8);                           /* 가로 */
    oled_clear(&O, 0); oled_line(&O, 2, 3, 2, 12, 1); CHK(count(&O) == 10);                         /* 세로 */
    oled_clear(&O, 0); oled_line(&O, 0, 0, 7, 7, 1); CHK(count(&O) == 8 && px(&O, 3, 3) && px(&O, 7, 7));   /* 대각선 */
    oled_clear(&O, 0); oled_line(&O, 7, 7, 0, 0, 1); CHK(count(&O) == 8);                           /* 거꾸로 */
    for (int i = 0; i < 6000; i++) {
        int m = i < 3000 ? 8 : 200;                                                                 /* 안쪽과 화면 밖으로 크게 벗어남 */
        line_check(rr(-m, OLED_W + m), rr(-m, OLED_H + m), rr(-m, OLED_W + m), rr(-m, OLED_H + m));
    }
    /* 모든 방향의 극단: 팔방 */
    for (int a = 0; a < 16; a++) line_check(64, 32, 64 + (int)(60 * cos(a * 3.14159265 / 8)), 32 + (int)(60 * sin(a * 3.14159265 / 8)));
    /* 지우기: 같은 선으로 켰다 끄면 깨끗 */
    oled_clear(&O, 0); oled_line(&O, 3, 60, 120, 4, 1); oled_line(&O, 3, 60, 120, 4, 0); CHK(count(&O) == 0);
    /* 이동 불변 (잘림) */
    for (int i = 0; i < 500; i++) {
        int x0 = rr(20, 60), y0 = rr(10, 40), x1 = rr(20, 100), y1 = rr(10, 50);
        int sx = rr(-70, 70), sy = rr(-40, 40);
        oled_clear(&O, 0); oled_line(&O, x0, y0, x1, y1, 1);
        oled_clear(&P, 0); oled_line(&P, x0 + sx, y0 + sy, x1 + sx, y1 + sy, 1);
        CHK(translate_equal(&O, &P, sx, sy));
    }
    /* 좌표 한계: 넘으면 그리지 않음, 한계 값은 그림 */
    oled_clear(&O, 0); oled_line(&O, 0, 0, OLED_COORD_LIM + 1, 0, 1); CHK(count(&O) == 0);
    oled_line(&O, INT_MIN, 0, INT_MAX, 5, 1); oled_line(&O, 0, INT_MIN, 10, INT_MAX, 1); CHK(count(&O) == 0);
    oled_line(&O, -OLED_COORD_LIM, 10, OLED_COORD_LIM, 10, 1); CHK(count(&O) == OLED_W);
}

/* ---- 원 ---- */
static void t_circle(void)
{
    double worst = 0;
    for (int r = 0; r <= 31; r++) {
        oled_clear(&O, 0); oled_circle(&O, 64, 32, r, 1);
        int n = count(&O);
        CHK(n > 0);
        if (r == 0) CHK(n == 1 && px(&O, 64, 32));
        if (r == 1) CHK(n == 4);                                                  /* 십자 모양 */
        int rows = 0, cols = 0;
        for (int y = 0; y < OLED_H; y++) {
            int any = 0;
            for (int x = 0; x < OLED_W; x++) if (px(&O, x, y)) {
                any = 1;
                double d = hypot(x - 64, y - 32); double e = fabs(d - r); if (e > worst) worst = e;
                CHK(px(&O, 128 - x, y) == 1 && px(&O, x, 64 - y) == 1);         /* 좌우·상하 대칭 */
                CHK(px(&O, 64 + (y - 32), 32 + (x - 64)) == 1);                /* 대각선 대칭 (dx,dy)→(dy,dx) */
                int nb = 0;                                                       /* 이웃 8칸 중 하나는 점 → 고리가 끊기지 않음 */
                for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) if ((i || j) && x + i >= 0 && x + i < OLED_W && y + j >= 0 && y + j < OLED_H) nb += px(&O, x + i, y + j);
                if (r >= 1) CHK(nb >= 2 || r == 1);
            }
            rows += any;
        }
        for (int x = 0; x < OLED_W; x++) { int any = 0; for (int y = 0; y < OLED_H; y++) any |= px(&O, x, y); cols += any; }
        CHK(rows == 2 * r + 1 && cols == 2 * r + 1);                              /* 모든 행·열에 점이 있다(빈틈 없음) */
    }
    CHK(worst <= 0.5 + 1e-9);                                                      /* 중점 원: 모든 점이 반지름에서 0.5 칸 이내(r ≤ 31 에서 측정) */
    printf("  (원 최대 편차 %.4f 칸)\n", worst);
    /* 큰 반지름(화면 밖으로 잘림)에서도 같은 성질: 이동 불변 */
    for (int i = 0; i < 400; i++) {
        int r = rr(0, 30), cx = rr(r, OLED_W - 1 - r), cy = rr(r, OLED_H - 1 - r);
        int sx = rr(-100, 100), sy = rr(-60, 60);
        oled_clear(&O, 0); oled_circle(&O, cx, cy, r, 1);
        oled_clear(&P, 0); oled_circle(&P, cx + sx, cy + sy, r, 1);
        CHK(translate_equal(&O, &P, sx, sy));
    }
    /* 화면보다 큰 원, 음수·한계 */
    oled_clear(&O, 0); oled_circle(&O, 64, 32, -1, 1); CHK(count(&O) == 0);
    oled_circle(&O, 64, 32, OLED_COORD_LIM + 1, 1); oled_circle(&O, INT_MAX, 0, 5, 1); oled_circle(&O, 0, INT_MIN, 5, 1); CHK(count(&O) == 0);
    oled_circle(&O, 64, 32, 500, 1); CHK(count(&O) == 0);                          /* 화면 전체가 원 안쪽 → 외곽선이 안 보임 */
    oled_clear(&O, 0); oled_circle(&O, 64, 32, 40, 1); CHK(count(&O) > 0);        /* 위아래가 잘린 큰 원 */
    oled_clear(&O, 0); oled_circle(&O, 64, 32, OLED_COORD_LIM, 1); CHK(count(&O) == 0);
    /* 채운 원 */
    for (int r = 0; r <= 60; r++) for (int k = 0; k < 4; k++) {
        int cx = k == 0 ? 64 : rr(-30, 160), cy = k == 0 ? 32 : rr(-30, 100);
        oled_clear(&O, 0); oled_fill_circle(&O, cx, cy, r, 1);
        int bad = 0;
        for (int y = 0; y < OLED_H; y++) for (int x = 0; x < OLED_W; x++) {
            long dx = x - cx, dy = y - cy;
            int want = dx * dx + dy * dy <= (long)r * r + r;
            if (px(&O, x, y) != want) bad++;
        }
        CHK(bad == 0);
        /* 외곽선은 채운 원의 부분집합 */
        oled_clear(&P, 0); oled_circle(&P, cx, cy, r, 1);
        for (int y = 0; y < OLED_H; y++) for (int x = 0; x < OLED_W; x++) if (px(&P, x, y)) CHK(px(&O, x, y));
    }
    oled_clear(&O, 0); oled_fill_circle(&O, 64, 32, -1, 1); oled_fill_circle(&O, INT_MAX, 5, 3, 1); oled_fill_circle(&O, 5, 5, OLED_COORD_LIM + 1, 1); CHK(count(&O) == 0);
    oled_fill_circle(&O, 10, 10, 3, 0); CHK(count(&O) == 0);
    oled_clear(&O, 1); oled_fill_circle(&O, 64, 32, 5, 0);
    {
        int k = 0; for (int dy = -5; dy <= 5; dy++) for (int dx = -5; dx <= 5; dx++) if (dx * dx + dy * dy <= 30) k++;
        CHK(count(&O) == OLED_W * OLED_H - k);                                         /* 지우기 */
    }
}

/* ---- 삼각형 ---- */
static double seg_dist(double px_, double py_, double ax, double ay, double bx, double by)
{
    double vx = bx - ax, vy = by - ay, wx = px_ - ax, wy = py_ - ay, L = vx * vx + vy * vy;
    double t = L == 0 ? 0 : (wx * vx + wy * vy) / L; if (t < 0) t = 0; if (t > 1) t = 1;
    return hypot(px_ - (ax + t * vx), py_ - (ay + t * vy));
}
static double cross(double ax, double ay, double bx, double by, double cx, double cy) { return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax); }

static void tri_check(int x0, int y0, int x1, int y1, int x2, int y2)
{
    oled_clear(&O, 0); oled_triangle(&O, x0, y0, x1, y1, x2, y2, 1, 1);
    oled_clear(&P, 0); oled_triangle(&P, x0, y0, x1, y1, x2, y2, 0, 1);
    double area2 = cross(x0, y0, x1, y1, x2, y2);
    int bad = 0;
    for (int y = 0; y < OLED_H; y++) for (int x = 0; x < OLED_W; x++) {
        double d = fmin(seg_dist(x, y, x0, y0, x1, y1), fmin(seg_dist(x, y, x1, y1, x2, y2), seg_dist(x, y, x2, y2, x0, y0)));
        double c0 = cross(x0, y0, x1, y1, x, y), c1 = cross(x1, y1, x2, y2, x, y), c2 = cross(x2, y2, x0, y0, x, y);
        int inside = area2 != 0 && ((c0 >= 0 && c1 >= 0 && c2 >= 0) || (c0 <= 0 && c1 <= 0 && c2 <= 0));
        if (px(&O, x, y)) { if (!inside && d > 0.5 + 1e-9) bad++; }                  /* 칠한 점은 삼각형 안이거나 경계에서 0.5 칸 이내 */
        else if (inside && d >= 1.0) bad++;                                           /* 경계에서 1 칸 이상 안쪽 점은 반드시 칠해짐 */
        if (px(&P, x, y) && !px(&O, x, y)) bad++;                                     /* 외곽선 ⊂ 채움 */
        if (px(&P, x, y) && d > 0.5 + 1e-9) bad++;                                    /* 외곽선은 변에서 0.5 칸 이내 */
    }
    if (bad) { fails++; printf("FAIL triangle (%d,%d) (%d,%d) (%d,%d): bad=%d\n", x0, y0, x1, y1, x2, y2, bad); }
}

static void t_triangle(void)
{
    for (int i = 0; i < 1500; i++) tri_check(rr(0, 127), rr(0, 63), rr(0, 127), rr(0, 63), rr(0, 127), rr(0, 63));
    for (int i = 0; i < 700; i++) tri_check(rr(-60, 190), rr(-40, 100), rr(-60, 190), rr(-40, 100), rr(-60, 190), rr(-40, 100));   /* 잘림 */
    tri_check(10, 10, 10, 10, 10, 10);                                              /* 한 점 */
    tri_check(5, 5, 50, 5, 100, 5);                                                 /* 가로로 한 줄 */
    tri_check(5, 5, 5, 40, 5, 20);                                                  /* 세로로 한 줄 */
    tri_check(0, 0, 40, 40, 20, 20);                                                /* 대각선으로 한 줄 */
    tri_check(0, 0, 127, 0, 0, 63);                                                 /* 직각 */
    tri_check(0, 0, 127, 0, 127, 63);
    tri_check(10, 10, 11, 10, 10, 11);                                              /* 아주 작음 */
    /* 이동 불변 */
    for (int i = 0; i < 300; i++) {
        int x0 = rr(30, 60), y0 = rr(15, 35), x1 = rr(30, 100), y1 = rr(15, 45), x2 = rr(30, 100), y2 = rr(15, 45);
        int sx = rr(-80, 80), sy = rr(-50, 50), f = i & 1;
        oled_clear(&O, 0); oled_triangle(&O, x0, y0, x1, y1, x2, y2, f, 1);
        oled_clear(&P, 0); oled_triangle(&P, x0 + sx, y0 + sy, x1 + sx, y1 + sy, x2 + sx, y2 + sy, f, 1);
        CHK(translate_equal(&O, &P, sx, sy));
    }
    /* 한계, 지우기 */
    oled_clear(&O, 0); oled_triangle(&O, 0, 0, OLED_COORD_LIM + 1, 0, 0, 10, 1, 1); CHK(count(&O) == 0);
    oled_triangle(&O, INT_MIN, 0, 5, 5, INT_MAX, 9, 1, 1); CHK(count(&O) == 0);
    oled_clear(&O, 1); oled_triangle(&O, 10, 10, 60, 10, 10, 50, 1, 0);
    oled_clear(&P, 0); oled_triangle(&P, 10, 10, 60, 10, 10, 50, 1, 1);
    for (int y = 0; y < OLED_H; y++) for (int x = 0; x < OLED_W; x++) CHK(px(&O, x, y) == !px(&P, x, y));
    oled_clear(&O, 0); oled_triangle(&O, -OLED_COORD_LIM, -OLED_COORD_LIM, OLED_COORD_LIM, -OLED_COORD_LIM, 0, OLED_COORD_LIM, 1, 1);   /* 화면 전체를 덮는 큰 삼각형 */
    CHK(count(&O) == OLED_W * OLED_H);
}

/* ---- 비트맵 ---- */
static void t_bitmap(void)
{
    /* 9x3: 한 행 2바이트, 마지막 바이트는 최상위 1비트만 사용 */
    const uint8_t bm[] = { 0xA5, 0x80,   0x00, 0x00,   0xFF, 0x80 };
    oled_clear(&O, 0); oled_bitmap(&O, 10, 20, bm, 9, 3, 1, OLED_TRANSPARENT);
    for (int j = 0; j < 3; j++) for (int i = 0; i < 9; i++) {
        int bit = (bm[j * 2 + i / 8] >> (7 - i % 8)) & 1;
        CHK(px(&O, 10 + i, 20 + j) == bit);
    }
    CHK(count(&O) == 14);                                                           /* 첫 행 0xA5(1010 0101)=4 + 9번째 열 1 = 5, 둘째 행 0, 셋째 행 0xFF=8 + 1 = 9 */
    oled_clear(&O, 0); oled_bitmap(&O, 10, 20, bm, 9, 3, 1, 0);
    CHK(count(&O) == 14);
    CHK(px(&O, 18, 20) == 1 && px(&O, 17, 22) == 1);                               /* 9번째 열 */
    /* 투명: 바탕이 유지됨 / 불투명: 바탕이 지워짐 */
    oled_clear(&O, 1); oled_bitmap(&O, 10, 20, bm, 9, 3, 0, OLED_TRANSPARENT);
    CHK(count(&O) == OLED_W * OLED_H - 14);
    oled_clear(&O, 1); oled_bitmap(&O, 10, 20, bm, 9, 3, 0, 1);
    CHK(count(&O) == OLED_W * OLED_H - 14);                                         /* 1 비트 14개만 0 으로 지워지고 0 비트 자리는 1 로 칠해져 그대로 */
    /* 잘림 (이동 불변) */
    const uint8_t big[] = { 0x81,0xC3,0xE7,0xFF,0xFF,0xE7,0xC3,0x81,   0x18,0x24,0x42,0x81,0x81,0x42,0x24,0x18 };   /* 8 열 x 16 행: 행마다 1바이트 */
    for (int i = 0; i < 300; i++) {
        int x = rr(10, 100), y = rr(10, 40), sx = rr(-110, 110), sy = rr(-60, 60);
        oled_clear(&O, 0); oled_bitmap(&O, x, y, big, 8, 16, 1, 0);
        oled_clear(&P, 0); oled_bitmap(&P, x + sx, y + sy, big, 8, 16, 1, 0);
        CHK(translate_equal(&O, &P, sx, sy));
    }
    oled_clear(&O, 0);
    oled_bitmap(&O, 0, 0, NULL, 8, 8, 1, 0); oled_bitmap(&O, 0, 0, big, 0, 8, 1, 0); oled_bitmap(&O, 0, 0, big, 8, 0, 1, 0); oled_bitmap(&O, 0, 0, big, -1, 8, 1, 0);
    oled_bitmap(&O, INT_MAX, 0, big, 8, 8, 1, 0); oled_bitmap(&O, 0, INT_MIN, big, 8, 8, 1, 0); oled_bitmap(&O, 0, 0, big, OLED_COORD_LIM + 1, 1, 1, 0);
    CHK(count(&O) == 0);
    /* 넓은 비트맵(폭 1~40, 무작위 데이터)을 점마다 독립 계산과 비교, 불투명 배경 포함 */
    for (int it = 0; it < 400; it++) {
        int w = rr(1, 40), h = rr(1, 6), st = (w + 7) / 8; uint8_t d[6 * 5];
        for (int k = 0; k < h * st; k++) d[k] = (uint8_t)rnd();
        int x = rr(0, 80), y = rr(0, 50), bg = (it & 1) ? 0 : OLED_TRANSPARENT;
        oled_clear(&O, 1); oled_bitmap(&O, x, y, d, w, h, 0, bg);
        for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) {
            int bit = (d[j * st + i / 8] >> (7 - i % 8)) & 1;
            int want = bit ? 0 : (bg == 0 ? 0 : 1);   /* fg=0, bg=0 이면 전부 0, 투명이면 0 비트 자리는 원래 1 */
            if (bg == 0) want = 0;
            CHK(px(&O, x + i, y + j) == want);
        }
        oled_clear(&O, 0); oled_bitmap(&O, x, y, d, w, h, 1, bg);
        for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) CHK(px(&O, x + i, y + j) == ((d[j * st + i / 8] >> (7 - i % 8)) & 1));
    }
    /* 8의 배수 폭: 한 행 1바이트 */
    const uint8_t b8[] = { 0xF0, 0x0F };
    oled_clear(&O, 0); oled_bitmap(&O, 0, 0, b8, 8, 2, 1, OLED_TRANSPARENT);
    CHK(px(&O, 0, 0) && px(&O, 3, 0) && !px(&O, 4, 0) && !px(&O, 3, 1) && px(&O, 4, 1) && px(&O, 7, 1) && count(&O) == 8);
}

/* ---- 글꼴과 글자 ---- */
static void t_font(void)
{
    /* 구조 */
    for (int c = 0x20; c <= 0x7E; c++) {
        const uint8_t *g = oled_font5x7(c); int any = 0;
        for (int k = 0; k < 5; k++) { CHK((g[k] & 0x80) == 0); any |= g[k]; }          /* 7행만 사용 */
        CHK((c == ' ') == (any == 0));                                                  /* 공백만 비어 있음 */
        for (int d = c + 1; d <= 0x7E; d++) CHK(memcmp(g, oled_font5x7(d), 5) != 0);   /* 모든 글자가 서로 다르다 */
    }
    CHK(oled_font5x7(0x1F) == oled_font5x7('?') && oled_font5x7(0x7F) == oled_font5x7('?') && oled_font5x7(-1) == oled_font5x7('?') && oled_font5x7(300) == oled_font5x7('?'));
    /* 사람이 눈으로 확인해 적은 모양 */
    struct { int ch; const char *art[7]; } want[] = {
        { 'A', { ".###.", "#...#", "#...#", "#...#", "#####", "#...#", "#...#" } },
        { 'E', { "#####", "#....", "#....", "####.", "#....", "#....", "#####" } },
        { '0', { ".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###." } },
        { '?', { ".###.", "#...#", "....#", "...#.", "..#..", ".....", "..#.." } },
        { 'a', { ".....", ".....", ".###.", "....#", ".####", "#...#", ".####" } },
        { 'p', { ".....", ".....", "####.", "#...#", "####.", "#....", "#...." } },
        { '~', { ".....", ".#...", "#.#.#", "...#.", ".....", ".....", "....." } },
    };
    for (unsigned k = 0; k < sizeof want / sizeof *want; k++) {
        const uint8_t *g = oled_font5x7(want[k].ch);
        for (int r = 0; r < 7; r++) for (int c = 0; c < 5; c++)
            if (((g[c] >> r) & 1) != (want[k].art[r][c] == '#')) { fails++; printf("FAIL font '%c' row %d col %d\n", want[k].ch, r, c); }
    }
}

static void t_print(void)
{
    const uint8_t *gH = oled_font5x7('H');
    oled_clear(&O, 0);
    int nx = oled_print(&O, 10, 20, "Hi", 1, 1, OLED_TRANSPARENT);
    CHK(nx == 22);
    for (int c = 0; c < 5; c++) for (int r = 0; r < 7; r++) CHK(px(&O, 10 + c, 20 + r) == ((gH[c] >> r) & 1));
    const uint8_t *gi = oled_font5x7('i');
    for (int c = 0; c < 5; c++) for (int r = 0; r < 7; r++) CHK(px(&O, 16 + c, 20 + r) == ((gi[c] >> r) & 1));
    for (int r = 0; r < 8; r++) { CHK(px(&O, 15, 20 + r) == 0); CHK(px(&O, 21, 20 + r) == 0); }          /* 간격 열 */
    for (int c = 0; c < 12; c++) CHK(px(&O, 10 + c, 27) == 0);                                          /* 간격 행 */
    /* 확대 */
    for (int sc = 2; sc <= 4; sc++) {
        oled_clear(&O, 0);
        nx = oled_print(&O, 3, 2, "H", sc, 1, OLED_TRANSPARENT);
        CHK(nx == 3 + 6 * sc);
        for (int c = 0; c < 5; c++) for (int r = 0; r < 7; r++)
            for (int j = 0; j < sc; j++) for (int i = 0; i < sc; i++)
                CHK(px(&O, 3 + c * sc + i, 2 + r * sc + j) == ((gH[c] >> r) & 1));
    }
    /* 불투명 배경: 칸 6x8(확대 포함)을 모두 칠한다 */
    for (int sc = 1; sc <= 2; sc++) {
        oled_clear(&O, 1);
        oled_print(&O, 20, 10, "A", sc, 1, 0);
        const uint8_t *gA = oled_font5x7('A'); int ones_in = 0;
        for (int y = 0; y < OLED_H; y++) for (int x = 0; x < OLED_W; x++) {
            int in = x >= 20 && x < 20 + 6 * sc && y >= 10 && y < 10 + 8 * sc;
            if (!in) CHK(px(&O, x, y) == 1);
            else {
                int c = (x - 20) / sc, r = (y - 10) / sc;
                int want = (c < 5 && r < 7) ? (gA[c] >> r) & 1 : 0;
                CHK(px(&O, x, y) == want); ones_in += px(&O, x, y);
            }
        }
        CHK(ones_in > 0);
    }
    /* 투명 + 지우기(fg = 0) */
    oled_clear(&O, 1); oled_print(&O, 20, 10, "A", 1, 0, OLED_TRANSPARENT);
    { const uint8_t *gA = oled_font5x7('A'); int k = 0; for (int c = 0; c < 5; c++) for (int r = 0; r < 7; r++) k += (gA[c] >> r) & 1; CHK(count(&O) == OLED_W * OLED_H - k); }
    /* 줄바꿈과 반환값 */
    oled_clear(&O, 0); nx = oled_print(&O, 10, 5, "A\nBC", 2, 1, OLED_TRANSPARENT);
    CHK(nx == 10 + 2 * 12);
    oled_clear(&P, 0); oled_print(&P, 10, 5, "A", 2, 1, OLED_TRANSPARENT); oled_print(&P, 10, 5 + 16, "BC", 2, 1, OLED_TRANSPARENT);
    CHK(memcmp(O.fb, P.fb, sizeof O.fb) == 0);
    /* 두 줄 이상, 줄 끝 */
    oled_clear(&O, 0); nx = oled_print(&O, 4, 4, "\n\nX", 1, 1, OLED_TRANSPARENT); CHK(nx == 4 + 6);
    oled_clear(&P, 0); oled_print(&P, 4, 4 + 16, "X", 1, 1, OLED_TRANSPARENT); CHK(memcmp(O.fb, P.fb, sizeof O.fb) == 0);
    /* 배율 범위 */
    oled_clear(&O, 0); oled_print(&O, 0, 0, "Q", 0, 1, OLED_TRANSPARENT);
    oled_clear(&P, 0); oled_print(&P, 0, 0, "Q", 1, 1, OLED_TRANSPARENT); CHK(memcmp(O.fb, P.fb, sizeof O.fb) == 0);
    oled_clear(&O, 0); nx = oled_print(&O, 0, 0, "Q", 100, 1, OLED_TRANSPARENT); CHK(nx == 48);
    oled_clear(&P, 0); oled_print(&P, 0, 0, "Q", 8, 1, OLED_TRANSPARENT); CHK(memcmp(O.fb, P.fb, sizeof O.fb) == 0);
    oled_clear(&O, 0); nx = oled_print(&O, 0, 0, "Q", -5, 1, OLED_TRANSPARENT); CHK(nx == 6);
    /* 범위 밖 문자는 '?' */
    oled_clear(&O, 0); oled_print(&O, 0, 0, "\x01\x7F\xFF\t", 1, 1, OLED_TRANSPARENT);
    oled_clear(&P, 0); oled_print(&P, 0, 0, "????", 1, 1, OLED_TRANSPARENT); CHK(memcmp(O.fb, P.fb, sizeof O.fb) == 0);
    /* 널·빈 문자열 */
    oled_clear(&O, 0); CHK(oled_print(&O, 7, 8, "", 1, 1, 0) == 7 && oled_print(&O, 7, 8, NULL, 1, 1, 0) == 7 && count(&O) == 0);
    /* 이동 불변(잘림) */
    for (int i = 0; i < 200; i++) {
        int sc = 1 + i % 3, x = rr(20, 40), y = rr(10, 20), sx = rr(-90, 90), sy = rr(-40, 40);
        oled_clear(&O, 0); oled_print(&O, x, y, "Ag", sc, 1, 0);
        oled_clear(&P, 0); oled_print(&P, x + sx, y + sy, "Ag", sc, 1, 0);
        CHK(x + 12 * sc <= OLED_W && y + 8 * sc <= OLED_H);                         /* 원본은 화면 안에 완전히 들어옴 → 평행 이동과 정확히 같아야 함 */
        CHK(translate_equal(&O, &P, sx, sy));
    }
    /* 한계 */
    oled_clear(&O, 0);
    CHK(oled_print(&O, INT_MAX, 0, "ABC", 8, 1, 0) == INT_MAX && oled_print(&O, 0, INT_MIN, "ABC", 8, 1, 0) == 0 && count(&O) == 0);
    /* 폭 */
    CHK(oled_text_width("", 1) == 0 && oled_text_width(NULL, 1) == 0 && oled_text_width("A", 1) == 5 && oled_text_width("AB", 1) == 11);
    CHK(oled_text_width("AB", 2) == 22 && oled_text_width("AB\nCDE", 1) == 11 && oled_text_width("\nABC", 1) == 0);
    CHK(oled_text_width("A", 0) == 5 && oled_text_width("A", 99) == 40);
    /* 책 스타일 사용: 도형·글자·비트맵을 한 화면에 */
    oled_clear(&O, 0);
    oled_rect(&O, 0, 0, 128, 64, 0, 1); oled_line(&O, 0, 0, 127, 63, 1); oled_circle(&O, 100, 20, 10, 1); oled_fill_circle(&O, 100, 20, 3, 1);
    oled_triangle(&O, 10, 50, 30, 50, 20, 35, 1, 1); oled_print(&O, 4, 4, "OLED 23", 1, 1, 0);
    CHK(count(&O) > 100);
}

/* 무작위(극단 값 포함) 호출: ASan/UBSan 에서 범위 밖 접근·정수 넘침이 없어야 한다 */
static int wild(void)
{
    static const int ext[] = { INT_MIN, INT_MIN + 1, -OLED_COORD_LIM - 1, -OLED_COORD_LIM, -1, 0, 1, 63, 64, 127, 128, OLED_COORD_LIM, OLED_COORD_LIM + 1, INT_MAX - 1, INT_MAX };
    unsigned k = rnd() % 4;
    if (k == 0) return ext[rnd() % (sizeof ext / sizeof *ext)];
    if (k == 1) return rr(-300, 300);
    return rr(-20, 150);
}
static void t_fuzz(void)
{
    static uint8_t buf[4096]; for (unsigned i = 0; i < sizeof buf; i++) buf[i] = (uint8_t)rnd();
    char str[40];
    for (long it = 0; it < 60000; it++) {
        switch (rnd() % 12) {
        case 0: oled_line(&O, wild(), wild(), wild(), wild(), rnd() & 1); break;
        case 1: oled_circle(&O, wild(), wild(), rnd() % 3 ? rr(-3, 80) : wild(), rnd() & 1); break;
        case 2: oled_fill_circle(&O, wild(), wild(), rnd() % 3 ? rr(-3, 80) : wild(), rnd() & 1); break;
        case 3: oled_triangle(&O, wild(), wild(), wild(), wild(), wild(), wild(), rnd() & 1, rnd() & 1); break;
        case 4: { int w = rr(-3, 300), h = rr(-3, 300); if (((w + 7) / 8) * (h > 0 ? h : 0) <= (int)sizeof buf) oled_bitmap(&O, wild(), wild(), buf, w, h, rnd() & 1, rr(-1, 1)); break; }
        case 5: oled_hline(&O, wild(), wild(), wild(), rnd() & 1); break;
        case 6: oled_rect(&O, wild(), wild(), wild(), wild(), rnd() & 1, rnd() & 1); break;
        case 7: case 8: {
            unsigned n = rnd() % 12; for (unsigned i = 0; i < n; i++) str[i] = (char)(rnd() % 4 ? 0x20 + rnd() % 95 : rnd() % 256); str[n ? n : 0] = 0;
            for (unsigned i = 0; i < n; i++) if (str[i] == 0) str[i] = 'x';
            if (rnd() & 1) oled_print(&O, wild(), wild(), str, wild(), rr(-1, 1), rr(-1, 1)); else oled_text(&O, wild(), wild(), str, wild(), rnd() & 1);
            (void)oled_text_width(str, wild()); break; }
        case 9: oled_pixel(&O, wild(), wild(), rnd() & 1); break;
        case 10: oled_clear(&O, rnd() & 1); break;
        default: (void)oled_font5x7(wild()); break;
        }
    }
    CHK(1);
}

/* 변형 검사에서 살아남은 글꼴·배율 부분을 겨냥 */
static void t_fonts_pinned(void)
{
    /* 3x5 글꼴: 글자마다 눈으로 확인한 모양 (행 5개, 열 3개) */
    static const struct { char ch; const char *art[5]; } G[] = {
        { '0', { "###", "#.#", "#.#", "#.#", "###" } }, { '1', { ".#.", "##.", ".#.", ".#.", "###" } },
        { '2', { "###", "..#", "###", "#..", "###" } }, { '3', { "###", "..#", "###", "..#", "###" } },
        { '4', { "#.#", "#.#", "###", "..#", "..#" } }, { '5', { "###", "#..", "###", "..#", "###" } },
        { '6', { "###", "#..", "###", "#.#", "###" } }, { '7', { "###", "..#", "..#", "..#", "..#" } },
        { '8', { "###", "#.#", "###", "#.#", "###" } }, { '9', { "###", "#.#", "###", "..#", "###" } },
        { '-', { "...", "...", "###", "...", "..." } }, { '.', { "...", "...", "...", "...", ".#." } },
        { ':', { "...", ".#.", "...", ".#.", "..." } }, { ' ', { "...", "...", "...", "...", "..." } },
    };
    static oled_t o, o2;
    for (unsigned k = 0; k < sizeof G / sizeof *G; k++) {
        for (int scale = 1; scale <= 3; scale += 1) {
            oled_clear(&o, 0); char t[2] = { G[k].ch, 0 }; int nx = oled_text(&o, 10, 20, t, scale, 1);
            CHK(nx == 10 + 4 * scale);
            for (int r = 0; r < 5; r++) for (int c = 0; c < 3; c++)
                for (int dy = 0; dy < scale; dy++) for (int dx = 0; dx < scale; dx++)
                    CHK(px(&o, 10 + c * scale + dx, 20 + r * scale + dy) == (G[k].art[r][c] == '#'));
            int want_n = 0; for (int r = 0; r < 5; r++) for (int c = 0; c < 3; c++) want_n += G[k].art[r][c] == '#';
            CHK(count(&o) == want_n * scale * scale);              /* 글자 밖에는 아무것도 안 그림 */
            oled_text(&o, 10, 20, t, scale, 0); CHK(count(&o) == 0);   /* on=0 이면 지움 */
        }
    }
    oled_clear(&o, 0); CHK(oled_text(&o, 5, 5, "x", 1, 1) == 9 && count(&o) == 0);     /* 없는 글자는 안 그리고 폭만 지남 */
    oled_clear(&o, 0); CHK(oled_text(&o, 5, 5, "", 1, 1) == 5 && oled_text(&o, 5, 5, "10:", 1, 1) == 17);
    /* 배율 범위 맞춤: 0 이하는 1, 9 이상은 8 */
    for (int sc = -2; sc <= 0; sc++) { oled_clear(&o, 0); oled_clear(&o2, 0); oled_text(&o, 0, 0, "28", sc, 1); oled_text(&o2, 0, 0, "28", 1, 1); CHK(!memcmp(o.fb, o2.fb, sizeof o.fb)); }
    for (int sc = 9; sc <= 12; sc++) { oled_clear(&o, 0); oled_clear(&o2, 0); CHK(oled_text(&o, 0, 0, "7", sc, 1) == 32); oled_text(&o2, 0, 0, "7", 8, 1); CHK(!memcmp(o.fb, o2.fb, sizeof o.fb)); }
    oled_clear(&o, 0); oled_clear(&o2, 0); oled_text(&o, 0, 0, "7", 8, 1); oled_text(&o2, 0, 0, "7", 7, 1); CHK(memcmp(o.fb, o2.fb, sizeof o.fb) != 0);   /* 7 과 8 은 다르다 */
    for (int sc = -2; sc <= 0; sc++) { oled_clear(&o, 0); oled_clear(&o2, 0); oled_print(&o, 0, 0, "Ab", sc, 1, 0); oled_print(&o2, 0, 0, "Ab", 1, 1, 0); CHK(!memcmp(o.fb, o2.fb, sizeof o.fb)); }
    for (int sc = 9; sc <= 12; sc++) { oled_clear(&o, 0); oled_clear(&o2, 0); oled_print(&o, 0, 0, "A", sc, 1, 0); oled_print(&o2, 0, 0, "A", 8, 1, 0); CHK(!memcmp(o.fb, o2.fb, sizeof o.fb)); }
    oled_clear(&o, 0); oled_clear(&o2, 0); oled_print(&o, 0, 0, "A", 8, 1, 0); oled_print(&o2, 0, 0, "A", 7, 1, 0); CHK(memcmp(o.fb, o2.fb, sizeof o.fb) != 0);
    /* 5x7 글꼴 표 전체: 기억에 의존해 입력한 표를 눈으로 확인한 뒤의 고정값(FNV-1a). 모양이 맞다는 근거가 아니라 실수로 바뀌는 것을 막는 회귀 방지용 */
    uint64_t h = 1469598103934665603ull;
    for (int ch = 32; ch < 127; ch++) { const uint8_t *g = oled_font5x7(ch); for (int i = 0; i < 5; i++) { h ^= g[i]; h *= 1099511628211ull; } }
    CHK(h == 755867975862490404ull);
}

int main(void)
{
    t_invert_and_consts(); t_line(); t_circle(); t_triangle(); t_bitmap(); t_font(); t_print(); t_fonts_pinned(); t_fuzz();
    if (fails) { printf("%d FAILED\n", fails); return 1; }
    printf("ALL PASS\n");
    return 0;
}
