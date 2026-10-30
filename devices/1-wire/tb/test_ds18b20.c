/* DS18B20 드라이버 호스트 시험. 모의 소자는 1-Wire 마스터 IP 의 레지스터(rtl/README.md)와 DS18B20 슬레이브를 비트 단위로 흉내 낸다.
 * 한계: 모델이 레지스터 의미와 데이터시트 요약을 내가 해석해 만든 것이라 해석이 틀리면 시험도 같이 틀린다(순환). RTL 과 연결되지 않았다. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "ds18b20.h"

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

/* ---------- 독립 구현 CRC: 비반사형 MSB 먼저 다항식 나눗셈 (드라이버의 반사형 비트 연산과 다른 방식) ---------- */
static uint8_t crc_ref(const uint8_t *p, int n)
{
    unsigned reg = 0;                                  /* 8비트 레지스터, 0x31 = x^8 의 아래 항들 (x^5+x^4+1) */
    for (int i = 0; i < n; i++) {
        /* 1-Wire 는 각 바이트를 LSB 먼저 보낸다. 비트 순서대로 흘려 보낸다 */
        for (int b = 0; b < 8; b++) {
            unsigned in = (p[i] >> b) & 1;
            unsigned fb = ((reg >> 0) & 1) ^ in;       /* 쉬프트 레지스터 LFSR, 오른쪽으로 이동(데이터시트 그림 구조) */
            reg >>= 1;
            if (fb) reg ^= 0x8C;
        }
    }
    return (uint8_t)reg;
}
/* 위 함수는 구조가 드라이버와 비슷하다. 진짜 독립 비교를 위해 다항식 몫 계산(GF(2) 긴 나눗셈)도 둔다 */
static uint8_t crc_longdiv(const uint8_t *p, int n)
{
    /* 비트열을 LSB 먼저 늘어놓고 x^8 을 곱한 뒤 g(x)=x^8+x^5+x^4+1 로 나눈 나머지. 다항식 계수 순서는 먼저 온 비트가 최고차 */
    int nbits = n * 8 + 8;
    unsigned char bits[8 * 64 + 8]; memset(bits, 0, sizeof bits);
    for (int i = 0; i < n; i++) for (int b = 0; b < 8; b++) bits[i * 8 + b] = (p[i] >> b) & 1;
    for (int i = 0; i < n * 8; i++) if (bits[i]) { bits[i] ^= 1; bits[i + 3] ^= 1; bits[i + 4] ^= 1; bits[i + 8] ^= 1; }
    /* 나머지의 계수 bits[n*8 .. n*8+7] 가 x^7 ... x^0 순. 직렬로 보낼 때 CRC 바이트의 LSB 가 x^7 이다 */
    uint8_t r = 0;
    for (int k = 0; k < 8; k++) if (bits[n * 8 + k]) r |= (uint8_t)(1u << k);
    (void)nbits;
    return r;
}

/* ---------- 모의 1-Wire 마스터 + DS18B20 ---------- */
enum { M_ROMCMD, M_FUNC, M_SEND, M_RECV, M_IGNORE };
static struct {
    int present, dead, dead_mode, die_after, conv_never, crc_flip_byte, powered_parasite;
    uint32_t stat_reads, nops, violations;
    uint32_t cmd_busy_polls;          /* CMD 뒤 busy 로 보일 STAT 읽기 횟수 */
    uint32_t conv_slots;              /* 변환 중 읽기 슬롯 수(0 을 돌려주는 횟수) */
    int16_t true_raw;                 /* 변환이 끝나면 온도 레지스터가 되는 값(12비트 기준) */
    uint8_t sp[9];                    /* 스크래치패드 */
    int mode, bitn, byte_acc, sendpos, recvpos, sendbit;
    uint32_t conv_left; int converting;
    uint32_t busy_left, stat;
    uint8_t cmdlog[64]; int ncmd;     /* 슬레이브가 받은 바이트(리셋 사이 구분 없이 이어 붙임) */
    int resets, wr_bits, rd_bits;
} S;

