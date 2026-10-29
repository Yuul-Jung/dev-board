/* HTTP 클라이언트 시험: 요청 만들기, 응답 해석(통째로·1바이트씩·3바이트씩 넣어 같은 결과인지), W5500 모델 위의 http_get(상대 서버는 이 파일의 훅).
 * 한계: 응답은 내가 쓴 문자열이지 실제 서버 캡처가 아님. RFC 9112 는 요약 경유로만 읽음. 설계안 항목(헤더 파일 머리말)은 내 선택일 뿐 규격 확인이 아님. */
#include "../../spi/tb/w5500_model.h"
#include "../sw/http.h"

typedef struct { uint8_t b[8192]; unsigned n; int abort_at; } acc_t;
static int sink_acc(void *c, const uint8_t *d, unsigned n)
{
    acc_t *a = c;
    if (a->abort_at >= 0 && a->n + n > (unsigned)a->abort_at) return 1;
    if (a->n + n > sizeof a->b) return 1;
    memcpy(a->b + a->n, d, n); a->n += n; return 0;
}
/* 응답 문자열을 step 바이트씩 넣는다. 반환: 마지막 feed 결과. done 이 아니면 eof 도 호출해 *eof 에 */
static int run(const char *resp, unsigned len, unsigned step, acc_t *a, int *eof, http_parser_t *pp)
{
    memset(a, 0, sizeof *a); a->abort_at = -1;
    http_parser_init(pp, sink_acc, a);
    int r = 0;
    for (unsigned i = 0; i < len && r == 0; i += step) { unsigned c = len - i < step ? len - i : step; r = http_parser_feed(pp, (const uint8_t *)resp + i, c); }
    *eof = (r >= 0) ? http_parser_eof(pp) : 99;
    return r;
}
#define RUN3(resp, exp_r, exp_eof, exp_body, exp_status, msg) do { \
    static const char R_[] = resp; const unsigned steps_[3] = { sizeof R_ - 1, 1, 3 }; \
    for (int k_ = 0; k_ < 3; k_++) { acc_t a_; http_parser_t p_; int eof_; int r_ = run(R_, sizeof R_ - 1, steps_[k_], &a_, &eof_, &p_); \
        CHECK(r_ == (exp_r) && eof_ == (exp_eof) && a_.n == sizeof exp_body - 1 && memcmp(a_.b, exp_body, a_.n) == 0 && p_.status == (exp_status), msg); \
        if (!(r_ == (exp_r) && eof_ == (exp_eof))) printf("    (step %u: r=%d eof=%d n=%u status=%u)\n", steps_[k_], r_, eof_, a_.n, p_.status); } } while (0)

