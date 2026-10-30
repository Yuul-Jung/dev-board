/* Pmod BT2 — RN-42 (클래식 블루투스 2.1+EDR, SPP) 명령 층.
 *
 * 프로토콜 요약 (근거: SparkFun RN-42 Quick Start Guide PDF, D1 요약 경유):
 *   명령 모드 진입: "$$$" (CR/LF 없음) → "CMD\r\n" 응답.
 *                  전원 투입 후 60초(기본) 이내에 보내야 함 (미확인: 정확한 시간).
 *   명령 형식: "<명령>[,<인수>]\r\n" → "AOK\r\n" (성공) / "ERR\r\n" (오류) / "?\r\n" (미인식).
 *   명령 모드 종료: "---\r\n" → "END\r\n".
 *   기본 보레이트: 115200 baud.
 *   명령 대소문자: 구별 없음(설계안: 대문자 사용).
 *
 * 한계:
 *   - Microchip 공식 User Guide 원문 미열람 (D3 이하).
 *   - ERR vs "?" 응답 차이, 명령 간 최소 간격 미확인.
 *   - 명령 후 보레이트 변경 시 다음 명령까지의 대기 시간 미확인.
 *   - IO 함수(rn42_enter_cmd 등)는 bt_io_t.quiet_ms 값 요구사항 미확인.
 */
#ifndef RN42_H
#define RN42_H
#include "btmod.h"

#define RN42_NAME_MAX    20u   /* SN 명령 이름 최대 글자 수 (D1) */
#define RN42_FNAME_MAX   15u   /* S- 친근 이름 최대 글자 수 (D1; 뒤에 MAC 2바이트 자동 추가) */
#define RN42_BAUD_DEFAULT 115200u

/* ── 보레이트 코드 ─────────────────────────────────────────────────────────
 * "SU,XX" 에서 XX = 보레이트 10진 표현의 앞 두 자리 (D1 요약 경유).
 * 지원 보레이트: 1200~921600. rn42_baud_code(baud) 로 얻는다.              */

/* ── 명령 만들기 ──────────────────────────────────────────────────────────
 * 반환값: 쓴 바이트 수(NUL 제외) 또는 음수 오류(BT_ERR_NOSPACE / BT_ERR_VALUE). */
int rn42_cmd_enter(char *o, size_t cap);                    /* "$$$" — CR/LF 없음 */
int rn42_cmd_exit(char *o, size_t cap);                     /* "---\r\n" */
int rn42_cmd_at(char *o, size_t cap);                       /* "AT\r\n" (핑) */
int rn42_cmd_name(char *o, size_t cap, const char *name);   /* "SN,<name>\r\n" (최대 20자) */
int rn42_cmd_fname(char *o, size_t cap, const char *name);  /* "S-,<name>\r\n" (최대 15자; MAC 2바이트 자동 추가) */
int rn42_cmd_baud(char *o, size_t cap, unsigned baud);      /* "SU,<code>\r\n" */
int rn42_cmd_factory(char *o, size_t cap);                  /* "SF,1\r\n" (공장 초기화) */
int rn42_cmd_reboot(char *o, size_t cap);                   /* "R,1\r\n" (재시동) */

/* 보레이트 → 2자리 문자열 코드 반환 (내부 버퍼 포인터, 호출자 복사 불필요).
 * 지원하지 않는 보레이트는 NULL 반환. */
const char *rn42_baud_code(unsigned baud);

/* ── 응답 확인 ────────────────────────────────────────────────────────────
 * 반환값: 0 = 일치, BT_ERR_REPLY = 불일치. */
int rn42_reply_ok(const char *r, size_t n);    /* "AOK" 포함 여부 */
int rn42_reply_cmd(const char *r, size_t n);   /* "CMD" 포함 여부 (명령 모드 진입 확인) */
int rn42_reply_end(const char *r, size_t n);   /* "END" 포함 여부 */

/* ── IO 고수준 (bt_io_t 필요) ─────────────────────────────────────────────
 * rn42_enter_cmd: "$$$" 전송 후 "CMD" 응답 확인.
 * rn42_reboot: "R,1\r\n" 전송. 재시동 후 응답이 없을 수 있어 BT_ERR_TIMEOUT 도 BT_OK 로 처리.
 * 나머지는 명령 전송 후 "AOK" 확인.                                         */
int rn42_enter_cmd(const bt_io_t *io);
int rn42_exit_cmd(const bt_io_t *io);
int rn42_probe(const bt_io_t *io);
int rn42_set_name(const bt_io_t *io, const char *name);
int rn42_set_fname(const bt_io_t *io, const char *name);
int rn42_set_baud(const bt_io_t *io, unsigned baud);
int rn42_factory_reset(const bt_io_t *io);
int rn42_reboot(const bt_io_t *io);

#endif /* RN42_H */
