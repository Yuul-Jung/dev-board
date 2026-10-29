/* JSON 해석기 시험: 문법 경계(RFC 8259), 깊이, 숫자 변환(손계산), 문자열 풀기(\u·서러게이트), 경로 조회, 잘림 훑기(정확한 힙 버퍼), W5500 모델 위 http_get 본문 → 값 꺼내기.
 * 한계: 시험 문서는 내가 만든 것이다(날씨 API 의 실제 응답 형식을 확인한 것이 아님). RFC 8259 는 원문 요약(D1, 요약 경유)을 근거로 했고 전문 대조는 하지 않았다. */
#include "../../spi/tb/w5500_model.h"
#include "../sw/http.h"
#include "../sw/json.h"
#include <stdlib.h>

#define V_(s) json_validate((s), (unsigned)strlen(s))
static int ok(const char *s) { return V_(s) == 0; }

static void t_syntax(void)
{
    printf("[문법]\n");
    const char *good[] = { "0", "-0", "1", "-1", "10", "1.5", "-1.5", "0.1", "1e5", "1E5", "1e+5", "1e-5", "0.1e1", "-1.5E-10", "0e0", "123456789012345678901234567890",
        "true", "false", "null", "\"\"", "\"a\"", "\"\\\"\\\\\\/\\b\\f\\n\\r\\t\"", "\"\\u0041\"", "\"\\u00e9\\u00E9\"", "\"\\u00FF\\uFFFF\\uabcd\\uABCD\"", "\"\\ud800\"", "\"\\udc00\\ud800\"", "\"\x7f\"", "\"\xC3\xA9\"", "\"\xff\xfe\"",
        "[]", "{}", "[1,2,3]", "[ ]", "{ }", " [ 1 , 2 ] ", "\t[\r\n1\n,\t2\r]\n", "{\"a\":1}", "{ \"a\" : 1 , \"b\" : [ ] }", "{\"a\":{\"b\":[1,{\"c\":null}]}}", "[[],[[]],{}]",
        "{\"a\":1,\"a\":2}", "{\"\":1}", "{\"\\u0041\":1}", "[true,false,null,0,\"x\",[],{}]", "[1.0,1e0,0.0e-0]", "\xEF\xBB\xBF{}", "\xEF\xBB\xBF  1" };
    for (unsigned i = 0; i < sizeof good / sizeof good[0]; i++) if (!ok(good[i])) { printf("    (유효해야 함: #%u %s)\n", i, good[i]); fails++; }
    const char *bad[] = { "", " ", "\n", "01", "00", "-", "-a", "+1", "1.", ".5", "-.5", "1e", "1e+", "1e-", "1.e5", "--1", "1.2.3", "0x10", "1_000", "NaN", "Infinity", "-Infinity",
        "tru", "True", "TRUE", "NULL", "nul", "truex", "nullnull", "fals", "falsey", "tr ue",
        "[1,]", "[,1]", "[,]", "[1 2]", "[1,,2]", "[1;2]", "[", "]", "[1", "[1]]", "[[1]", "{\"a\":1,}", "{,}", "{\"a\"}", "{\"a\":}", "{\"a\" 1}", "{a:1}", "{'a':1}", "{\"a\":1 \"b\":2}", "{1:2}", "{\"a\":1", "{", "}", "{\"a\":1}}",
        "'a'", "\"abc", "\"\\", "\"\\x\"", "\"\\u12\"", "\"\\u12G4\"", "\"\\u\"", "\"\\U0041\"", "\"a\nb\"", "\"a\tb\"", "\"a\rb\"", "\"\x01\"", "\"\x1f\"", "\"\\\"",
        "\"\\u123\"", "\"\\u123x\"", "\"\\u1\"", "trud", "falsf", "nulk", "tru\x01", "{'a\":1}", "{\"a\"=1}", "{\"a\"\x01" "1}", "\xEF\xBB\xBE{}", "\xEF\xBA\xBF{}", "\xEF\x41\x42{}", "\xEF\xBB{}", "{}{}", "1 2", "[] []", "{} x", "1,", "// c\n1", "/* c */1", "[1]//", "\x0c" "1", "\x0b" "1", "\xC2\xA0" "1", "1\x0c", "\xEF\xBB\xBF\xEF\xBB\xBF{}", "\xEF\xBB\xBF", "\xEF\xBB", "[1]\xEF\xBB\xBF" };
    int allbad = 1;
    for (unsigned i = 0; i < sizeof bad / sizeof bad[0]; i++) if (V_(bad[i]) == 0) { allbad = 0; printf("    (거절해야 함: #%u %s)\n", i, bad[i]); }
    CHECK(allbad, "무효한 텍스트 거절 (숫자·리터럴·쉼표·괄호·이스케이프·제어 문자·공백 종류·BOM)");
    CHECK(json_validate("1\0" "2", 3) != 0 && json_validate("[1]\0", 4) != 0 && json_validate("\"a\0b\"", 5) != 0, "NUL 바이트는 어디서든 거절");
    CHECK(json_validate("[1]xxx", 3) == 0 && json_validate("[1]xxx", 4) != 0 && json_validate("[1", 2) != 0, "길이 n 밖은 보지 않음");
    CHECK(json_validate("tru", 3) == JSON_ERR_SYNTAX && json_validate("truee", 4) == 0, "리터럴이 길이에서 잘리면 거절");
    CHECK(json_validate(0, 0) == JSON_ERR_ARGS && json_parse("1", 1, 0) == JSON_ERR_ARGS, "NULL");
    json_val_t r;
    CHECK(json_parse("  [1]  ", 7, &r) == 0 && r.type == JSON_ARR && r.len == 3 && memcmp(r.p, "[1]", 3) == 0, "최상위 값 조각은 공백 제외");
    CHECK(json_parse("\xEF\xBB\xBF" "7", 4, &r) == 0 && r.type == JSON_NUM && r.len == 1, "BOM 건너뜀");
    CHECK(json_parse("\"x\"", 3, &r) == 0 && r.type == JSON_STR && r.len == 3, "문자열 조각은 따옴표 포함");
    const struct { const char *s; unsigned t; } ty[] = { { "null", JSON_NULL }, { "false", JSON_FALSE }, { "true", JSON_TRUE }, { "-1.5e3", JSON_NUM }, { "\"\"", JSON_STR }, { "[]", JSON_ARR }, { "{}", JSON_OBJ } };
    int tok = 1; for (unsigned i = 0; i < 7; i++) if (json_parse(ty[i].s, (unsigned)strlen(ty[i].s), &r) != 0 || r.type != ty[i].t) tok = 0;
    CHECK(tok, "값 종류 7가지");
}