static void sp_refresh_crc(void) { S.sp[8] = crc_longdiv(S.sp, 8); }
static void dev_reset_dev(void) { memset(&S, 0, sizeof S); S.present = 1; S.sp[0] = 0x50; S.sp[1] = 0x05; S.sp[2] = 0x4B; S.sp[3] = 0x46; S.sp[4] = 0x7F; S.sp[5] = 0xFF; S.sp[6] = 0x0C; S.sp[7] = 0x10; sp_refresh_crc(); S.cmd_busy_polls = 2; S.conv_slots = 30; }

static void apply_conversion(void)
{
    int16_t raw = S.true_raw;
    unsigned res = 9 + ((S.sp[4] >> 5) & 3);
    int undef = 12 - (int)res;
    int16_t v = (int16_t)((raw >> undef) * (1 << undef));           /* 미정의 하위 비트: 데이터시트는 "정의 안 됨" → 1 로 채워 마스킹 안 하면 틀리게 한다 */
    v = (int16_t)(v | ((1 << undef) - 1));
    S.sp[0] = (uint8_t)(v & 0xFF); S.sp[1] = (uint8_t)((v >> 8) & 0xFF);
    sp_refresh_crc();
}

static void slave_byte(uint8_t b)
{
    if (S.ncmd < 64) S.cmdlog[S.ncmd++] = b;
    switch (S.mode) {
    case M_ROMCMD: S.mode = (b == 0xCC) ? M_FUNC : M_IGNORE; break;
    case M_FUNC:
        if (b == 0x44) { S.converting = 1; S.conv_left = S.conv_slots; S.mode = M_IGNORE; }
        else if (b == 0xBE) { S.mode = M_SEND; S.sendpos = 0; S.sendbit = 0; }
        else if (b == 0x4E) { S.mode = M_RECV; S.recvpos = 2; }
        else S.mode = M_IGNORE;
        break;
    case M_RECV:
        S.sp[S.recvpos++] = b; if (S.recvpos > 4) { sp_refresh_crc(); S.mode = M_IGNORE; }
        break;
    default: break;
    }
}
static int slave_read_bit(void)
{
    if (!S.present) return 1;                                     /* 풀업 */
    if (S.converting) {
        if (S.conv_never) return 0;
        if (S.conv_left) { S.conv_left--; return 0; }
        S.converting = 0; apply_conversion(); return 1;
    }
    if (S.mode == M_SEND) {
        uint8_t byte = S.sp[S.sendpos];
        if (S.crc_flip_byte >= 0 && S.sendpos == S.crc_flip_byte - 1 + 0 && 0) {}
        if (S.crc_flip_byte > 0 && S.sendpos == S.crc_flip_byte - 1) byte ^= 0x10;
        int bit = (byte >> S.sendbit) & 1;
        if (++S.sendbit == 8) { S.sendbit = 0; if (++S.sendpos == 9) S.mode = M_IGNORE; }
        return bit;
    }
    return 1;
}
static void slave_write_bit(int bit)
{
    if (!S.present) return;
    S.byte_acc |= bit << S.bitn;
    if (++S.bitn == 8) { slave_byte((uint8_t)S.byte_acc); S.bitn = 0; S.byte_acc = 0; }
}

uint32_t mock_ow_rd(uint32_t off)
{
    if (off != 0x04) return 0;                         /* STATUS: 리터럴(rtl/README.md): [0] busy [1] done [2] presence [3] rbit */
    S.stat_reads++;
    if (S.dead || (S.die_after >= 0 && S.nops >= (uint32_t)S.die_after)) return S.dead_mode == 1 ? 0x3 : S.dead_mode == 2 ? 0x0 : 0x1;   /* 0: busy 만, 1: busy+done, 2: 아무것도 없음 */
    if (S.busy_left) { S.busy_left--; return 0x1; }
    return S.stat;
}
void mock_ow_wr(uint32_t off, uint32_t v)
{
    if (off != 0x00) return;                           /* CMD 리터럴: [1:0] 0=리셋 1=쓰기 2=읽기, [2] 쓸 비트 */
    if (S.busy_left) S.violations++;                   /* busy 중에 명령을 쓰면 안 된다 */
    S.nops++;
    uint32_t op = v & 3, bit = (v >> 2) & 1;
    S.busy_left = S.cmd_busy_polls;
    S.stat = 0x2;
    if (op == 0) {
        S.resets++;
        if (S.present) S.stat |= 0x4;
        S.mode = M_ROMCMD; S.bitn = 0; S.byte_acc = 0; S.converting = S.converting;   /* 변환은 리셋으로 끊기지 않게 둔다 */
    } else if (op == 1) { S.wr_bits++; slave_write_bit((int)bit); }
    else if (op == 2) { S.rd_bits++; if (slave_read_bit()) S.stat |= 0x8; }
}

