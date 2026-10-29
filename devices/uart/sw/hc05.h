/* HC-05 AT 명령 도우미. 명령 문자열은 2차 자료(D3) 기준, 응답 문자열 'OK' 외에는 미확인.
 * AT 모드 진입(PIO11 High 후 전원)과 AT 모드 보레이트는 미확인 → 호출자가 보드에서 확인. */
#ifndef HC05_H
#define HC05_H
#include "uartlite.h"
#include <stddef.h>
/* 명령 문자열 생성(끝에 \r\n). 반환: 길이, 버퍼 부족 시 -1 */
int hc05_cmd_at(char *o, size_t cap);
int hc05_cmd_name(char *o, size_t cap, const char *name);
int hc05_cmd_pswd(char *o, size_t cap, const char *pin);
int hc05_cmd_uart(char *o, size_t cap, unsigned baud, unsigned stop, unsigned parity);
int hc05_cmd_role(char *o, size_t cap, unsigned role);   /* 0 슬레이브, 1 마스터, 2 슬레이브 루프 */
/* 송신 후 응답 한 줄 수신. poll() 은 호출자가 시간 초과를 판단하는 콜백(1 계속, 0 중단). 'OK' 이면 0, 아니면 음수 */
int hc05_send(uartlite_t *u, const char *cmd, int (*poll)(void *), void *ctx);
#endif