static void nest(char *b, unsigned depth, char open, char close)
{
    unsigned k = 0; for (unsigned i = 0; i < depth; i++) b[k++] = open; for (unsigned i = 0; i < depth; i++) b[k++] = close; b[k] = 0;
}
static void t_depth(void)
{
    printf("[깊이]\n");
    static char b[200100];
    nest(b, 16, '[', ']'); CHECK(V_(b) == 0, "배열 16단계 허용");
    nest(b, 17, '[', ']'); CHECK(V_(b) == JSON_ERR_DEPTH, "배열 17단계 거부");
    { char *p = b; for (int i = 0; i < 16; i++) { memcpy(p, "{\"a\":", 5); p += 5; } *p++ = '1'; for (int i = 0; i < 16; i++) *p++ = '}'; *p = 0; CHECK(V_(b) == 0, "객체 16단계 허용"); }
    { char *p = b; for (int i = 0; i < 17; i++) { memcpy(p, "{\"a\":", 5); p += 5; } *p++ = '1'; for (int i = 0; i < 17; i++) *p++ = '}'; *p = 0; CHECK(V_(b) == JSON_ERR_DEPTH, "객체 17단계 거부"); }
    { char *p = b; for (int i = 0; i < 8; i++) { *p++ = '['; memcpy(p, "{\"a\":", 5); p += 5; } *p++ = '0'; for (int i = 0; i < 8; i++) { *p++ = '}'; *p++ = ']'; } *p = 0; CHECK(V_(b) == 0, "배열·객체 섞어 16단계 허용"); }
    memset(b, '[', 200000); b[200000] = 0;
    CHECK(V_(b) == JSON_ERR_DEPTH, "20만 단계 열기만 해도 깊이 오류로 끝남(스택 넘침 없음)");
    memset(b, '{', 100000); b[100000] = 0; CHECK(V_(b) != 0, "객체 10만 개 열기");
    nest(b, 16, '[', ']'); b[31] = 0; CHECK(V_(b) != 0, "닫는 괄호가 모자라면 거부");
    nest(b, 17, '[', ']'); b[34] = 0; CHECK(V_(b) == JSON_ERR_DEPTH, "깊이 오류는 문법 오류보다 먼저(17번째 열기에서)");
    { const char *s16 = "[[[[[[[[[[[[[[[[1]]]]]]]]]]]]]]]]", *s17 = "[[[[[[[[[[[[[[[[[1]]]]]]]]]]]]]]]]]";
      CHECK(strlen(s16) == 33 && strlen(s17) == 35 && V_(s16) == 0 && V_(s17) == JSON_ERR_DEPTH, "리터럴로 직접 센 16 / 17단계(길이 33 / 35)"); }
}