static void t_build(void)
{
    printf("[요청 만들기]\n");
    char b[256];
    const char *want = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n";
    int n = http_build_get(b, sizeof b, "example.com", "/index.html");
    CHECK(n == (int)strlen(want) && strcmp(b, want) == 0, "정확한 요청 문자열");
    char c[256]; CHECK(http_build_get(c, (size_t)n + 1, "example.com", "/index.html") == n && strcmp(c, want) == 0, "NUL 자리까지 딱 맞는 버퍼");
    CHECK(http_build_get(c, (size_t)n, "example.com", "/index.html") == HTTP_ERR_NOSPACE, "NUL 자리가 모자라면 NOSPACE");
    CHECK(http_build_get(c, 5, "example.com", "/index.html") == HTTP_ERR_NOSPACE, "많이 모자람");
    CHECK(http_build_get(c, sizeof c, "h", "/") == (int)strlen("GET / HTTP/1.1\r\nHost: h\r\nConnection: close\r\n\r\n"), "최소 요청");
    CHECK(http_build_get(c, sizeof c, "a.b:8080", "/x?y=1&z=%20") > 0 && strstr(c, "Host: a.b:8080\r\n") && strstr(c, "GET /x?y=1&z=%20 HTTP"), "포트·쿼리 허용");
    CHECK(http_build_get(c, sizeof c, "[::1]", "/") > 0, "대괄호 호스트");
    CHECK(http_build_get(c, sizeof c, 0, "/") == HTTP_ERR_ARG && http_build_get(c, sizeof c, "", "/") == HTTP_ERR_ARG, "빈 호스트 거절");
    CHECK(http_build_get(c, sizeof c, "h", 0) == HTTP_ERR_ARG && http_build_get(c, sizeof c, "h", "") == HTTP_ERR_ARG, "빈 경로 거절");
    CHECK(http_build_get(c, sizeof c, "h", "x") == HTTP_ERR_ARG, "슬래시로 시작하지 않는 경로 거절");
    CHECK(http_build_get(c, sizeof c, "h i", "/") == HTTP_ERR_ARG && http_build_get(c, sizeof c, "h\r\nX: 1", "/") == HTTP_ERR_ARG, "호스트의 공백·CRLF 거절(헤더 주입)");
    CHECK(http_build_get(c, sizeof c, "h/x", "/") == HTTP_ERR_ARG, "호스트의 슬래시 거절");
    CHECK(http_build_get(c, sizeof c, "h", "/a b") == HTTP_ERR_ARG && http_build_get(c, sizeof c, "h", "/a\r\nX: 1") == HTTP_ERR_ARG, "경로의 공백·CRLF 거절(요청 분할)");
    CHECK(http_build_get(c, sizeof c, "h", "/a\x7f") == HTTP_ERR_ARG && http_build_get(c, sizeof c, "h", "/a\x80") == HTTP_ERR_ARG && http_build_get(c, sizeof c, "h", "/a\x01") == HTTP_ERR_ARG, "DEL·비ASCII·제어문자 거절");
    CHECK(http_build_get(c, sizeof c, "h", "/\x21") > 0 && http_build_get(c, sizeof c, "h", "/\x7e") > 0, "0x21, 0x7E 허용");
    CHECK(http_build_get(0, 10, "h", "/") == QSPI_ERR_ARGS && http_build_get(c, 0, "h", "/") == QSPI_ERR_ARGS, "NULL·0 크기");
}