static void fresh(void) { dev_reset_dev(); S.crc_flip_byte = -1; S.die_after = -1; }
#define BASE ((uintptr_t)0x44A00000u)

static void t_crc(void)
{
    uint8_t z[8] = {0}; CHECK(ds18b20_crc8(z, 8) == 0); CHECK(ds18b20_crc8(z, 0) == 0);
    /* 데이터시트(요약): 데이터 뒤에 CRC 를 붙여 다시 돌리면 0 */
    srand(7);
    for (int t = 0; t < 2000; t++) {
        int n = 1 + rand() % 12; uint8_t d[16];
        for (int i = 0; i < n; i++) d[i] = (uint8_t)rand();
        uint8_t c = ds18b20_crc8(d, (unsigned)n);
        CHECK(c == crc_longdiv(d, n));
        CHECK(c == crc_ref(d, n));
        d[n] = c; CHECK(ds18b20_crc8(d, (unsigned)n + 1) == 0);
        int bi = rand() % (n * 8); d[bi / 8] ^= (uint8_t)(1u << (bi % 8)); d[n] = c;       /* 한 비트 오류는 반드시 잡힌다 */
        CHECK(ds18b20_crc8(d, (unsigned)n + 1) != 0);
    }
    /* Maxim AN27 의 예(기억에서 옮김, 문서는 열지 못함 → 미확인): 02 1C B8 01 00 00 00 → A2 */
    uint8_t rom[7] = {0x02, 0x1C, 0xB8, 0x01, 0, 0, 0}; CHECK(ds18b20_crc8(rom, 7) == 0xA2);
    /* 전원 투입 스크래치패드(50 05 4B 46 7F FF 0C 10)에 대한 CRC 가 모델과 같은지는 아래 읽기 시험이 확인 */
}

static void t_raw(void)
{
    CHECK(ds18b20_raw_to_uc(0x0550, 12) == 85000000);          /* +85 °C 전원 투입 값 */
    CHECK(ds18b20_raw_to_uc(0x07D0, 12) == 125000000);
    CHECK(ds18b20_raw_to_uc(0x0191, 12) == 25062500);
    CHECK(ds18b20_raw_to_uc(0x00A2, 12) == 10125000);
    CHECK(ds18b20_raw_to_uc(0x0008, 12) == 500000);
    CHECK(ds18b20_raw_to_uc(0x0001, 12) == 62500);
    CHECK(ds18b20_raw_to_uc(0x0000, 12) == 0);
    CHECK(ds18b20_raw_to_uc((int16_t)0xFF5E, 12) == -10125000);
    CHECK(ds18b20_raw_to_uc((int16_t)0xFC90, 12) == -55000000);
    CHECK(ds18b20_raw_to_uc((int16_t)0xFFFF, 12) == -62500);
    /* 하위 비트 미정의: 9비트(하위 3), 10(2), 11(1) */
    CHECK(ds18b20_raw_to_uc(0x0197, 9)  == 25000000);          /* 0x0191 | 하위 비트 쓰레기 0x6 → 0x190 = 25.0 */
    CHECK(ds18b20_raw_to_uc(0x0193, 10) == 25000000 + 0);      /* 하위 2비트 버림: 0x190 */
    CHECK(ds18b20_raw_to_uc(0x0191, 11) == 25000000);          /* 하위 1비트 버림: 0x190 → 25.0 */
    CHECK(ds18b20_raw_to_uc((int16_t)0xFFFF, 9) == -500000 * 0 + ds18b20_raw_to_uc((int16_t)0xFFF8, 12)); /* 음수는 내림(-0.5) */
    CHECK(ds18b20_raw_to_uc((int16_t)0xFFFF, 9) == -500000);
    CHECK(ds18b20_raw_to_uc((int16_t)0xFFFF, 10) == -250000);
    CHECK(ds18b20_raw_to_uc((int16_t)0xFFFF, 11) == -125000);
    CHECK(ds18b20_raw_to_uc(0x0191, 8) == 25062500);           /* 범위 밖 비트 수는 12 로 취급 */
    CHECK(ds18b20_raw_to_uc(0x0191, 13) == 25062500);
    CHECK(ds18b20_resolution_bits(0x1F) == 9 && ds18b20_resolution_bits(0x3F) == 10 && ds18b20_resolution_bits(0x5F) == 11 && ds18b20_resolution_bits(0x7F) == 12);
    CHECK(ds18b20_resolution_bits(0x00) == 9 && ds18b20_resolution_bits(0x80 | 0x1F) == 9);   /* 비트 7·4:0 은 무시 */
    CHECK(ds18b20_resolution_bits(0x60) == 12);
}