static json_val_t N(const char *s) { json_val_t v; if (json_parse(s, (unsigned)strlen(s), &v)) { v.p = 0; v.len = 0; v.type = 0; } return v; }
static void t_num(void)
{
    printf("[숫자]\n");
    int32_t o = 77; json_val_t v;
    const struct { const char *s; int32_t w; } ints[] = { { "0", 0 }, { "-0", 0 }, { "42", 42 }, { "-42", -42 }, { "2147483647", 2147483647 }, { "-2147483648", (int32_t)-2147483647 - 1 }, { "1000000000", 1000000000 }, { "-1", -1 } };
    for (unsigned i = 0; i < sizeof ints / sizeof ints[0]; i++) { v = N(ints[i].s); o = 77; CHECK(json_int(&v, &o) == 0 && o == ints[i].w, ints[i].s); }
    const char *range[] = { "2147483648", "-2147483649", "99999999999999999999", "-99999999999999999999", "4294967296", "4294967295", "10000000000", "-3000000000" };
    for (unsigned i = 0; i < sizeof range / sizeof range[0]; i++) { v = N(range[i]); CHECK(json_int(&v, &o) == JSON_ERR_RANGE, range[i]); }
    const char *notint[] = { "1.0", "1e2", "1E2", "0.5", "-1.5", "1e-2", "0e0", "1.5e3" };
    for (unsigned i = 0; i < sizeof notint / sizeof notint[0]; i++) { v = N(notint[i]); o = 77; CHECK(json_int(&v, &o) == JSON_ERR_TYPE && o == 77, notint[i]); }
    v = N("\"5\""); CHECK(json_int(&v, &o) == JSON_ERR_TYPE, "문자열은 TYPE"); v = N("true"); CHECK(json_int(&v, &o) == JSON_ERR_TYPE, "true 는 TYPE"); v = N("null"); CHECK(json_int(&v, &o) == JSON_ERR_TYPE, "null 은 TYPE");
    v = N("5"); CHECK(json_int(0, &o) == JSON_ERR_ARGS && json_int(&v, 0) == JSON_ERR_ARGS, "NULL");

    const struct { const char *s; unsigned d; int32_t w; } fx[] = {
        { "12.349", 2, 1234 }, { "-12.349", 2, -1234 }, { "12.349", 0, 12 }, { "12.349", 3, 12349 }, { "12.349", 4, 123490 }, { "0.05", 2, 5 }, { "0.05", 1, 0 }, { "0.05", 3, 50 }, { "-0.05", 1, 0 },
        { "1e3", 0, 1000 }, { "1E+3", 2, 100000 }, { "1e-3", 3, 1 }, { "1e-3", 2, 0 }, { "1.5e1", 0, 15 }, { "1.5e-1", 2, 15 }, { "15e-1", 1, 15 },
        { "2147483647", 0, 2147483647 }, { "-2147483648", 0, (int32_t)-2147483647 - 1 }, { "21474836.47", 2, 2147483647 }, { "-21474836.48", 2, (int32_t)-2147483647 - 1 },
        { "2.147483647", 9, 2147483647 }, { "1", 9, 1000000000 }, { "0", 9, 0 }, { "-0", 5, 0 }, { "-0.0001", 3, 0 }, { "0.000000001", 9, 1 }, { "0.0000000001", 9, 0 },
        { "0.1234567890123456789012", 9, 123456789 }, { "0.000000000000000000001", 9, 0 }, { "0", 0, 0 }, { "10", 0, 10 }, { "100", 1, 1000 }, { "1e0", 0, 1 }, { "0e999999", 0, 0 }, { "1e-999999", 9, 0 },
        { "123456789012345678", 0, 0 }, { "1234567890123456789012345e-20", 2, 1234567 }, { "100000000000000000000e-18", 0, 100 }, { "12345678901234567890.5e-10", 0, 1234567890 }, { "71", 0, 71 }, { "12.34", 1, 123 }, { "-1.5", 1, -15 }, { "37.57", 2, 3757 }, { "0.5", 0, 0 }, { "-0.5", 0, 0 }, { "0.999", 0, 0 }, { "1.999", 0, 1 } };
    for (unsigned i = 0; i < sizeof fx / sizeof fx[0]; i++) {
        v = N(fx[i].s); o = 77; int r = json_fixed(&v, fx[i].d, &o);
        if (strcmp(fx[i].s, "123456789012345678") == 0) { CHECK(r == JSON_ERR_RANGE, "18자리 정수는 RANGE"); continue; }
        if (r != 0 || o != fx[i].w) { printf("    (%s ×10^%u → %d, 기대 %d, r=%d)\n", fx[i].s, fx[i].d, o, fx[i].w, r); fails++; }
    }
    const struct { const char *s; unsigned d; } fr[] = { { "21474836.48", 2 }, { "-21474836.49", 2 }, { "3", 9 }, { "2.147483648", 9 }, { "1e999999", 0 }, { "1e11", 0 }, { "1e10", 0 }, { "214748364.8", 1 }, { "1234567890123456789012345.5", 0 }, { "100000000000000000000", 0 }, { "1e-999999", 0 } };
    for (unsigned i = 0; i < sizeof fr / sizeof fr[0]; i++) {
        v = N(fr[i].s); o = 77; int r = json_fixed(&v, fr[i].d, &o);
        if (strcmp(fr[i].s, "1e-999999") == 0) { CHECK(r == 0 && o == 0, "1e-999999 → 0"); continue; }
        if (r != JSON_ERR_RANGE || o != 77) { printf("    (%s ×10^%u → r=%d o=%d, RANGE 기대)\n", fr[i].s, fr[i].d, r, o); fails++; }
    }
    { /* 앞의 0 이 많은 소수 + 큰 지수: 선행 0 은 유효숫자로 세지 않고 지수는 정확히 상쇄 */
      char b[400]; json_val_t bv;
      snprintf(b, sizeof b, "0.%.20s123e22", "00000000000000000000000000"); bv = N(b); o = 77;
      CHECK(json_fixed(&bv, 1, &o) == 0 && o == 123, "0.(0 20개)123e22 ×10 = 123 (선행 0 이 유효숫자 칸을 차지하지 않음)");
      char z[260]; memset(z, '0', 200); z[200] = 0;
      snprintf(b, sizeof b, "0.%s1e201", z); bv = N(b); o = 77;
      CHECK(json_fixed(&bv, 0, &o) == 0 && o == 1, "0.(0 200개)1e201 = 1 (지수 200 대 상쇄)");
      snprintf(b, sizeof b, "0.%s1e202", z); bv = N(b); o = 77;
      CHECK(json_fixed(&bv, 0, &o) == 0 && o == 10, "0.(0 200개)1e202 = 10"); }
    v = N("2"); CHECK(json_fixed(&v, 10, &o) == JSON_ERR_ARGS && json_fixed(&v, 9, &o) == 0 && o == 2000000000 && json_fixed(0, 1, &o) == JSON_ERR_ARGS && json_fixed(&v, 1, 0) == JSON_ERR_ARGS, "decimals 10 거부, NULL");
    v = N("\"1.5\""); CHECK(json_fixed(&v, 1, &o) == JSON_ERR_TYPE, "문자열은 TYPE");
    v = N("1.5e1"); CHECK(json_fixed(&v, 1, &o) == 0 && o == 150, "1.5e1 → ×10 = 150");
    v = N("1e10"); CHECK(json_fixed(&v, 0, &o) == JSON_ERR_RANGE, "1e10 RANGE"); v = N("2e9"); CHECK(json_fixed(&v, 0, &o) == 0 && o == 2000000000, "2e9");
}

