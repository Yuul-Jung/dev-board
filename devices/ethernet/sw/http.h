/* HTTP/1.1 GET 클라이언트(응답은 스트리밍 해석). 요청 만들기·응답 해석은 입출력 없는 순수 함수(http.c), W5500 TCP 위의 http_get 은 http_client.c.
 * 근거(등급): RFC 9112 (datatracker 본문의 일부 — WebFetch 요약 경유, D1(요약 경유); 요약이 앞 100000자만 읽었음):
 *   요청줄 `method SP request-target SP HTTP-version`, 상태줄 `HTTP-version SP status-code SP [reason-phrase]`, HTTP/1.1 요청에는 Host 필수,
 *   줄 끝은 CRLF(수신자는 LF 만 와도 받아들일 수 있음), 빈 줄이 헤더의 끝, 헤더 이름은 대소문자 무시·값 앞뒤 공백(OWS)은 값이 아님,
 *   본문 길이: HEAD·1xx·204·304 응답은 본문 없음 / chunked 이면 청크를 복호 / Content-Length 의 십진수 / 둘 다 없으면 서버가 연결을 닫을 때까지,
 *   `close` 옵션 = 보낸 쪽이 응답이 끝나면 연결을 닫음, 청크 크기는 16진수 1자리 이상, 마지막 청크는 크기 0.
 * 설계안(문서로 확인 못 함): Transfer-Encoding 과 Content-Length 가 같이 오면 chunked 우선, Content-Length 가 서로 다르면 오류, 줄 이어쓰기(obs-fold)·CR 단독은 오류,
 *   1xx 는 건너뛰고 다음 응답을 기다림, 마지막 코딩이 chunked 가 아닌 Transfer-Encoding 은 "닫을 때까지"로 취급, 청크 확장·트레일러는 무시, 헤더 한 줄은 95자까지만 보관(중요한 두 헤더가 잘리면 오류), 상태줄의 사유 문구가 없고 공백도 없는 "HTTP/1.1 200" 은 받아들임, 청크 크기는 16진수 8자리까지, 시작 앞의 빈 줄은 무시.
 * 하지 않는 것: HTTPS, 리다이렉트, Content-Encoding(gzip) 복호, 연결 유지, 요청 본문, DNS(IP 로 직접 접속). */
#ifndef HTTP_H
#define HTTP_H
#include <stdint.h>
#include <stddef.h>
#include "w5500.h"

#define HTTP_ERR_STATUS  (-90)   /* 상태줄 형식 오류 */
#define HTTP_ERR_HEADER  (-91)   /* 헤더 형식 오류(콜론 없음, 이어쓰기, CR 단독, 너무 긴 중요 헤더 …) */
#define HTTP_ERR_LENGTH  (-92)   /* Content-Length 가 숫자가 아니거나 넘침, 서로 다른 값이 둘 이상 */
#define HTTP_ERR_CHUNK   (-93)   /* 청크 형식 오류 */
#define HTTP_ERR_TRUNC   (-94)   /* 본문이 다 오기 전에 연결이 끝남 */
#define HTTP_ERR_SINK    (-95)   /* 본문 받는 쪽(콜백)이 중단시킴 */
#define HTTP_ERR_NOSPACE (-96)   /* 요청 버퍼가 작음 */
#define HTTP_ERR_TIMEOUT (-97)
#define HTTP_ERR_ARG     (-98)   /* host/path 에 쓸 수 없는 문자(공백·제어문자)가 있거나 비어 있음 */

#define HTTP_LINE_MAX 96u

typedef int (*http_sink_fn)(void *ctx, const uint8_t *d, unsigned n);   /* 본문 조각. 0 이 아닌 값을 돌려주면 중단 */

typedef struct {
    uint8_t st, cr, trunc, has_len, chunked, saw_te;
    uint16_t status;
    uint32_t content_length, remain, chunk_left, body_bytes;
    uint8_t ll; char line[HTTP_LINE_MAX];
    http_sink_fn sink; void *ctx;
    int err;
} http_parser_t;

int http_build_get(char *buf, size_t cap, const char *host, const char *path);   /* 길이 또는 음수 오류. NUL 로 끝맺음(cap 에 NUL 자리 포함) */
void http_parser_init(http_parser_t *p, http_sink_fn sink, void *ctx);
int http_parser_feed(http_parser_t *p, const uint8_t *d, unsigned n);   /* 0: 계속, 1: 응답 완료(남은 바이트는 버림), 음수: 오류(이후 같은 값 반복) */
int http_parser_eof(http_parser_t *p);                                  /* 연결이 닫혔음을 알림. 0: 정상 완료, HTTP_ERR_TRUNC 등 */

/* W5500 소켓 sock 을 TCP 로 열어(로컬 포트 local_port) ip:port 에 접속, GET 을 보내고 본문을 sink 로 넘긴다. 끝나면 소켓을 닫는다.
 * timeout_us 는 접속·송신·수신 각 대기의 제한이고 수신 쪽은 "데이터가 안 오고 지나간 시간"만 센다(데이터가 오면 0 부터 다시) — 전체 소요 시간 제한이 아님. 성공하면 0 이고 *status 에 상태 코드(상태 코드를 받은 뒤 오류가 나도 채워 둠, 못 받았으면 0). 2xx 가 아니어도 0 이다(호출자가 판단). */
int http_get(w5500_t *w, unsigned sock, uint16_t local_port, const uint8_t ip[4], uint16_t port, const char *host, const char *path, uint32_t timeout_us, http_sink_fn sink, void *ctx, int *status);
#endif
