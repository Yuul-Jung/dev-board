/* HM-10 계열(CC2541) BLE UART 모듈 AT 명령. 명령은 대문자, 줄 끝 없음. 응답은 "OK" 또는 "OK+<태그>[:<값>]" (줄 끝 없음).
 * 근거(등급): HM-10 데이터시트 2건(D1, WebFetch 요약 경유 — 문서 안의 불일치가 많아 아래에 따로 적음):
 *   A = Itead 사본 DS_IM130614001 (https://shop-archive.boxtec.ch/pub/iteadstudio/DS_IM130614001_Serial_Port_BLE_Module_Master_Slave_HM-10.pdf), B = DSD TECH "HM-10 DataSheet" Last Version V545 2017-01 (Cornell 강의 사본).
 *   기본값: 이름 HMSoft(최대 12자), 9600 8N1, PIN 000000, 슬레이브(주변장치) · "Only accept AT Command from UART interface when Bluetooth device is not connected with remote device"(전송 버전) ·
 *   AT+BAUD: 0=9600 1=19200 2=38400 3=57600 4=115200 (A·B 일치), 5=4800 6=2400 7=1200 8=230400 (B 만; 7 은 "다음 전원부터 PIO0 를 누르기 전에는 AT 를 받지 않음" 경고) ·
 *   AT+ROLE 0=Peripheral 1=Central(A 는 S/M 표기) · AT+MODE 0=전송 1=PIO 수집 2=원격 제어 · AT+PASS 000000~999999 · AT+TYPE 0~3(3 은 V524 부터, V515 미만은 쓰지 말 것) · AT+POWE 0~3 ·
 *   AT+NOTI 0/1(1 이면 연결·끊김을 UART 로 OK+CONN / OK+LOST) · AT+RESET → OK+RESET · AT+RENEW → OK+RENEW · AT+SLEEP → OK+SLEEP · AT+START → OK+START · 공급 3.3 V(B: +2.5~3.3 V, 50 mA) ·
 *   BLE 서비스 0xFFE0, 특성 0xFFE1 · 모드 1·2 에서 UART 로 보내는 데이터는 한 번에 20바이트 미만 · 절전에서 깨우기: 80자 넘는 문자열 또는 키 1초 이상.
 * 문서 안의 불일치(그대로 둠): 응답 접두가 "OK+Get:" / "OK+Set:" 인데 이름은 "OK+Set<값>"(표) 대 "OK+SetName:<값>"(예), PASS 조회는 "OK+Get:"(표) 대 "OK+PIN:"(예), AT+NAME 조회에 전각 물음표(？)가 쓰임(조판 오류로 보고 ASCII ? 사용),
 *   AT+UUID 대 AT+CHAR, PWRM 의 의미 뒤바뀜, 펌웨어 V5xx 만 다룸(V6xx·V7xx 는 열지 못함 — 미확인).
 * 미확인: 제조사(JNHuaMao) 최신 데이터시트, 응답 대기 시간, 복제 모듈(펌웨어가 다름)에서의 동작, 전원 투입 후 AT 를 받기까지의 대기. Basys3 의 Pmod 는 3.3 V 라 이 모듈의 전원(3.3 V)과 맞지만 I/O 레벨은 문서에 없음. */
#ifndef HM10_H
#define HM10_H
#include "btmod.h"

#define HM10_NAME_MAX      12u
#define HM10_PASS_LEN       6u
#define HM10_BAUD_DEFAULT   9600u
#define HM10_PKT_MAX        20u     /* 모드 1·2 에서 UART 로 보낼 수 있는 한 번의 데이터(미만이라고 쓰여 있음 → 19 이하가 안전, 설계안) */
#define HM10_BAUD_0 9600u
#define HM10_BAUD_1 19200u
#define HM10_BAUD_2 38400u
#define HM10_BAUD_3 57600u
#define HM10_BAUD_4 115200u
#define HM10_BAUD_5 4800u
#define HM10_BAUD_6 2400u
#define HM10_BAUD_7 1200u
#define HM10_BAUD_8 230400u

typedef struct { char tag[16]; char value[32]; int ok_only; } hm10_reply_t;   /* "OK" → ok_only=1, "OK+Set:1" → tag "Set", value "1" */

int hm10_cmd_at(char *o, size_t cap);
int hm10_cmd_query(char *o, size_t cap, const char *what);     /* NAME ADDR VERS BAUD ROLE MODE PASS TYPE POWE NOTI 만 허용 → "AT+NAME?" */
int hm10_cmd_name(char *o, size_t cap, const char *name);      /* 0x21~0x7E 1~12자, '?' 금지(설계안: 조회 명령과 구분) */
int hm10_cmd_baud(char *o, size_t cap, unsigned baud, int v545);   /* v545=0 이면 0~4 만(9600~115200), 1 이면 4800·2400·230400 도. 1200 은 복구 불가 경고 때문에 거절 */
int hm10_cmd_role(char *o, size_t cap, unsigned role);         /* 0, 1 */
int hm10_cmd_mode(char *o, size_t cap, unsigned mode);         /* 0~2 */
int hm10_cmd_pass(char *o, size_t cap, const char *pass);      /* 숫자 6자리 */
int hm10_cmd_type(char *o, size_t cap, unsigned type);         /* 0~3 (3 은 V524+) */
int hm10_cmd_power(char *o, size_t cap, unsigned p);           /* 0~3 */
int hm10_cmd_notify(char *o, size_t cap, int on);
int hm10_cmd_simple(char *o, size_t cap, const char *what);    /* RESET RENEW SLEEP START 만 허용 → "AT+RESET" */
int hm10_baud_code(unsigned baud, int v545);                   /* 0~8 또는 -1 (1200 은 -1) */

/* 응답 해석: "OK" 또는 "OK+" 로 시작해야 함. 태그는 ':' 앞(없으면 끝까지), 값은 ':' 뒤. 0 이면 해석됨, 아니면 BT_ERR_REPLY(접두 불일치) / BT_ERR_NOSPACE(태그 15자·값 31자 초과) */
int hm10_parse(const char *r, size_t n, hm10_reply_t *o);

/* 응답에서 값을 꺼낸다: 태그가 want 로 시작할 때, ':' 가 있었으면 그 뒤 값(태그가 want 와 정확히 같아야 함), 없었으면 태그의 나머지("OK+NAMEHMSoft" → "HMSoft"). 맞지 않으면 NULL */
const char *hm10_value(const hm10_reply_t *t, const char *want);

int hm10_probe(const bt_io_t *io);                              /* AT → "OK" */
int hm10_set_name(const bt_io_t *io, const char *name);         /* 응답 태그가 Set 으로 시작하면 성공 */
int hm10_set_baud(const bt_io_t *io, unsigned baud, int v545);
int hm10_query(const bt_io_t *io, const char *what, hm10_reply_t *o);
#endif