static void t_str(void)
{
    printf("[문자열 풀기]\n");
    char o[64]; unsigned l = 99; json_val_t v;
    const struct { const char *s; const char *w; unsigned wl; } ok_[] = {
        { "\"\"", "", 0 }, { "\"abc\"", "abc", 3 }, { "\"a\\nb\"", "a\nb", 3 }, { "\"\\\"\\\\\\/\"", "\"\\/", 3 }, { "\"\\b\\f\\r\\t\"", "\b\f\r\t", 4 },
        { "\"\\u0041BC\"", "ABC", 3 }, { "\"\\u00e9\"", "\xC3\xA9", 2 }, { "\"\\u00E9\"", "\xC3\xA9", 2 }, { "\"\\u20ac\"", "\xE2\x82\xAC", 3 }, { "\"\\u07ff\"", "\xDF\xBF", 2 }, { "\"\\u0800\"", "\xE0\xA0\x80", 3 },
        { "\"\\u007f\"", "\x7f", 1 }, { "\"\\u0080\"", "\xC2\x80", 2 }, { "\"\\ud834\\udd1e\"", "\xF0\x9D\x84\x9E", 4 }, { "\"\\uD834\\uDD1E\"", "\xF0\x9D\x84\x9E", 4 }, { "\"\\ud800\\udc00\"", "\xF0\x90\x80\x80", 4 }, { "\"\\udbff\\udfff\"", "\xF4\x8F\xBF\xBF", 4 },
        { "\"\\uffff\"", "\xEF\xBF\xBF", 3 }, { "\"\\u00FF\"", "\xC3\xBF", 2 }, { "\"\\ud840\\udc00\"", "\xF0\xA0\x80\x80", 4 }, { "\"\\ud87e\\ude1d\"", "\xF0\xAF\xA8\x9D", 4 }, { "\"\\ud83d\\ude00\"", "\xF0\x9F\x98\x80", 4 }, { "\"\\ue000\"", "\xEE\x80\x80", 3 }, { "\"\\ud7ff\"", "\xED\x9F\xBF", 3 }, { "\"\xC3\xA9\"", "\xC3\xA9", 2 }, { "\"\\u0000x\"", "\0x", 2 }, { "\"a\\u0041\\u0042c\"", "aABc", 4 } };
    for (unsigned i = 0; i < sizeof ok_ / sizeof ok_[0]; i++) {
        v = N(ok_[i].s); memset(o, 0x55, sizeof o); l = 99; int r = json_str(&v, o, sizeof o, &l);
        if (r != 0 || l != ok_[i].wl || memcmp(o, ok_[i].w, ok_[i].wl) != 0 || o[ok_[i].wl] != 0) { printf("    (#%u %s → r=%d len=%u)\n", i, ok_[i].s, r, l); fails++; }
    }
    const char *lone[] = { "\"\\ud800\"", "\"\\udbff\"", "\"\\udc00\"", "\"\\udfff\"", "\"\\ud800x\"", "\"\\ud800\\u0041\"", "\"\\ud800\\ud800\"", "\"\\udc00\\ud800\"", "\"\\ud800\\n\"", "\"\\ud800\\ue000\"" };
    int lok = 1; for (unsigned i = 0; i < sizeof lone / sizeof lone[0]; i++) { v = N(lone[i]); if (json_str(&v, o, sizeof o, &l) != JSON_ERR_UTF16) { lok = 0; printf("    (UTF16 기대: %s)\n", lone[i]); } }
    CHECK(lok, "짝 없는 서러게이트 10가지 → UTF16 (문법상은 유효)");
    /* 버퍼 경계: 정확한 크기의 힙 버퍼 */
    const struct { const char *s; unsigned need; } cap[] = { { "\"\"", 1 }, { "\"abc\"", 4 }, { "\"\\u00e9\"", 3 }, { "\"\\u20ac\"", 4 }, { "\"\\ud834\\udd1e\"", 5 }, { "\"a\\nb\"", 4 } };
    int cok = 1;
    for (unsigned i = 0; i < sizeof cap / sizeof cap[0]; i++) {
        v = N(cap[i].s);
        char *b = malloc(cap[i].need); int r = json_str(&v, b, cap[i].need, &l); free(b);
        if (r != 0) { cok = 0; printf("    (cap=%u 이면 성공해야 함: %s)\n", cap[i].need, cap[i].s); }
        b = malloc(cap[i].need - 1 ? cap[i].need - 1 : 1);
        r = cap[i].need > 1 ? json_str(&v, b, cap[i].need - 1, &l) : JSON_ERR_NOSPACE; free(b);
        if (cap[i].need > 1 && r != JSON_ERR_NOSPACE) { cok = 0; printf("    (cap=%u 이면 NOSPACE: %s)\n", cap[i].need - 1, cap[i].s); }
    }
    CHECK(cok, "정확한 크기에서 성공, 한 칸 작으면 NOSPACE (ASan 으로 넘침 검사)");
    v = N("\"x\""); CHECK(json_str(&v, o, 0, &l) == JSON_ERR_ARGS && json_str(&v, 0, 4, &l) == JSON_ERR_ARGS && json_str(0, o, 4, &l) == JSON_ERR_ARGS && json_str(&v, o, 2, 0) == 0, "NULL·cap 0, len 은 NULL 허용");
    v = N("5"); CHECK(json_str(&v, o, 8, &l) == JSON_ERR_TYPE, "숫자는 TYPE"); v = N("null"); CHECK(json_str(&v, o, 8, &l) == JSON_ERR_TYPE, "null 은 TYPE");
    /* 위조한 조각(검증 안 된 입력)도 범위 밖을 읽지 않음: 닫는 따옴표를 할당하지 않은 정확한 힙 버퍼, 길이는 닫는 따옴표가 있는 척 */
    { const char *f[] = { "\\", "\\u", "\\u00", "\\ud834\\u", "\\ud834\\udd", "\\ud834\\", "\\ud834", "\\x", "\\ud834\\u0041", "\\udc00" }; int fo = 1;
      for (unsigned i = 0; i < sizeof f / sizeof f[0]; i++) {
          unsigned rl = (unsigned)strlen(f[i]); char *h = malloc(1 + rl); h[0] = '"'; memcpy(h + 1, f[i], rl);
          json_val_t fv = { h, rl + 2, JSON_STR };
          int r = json_str(&fv, o, sizeof o, &l); free(h);
          if (r == 0) { fo = 0; printf("    (위조 조각이 성공함: %s)\n", f[i]); }
      }
      CHECK(fo, "잘리거나 틀린 이스케이프의 위조 조각은 오류, 범위 밖 읽기 없음(ASan)"); }
    { json_val_t f0 = { "\"", 1, JSON_STR }, f1 = { "", 0, JSON_STR }, f2 = { "\"\"", 2, JSON_STR };
      CHECK(json_str(&f0, o, sizeof o, &l) == JSON_ERR_TYPE && json_str(&f1, o, sizeof o, &l) == JSON_ERR_TYPE && json_str(&f2, o, sizeof o, &l) == 0 && l == 0, "길이 0·1 인 문자열 조각은 TYPE, 2 는 빈 문자열"); }
}

