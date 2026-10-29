/* 블루투스 UART 모듈(HC-06, HM-10 계열) 공통 송수신. 이 모듈들의 AT 명령은 줄 끝 문자 없이 보내고 응답도 줄 끝이 없다 → 응답의 끝은 "무음 구간"으로 판단한다.
 * 근거(등급): HC-06: 명령은 대문자·줄 끝 없음, 응답 "OK" 등 — Martyn Currey 의 HC-06 글 2건(같은 저자, D3). HM-10: "without any other symbol (e.g. \r or \n)" — HM-10 데이터시트 Itead 사본 + DSD TECH V545 (D1, 요약 경유).
 * 무음 구간(quiet_ms)을 얼마로 둬야 하는지는 문서에 없다 → 호출자가 정함(미확인). 응답이 두 조각으로 나뉘어 quiet_ms 보다 길게 쉬면 앞 조각에서 끊긴다. */
#ifndef BTMOD_H
#define BTMOD_H
#include <stddef.h>
#include <stdint.h>
#include "uartlite.h"

#define BT_OK            0
#define BT_ERR_ARGS    (-140)
#define BT_ERR_TIMEOUT (-141)   /* first_ms 안에 한 바이트도 안 옴 */
#define BT_ERR_NOSPACE (-142)   /* 응답 버퍼 또는 명령 버퍼가 작음(넘친 응답 바이트는 버림) */
#define BT_ERR_UART    (-143)   /* UART 오류 비트(오버런·프레임·패리티): 보드레이트 불일치 의심. 받은 데이터는 resp 에 있음 */
#define BT_ERR_REPLY   (-144)   /* 응답이 기대와 다름 */
#define BT_ERR_VALUE   (-145)   /* 명령에 넣을 값이 허용 범위 밖 */
#define BT_FLUSH_MAX   512u     /* 명령을 보내기 전에 버리는 묵은 수신 바이트의 상한 */

typedef struct {
    uartlite_t *u;
    void (*delay_us)(void *ctx, uint32_t us);   /* 필수: 1000µs 단위로 부름 */
    void *ctx;
    uint32_t first_ms;    /* 첫 바이트를 기다리는 시간 */
    uint32_t quiet_ms;    /* 마지막 바이트 뒤로 이만큼 조용하면 응답이 끝난 것 */
} bt_io_t;

/* 수신 FIFO 의 묵은 바이트를 버리고 cmd 를 보낸 뒤 응답을 resp 에 모은다(NUL 로 끝남). *rlen = 받은 바이트 수. 0 이면 응답 있음 */
int bt_xfer(const bt_io_t *io, const char *cmd, size_t cmdlen, char *resp, size_t cap, size_t *rlen);
#endif
