#include "http.h"
#include <string.h>

enum { ST_STATUS, ST_HEADERS, ST_BODY_LEN, ST_BODY_CLOSE, ST_CHUNK_SIZE, ST_CHUNK_DATA, ST_CHUNK_END, ST_TRAILER, ST_DONE, ST_ERROR };

static int ap(char *b, size_t cap, size_t *n, const char *s)
{
    for (; *s; s++) { if (*n + 1 >= cap) return HTTP_ERR_NOSPACE; b[(*n)++] = *s; }
    return 0;
}
static int tok_ok(const char *s, int allow_slash)
{
    if (!s || !*s) return 0;
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c <= 0x20 || c >= 0x7F) return 0;
        if (!allow_slash && c == '/') return 0;
    }
    return 1;
}

int http_build_get(char *buf, size_t cap, const char *host, const char *path)
{
    if (!buf || cap == 0) return QSPI_ERR_ARGS;
    if (!tok_ok(host, 0) || !tok_ok(path, 1) || path[0] != '/') return HTTP_ERR_ARG;
    size_t n = 0; int e;
    if ((e = ap(buf, cap, &n, "GET "))) return e;
    if ((e = ap(buf, cap, &n, path))) return e;
    if ((e = ap(buf, cap, &n, " HTTP/1.1\r\nHost: "))) return e;
    if ((e = ap(buf, cap, &n, host))) return e;
    if ((e = ap(buf, cap, &n, "\r\nConnection: close\r\n\r\n"))) return e;
    buf[n] = 0;
    return (int)n;
}

void http_parser_init(http_parser_t *p, http_sink_fn sink, void *ctx)
{
    memset(p, 0, sizeof *p);
    p->st = ST_STATUS; p->sink = sink; p->ctx = ctx;
}

static int fail(http_parser_t *p, int e) { p->st = ST_ERROR; p->err = e; return e; }
static int lower(int c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }
static int is_ows(char c) { return c == ' ' || c == '\t'; }
static int hexv(char c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1; }

/* 이름이 want(소문자) 와 같은지(대소문자 무시), 길이 nl */
static int name_is(const char *s, unsigned nl, const char *want)
{
    unsigned i = 0;
    for (; want[i]; i++) if (i >= nl || lower((unsigned char)s[i]) != want[i]) return 0;
    return i == nl;
}

static int status_line(http_parser_t *p)
{
    const char *s = p->line; unsigned n = p->ll, i = 0;
    if (n < 8 || memcmp(s, "HTTP/", 5) != 0 || s[5] < '0' || s[5] > '9' || s[6] != '.' || s[7] < '0' || s[7] > '9') return fail(p, HTTP_ERR_STATUS);
    i = 8;
    if (i >= n || s[i] != ' ') return fail(p, HTTP_ERR_STATUS);
    i++;
    if (i + 3 > n) return fail(p, HTTP_ERR_STATUS);
    unsigned v = 0;
    for (unsigned k = 0; k < 3; k++) { char c = s[i + k]; if (c < '0' || c > '9') return fail(p, HTTP_ERR_STATUS); v = v * 10 + (unsigned)(c - '0'); }
    i += 3;
    if (i < n && s[i] != ' ') return fail(p, HTTP_ERR_STATUS);
    if (v < 100) return fail(p, HTTP_ERR_STATUS);
    p->status = (uint16_t)v;
    p->st = ST_HEADERS;
    return 0;
}

static int header_line(http_parser_t *p)
{
    const char *s = p->line; unsigned n = p->ll, c = 0;
    /* 줄 이어쓰기(공백으로 시작하는 줄)는 아래 "이름에 공백 금지" 검사에 걸려 오류가 된다 */
    while (c < n && s[c] != ':') c++;
    if (c == n || c == 0) return fail(p, HTTP_ERR_HEADER);
    for (unsigned k = 0; k < c; k++) if (is_ows(s[k])) return fail(p, HTTP_ERR_HEADER);
    int cl = name_is(s, c, "content-length"), te = name_is(s, c, "transfer-encoding");
    if (!cl && !te) return 0;
    if (p->trunc) return fail(p, HTTP_ERR_HEADER);                            /* 중요한 헤더가 잘렸음 */
    unsigned vb = c + 1, ve = n;
    while (vb < ve && is_ows(s[vb])) vb++;
    while (ve > vb && is_ows(s[ve - 1])) ve--;
    if (cl) {
        if (vb == ve) return fail(p, HTTP_ERR_LENGTH);
        uint64_t v = 0;
        for (unsigned k = vb; k < ve; k++) {
            if (s[k] < '0' || s[k] > '9') return fail(p, HTTP_ERR_LENGTH);
            v = v * 10 + (unsigned)(s[k] - '0');
            if (v > 0xFFFFFFFFull) return fail(p, HTTP_ERR_LENGTH);
        }
        if (p->has_len && p->content_length != (uint32_t)v) return fail(p, HTTP_ERR_LENGTH);
        p->has_len = 1; p->content_length = (uint32_t)v;
    } else {
        /* 마지막 쉼표 뒤 토큰이 chunked 인지 */
        unsigned ts = ve;
        while (ts > vb && s[ts - 1] != ',') ts--;
        while (ts < ve && is_ows(s[ts])) ts++;
        p->saw_te = 1;
        p->chunked = name_is(s + ts, ve - ts, "chunked") ? 1 : 0;
    }
    return 0;
}