static void t_read(void)
{
    fresh(); S.true_raw = 0x0191;
    int32_t uc = 12345; int r = ds18b20_read_uc(BASE, 1000, &uc);
    CHECK(r == DS_OK && uc == 25062500);
    CHECK(S.ncmd == 4 && S.cmdlog[0] == 0xCC && S.cmdlog[1] == 0x44 && S.cmdlog[2] == 0xCC && S.cmdlog[3] == 0xBE);   /* 변환 → 읽기 순서 */
    CHECK(S.resets == 2);
    CHECK(S.rd_bits == 31 + 72);                                /* 변환 폴링 31 회(0 이 30 번 뒤 1) + 스크래치패드 72 비트 */
    /* 변환을 기다리지 않으면 전원 투입 값 85 °C 가 읽힌다 → 대기가 있어야 25.06 */
    CHECK(uc != 85000000);
    /* 음수 온도 */
    fresh(); S.true_raw = (int16_t)0xFF5E; CHECK(ds18b20_read_uc(BASE, 1000, &uc) == DS_OK && uc == -10125000);
    fresh(); S.true_raw = (int16_t)0xFC90; CHECK(ds18b20_read_uc(BASE, 1000, &uc) == DS_OK && uc == -55000000);
    fresh(); S.true_raw = 0x07D0;          CHECK(ds18b20_read_uc(BASE, 1000, &uc) == DS_OK && uc == 125000000);
    /* 변환 대기 경계: 0 이 conv_slots 번 나오고 그다음 읽기가 1 → 폴링 횟수 conv_slots+1 */
    fresh(); S.true_raw = 0x0191; S.conv_slots = 7;
    CHECK(ds18b20_read_uc(BASE, 8, &uc) == DS_OK);
    fresh(); S.true_raw = 0x0191; S.conv_slots = 7; uc = 99;
    CHECK(ds18b20_read_uc(BASE, 7, &uc) == DS_ERR_TIMEOUT && uc == 99);
    fresh(); S.true_raw = 0x0191; S.conv_slots = 0; CHECK(ds18b20_read_uc(BASE, 1, &uc) == DS_OK && uc == 25062500);
    fresh(); S.true_raw = 0x0191; S.conv_slots = 0; uc = 99; CHECK(ds18b20_read_uc(BASE, 0, &uc) == DS_ERR_TIMEOUT && uc == 99);
    fresh(); S.conv_never = 1; CHECK(ds18b20_convert(BASE, 5000) == DS_ERR_TIMEOUT);       /* 기생 전원처럼 상태 비트가 영영 1 이 안 됨 */
}

