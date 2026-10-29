/* JSON 해석기 무작위 시험(고정 시드): 유효한 문서를 무작위로 망가뜨리거나 자르거나 토큰을 끼워 넣어도
 * (1) ASan/UBSan 오류 없이 끝나고 (2) 검증 결과가 SYNTAX/DEPTH/0 중 하나이며 (3) 성공이면 최상위 조각이 버퍼 안에 있고 종류가 첫 글자와 맞으며
 * (4) json_get 이 성공해 돌려준 조각은 그 자체로 유효한 JSON 텍스트이고 버퍼 안에 있으며 (5) 숫자·문자열 변환이 죽지 않는지 확인한다.
 * `fuzz_json dump` 는 앞의 3만 건의 "검증 결과 코드 + 문서 16진수"를 출력한다(json_diff.py 가 파이썬 json 모듈과 비교하는 데 씀). */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../sw/json.h"

static uint32_t rs = 1357924680u;
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
static const char *seeds[] = {
    "{\"coord\":{\"lon\":126.98,\"lat\":37.57},\"weather\":[{\"id\":500,\"main\":\"Rain\",\"description\":\"light rain\"}],\"main\":{\"temp\":-12.34e0,\"humidity\":71},\"name\":\"Se\\u00e9oul\",\"ok\":true,\"x\":null,\"y\":false}",
    "[1,2.5,-3e+2,\"a\\n\\\"b\\\\\",[],{},[[1,[2]]],{\"k\":{\"k\":[{}]}},\"\\ud834\\udd1e\"]",
    " \t\r\n{ \"a\" : [ 0 , -0 , 0.0e-0 , 1E5 ] , \"b\" : \"\\/\\b\\f\\r\\t\" }\n",
    "\"\\u0041\\u00e9\\u20ac\"", "-0.5", "[true,false,null]" };
static const char *toks[] = { "{", "}", "[", "]", ",", ":", "\"", "\\", "\\u", "\\ud800", "-", "e", ".", "0", "00", "true", "nul", "\xEF\xBB\xBF", "\n", "\x01", "1e999", " " };
static const char *paths[] = { "", "a", "main.temp", "weather[0].id", "[0]", "x.y[2]", "k.k[0]", "a[1]", "[2][1]", "name" };

int main(int argc, char **argv)
{
    int dump = argc > 1 && strcmp(argv[1], "dump") == 0;
    unsigned total = 0, okc = 0, bad = 0, gets = 0;
    for (unsigned it = 0; it < 400000; it++) {
        char buf[600]; const char *sd = seeds[rnd() % (sizeof seeds / sizeof seeds[0])];
        unsigned len = (unsigned)strlen(sd); memcpy(buf, sd, len);
        unsigned nm = rnd() % 5;
        for (unsigned k = 0; k < nm; k++) {
            unsigned op = rnd() % 6, pos = rnd() % (len ? len : 1);
            if (op == 0) buf[pos] = (char)rnd();
            else if (op == 1 && len > 1) { memmove(buf + pos, buf + pos + 1, len - pos - 1); len--; }
            else if (op == 2 && len < sizeof buf - 2) { memmove(buf + pos + 1, buf + pos, len - pos); buf[pos] = (char)rnd(); len++; }
            else if (op == 3) {
                const char *t = toks[rnd() % (sizeof toks / sizeof toks[0])]; unsigned tl = (unsigned)strlen(t);
                if (len + tl < sizeof buf - 2) { memmove(buf + pos + tl, buf + pos, len - pos); memcpy(buf + pos, t, tl); len += tl; }
            }
            else buf[pos] ^= (char)(1u << (rnd() % 8));
        }
        if (rnd() % 8 == 0) len = rnd() % (len + 1);
        char *h = malloc(len ? len : 1); memcpy(h, buf, len);          /* 정확한 크기의 힙 복사: 한 바이트 넘어 읽으면 ASan 이 잡음 */
        int r = json_validate(h, len);
        total++;
        if (dump && it < 30000) { printf("%d ", r); for (unsigned i = 0; i < len; i++) printf("%02x", (unsigned char)h[i]); printf("\n"); }
        if (r != 0 && r != JSON_ERR_SYNTAX && r != JSON_ERR_DEPTH) { if (bad++ < 5) printf("UNKNOWN code %d it=%u\n", r, it); free(h); continue; }
        json_val_t root;
        int rp = json_parse(h, len, &root);
        if (rp != r) { if (bad++ < 5) printf("parse/validate mismatch it=%u\n", it); }
        if (r == 0) {
            okc++;
            int inside = root.p >= h && root.p + root.len <= h + len && root.len > 0;
            const char c0 = root.p[0];
            int tyok = (c0 == '{' && root.type == JSON_OBJ) || (c0 == '[' && root.type == JSON_ARR) || (c0 == '"' && root.type == JSON_STR) || (c0 == 't' && root.type == JSON_TRUE)
                    || (c0 == 'f' && root.type == JSON_FALSE) || (c0 == 'n' && root.type == JSON_NULL) || ((c0 == '-' || (c0 >= '0' && c0 <= '9')) && root.type == JSON_NUM);
            if (!inside || !tyok) { if (bad++ < 5) printf("root slice invalid it=%u\n", it); }
        }
        const char *path = paths[rnd() % (sizeof paths / sizeof paths[0])];
        json_val_t v; int g = json_get(h, len, path, &v);
        int gok = g == 0 || g == JSON_ERR_NOTFOUND || g == JSON_ERR_TYPE || g == JSON_ERR_SYNTAX || g == JSON_ERR_DEPTH;
        if (!gok || ((g == JSON_ERR_SYNTAX || g == JSON_ERR_DEPTH) != (r != 0))) { if (bad++ < 5) printf("get code %d (validate %d) path=%s it=%u\n", g, r, path, it); }
        if (g == 0) {
            gets++;
            if (!(v.p >= h && v.p + v.len <= h + len) || json_validate(v.p, v.len) != 0) { if (bad++ < 5) printf("get slice invalid path=%s it=%u\n", path, it); }
            int32_t n32; char out[16]; unsigned ol;
            (void)json_int(&v, &n32); (void)json_fixed(&v, rnd() % 10, &n32); (void)json_str(&v, out, sizeof out, &ol);
            if (v.type == JSON_STR && json_str(&v, out, sizeof out, &ol) == 0 && (ol >= sizeof out || out[ol] != 0)) { if (bad++ < 5) printf("str terminator it=%u\n", it); }
        }
        free(h);
    }
    if (!dump) printf("%u cases, %u valid, %u gets, %u problems\n", total, okc, gets, bad);
    if (bad) return 1;
    if (!dump) printf("ALL PASS (json fuzz)\n");
    return 0;
}