static int end_of_headers(http_parser_t *p)
{
    if (p->status >= 100 && p->status < 200) {                                  /* 1xx: 본문 없음, 다음 응답을 기다림 */
        p->st = ST_STATUS; p->has_len = 0; p->chunked = 0; p->saw_te = 0; p->content_length = 0;
        return 0;
    }
    if (p->status == 204 || p->status == 304) { p->st = ST_DONE; return 1; }
    if (p->chunked) { p->st = ST_CHUNK_SIZE; return 0; }
    if (p->has_len && !p->saw_te) {
        p->remain = p->content_length;
        if (p->remain == 0) { p->st = ST_DONE; return 1; }
        p->st = ST_BODY_LEN; return 0;
    }
    p->st = ST_BODY_CLOSE;
    return 0;
}

static int chunk_size_line(http_parser_t *p)
{
    const char *s = p->line; unsigned n = p->ll, i = 0; uint32_t v = 0;
    while (i < n && hexv(s[i]) >= 0) {
        if (i >= 8) return fail(p, HTTP_ERR_CHUNK);
        v = (v << 4) | (uint32_t)hexv(s[i]); i++;
    }
    if (i == 0) return fail(p, HTTP_ERR_CHUNK);
    if (i < n && s[i] != ';' && !is_ows(s[i])) return fail(p, HTTP_ERR_CHUNK);
    if (v == 0) { p->st = ST_TRAILER; return 0; }
    p->chunk_left = v; p->st = ST_CHUNK_DATA;
    return 0;
}

static int deliver(http_parser_t *p, const uint8_t *d, unsigned n)
{
    p->body_bytes += n;
    if (p->sink && n && p->sink(p->ctx, d, n) != 0) return fail(p, HTTP_ERR_SINK);
    return 0;
}

int http_parser_feed(http_parser_t *p, const uint8_t *d, unsigned n)
{
    if (!p || (!d && n)) return QSPI_ERR_ARGS;
    unsigned i = 0;
    while (i < n) {
        switch (p->st) {
        case ST_ERROR: return p->err;
        case ST_DONE: return 1;
        case ST_BODY_CLOSE: { int e = deliver(p, d + i, n - i); return e ? e : 0; }
        case ST_BODY_LEN: {
            unsigned c = n - i; if (c > p->remain) c = p->remain;
            int e = deliver(p, d + i, c); if (e) return e;
            i += c; p->remain -= c;
            if (p->remain == 0) { p->st = ST_DONE; return 1; }
            break; }
        case ST_CHUNK_DATA: {
            unsigned c = n - i; if (c > p->chunk_left) c = p->chunk_left;
            int e = deliver(p, d + i, c); if (e) return e;
            i += c; p->chunk_left -= c;
            if (p->chunk_left == 0) { p->st = ST_CHUNK_END; p->cr = 0; }
            break; }
        case ST_CHUNK_END: {                                                  /* 청크 데이터 뒤 CRLF */
            uint8_t b = d[i++];
            if (b == '\r' && !p->cr) p->cr = 1;
            else if (b == '\n') { p->cr = 0; p->st = ST_CHUNK_SIZE; p->ll = 0; p->trunc = 0; }
            else return fail(p, HTTP_ERR_CHUNK);
            break; }
        default: {                                                            /* 줄 단위 상태: 상태줄, 헤더, 청크 크기, 트레일러 */
            uint8_t b = d[i++];
            if (p->cr) {
                if (b != '\n') return fail(p, p->st == ST_CHUNK_SIZE ? HTTP_ERR_CHUNK : p->st == ST_STATUS ? HTTP_ERR_STATUS : HTTP_ERR_HEADER);
                p->cr = 0;
            } else if (b == '\r') { p->cr = 1; break; }
            else if (b != '\n') {
                if (p->ll < HTTP_LINE_MAX - 1) p->line[p->ll++] = (char)b; else p->trunc = 1;
                break;
            }
            /* 줄 끝(CRLF 또는 LF 단독) */
            int r = 0, was_empty = (p->ll == 0 && !p->trunc);
            if (p->st == ST_STATUS) r = was_empty ? 0 : status_line(p);        /* 앞의 빈 줄은 무시 */
            else if (p->st == ST_HEADERS) r = was_empty ? end_of_headers(p) : header_line(p);
            else if (p->st == ST_CHUNK_SIZE) r = chunk_size_line(p);
            else if (p->st == ST_TRAILER) { if (was_empty) { p->st = ST_DONE; r = 1; } }
            p->ll = 0; p->trunc = 0;
            if (r < 0) return r;
            if (r == 1) return 1;
            break; }
        }
    }
    return p->st == ST_DONE ? 1 : p->st == ST_ERROR ? p->err : 0;
}

int http_parser_eof(http_parser_t *p)
{
    if (!p) return QSPI_ERR_ARGS;
    if (p->st == ST_DONE) return 0;
    if (p->st == ST_BODY_CLOSE) { p->st = ST_DONE; return 0; }
    if (p->st == ST_ERROR) return p->err;
    return HTTP_ERR_TRUNC;
}
