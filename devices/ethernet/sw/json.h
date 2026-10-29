/* JSON 해석기(RFC 8259), 할당 없음·재귀 깊이 JSON_MAX_DEPTH. 입력은 메모리에 통째로 있는 텍스트(예: http_get 의 본문을 모은 버퍼).
 * 근거(등급): RFC 8259 (rfc-editor.org, WebFetch 요약 경유 — D1(요약 경유)):
 *   공백은 space·tab·LF·CR 네 가지, 구조 문자 6개 [ ] { } : , · 리터럴 true/false/null 은 소문자만(3절) · 숫자 = [-] int [frac] [exp], int 는 0 또는 1~9 로 시작하는 숫자열(앞의 0 금지),
 *   frac 은 . 뒤 1자리 이상, exp 는 e/E + 선택 부호 + 1자리 이상(6절) · 문자열: 따옴표·역슬래시·U+0000~U+001F 는 이스케이프해야 함, 이스케이프는 \" \\ \/ \b \f \n \r \t \uXXXX(16진수 대소문자 모두),
 *   BMP 밖 문자는 서러게이트 쌍 12글자(7절) · 객체 이름은 유일해야 "한다(SHOULD)", 아니면 동작이 예측 불가(4절) · 최상위는 아무 값이나 가능(2절) · 텍스트는 UTF-8(8.1절), 파서는 BOM 을 무시해도 됨 ·
 *   파서는 문법에 맞는 모든 텍스트를 받아야 하고(MUST) 크기·깊이·숫자 범위·문자열 길이를 제한해도 됨(9절).
 * 설계안(문서로 확인하지 못한 선택): 깊이 16 초과 거부 / 앞의 UTF-8 BOM(EF BB BF) 한 번은 건너뜀 / 문자열 안의 0x80 이상 바이트는 UTF-8 검증 없이 통과 / 이름이 겹치면 먼저 나온 것 / 짝 없는 서러게이트(\uD800 단독 등)는 문법상 유효하나
 *   json_str 이 JSON_ERR_UTF16 으로 거부 / 숫자는 정수 또는 소수점 고정 소수로만 읽음(부동소수점 없음) / 소수 자릿수가 넘치면 0 쪽으로 버림 / 경로 문법 a.b[2].c (키에 . [ ] 를 쓸 수 없음).
 * 하지 않는 것: 문서 만들기(쓰기), 스트리밍 해석(조각으로 나눠 받으며 해석), 부동소수점, 키 이름에 . [ ] 가 있는 경우, UTF-8 유효성 검사, 중복 이름 검출. */
#ifndef JSON_H
#define JSON_H
#include <stdint.h>

#define JSON_MAX_DEPTH 16u            /* 배열·객체가 겹칠 수 있는 최대 단계 수 (설계안) */
#define JSON_CTRL_MAX  0x1Fu          /* 문자열 안에서 이스케이프해야 하는 제어 문자의 끝: U+0000~U+001F (RFC 8259 7절) */
#define JSON_HEX_DIGITS 4u            /* \uXXXX 의 16진수 자리 수 (7절) */
#define JSON_ESC_U_LEN 6u             /* \uXXXX 한 개의 글자 수. BMP 밖 문자는 이것 둘 = 12글자 서러게이트 쌍 (7절) */

#define JSON_ERR_SYNTAX   (-130)
#define JSON_ERR_DEPTH    (-131)
#define JSON_ERR_NOTFOUND (-132)      /* 경로의 키·인덱스가 없음 */
#define JSON_ERR_TYPE     (-133)      /* 값의 종류가 요청과 다름(객체가 아닌데 키, 정수가 아닌데 json_int 등) */
#define JSON_ERR_RANGE    (-134)      /* int32 에 안 들어감 */
#define JSON_ERR_NOSPACE  (-135)      /* 출력 버퍼가 작음 */
#define JSON_ERR_UTF16    (-136)      /* 짝 없는 서러게이트 */
#define JSON_ERR_PATH     (-137)      /* 경로 문법 오류 */
#define JSON_ERR_ARGS     (-138)      /* NULL 인자, decimals > 9, cap 0 */

enum { JSON_NULL = 1, JSON_FALSE, JSON_TRUE, JSON_NUM, JSON_STR, JSON_ARR, JSON_OBJ };

typedef struct { const char *p; unsigned len; unsigned type; } json_val_t;   /* 원문 안의 조각(문자열은 따옴표 포함) */

int json_validate(const char *s, unsigned n);                          /* 0 또는 오류. 텍스트 전체가 값 하나(앞뒤 공백 허용) */
int json_parse(const char *s, unsigned n, json_val_t *root);           /* 검증하고 최상위 값을 돌려줌 */
int json_get(const char *s, unsigned n, const char *path, json_val_t *v);   /* 전체를 검증한 뒤 경로의 값. 경로 "" = 최상위. 예: "main.temp", "weather[0].description" */
int json_int(const json_val_t *v, int32_t *out);                       /* 소수점·지수 없는 정수만 */
int json_fixed(const json_val_t *v, unsigned decimals, int32_t *out);  /* 숫자 × 10^decimals 를 0 쪽으로 버려 정수로 (decimals 0~9). 12.349, 2 → 1234 */
int json_str(const json_val_t *v, char *out, unsigned cap, unsigned *len);  /* 이스케이프를 풀어 UTF-8 + NUL, cap 은 NUL 포함 */
int json_get_int(const char *s, unsigned n, const char *path, int32_t *out);
int json_get_fixed(const char *s, unsigned n, const char *path, unsigned decimals, int32_t *out);
int json_get_str(const char *s, unsigned n, const char *path, char *out, unsigned cap, unsigned *len);
#endif