static const char DOC[] = "{\"coord\":{\"lon\":126.98,\"lat\":37.57},\"weather\":[{\"id\":500,\"main\":\"Rain\",\"description\":\"light rain\"},{\"id\":701,\"main\":\"Mist\",\"description\":\"mist\"}],"
    "\"main\":{\"temp\":12.34,\"feels_like\":-1.5,\"humidity\":71},\"name\":\"Seoul\",\"cod\":200,\"empty\":{},\"list\":[],\"t\":true,\"f\":false,\"nul\":null,\"esc\\u0041\":\"x\",\"dup\":1,\"dup\":2,\"\\u00e9\":5}";
#define G(p, v) json_get(DOC, (unsigned)strlen(DOC), p, v)
static void t_path(void)
{
    printf("[경로 조회]\n");
    json_val_t v; int32_t i; char s[64]; unsigned l; unsigned n = (unsigned)strlen(DOC);
    CHECK(json_get_fixed(DOC, n, "main.temp", 1, &i) == 0 && i == 123, "main.temp ×10 = 123");
    CHECK(json_get_fixed(DOC, n, "main.temp", 2, &i) == 0 && i == 1234, "main.temp ×100 = 1234");
    CHECK(json_get_fixed(DOC, n, "main.feels_like", 1, &i) == 0 && i == -15, "음수 -1.5 ×10");
    CHECK(json_get_int(DOC, n, "main.humidity", &i) == 0 && i == 71, "main.humidity");
    CHECK(json_get_fixed(DOC, n, "coord.lat", 2, &i) == 0 && i == 3757 && json_get_fixed(DOC, n, "coord.lon", 2, &i) == 0 && i == 12698, "coord.lat/lon");
    CHECK(json_get_str(DOC, n, "weather[0].description", s, sizeof s, &l) == 0 && l == 10 && strcmp(s, "light rain") == 0, "weather[0].description");
    CHECK(json_get_str(DOC, n, "weather[1].main", s, sizeof s, &l) == 0 && strcmp(s, "Mist") == 0 && json_get_int(DOC, n, "weather[1].id", &i) == 0 && i == 701, "weather[1]");
    CHECK(json_get_str(DOC, n, "name", s, sizeof s, &l) == 0 && strcmp(s, "Seoul") == 0 && json_get_int(DOC, n, "cod", &i) == 0 && i == 200, "name, cod");
    { char *h = malloc(6); int r = json_get_str(DOC, n, "name", h, 6, &l); free(h); char *h2 = malloc(5); int r2 = json_get_str(DOC, n, "name", h2, 5, &l); free(h2);
      CHECK(r == 0 && r2 == JSON_ERR_NOSPACE, "json_get_str 은 cap 을 그대로 전달: 6 은 성공, 5 는 NOSPACE"); }
    CHECK(G("", &v) == 0 && v.type == JSON_OBJ && v.p == DOC && v.len == n, "빈 경로 = 최상위");
    CHECK(G("t", &v) == 0 && v.type == JSON_TRUE && G("f", &v) == 0 && v.type == JSON_FALSE && G("nul", &v) == 0 && v.type == JSON_NULL, "true/false/null");
    CHECK(G("empty", &v) == 0 && v.type == JSON_OBJ && v.len == 2 && G("list", &v) == 0 && v.type == JSON_ARR && v.len == 2, "빈 객체·배열");
    CHECK(G("weather", &v) == 0 && v.type == JSON_ARR && v.p[0] == '[' && v.p[v.len - 1] == ']', "배열 조각");
    CHECK(json_get_str(DOC, n, "escA", s, sizeof s, &l) == 0 && strcmp(s, "x") == 0, "이름의 \\u0041 이 'A' 로 풀려 escA 와 일치");
    CHECK(json_get_int(DOC, n, "\xC3\xA9", &i) == 0 && i == 5, "이름 \\u00e9 가 UTF-8 é 와 일치");
    CHECK(json_get_int(DOC, n, "dup", &i) == 0 && i == 1, "이름이 겹치면 먼저 나온 것(설계안)");
    const char *nf[] = { "missing", "main.missing", "mai", "main2", "temp", "Main", "weather[2]", "weather[99]", "weather[999999999]", "empty.a", "list[0]", "weather[0].zzz", "coord.LAT" };
    int nok = 1; for (unsigned k = 0; k < sizeof nf / sizeof nf[0]; k++) if (G(nf[k], &v) != JSON_ERR_NOTFOUND) { nok = 0; printf("    (NOTFOUND 기대: %s)\n", nf[k]); }
    CHECK(nok, "없는 키·인덱스 13가지 → NOTFOUND");
    const char *tp[] = { "main.temp.x", "name.x", "name[0]", "weather.x", "weather[0][0]", "t.x", "nul.a", "main[0]", "cod[0]", "[0]", "weather[0].id.q" };
    int tok = 1; for (unsigned k = 0; k < sizeof tp / sizeof tp[0]; k++) if (G(tp[k], &v) != JSON_ERR_TYPE) { tok = 0; printf("    (TYPE 기대: %s)\n", tp[k]); }
    CHECK(tok, "종류가 안 맞는 경로 11가지 → TYPE");
    const char *pb[] = { ".a", "a.", "a..b", "main.", ".", "..", "a[", "a[]", "a[x]", "a[1", "a]b", "a]", "]", "[", "[]", "a[0]b", "weather[0]x", "weather[1234567890]", "weather[-1]", "weather[ 1]", "main.[0]", "weather[0]..id", "a.[0]" };
    int pok = 1; for (unsigned k = 0; k < sizeof pb / sizeof pb[0]; k++) if (G(pb[k], &v) != JSON_ERR_PATH) { pok = 0; printf("    (PATH 기대: %s → %d)\n", pb[k], G(pb[k], &v)); }
    CHECK(pok, "경로 문법 오류 23가지 → PATH");
    CHECK(G(0, &v) == JSON_ERR_ARGS && json_get(0, 1, "a", &v) == JSON_ERR_ARGS && json_get(DOC, n, "a", 0) == JSON_ERR_ARGS, "NULL");
    CHECK(json_get_fixed(DOC, n, "name", 1, &i) == JSON_ERR_TYPE && json_get_int(DOC, n, "main.temp", &i) == JSON_ERR_TYPE && json_get_str(DOC, n, "cod", s, sizeof s, &l) == JSON_ERR_TYPE, "값 종류가 다르면 TYPE");
    CHECK(json_get_int(DOC, n, "missing", &i) == JSON_ERR_NOTFOUND && json_get_fixed(DOC, n, "missing", 1, &i) == JSON_ERR_NOTFOUND && json_get_str(DOC, n, "missing", s, sizeof s, &l) == JSON_ERR_NOTFOUND, "감싸는 함수도 오류를 그대로");
    /* 전체를 먼저 검증: 경로 앞부분이 맞아도 뒤가 깨졌으면 거부 */
    { const char *b1 = "{\"a\":1,}"; CHECK(json_get(b1, (unsigned)strlen(b1), "a", &v) == JSON_ERR_SYNTAX, "뒤에 쉼표가 남은 객체: a 는 보이지만 거부");
      const char *b2 = "{\"a\":1} x"; CHECK(json_get(b2, (unsigned)strlen(b2), "a", &v) == JSON_ERR_SYNTAX, "뒤에 쓰레기");
      const char *b3 = "{\"a\":[1,2"; CHECK(json_get(b3, (unsigned)strlen(b3), "a[0]", &v) == JSON_ERR_SYNTAX, "잘린 문서"); }
    { const char *a = "[10,20,[30,40]]";
      CHECK(json_get_int(a, (unsigned)strlen(a), "[1]", &i) == 0 && i == 20 && json_get_int(a, (unsigned)strlen(a), "[2][1]", &i) == 0 && i == 40 && json_get_int(a, (unsigned)strlen(a), "[0]", &i) == 0 && i == 10, "최상위 배열 인덱스");
      CHECK(json_get(a, (unsigned)strlen(a), "[3]", &v) == JSON_ERR_NOTFOUND, "범위 밖"); }
    { /* 키 길이 한계: 63바이트까지 */
      char d[300], k63[64], k64[65]; memset(k63, 'k', 63); k63[63] = 0; memset(k64, 'k', 64); k64[64] = 0;
      snprintf(d, sizeof d, "{\"%s\":1,\"%s\":2}", k63, k64);
      CHECK(json_get_int(d, (unsigned)strlen(d), k63, &i) == 0 && i == 1, "키 63바이트는 찾음");
      CHECK(json_get(d, (unsigned)strlen(d), k64, &v) == JSON_ERR_NOTFOUND, "키 64바이트는 찾지 못함(설계안: 이름 풀이 버퍼 64)"); }
    { /* 큰 문서 안 끝쪽 */
      static char big[40000]; unsigned k = 0; k += (unsigned)snprintf(big + k, sizeof big - k, "{\"arr\":[");
      for (int j = 0; j < 3000; j++) k += (unsigned)snprintf(big + k, sizeof big - k, "%s%d", j ? "," : "", j);
      k += (unsigned)snprintf(big + k, sizeof big - k, "],\"last\":42}");
      CHECK(json_get_int(big, k, "arr[2999]", &i) == 0 && i == 2999 && json_get_int(big, k, "arr[1500]", &i) == 0 && i == 1500 && json_get_int(big, k, "last", &i) == 0 && i == 42 && json_get(big, k, "arr[3000]", &v) == JSON_ERR_NOTFOUND, "원소 3000개 배열의 끝·중간·뒤의 키"); }
}