static void t_parse(void)
{
    printf("[응답 해석]\n");
    RUN3("HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, world!", 1, 0, "Hello, world!", 200, "Content-Length 본문");
    RUN3("HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n", 1, 0, "", 200, "본문 0바이트");
    RUN3("HTTP/1.1 404 Not Found\r\ncontent-LENGTH:   4  \r\nX-A: b\r\n\r\nnope", 1, 0, "nope", 404, "이름 대소문자 무시, 값 앞뒤 공백");
    RUN3("HTTP/1.0 200 OK\r\nServer: x\r\n\r\nabc", 0, 0, "abc", 200, "닫을 때까지 본문(HTTP/1.0): eof 에서 완료");
    RUN3("HTTP/1.1 200\r\nContent-Length: 1\r\n\r\nz", 1, 0, "z", 200, "사유 문구와 공백이 없는 상태줄");
    RUN3("HTTP/1.1 503 \r\nContent-Length: 1\r\n\r\nz", 1, 0, "z", 503, "사유 문구가 빈 상태줄");
    RUN3("\r\n\r\nHTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\nz", 1, 0, "z", 200, "시작 앞 빈 줄 무시");
    RUN3("HTTP/1.1 200 OK\nContent-Length: 2\n\nhi", 1, 0, "hi", 200, "LF 단독 줄바꿈 허용");
    RUN3("HTTP/1.1 200 OK\r\nContent-Length: 8\r\n\r\na\r\n\r\nb\r\n", 1, 0, "a\r\n\r\nb\r\n", 200, "본문 안의 CRLF 는 해석하지 않음");
    RUN3("HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nabcdefgh", 1, 0, "abc", 200, "길이를 넘는 바이트는 버림");
    RUN3("HTTP/1.1 200 OK\r\nContent-Length: 3\r\nContent-Length: 3\r\n\r\nabc", 1, 0, "abc", 200, "같은 Content-Length 가 둘이면 허용");
    RUN3("HTTP/1.1 204 No Content\r\nContent-Length: 5\r\n\r\n", 1, 0, "", 204, "204 는 본문 없음");
    RUN3("HTTP/1.1 304 Not Modified\r\nContent-Length: 10\r\n\r\n", 1, 0, "", 304, "304 는 본문 없음");
    RUN3("HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nhi", 1, 0, "hi", 200, "1xx 건너뛰고 다음 응답");
    RUN3("HTTP/1.1 103 Early Hints\r\nContent-Length: 99\r\n\r\nHTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nhi", 1, 0, "hi", 200, "1xx 의 Content-Length 는 다음 응답에 영향 없음");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nHello\r\n7\r\n, world\r\n0\r\n\r\n", 1, 0, "Hello, world", 200, "chunked");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\nA\r\n0123456789\r\na\r\nabcdefghij\r\n0\r\n\r\n", 1, 0, "0123456789abcdefghij", 200, "16진 대소문자 A, a");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n3;ext=1\r\nabc\r\n2 ;x\r\nde\r\n0;last\r\n\r\n", 1, 0, "abcde", 200, "청크 확장 무시");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabc\r\n0\r\nX-Trailer: 1\r\nY: 2\r\n\r\n", 1, 0, "abc", 200, "트레일러 건너뜀");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n", 1, 0, "", 200, "빈 chunked");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n7\r\n\r\n0\r\n\r\n\r\n0\r\n\r\n", 1, 0, "\r\n0\r\n\r\n", 200, "청크 데이터 안의 0 크기 모양 문자열은 데이터");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip, chunked\r\n\r\n2\r\nhi\r\n0\r\n\r\n", 1, 0, "hi", 200, "마지막 코딩이 chunked");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: Chunked\r\n\r\n2\r\nhi\r\n0\r\n\r\n", 1, 0, "hi", 200, "Chunked 대소문자");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked, gzip\r\n\r\nabc", 0, 0, "abc", 200, "마지막 코딩이 chunked 아님 → 닫을 때까지");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip\r\nContent-Length: 3\r\n\r\nabcdef", 0, 0, "abcdef", 200, "chunked 아닌 TE 와 Content-Length → 닫을 때까지(설계안)");
    RUN3("HTTP/1.1 200 OK\r\nContent-Length: 3\r\nTransfer-Encoding: gzip\r\n\r\nabcdef", 0, 0, "abcdef", 200, "(순서 바꿈) 닫을 때까지");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n1a\r\nabcdefghijklmnopqrstuvwxyz\r\n10\r\n0123456789ABCDEF\r\n0\r\n\r\n", 1, 0, "abcdefghijklmnopqrstuvwxyz0123456789ABCDEF", 200, "두 자리 청크 크기 1a(26), 10(16)");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n00000002\r\nhi\r\n0\r\n\r\n", 1, 0, "hi", 200, "8자리 청크 크기(앞의 0)");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n000000002\r\nhi\r\n0\r\n\r\n", HTTP_ERR_CHUNK, 99, "", 200, "9자리는 거절(한계)");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nContent-Length: 99\r\n\r\n2\r\nhi\r\n0\r\n\r\n", 1, 0, "hi", 200, "chunked 와 Content-Length 가 같이 오면 chunked");
    RUN3("HTTP/1.1 200 OK\r\nContent-Length: 99\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nhi\r\n0\r\n\r\n", 1, 0, "hi", 200, "(순서 바꿈) chunked 우선");
    RUN3("HTTP/1.1 200 OK\r\nX-Long: 0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\r\nContent-Length: 2\r\n\r\nhi", 1, 0, "hi", 200, "긴 무관 헤더는 잘라서 무시");

    /* 오류 */
    RUN3("FTP/1.1 200 OK\r\n\r\n", HTTP_ERR_STATUS, 99, "", 0, "버전 형식 오류");
    RUN3("HTTP/1.1 20 OK\r\n\r\n", HTTP_ERR_STATUS, 99, "", 0, "상태 코드 2자리");
    RUN3("HTTP/1.1 2000 OK\r\n\r\n", HTTP_ERR_STATUS, 99, "", 0, "상태 코드 4자리");
    RUN3("HTTP/1.1 2x0 OK\r\n\r\n", HTTP_ERR_STATUS, 99, "", 0, "상태 코드에 문자");
    RUN3("HTTP/1.1 099 X\r\n\r\n", HTTP_ERR_STATUS, 99, "", 0, "상태 코드 100 미만");
    RUN3("HTTP/1.1200 OK\r\n\r\n", HTTP_ERR_STATUS, 99, "", 0, "버전 뒤 공백 없음");
    RUN3("HTTP/1. 200 OK\r\n\r\n", HTTP_ERR_STATUS, 99, "", 0, "버전 소수부 없음");
    RUN3("HTTP/x.1 200 OK\r\n\r\n", HTTP_ERR_STATUS, 99, "", 0, "버전 메이저가 숫자 아님");
    RUN3("HTTP/1x1 200 OK\r\n\r\n", HTTP_ERR_STATUS, 99, "", 0, "버전에 점이 없음");
    RUN3("HTTP/1.x 200 OK\r\n\r\n", HTTP_ERR_STATUS, 99, "", 0, "버전 마이너가 숫자 아님");
    RUN3("HTTP/1.1x200 OK\r\n\r\n", HTTP_ERR_STATUS, 99, "", 0, "버전 뒤가 공백이 아님");
    /* 앞선 줄이 line 버퍼에 남긴 글자를 읽으면 안 됨: 1xx 헤더의 찌꺼기 뒤에 짧은 상태줄 */
    RUN3("HTTP/1.1 100 C\r\nX-Y: 123456789\r\n\r\nHTTP/1.1 20\r\n\r\n", HTTP_ERR_STATUS, 99, "", 100, "짧은 상태줄(\"HTTP/1.1 20\")은 이전 줄 찌꺼기를 읽지 않고 거절");
    RUN3("HTTP/1.1 199 X\r\n\r\nHTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\nq", 1, 0, "q", 200, "199 도 1xx");
    RUN3("HTTP/1.1 200 OK\r\nNoColonHere\r\n\r\n", HTTP_ERR_HEADER, 99, "", 200, "콜론 없는 헤더");
    RUN3("HTTP/1.1 200 OK\r\n: v\r\n\r\n", HTTP_ERR_HEADER, 99, "", 200, "이름 없는 헤더");
    RUN3("HTTP/1.1 200 OK\r\nX-A: 1\r\n  folded\r\n\r\n", HTTP_ERR_HEADER, 99, "", 200, "줄 이어쓰기 거절");
    RUN3("HTTP/1.1 200 OK\r\nX Y: 1\r\n\r\n", HTTP_ERR_HEADER, 99, "", 200, "이름 중간의 공백");
    RUN3("HTTP/1.1 200 OK\r\nContent-Length : 1\r\n\r\nx", HTTP_ERR_HEADER, 99, "", 200, "콜론 앞 공백");
    RUN3("HTTP/1.1 200 OK\r\nX: 1\rY: 2\r\n\r\n", HTTP_ERR_HEADER, 99, "", 200, "헤더 줄 안의 CR 단독");
    RUN3("HTTP/1.1 200 OK\rX: 1\r\n\r\n", HTTP_ERR_STATUS, 99, "", 0, "상태줄 안의 CR 단독");
    RUN3("HTTP/1.1 200 OK\r\nContent-Length: abc\r\n\r\n", HTTP_ERR_LENGTH, 99, "", 200, "Content-Length 숫자 아님");
    RUN3("HTTP/1.1 200 OK\r\nContent-Length: 1 2\r\n\r\n", HTTP_ERR_LENGTH, 99, "", 200, "Content-Length 에 공백 섞임");
    RUN3("HTTP/1.1 200 OK\r\nContent-Length: 5, 5\r\n\r\n", HTTP_ERR_LENGTH, 99, "", 200, "Content-Length 목록");
    RUN3("HTTP/1.1 200 OK\r\nContent-Length:\r\n\r\n", HTTP_ERR_LENGTH, 99, "", 200, "Content-Length 비어 있음");
    RUN3("HTTP/1.1 200 OK\r\nContent-Length: -1\r\n\r\n", HTTP_ERR_LENGTH, 99, "", 200, "Content-Length 음수");
    RUN3("HTTP/1.1 200 OK\r\nContent-Length: 4294967296\r\n\r\n", HTTP_ERR_LENGTH, 99, "", 200, "Content-Length 2^32 (넘침)");
    RUN3("HTTP/1.1 200 OK\r\nContent-Length: 99999999999999999999\r\n\r\n", HTTP_ERR_LENGTH, 99, "", 200, "Content-Length 아주 큼");
    RUN3("HTTP/1.1 200 OK\r\nContent-Length: 3\r\nContent-Length: 4\r\n\r\nabcd", HTTP_ERR_LENGTH, 99, "", 200, "서로 다른 Content-Length");
    RUN3("HTTP/1.1 200 OK\r\nContent-Length:                                                                                                    5\r\n\r\nhello", HTTP_ERR_HEADER, 99, "", 200, "잘린 Content-Length 줄(공백 100개) 거절");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\nG\r\n", HTTP_ERR_CHUNK, 99, "", 200, "16진수 아님");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n\r\n", HTTP_ERR_CHUNK, 99, "", 200, "크기 없음");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n123456789\r\n", HTTP_ERR_CHUNK, 99, "", 200, "크기 9자리");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n3x\r\n", HTTP_ERR_CHUNK, 99, "", 200, "크기 뒤 이상한 문자");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabcX\r\n", HTTP_ERR_CHUNK, 99, "abc", 200, "청크 데이터 뒤 CRLF 없음");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabc\r\r\n", HTTP_ERR_CHUNK, 99, "abc", 200, "청크 뒤 CR 두 개");

    /* 연결이 일찍 끝남 */
    RUN3("HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nhello", 0, HTTP_ERR_TRUNC, "hello", 200, "Content-Length 보다 적게 오고 끝남");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n", 0, HTTP_ERR_TRUNC, "hello", 200, "chunked 가 끝나기 전에 끝남");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhel", 0, HTTP_ERR_TRUNC, "hel", 200, "청크 도중에 끝남");
    RUN3("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n", 0, HTTP_ERR_TRUNC, "", 200, "트레일러 도중에 끝남");
    RUN3("HTTP/1.1 200 OK\r\nContent-Le", 0, HTTP_ERR_TRUNC, "", 200, "헤더 도중에 끝남");
    RUN3("HTTP/1.1 20", 0, HTTP_ERR_TRUNC, "", 0, "상태줄 도중에 끝남");
    RUN3("", 0, HTTP_ERR_TRUNC, "", 0, "아무것도 안 옴");
    RUN3("HTTP/1.1 100 Continue\r\n\r\n", 0, HTTP_ERR_TRUNC, "", 100, "1xx 만 오고 끝남");

    /* 콜백·API */
    {
        acc_t a; http_parser_t p; memset(&a, 0, sizeof a); a.abort_at = 3;
        http_parser_init(&p, sink_acc, &a);
        const char *r = "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n0123456789";
        CHECK(http_parser_feed(&p, (const uint8_t *)r, (unsigned)strlen(r)) == HTTP_ERR_SINK, "sink 가 중단시키면 SINK");
        CHECK(http_parser_feed(&p, (const uint8_t *)"x", 1) == HTTP_ERR_SINK && http_parser_eof(&p) == HTTP_ERR_SINK, "오류 뒤에는 같은 오류 유지");
        http_parser_init(&p, 0, 0);
        CHECK(http_parser_feed(&p, (const uint8_t *)r, (unsigned)strlen(r)) == 1 && p.body_bytes == 10, "sink 가 NULL 이어도 동작, 본문 바이트 수 집계");
        CHECK(http_parser_feed(&p, (const uint8_t *)"zz", 2) == 1 && http_parser_feed(&p, 0, 0) == 1 && http_parser_eof(&p) == 0, "완료 후 더 넣어도 완료 유지");
        CHECK(http_parser_feed(0, (const uint8_t *)"x", 1) == QSPI_ERR_ARGS && http_parser_feed(&p, 0, 1) == QSPI_ERR_ARGS && http_parser_eof(0) == QSPI_ERR_ARGS, "NULL 거절");
        http_parser_init(&p, 0, 0);
        CHECK(http_parser_feed(&p, (const uint8_t *)"HTTP/1.1 200 OK\r\n\r\n", 0) == 0, "0바이트 입력");
        CHECK(http_parser_feed(&p, 0, 0) == 0, "NULL, 0바이트 입력은 허용");
    }
}

