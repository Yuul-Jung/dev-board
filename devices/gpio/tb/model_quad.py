#!/usr/bin/env python3
"""rtl/quad_engine.v 의 전이 표(00->10->11->01->00 = +1)와 필터를 옮긴 파이썬 모델. Verilog 검증이 아님."""
INC = {(0b00,0b10),(0b10,0b11),(0b11,0b01),(0b01,0b00)}
DEC = {(b,a) for a,b in INC}
BAD = {(0b00,0b11),(0b11,0b00),(0b01,0b10),(0b10,0b01)}
def decode(seq, invert=0):
    cnt = err = 0; prev = seq[0]
    for cur in seq[1:]:
        if (prev, cur) in INC:   cnt += -1 if invert else 1
        elif (prev, cur) in DEC: cnt += 1 if invert else -1
        elif (prev, cur) in BAD: err += 1
        prev = cur
    return cnt, err
def debounce(raw, lim):           # raw: 클럭마다 0/1, lim: 안정 유지 클럭 수 (필터 모델)
    f = raw[0]; c = 0; out = []
    for v in raw:
        if v != f:
            if c >= lim - 1: f = v; c = 0
            else: c += 1
        else: c = 0
        out.append(f)
    return out
cw = [0b00,0b10,0b11,0b01,0b00,0b10,0b11,0b01,0b00]       # 정방향 2 주기 = +8
ccw = cw[::-1]
assert decode(cw) == (8, 0) and decode(ccw) == (-8, 0) and decode(cw, 1) == (-8, 0)
assert decode([0b00,0b11,0b00]) == (0, 2)                    # 불가능 전이는 세지 않음
# 필터: 채터(짧은 글리치)는 제거, 긴 변화는 통과
raw = [0]*50 + [1]*3 + [0]*50 + [1]*200
out = debounce(raw, 20)
assert out[100] == 0 and out[-1] == 1 and max(out[:103]) == 0
print("ALL PASS")