static void t_errors(void)
{
    int32_t uc = 5; uint8_t sp[9];
    fresh(); S.present = 0;
    CHECK(ds18b20_read_uc(BASE, 10, &uc) == DS_ERR_NODEV && uc == 5);
    CHECK(ds18b20_read_scratchpad(BASE, sp) == DS_ERR_NODEV);
    CHECK(ds18b20_convert(BASE, 10) == DS_ERR_NODEV);
    CHECK(ds18b20_set_resolution(BASE, 10, 0, 0) == DS_ERR_NODEV);
    fresh(); S.dead = 1; S.dead_mode = 0;
    CHECK(ds18b20_read_uc(BASE, 10, &uc) == DS_ERR_BUS && uc == 5);
    CHECK(ds18b20_read_scratchpad(BASE, sp) == DS_ERR_BUS);
    CHECK(ds18b20_convert(BASE, 10) == DS_ERR_BUS);
    CHECK(ds18b20_set_resolution(BASE, 10, 0, 0) == DS_ERR_BUS);
    /* CRC 오류: 스크래치패드의 각 바이트(0~8) 한 비트 뒤집기 */
    for (int k = 1; k <= 9; k++) {
        fresh(); S.true_raw = 0x0191; S.crc_flip_byte = k; memset(sp, 0xEE, 9); uc = 5;
        CHECK(ds18b20_read_uc(BASE, 100, &uc) == DS_ERR_CRC && uc == 5);
        fresh(); S.crc_flip_byte = k; CHECK(ds18b20_read_scratchpad(BASE, sp) == DS_ERR_CRC);
    }
    uint8_t keep[9]; memset(keep, 0xEE, 9); fresh(); S.crc_flip_byte = 3; memcpy(sp, keep, 9);
    CHECK(ds18b20_read_scratchpad(BASE, sp) == DS_ERR_CRC && memcmp(sp, keep, 9) == 0);      /* 실패 시 출력 안 건드림 */
    CHECK(ds18b20_read_uc(BASE, 10, NULL) == DS_ERR_ARGS);
    CHECK(ds18b20_read_scratchpad(BASE, NULL) == DS_ERR_ARGS);
    CHECK(ds18b20_set_resolution(BASE, 8, 0, 0) == DS_ERR_ARGS && ds18b20_set_resolution(BASE, 13, 0, 0) == DS_ERR_ARGS && ds18b20_set_resolution(BASE, 0, 0, 0) == DS_ERR_ARGS);
}

static void t_bus_faults(void)
{
    uint8_t sp[9]; int32_t uc = 7;
    /* busy+done 이 함께 계속 켜진 경우: 첫 번째 대기 루프만이 잡는다. STAT 읽기 횟수는 리터럴 100000 에 맞춘다 */
    fresh(); S.dead = 1; S.dead_mode = 1;
    CHECK(ds18b20_read_scratchpad(BASE, sp) == DS_ERR_BUS);
    CHECK(S.stat_reads == 100001 && S.nops == 0);
    /* busy 는 꺼졌는데 done 이 영영 안 오는 경우: 명령은 한 번 나가고 두 번째 루프가 잡는다(busy 확인 1회 + done 100001회) */
    fresh(); S.dead = 1; S.dead_mode = 2;
    CHECK(ds18b20_read_scratchpad(BASE, sp) == DS_ERR_BUS);
    CHECK(S.stat_reads == 100002 && S.nops == 1);
    /* 버스가 도중에 죽는 경우: k 번째 명령 뒤에 응답이 사라지면 어디서든 BUS 오류가 올라와야 한다 */
    fresh(); S.true_raw = 0x0191; S.conv_slots = 3;
    CHECK(ds18b20_read_uc(BASE, 100, &uc) == DS_OK);
    uint32_t total = S.nops; CHECK(total > 100);
    for (uint32_t k = 0; k <= total + 1; k++) {
        fresh(); S.true_raw = 0x0191; S.conv_slots = 3; S.die_after = (int)k; S.dead_mode = 2; uc = 7;
        int r = ds18b20_read_uc(BASE, 100, &uc);
        if (k > total) CHECK(r == DS_OK && uc == 25062500);
        else CHECK(r == DS_ERR_BUS && uc == 7);
    }
    for (uint32_t k = 0; k < 6; k++) {                              /* 같은 방식으로 set_resolution·scratchpad 단독 */
        fresh(); S.die_after = (int)k; S.dead_mode = 2; CHECK(ds18b20_set_resolution(BASE, 10, 0, 0) == DS_ERR_BUS || k >= 1 + 8 * 5 + 0);
        fresh(); S.die_after = (int)k; S.dead_mode = 2; CHECK(ds18b20_read_scratchpad(BASE, sp) == DS_ERR_BUS);
    }
    /* 명령은 busy 가 끝난 뒤에만 쓴다 */
    fresh(); S.cmd_busy_polls = 5; S.true_raw = 0x0191;
    CHECK(ds18b20_read_uc(BASE, 100, &uc) == DS_OK && S.violations == 0);
    fresh(); S.cmd_busy_polls = 0; CHECK(ds18b20_read_uc(BASE, 100, &uc) == DS_OK && S.violations == 0);
}

