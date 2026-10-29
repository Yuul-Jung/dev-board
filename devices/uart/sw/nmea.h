/* NMEA 줄 조립 + RMC 파서.
 * 근거(NMEA_Format_v0.1, NavSpark): 형식 $aaccc,c-c*hh<CR><LF>, 체크섬 = 8비트 XOR, 2자리 16진.
 * 미확인: XOR 범위에 '$','*' 포함 여부는 해당 문서가 명시하지 않음 → 일반 관례(둘 사이 문자)를 설계안으로 사용.
 * RMC 필드 순서: time,status(A/V),lat,N/S,lon,E/W,speed(knots),course,date(ddmmyy),(2칸 공란),mode.
 * GGA 필드 순서는 이번에 문서로 확인하지 못해 파서를 만들지 않았다. */
#ifndef NMEA_H
#define NMEA_H
#include <stdint.h>

#define NMEA_MAX 96

typedef struct { char buf[NMEA_MAX]; unsigned n; int in; } nmea_asm_t;
typedef struct {
    int valid;                 /* status == 'A' */
    int hh, mm, ss;
    int day, mon, year2;       /* ddmmyy */
    int32_t lat_e7, lon_e7;    /* 1e-7 도, 남/서는 음수 */
    int32_t speed_cent_kn;     /* 0.01 노트 */
} nmea_rmc_t;

void nmea_asm_init(nmea_asm_t *a);
/* 바이트 1개 투입. 한 줄 완성 시 1(a->buf 에 '$' 와 '\r\n' 제외 본문+*hh), 아니면 0, 넘침 -1 */
int  nmea_feed(nmea_asm_t *a, char c);
/* line: '$' 다음부터 '*hh' 까지. 체크섬 일치 1, 불일치/형식오류 0 */
int  nmea_check(const char *line);
/* RMC 이면 1(필드 채움), 다른 문장이면 0, 형식오류 -1 */
int  nmea_parse_rmc(const char *line, nmea_rmc_t *r);
#endif
