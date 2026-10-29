#!/usr/bin/env python3
"""rtl/ps2_rx_engine.v 의 알고리즘을 클럭 단위로 옮긴 파이썬 모델. Verilog 자체를 검증하지는 않는다(시뮬레이터 없음).
동기·필터·하강 에지 표본·패리티·정지·타임아웃 논리만 같은 순서로 옮겼다."""
CLK_MHZ, FILTER, TIMEOUT_US = 100, 8, 2000

class Eng:
    def __init__(s):
        s.cs = [1, 1]; s.ds = [1, 1]; s.cf = s.df = s.cp = 1; s.cc = s.dc = 0
        s.bit = 0; s.sh = 0; s.par = 0; s.idle = 0
        s.out = []; s.errs = {'par': 0, 'frm': 0, 'to': 0}
    def step(s, c, d):
        fall = s.cp and not s.cf                       # 이전 클럭의 값으로 판단(레지스터 의미 유지)
        # 필터 (비차단 대입과 같이 이전 값 기준)
        ncs = [c, s.cs[0]]; nds = [d, s.ds[0]]
        ncf, ncc = s.cf, s.cc
        if s.cs[1] != s.cf:
            if s.cc == FILTER - 1: ncf, ncc = s.cs[1], 0
            else: ncc = s.cc + 1
        else: ncc = 0
        ndf, ndc = s.df, s.dc
        if s.ds[1] != s.df:
            if s.dc == FILTER - 1: ndf, ndc = s.ds[1], 0
            else: ndc = s.dc + 1
        else: ndc = 0
        ncp = s.cf
        # 상태기계 (현재 값 기준)
        if fall:
            s.idle = 0
            if s.bit == 0:
                if s.df == 0: s.bit = 1
                else: s.errs['frm'] += 1
            elif s.bit == 9: s.par = s.df; s.bit = 10
            elif s.bit == 10:
                s.bit = 0
                if s.df != 1: s.errs['frm'] += 1
                elif (bin(s.sh).count('1') & 1) ^ s.par != 1: s.errs['par'] += 1
                else: s.out.append(s.sh)
            else:
                s.sh = (s.sh >> 1) | (s.df << 7); s.bit += 1
        elif s.bit != 0:
            if s.idle >= TIMEOUT_US * CLK_MHZ: s.bit = 0; s.idle = 0; s.errs['to'] += 1
            else: s.idle += 1
        s.cs, s.ds, s.cf, s.cc, s.df, s.dc, s.cp = ncs, nds, ncf, ncc, ndf, ndc, ncp

def run(frames, gap_us=100):
    e = Eng()
    def hold(c, d, us):
        for _ in range(int(us * CLK_MHZ)): e.step(c, d)
    def bit(b):
        hold(1, b, 5); hold(0, b, 20); hold(1, b, 15)   # 데이터 먼저, 클럭 Low 20 us, High 15 us
    hold(1, 1, 10)
    for kind, v in frames:
        if kind == 'partial':
            for b in v: bit(b)
            hold(1, 1, 2500); continue
        val, pflip, stop = v
        bit(0)
        for i in range(8): bit((val >> i) & 1)
        bit((~bin(val).count('1') & 1) ^ pflip)      # 홀수 패리티 비트
        bit(stop); hold(1, 1, gap_us)
    return e

e = run([('f', (0x1C, 0, 1)), ('f', (0xF0, 0, 1)), ('f', (0x1C, 1, 1)), ('f', (0x45, 0, 0)),
         ('partial', [0, 1, 0]), ('f', (0xE0, 0, 1))])
ok = e.out == [0x1C, 0xF0, 0xE0] and e.errs == {'par': 1, 'frm': 1, 'to': 1}
print(e.out, e.errs)
print("ALL PASS" if ok else "FAILED")
raise SystemExit(0 if ok else 1)
