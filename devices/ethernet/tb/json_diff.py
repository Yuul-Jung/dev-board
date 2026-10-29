#!/usr/bin/env python3
"""fuzz_json dump 의 출력(줄마다 "코드 16진수문서")을 파이썬 json 모듈의 판정과 비교한다. 사용: ./fuzz_json dump | python3 -I json_diff.py
파이썬 쪽을 RFC 8259 에 맞추려고: NaN·Infinity 거부, 앞의 BOM 하나는 벗김, 0x80 이상의 잘못된 UTF-8 은 surrogateescape 로 문자열 안에서만 통과시킴.
우리 쪽만 깊이 16 제한이 있으므로 DEPTH(-131) 는 비교에서 빼고(파이썬은 깊이를 따로 세어 16 초과면 건너뜀), 나머지는 판정이 같아야 한다."""
import sys, json

def depth(s):
    d = m = 0; instr = esc = False
    for ch in s:
        if instr:
            if esc: esc = False
            elif ch == '\\': esc = True
            elif ch == '"': instr = False
        elif ch == '"': instr = True
        elif ch in '[{':
            d += 1; m = max(m, d)
        elif ch in ']}': d -= 1
    return m

def bad_const(x): raise ValueError(x)

agree = disagree = skipped = 0
valid = 0
for line in sys.stdin:
    code, _, hx = line.rstrip('\n').partition(' ')
    code = int(code); raw = bytes.fromhex(hx)
    if code == -131: skipped += 1; continue
    s = raw.decode('utf-8', errors='surrogateescape')
    if s.startswith('﻿'): s = s[1:]
    if depth(s) > 16: skipped += 1; continue
    try:
        json.loads(s, parse_constant=bad_const); py = True
    except Exception:
        py = False
    mine = code == 0
    valid += mine
    if py == mine: agree += 1
    else:
        disagree += 1
        if disagree <= 10: print('DISAGREE mine=%s python=%s doc=%r' % (mine, py, raw[:120]))
print('agree %d, disagree %d, skipped %d, valid %d' % (agree, disagree, skipped, valid))
sys.exit(1 if disagree else 0)
