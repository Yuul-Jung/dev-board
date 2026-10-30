/* Pmod BLE — RN4871 (Bluetooth Low Energy 4.2, GATT/SPP) 명령 층.
 *
 * 프로토콜 요약:
 *   명령 모드 진입: "$$$" (CR/LF 없음) → "CMD>" 프롬프트 (D3; Microchip Dev Help 열람 실패).
 *                  연결 중에는 "$$$" 가 들어오지 않음.
 *   명령 형식: "<명령>[,<인수>]\r" → "AOK\r\n" (성공) / "Err\r\n" (오류) (D3 요약 경유).
 *             ※ RN-42 의 "\r\n" 와 달리 종결자가 "\r" 만임 (D3/미확인).
 *   명령 모드 종료: "---\r" → "END\r\n" (D3/미확인).
 *   기본 보레이트: 115200 baud (D3).
 *
 * 한계:
 *   - Microchip 공식 User Guide(DS50002466) 원문 열람 실패 → D3 이하.
 *   - 명령 종결자가 "\r" 인지 "\r\n" 인지 원문 미확인.
 *   - "CMD>" 프롬프트는 줄 끝이 없을 수 있어 bt_xfer quiet_ms 설정이 중요.
 *   - RN4871 의 BLE 특성(GATT, UUID 등) 설정 명령은 이 파일에 포함하지 않음.
 *   - 보레이트 코드 형식이 RN-42 와 같은지 미확인.
 */
#ifndef RN4871_H
#define RN4871_H
#include "btmod.h"

#define RN4871_NAME_MAX   20u   /* 미확인; RN-42 와 같다고 가정 */
#define RN4871_BAUD_DEFAULT 115200u

/* ── 명령 만들기 ──────────────────────────────────────────────────────────
 * 반환값: 쓴 바이트 수(NUL 제외) 또는 음수 오류(BT_ERR_NOSPACE / BT_ERR_VALUE). */
int rn4871_cmd_enter(char *o, size_t cap);                   /* "$$$" — CR 없음 */
int rn4871_cmd_exit(char *o, size_t cap);                    /* "---\r" */
int rn4871_cmd_at(char *o, size_t cap);                      /* "+\r" (핑; 미확인: AT 대신 + 사용) */
int rn4871_cmd_name(char *o, size_t cap, const char *name);  /* "SN,<name>\r" */
int rn4871_cmd_baud(char *o, size_t cap, unsigned baud);     /* "SB,<code>\r" (SU 가 아닌 SB; 미확인) */
int rn4871_cmd_factory(char *o, size_t cap);                 /* "SF,1\r" */
int rn4871_cmd_reboot(char *o, size_t cap);                  /* "R,1\r" */

/* 보레이트 코드: RN-42 와 같은 2자리 형식을 가정(D3/미확인).
 * 지원하지 않는 보레이트는 NULL 반환. */
const char *rn4871_baud_code(unsigned baud);

/* ── 응답 확인 ────────────────────────────────────────────────────────────
 * 반환값: 0 = 일치, BT_ERR_REPLY = 불일치. */
int rn4871_reply_ok(const char *r, size_t n);    /* "AOK" 포함 여부 */
int rn4871_reply_cmd(const char *r, size_t n);   /* "CMD>" 포함 여부 */
int rn4871_reply_end(const char *r, size_t n);   /* "END" 포함 여부 */
int rn4871_reply_err(const char *r, size_t n);   /* "Err" 포함 여부(오류 확인) */

/* ── IO 고수준 (bt_io_t 필요) ─────────────────────────────────────────────
 * rn4871_enter_cmd: "$$$" 전송 후 "CMD>" 응답 확인.
 * rn4871_reboot: 재시동 후 응답이 없을 수 있어 BT_ERR_TIMEOUT 도 BT_OK 로 처리. */
int rn4871_enter_cmd(const bt_io_t *io);
int rn4871_exit_cmd(const bt_io_t *io);
int rn4871_probe(const bt_io_t *io);
int rn4871_set_name(const bt_io_t *io, const char *name);
int rn4871_set_baud(const bt_io_t *io, unsigned baud);
int rn4871_factory_reset(const bt_io_t *io);
int rn4871_reboot(const bt_io_t *io);

#endif /* RN4871_H */