/* ---------- W5500 위 ---------- */
static const uint8_t SRV[4] = { 198, 51, 100, 7 };
static const char *resp; static unsigned resp_len, resp_pos, piece; static int close_after, first_now, ticks_after_done;
static char last_req[300]; static int req_seen;
static int peer_sock = -1;
static void deliver_piece(void)
{
    if (resp_pos >= resp_len) return;
    unsigned c = resp_len - resp_pos < piece ? resp_len - resp_pos : piece;
    inject_rx(peer_sock, (const uint8_t *)resp + resp_pos, c); resp_pos += c;
    if (resp_pos >= resp_len && close_after) S[peer_sock][3] = close_after == 2 ? 0x00 : 0x1C;      /* 1: CLOSE_WAIT(상대가 FIN), 2: CLOSED(상대가 RST·시간 초과) */
}
static void tick(void) { if (resp) deliver_piece(); (void)ticks_after_done; }
static void thook(int sock, int proto, const uint8_t ip[4], uint16_t port, const uint8_t *d, unsigned len)
{
    if (proto != 1 || port != 80 || memcmp(ip, SRV, 4) != 0) return;
    req_seen++; peer_sock = sock;
    unsigned c = len < sizeof last_req - 1 ? len : (unsigned)sizeof last_req - 1; memcpy(last_req, d, c); last_req[c] = 0;
    if (resp) { if (first_now) deliver_piece(); tick_hook = tick; }
}
static void setup(const char *r, unsigned len, unsigned pc, int closeaft, int first)
{
    resp = r; resp_len = len; resp_pos = 0; piece = pc; close_after = closeaft; first_now = first; req_seen = 0; last_req[0] = 0; peer_sock = -1;
    peer_hook = thook; tick_hook = 0;
}
static acc_t gacc;
static void t_get(void)
{
    printf("[http_get]\n");
    CHECK(start(1) == 0, "init");
    int st = -1;
    const char R1[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 26\r\n\r\nabcdefghijklmnopqrstuvwxyz";
    memset(&gacc, 0, sizeof gacc); gacc.abort_at = -1;
    int nd_before = n_discon;
    setup(R1, sizeof R1 - 1, 20, 0, 1);
    CHECK(http_get(&W, 3, 3000, SRV, 80, "example.org", "/a/b?c=d", 200000, sink_acc, &gacc, &st) == 0, "Content-Length 응답(20바이트씩 조각, 상대는 연결 유지)");
    CHECK(st == 200 && gacc.n == 26 && memcmp(gacc.b, "abcdefghijklmnopqrstuvwxyz", 26) == 0, "상태 200, 본문 26바이트");
    CHECK(n_discon == nd_before + 1, "정상 종료는 DISCON 1회");
    CHECK(strcmp(last_req, "GET /a/b?c=d HTTP/1.1\r\nHost: example.org\r\nConnection: close\r\n\r\n") == 0, "상대가 받은 요청");
    CHECK(S[3][3] == 0x00 && W.proto[3] == 0 && rd16s(3, 4) == 3000, "끝난 뒤 소켓 닫힘, 로컬 포트 3000");
    CHECK(viol == 0, "위반 없음");

    /* chunked, 7바이트씩, 상대가 닫음 */
    const char R2[] = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nHello\r\n7\r\n, world\r\n0\r\n\r\n";
    memset(&gacc, 0, sizeof gacc); gacc.abort_at = -1; st = -1;
    setup(R2, sizeof R2 - 1, 7, 1, 1);
    CHECK(http_get(&W, 3, 3000, SRV, 80, "h", "/", 200000, sink_acc, &gacc, &st) == 0 && st == 200 && gacc.n == 12 && memcmp(gacc.b, "Hello, world", 12) == 0, "chunked, 7바이트씩");
    CHECK(S[3][3] == 0x00 && W.proto[3] == 0, "닫힘");

    /* 닫을 때까지 본문: 상대가 닫아야 끝남 */
    const char R3[] = "HTTP/1.0 200 OK\r\n\r\nthe body ends when the server closes";
    memset(&gacc, 0, sizeof gacc); gacc.abort_at = -1; st = -1;
    setup(R3, sizeof R3 - 1, 10, 1, 1);
    CHECK(http_get(&W, 3, 3000, SRV, 80, "h", "/", 200000, sink_acc, &gacc, &st) == 0 && st == 200 && gacc.n == sizeof R3 - 1 - 19 && memcmp(gacc.b, "the body ends when the server closes", gacc.n) == 0, "닫을 때까지 본문");
    /* 데이터와 FIN 이 동시에 와도 데이터를 먼저 다 읽음 */
    memset(&gacc, 0, sizeof gacc); gacc.abort_at = -1; st = -1;
    setup(R3, sizeof R3 - 1, 1000, 1, 1);
    CHECK(http_get(&W, 3, 3000, SRV, 80, "h", "/", 200000, sink_acc, &gacc, &st) == 0 && gacc.n == sizeof R3 - 1 - 19, "데이터+FIN 동시 → 데이터를 다 받고 끝");

    /* 상대가 CLOSED 로 끝냄: 이미 닫혔으므로 DISCON 을 보내지 않고 정리 */
    memset(&gacc, 0, sizeof gacc); gacc.abort_at = -1; st = -1;
    setup(R3, sizeof R3 - 1, 1000, 2, 1);
    int nd2 = n_discon;
    CHECK(http_get(&W, 3, 3000, SRV, 80, "h", "/", 200000, sink_acc, &gacc, &st) == 0 && gacc.n == sizeof R3 - 1 - 19 && n_discon == nd2 && W.proto[3] == 0, "상대가 CLOSED 로 끝냄: 본문 완료, DISCON 없음");

    /* 4xx 도 0 으로 돌려주고 상태만 알림 */
    const char R4[] = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nnot found";
    memset(&gacc, 0, sizeof gacc); gacc.abort_at = -1; st = -1;
    setup(R4, sizeof R4 - 1, 100, 0, 1);
    CHECK(http_get(&W, 3, 3000, SRV, 80, "h", "/missing", 200000, sink_acc, &gacc, &st) == 0 && st == 404 && gacc.n == 9, "404 → 반환 0, 상태 404");

    /* 큰 본문: RX 버퍼(2 KB)보다 큼 */
    {
        static char big[6000]; static acc_t bacc;
        int hl = sprintf(big, "HTTP/1.1 200 OK\r\nContent-Length: 5000\r\n\r\n");
        for (int i = 0; i < 5000; i++) big[hl + i] = (char)('A' + (i * 7) % 26);
        memset(&bacc, 0, sizeof bacc); bacc.abort_at = -1; st = -1;
        setup(big, (unsigned)hl + 5000, 500, 0, 1);
        CHECK(http_get(&W, 3, 3000, SRV, 80, "h", "/big", 500000, sink_acc, &bacc, &st) == 0 && bacc.n == 5000, "5000바이트 본문(500바이트씩)");
        int ok = 1; for (int i = 0; i < 5000; i++) if (bacc.b[i] != (uint8_t)('A' + (i * 7) % 26)) ok = 0;
        CHECK(ok, "본문 내용 일치");
    }

    /* 중간에 끊김 */
    const char R5[] = "HTTP/1.1 200 OK\r\nContent-Length: 50\r\n\r\nonly a part";
    memset(&gacc, 0, sizeof gacc); gacc.abort_at = -1; st = -1;
    setup(R5, sizeof R5 - 1, 100, 1, 1);
    CHECK(http_get(&W, 3, 3000, SRV, 80, "h", "/", 200000, sink_acc, &gacc, &st) == HTTP_ERR_TRUNC && st == 200 && gacc.n == 11, "본문이 모자란 채 상대가 닫음 → TRUNC, 상태는 알려 줌");
    CHECK(S[3][3] == 0x00 && W.proto[3] == 0, "닫힘");

    /* 응답이 없음 */
    memset(&gacc, 0, sizeof gacc); gacc.abort_at = -1; st = -1;
    setup(0, 0, 0, 0, 0);
    uint64_t t0 = now_us;
    CHECK(http_get(&W, 3, 3000, SRV, 80, "h", "/", 50000, sink_acc, &gacc, &st) == HTTP_ERR_TIMEOUT && st == 0, "응답 없음 → TIMEOUT, 상태 0");
    CHECK(now_us - t0 >= 50000 && now_us - t0 < 70000, "제한 시간 근처에서 끝남");
    CHECK(S[3][3] == 0x00 && W.proto[3] == 0, "닫힘");

    /* 응답이 한참 뒤에 시작해도 데이터가 오는 동안엔 시간이 흐르지 않음 */
    memset(&gacc, 0, sizeof gacc); gacc.abort_at = -1; st = -1;
    setup(R1, sizeof R1 - 1, 3, 0, 0);          /* 3바이트씩, 첫 조각도 첫 대기(틱) 때 */
    CHECK(http_get(&W, 3, 3000, SRV, 80, "h", "/", 30000, sink_acc, &gacc, &st) == 0 && gacc.n == 26, "3바이트씩 느리게 와도(총 수십 ms) 대기 제한은 '멈춘 시간'만 센다");

    /* 형식 오류 */
    const char R6[] = "garbage\r\n\r\n";
    memset(&gacc, 0, sizeof gacc); gacc.abort_at = -1; st = -1;
    setup(R6, sizeof R6 - 1, 100, 0, 1);
    CHECK(http_get(&W, 3, 3000, SRV, 80, "h", "/", 50000, sink_acc, &gacc, &st) == HTTP_ERR_STATUS && st == 0, "상태줄 오류");
    CHECK(S[3][3] == 0x00 && W.proto[3] == 0, "닫힘");

    /* sink 중단 */
    memset(&gacc, 0, sizeof gacc); gacc.abort_at = 5; st = -1;
    setup(R1, sizeof R1 - 1, 100, 0, 1);
    CHECK(http_get(&W, 3, 3000, SRV, 80, "h", "/", 50000, sink_acc, &gacc, &st) == HTTP_ERR_SINK && st == 200, "sink 중단 → SINK");
    CHECK(S[3][3] == 0x00 && W.proto[3] == 0, "닫힘");

    /* 접속 거부 */
    memset(&gacc, 0, sizeof gacc); gacc.abort_at = -1; st = -1;
    setup(R1, sizeof R1 - 1, 100, 0, 1);
    connect_mode = CONN_REFUSE; int cr = n_cmds, nd0 = n_discon;
    CHECK(http_get(&W, 3, 3000, SRV, 80, "h", "/", 50000, sink_acc, &gacc, &st) != 0 && st == 0 && req_seen == 0 && n_cmds > cr, "접속 거부 → 오류, 요청 안 나감");
    CHECK(S[3][3] == 0x00 && W.proto[3] == 0 && n_discon == nd0, "닫힘(이미 닫힌 소켓에는 DISCON 을 보내지 않음)");
    connect_mode = CONN_ACCEPT;

    /* 송신 실패(재전송 한도): 접속은 됐지만 SEND 가 TIMEOUT */
    memset(&gacc, 0, sizeof gacc); gacc.abort_at = -1; st = -1;
    setup(R1, sizeof R1 - 1, 100, 0, 1);
    peer_dead = 1; nd0 = n_discon;
    CHECK(http_get(&W, 3, 3000, SRV, 80, "h", "/", 50000, sink_acc, &gacc, &st) == W5500_ERR_PEER && st == 0, "송신 실패 → PEER");
    CHECK(S[3][3] == 0x00 && W.proto[3] == 0 && n_discon == nd0, "닫힘");
    peer_dead = 0;

    /* 인자 오류는 소켓을 건드리지 않음 */
    {
        int nc = n_cmds;
        CHECK(http_get(&W, 3, 3000, SRV, 80, "h i", "/", 50000, sink_acc, &gacc, &st) == HTTP_ERR_ARG && n_cmds == nc, "나쁜 호스트: 소켓 명령 없음");
        CHECK(http_get(&W, 3, 3000, SRV, 80, "h", "no-slash", 50000, sink_acc, &gacc, &st) == HTTP_ERR_ARG && n_cmds == nc, "나쁜 경로: 소켓 명령 없음");
        CHECK(http_get(0, 3, 3000, SRV, 80, "h", "/", 5, sink_acc, &gacc, &st) == QSPI_ERR_ARGS && http_get(&W, 3, 0, SRV, 80, "h", "/", 5, sink_acc, &gacc, &st) == QSPI_ERR_ARGS && http_get(&W, 3, 3000, 0, 80, "h", "/", 5, sink_acc, &gacc, &st) == QSPI_ERR_ARGS
              && http_get(&W, 3, 3000, SRV, 0, "h", "/", 5, sink_acc, &gacc, &st) == QSPI_ERR_ARGS && http_get(&W, 3, 3000, SRV, 80, "h", "/", 5, sink_acc, &gacc, 0) == QSPI_ERR_ARGS, "인자 검사");
        char longpath[400]; memset(longpath, 'a', sizeof longpath); longpath[0] = '/'; longpath[399] = 0;
        CHECK(http_get(&W, 3, 3000, SRV, 80, "h", longpath, 50000, sink_acc, &gacc, &st) == HTTP_ERR_NOSPACE && n_cmds == nc, "요청이 256바이트 버퍼보다 길면 NOSPACE, 소켓 명령 없음");
    }
    CHECK(viol == 0, "위반 없음");
}

int main(void)
{
    t_build(); t_parse(); t_get();
    if (fails) { printf("FAIL: %d\n", fails); return 1; }
    printf("ALL PASS (http)\n");
    return 0;
}
