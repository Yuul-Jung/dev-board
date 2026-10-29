/* HC-06(ZS-040 등, 클래식 블루투스 SPP 슬레이브) AT 명령. 명령은 대문자, 줄 끝 없음.
 * 근거(등급): Martyn Currey 의 HC-06 글 2건(https://martyncurrey.com/?p=12932, ?p=1571 — 같은 저자라 서로 독립적이지 않음, 일부는 저자의 실험, D3, WebFetch 요약 경유):
 *   AT → OK · AT+VERSION → OKlinvorV1.8 (또는 hc01.comV2.0) · AT+NAMEx → OKsetname(이름 최대 20자, 새 이름은 전원을 다시 넣어야 보일 수 있음) · AT+PINxxxx(숫자 4자리, 기본 1234) → OKsetPIN ·
 *   AT+BAUDx(x = 16진수 한 자리 1~C: 1200 2400 4800 9600 19200 38400 57600 115200 230400 460800 921600 1382400, 기본 4 = 9600) → OK<보레이트> · AT+PN/PE/PO(패리티, 응답 미상) ·
 *   AT+LED0/1 · AT+ROLE=S/M (hc01.comV2.0 만, 응답 미상) · 모듈은 전원 투입 시 AT 모드이고, 연결되면 전송 모드가 되어 연결이 끊길 때까지 AT 를 받지 않음 · 보레이트·PIN 은 읽어 볼 수 없음 ·
 *   VCC 3.6~6 V(보드 레귤레이터 전제), RX 는 3.3 V 만(저자 견해), TX 는 3.3 V 라 FPGA 에 직결 가능(저자 견해).
 * 미확인: 제조사 데이터시트(열지 못함), 펌웨어별 응답 차이(한 독자가 일부 모듈에서 AT+BAUD 가 안 되고 AT+UART 가 된다고 보고 — 이 파일은 AT+UART 를 지원하지 않음), 명령 사이 시간 간격 요구(문서에 없음),
 *   보레이트 변경 직후 응답이 새 보레이트로 오는지 옛 보레이트로 오는지, 응답 대기 시간. AXI UART Lite 의 보레이트는 합성 시에 고정이므로 모듈 보레이트를 바꾸면 FPGA 설계도 다시 합성해야 한다. */
#ifndef HC06_H
#define HC06_H
#include "btmod.h"

#define HC06_NAME_MAX      20u
#define HC06_PIN_LEN        4u
#define HC06_BAUD_DEFAULT   9600u
#define HC06_BAUD_1 1200u
#define HC06_BAUD_2 2400u
#define HC06_BAUD_3 4800u
#define HC06_BAUD_4 9600u
#define HC06_BAUD_5 19200u
#define HC06_BAUD_6 38400u
#define HC06_BAUD_7 57600u
#define HC06_BAUD_8 115200u
#define HC06_BAUD_9 230400u
#define HC06_BAUD_A 460800u
#define HC06_BAUD_B 921600u
#define HC06_BAUD_C 1382400u

/* 명령 만들기: 쓴 바이트 수(NUL 제외), 버퍼가 작으면 BT_ERR_NOSPACE, 값이 규칙에 안 맞으면 BT_ERR_VALUE. 이름은 0x21~0x7E 글자 1~20자(설계안: 공백 금지), PIN 은 숫자 4자리 */
int hc06_cmd_at(char *o, size_t cap);
int hc06_cmd_version(char *o, size_t cap);
int hc06_cmd_name(char *o, size_t cap, const char *name);
int hc06_cmd_pin(char *o, size_t cap, const char *pin);
int hc06_cmd_baud(char *o, size_t cap, unsigned baud);               /* 표에 없는 보레이트는 BT_ERR_VALUE */
int hc06_cmd_parity(char *o, size_t cap, char kind);                 /* 'N' 'E' 'O' → AT+PN / AT+PE / AT+PO */
int hc06_cmd_led(char *o, size_t cap, int on);                       /* hc01.comV2.0 만 */
int hc06_cmd_role(char *o, size_t cap, char role);                   /* 'S' 'M' → AT+ROLE=S / AT+ROLE=M, hc01.comV2.0 만 */
int hc06_baud_code(unsigned baud);                                   /* '1'..'9','A'..'C' 또는 -1 */

/* 응답 확인: 0 이면 기대와 같음, 아니면 BT_ERR_REPLY (정확히 일치해야 함) */
int hc06_reply_ok(const char *r, size_t n);                          /* "OK" */
int hc06_reply_version(const char *r, size_t n, char *out, size_t cap);   /* "OK" + 비어 있지 않은 문자열 → out 에 복사 */
int hc06_reply_name(const char *r, size_t n);                        /* "OKsetname" */
int hc06_reply_pin(const char *r, size_t n);                         /* "OKsetPIN" */
int hc06_reply_baud(const char *r, size_t n, unsigned baud);         /* "OK" + 10진수 보레이트 */

/* 명령 + 거래 + 응답 확인 */
int hc06_probe(const bt_io_t *io);
int hc06_get_version(const bt_io_t *io, char *out, size_t cap);
int hc06_set_name(const bt_io_t *io, const char *name);
int hc06_set_pin(const bt_io_t *io, const char *pin);
int hc06_set_baud(const bt_io_t *io, unsigned baud);
#endif