static void t_consts(void)
{
    CHECK(DS_OK == 0 && DS_ERR_ARGS == -1 && DS_ERR_BUS == -2 && DS_ERR_NODEV == -3 && DS_ERR_CRC == -4 && DS_ERR_TIMEOUT == -5);
    CHECK(ds18b20_raw_to_uc(DS_POWERON_RAW, 12) == 85000000);
    CHECK(ds18b20_tconv_ms(9) == 94 && ds18b20_tconv_ms(10) == 188 && ds18b20_tconv_ms(11) == 375 && ds18b20_tconv_ms(12) == 750);
    CHECK(ds18b20_tconv_ms(8) == 750 && ds18b20_tconv_ms(13) == 750 && ds18b20_tconv_ms(0) == 750);
    CHECK(DS_CRC_POLY_REFLECTED == 0x8C && DS_CFG_BASE == 0x1F && DS_CFG_RES_SHIFT == 5);
}

static void t_scratchpad(void)
{
    uint8_t sp[9]; fresh();
    CHECK(ds18b20_read_scratchpad(BASE, sp) == DS_OK);
    static const uint8_t pw[8] = {0x50, 0x05, 0x4B, 0x46, 0x7F, 0xFF, 0x0C, 0x10};
    CHECK(memcmp(sp, pw, 8) == 0 && sp[8] == ds18b20_crc8(pw, 8));
    CHECK(S.ncmd == 2 && S.cmdlog[0] == 0xCC && S.cmdlog[1] == 0xBE && S.resets == 1 && S.rd_bits == 72);
}

static void t_resolution(void)
{
    static const uint8_t cfg[4] = {0x1F, 0x3F, 0x5F, 0x7F};
    for (unsigned bits = 9; bits <= 12; bits++) {
        fresh(); S.true_raw = 0x0197;                          /* 25.4375 °C */
        CHECK(ds18b20_set_resolution(BASE, bits, 0x4B, (int8_t)0xC9) == DS_OK);
        CHECK(S.ncmd == 5 && S.cmdlog[0] == 0xCC && S.cmdlog[1] == 0x4E && S.cmdlog[2] == 0x4B && S.cmdlog[3] == 0xC9 && S.cmdlog[4] == cfg[bits - 9]);
        CHECK(S.sp[2] == 0x4B && S.sp[3] == 0xC9 && S.sp[4] == cfg[bits - 9]);
        int32_t uc = 0; CHECK(ds18b20_read_uc(BASE, 100, &uc) == DS_OK);
        int undef = 12 - (int)bits; int32_t want = ((0x0197 >> undef) << undef) * 62500;
        CHECK(uc == want);
    }
    fresh(); S.true_raw = (int16_t)0xFF5E; ds18b20_set_resolution(BASE, 9, 0, 0);               /* -10.125 → 9비트는 -10.5 로 내림 */
    int32_t uc = 0; CHECK(ds18b20_read_uc(BASE, 100, &uc) == DS_OK && uc == -10500000);
}

static void t_property(void)
{
    srand(11);
    for (int t = 0; t < 3000; t++) {
        fresh(); unsigned bits = 9 + (unsigned)(rand() % 4);
        int16_t raw = (int16_t)((rand() % 1761) - 880);                    /* -55 ~ +125 °C 의 1/16 단위: -880..2000 */
        raw = (int16_t)(rand() % 2 ? raw : (rand() % 2001));
        S.true_raw = raw; S.conv_slots = (uint32_t)(rand() % 40);
        CHECK(ds18b20_set_resolution(BASE, bits, 0, 0) == DS_OK);
        int32_t uc; CHECK(ds18b20_read_uc(BASE, 100, &uc) == DS_OK);
        int undef = 12 - (int)bits;
        double want = floor((double)raw / (1 << undef)) * (1 << undef) / 16.0;
        CHECK(fabs(uc / 1e6 - want) < 1e-9);
    }
}

int main(void)
{
    t_crc(); t_raw(); t_read(); t_errors(); t_bus_faults(); t_consts(); t_scratchpad(); t_resolution(); t_property();
    printf("ds18b20: %d 검사, 실패 %d\n", checks, fails);
    if (fails) return 1;
    printf("ALL PASS\n");
    return 0;
}