static void t_sweep(void)
{
    printf("[잘림 훑기]\n");
    unsigned n = (unsigned)strlen(DOC); int bad = 0, valid = 0, got = 0;
    for (unsigned k = 0; k <= n; k++) {
        char *c = malloc(k ? k : 1); memcpy(c, DOC, k);
        int r = json_validate(c, k);
        if (k < n && r == 0) bad++;
        if (k == n && r == 0) valid++;
        json_val_t v; int g = json_get(c, k, "weather[1].description", &v);
        if (g == 0) got++;
        if (k < n && g == 0) bad++;
        char tmp[8]; unsigned l; (void)json_get_str(c, k, "name", tmp, sizeof tmp, &l);
        free(c);
    }
    CHECK(bad == 0 && valid == 1 && got == 1, "정확한 힙 버퍼로 모든 접두(0~n)를 검증·조회: 전체만 성공, ASan 오류 없음");
    /* 한 바이트씩 바꿔 보기: 죽지 않고 정해진 코드만 */
    int unknown = 0;
    static const unsigned char subs[] = { 0, '"', '\\', '{', '}', '[', ']', ',', ':', '-', '0', 'e', 'u', 0x80, 0xff, '\n' };
    for (unsigned k = 0; k < n; k++) for (unsigned s = 0; s < sizeof subs; s++) {
        char *c = malloc(n); memcpy(c, DOC, n); c[k] = (char)subs[s];
        int r = json_validate(c, n);
        if (r != 0 && r != JSON_ERR_SYNTAX && r != JSON_ERR_DEPTH) unknown++;
        json_val_t v; int g = json_get(c, n, "main.temp", &v);
        if (g != 0 && g != JSON_ERR_SYNTAX && g != JSON_ERR_DEPTH && g != JSON_ERR_NOTFOUND && g != JSON_ERR_TYPE) unknown++;
        free(c);
    }
    CHECK(unknown == 0, "문서의 모든 바이트를 16가지 값으로 바꿔도 죽지 않고 정해진 코드만(SYNTAX/DEPTH/NOTFOUND/TYPE)");
}

