/* 적대적 시험: 임의 바이트·변형된 RMC 문장을 NMEA 조립기/파서에 넣어 메모리 오류·행을 찾는다(ASan/UBSan 아래). */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../sw/nmea.h"
static uint32_t rs = 777;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs >> 8; }
int main(void)
{
    const char *base = "GPRMC,123519.00,A,4807.038,N,01131.000,E,022.4,084.4,230394,,,A";
    long parsed = 0, rejected = 0;
    for (long it = 0; it < 1000000; it++) {
        char s[160]; size_t n = strlen(base);
        memcpy(s, base, n + 1);
        int muts = rnd() % 4;
        for (int m = 0; m < muts; m++) {
            size_t pos = rnd() % n;
            switch (rnd() % 4) { case 0: s[pos] = (char)(rnd() & 0xFF); break; case 1: s[pos] = ','; break;
                                 case 2: s[pos] = '*'; break; default: memmove(s + pos, s + pos + 1, n - pos); n--; if (!n) n = 1, s[0] = 'X', s[1] = 0; }
        }
        uint8_t x = 0; for (char *p = s; *p; p++) x ^= (uint8_t)*p;
        char line[200]; snprintf(line, sizeof line, "%s*%02X", s, (rnd() % 3) ? x : (uint8_t)rnd());   /* 가끔 틀린 체크섬 */
        nmea_rmc_t r; int k = nmea_parse_rmc(line, &r);
        if (k == 1) {
            parsed++;
            if (r.hh > 99 || r.mm > 99 || r.ss > 99) { printf("범위 밖 시간\n"); return 1; }
            if (r.lat_e7 > 1800000000 || r.lat_e7 < -1800000000) { printf("위도 범위 밖 %d: %s\n", r.lat_e7, line); return 1; }
        } else rejected++;
    }
    /* 조립기: 임의 바이트 스트림 */
    nmea_asm_t a; nmea_asm_init(&a);
    for (long i = 0; i < 5000000; i++) { char c = (char)(rnd() & 0xFF); if (rnd() % 50 == 0) c = '$'; if (rnd() % 60 == 0) c = '\n'; (void)nmea_feed(&a, c); if (a.n >= NMEA_MAX) { printf("버퍼 넘침\n"); return 1; } }
    printf("파싱 %ld 거부 %ld\nALL PASS\n", parsed, rejected);
    return 0;
}