/* ---------- W5500 위 http_get → JSON ---------- */
static const uint8_t SRV[4] = { 198, 51, 100, 7 };
static const char *resp; static unsigned resp_len, resp_pos;
static int peer_sock = -1;
static void deliver(void)
{
    if (resp_pos >= resp_len) return;
    unsigned c = resp_len - resp_pos < 40 ? resp_len - resp_pos : 40;
    inject_rx(peer_sock, (const uint8_t *)resp + resp_pos, c); resp_pos += c;
}
static void thook(int sock, int proto, const uint8_t ip[4], uint16_t port, const uint8_t *d, unsigned len)
{
    (void)d; (void)len;
    if (proto != 1 || port != 80 || memcmp(ip, SRV, 4) != 0) return;
    peer_sock = sock; deliver(); tick_hook = deliver;
}
typedef struct { char b[600]; unsigned n; } body_t;
static int sink(void *c, const uint8_t *d, unsigned n) { body_t *b = c; if (b->n + n > sizeof b->b) return 1; memcpy(b->b + b->n, d, n); b->n += n; return 0; }
static void t_http(void)
{
    printf("[http_get → JSON]\n");
    CHECK(start(1) == 0, "init");
    static char r[1200]; const char *js = "{\"main\":{\"temp\":-3.25,\"humidity\":40},\"weather\":[{\"description\":\"\\ud55c\\uae30\"}]}";
    snprintf(r, sizeof r, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %u\r\n\r\n%s", (unsigned)strlen(js), js);
    resp = r; resp_len = (unsigned)strlen(r); resp_pos = 0; peer_hook = thook; tick_hook = 0;
    body_t body; memset(&body, 0, sizeof body); int st = 0;
    CHECK(http_get(&W, 3, 3000, SRV, 80, "api.example", "/w", 200000, sink, &body, &st) == 0 && st == 200 && body.n == strlen(js), "40바이트 조각으로 받은 본문이 JSON 원문과 같은 길이");
    int32_t t, h; char d[32]; unsigned l;
    CHECK(json_get_fixed(body.b, body.n, "main.temp", 1, &t) == 0 && t == -32, "본문에서 온도: -3.25 ×10 → -32 (0 쪽으로 버림)");
    CHECK(json_get_int(body.b, body.n, "main.humidity", &h) == 0 && h == 40, "습도 40");
    CHECK(json_get_str(body.b, body.n, "weather[0].description", d, sizeof d, &l) == 0 && l == 6 && memcmp(d, "\xED\x95\x9C\xEA\xB8\xB0", 6) == 0, "한글 \\ud55c\\uae30 → UTF-8 6바이트");
    CHECK(viol == 0, "모델 위반 없음");
}

int main(void)
{
    t_syntax(); t_depth(); t_num(); t_str(); t_path(); t_sweep(); t_http();
    if (fails) { printf("FAIL: %d\n", fails); return 1; }
    printf("ALL PASS (json)\n");
    return 0;
}
