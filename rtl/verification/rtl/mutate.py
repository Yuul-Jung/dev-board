#!/usr/bin/env python3
"""RTL 변형 검사 v2 — 각 과제 src/*.v 를 한 곳씩 망가뜨려 그 과제 TB 가 잡는지 본다.
v2 개선(ChatGPT 피드백 4-d 반영):
 1) 기준선(baseline): 변형 없는 원본이 컴파일·실행·「^PASS」 줄 출력까지 해야 그 과제의 변형 결과를 유효로 본다.
 2) 결과를 구분: 컴파일불가 / 시간초과 / 실행실패(종료코드≠0, $fatal) / PASS줄없음 / 생존. 단순 부분문자열 PASS 검색을 줄 시작(^PASS) 일치로 바꿈.
 3) 변형 집합 A(v1 연산자, 상한 60/파일, 222개, 이전 결과와 비교용)와 B(추가 연산자: + - & | << >> 상수-1, 상한 60/파일)를 분리해 보고.
 4) equivalent.txt 에 사람이 근거와 함께 「동치」로 판정한 변형을 적으면 실질 검출률(동치 제외)을 따로 낸다.
사용: python3 rtl/verification/rtl/mutate.py [--dry]  (--dry 는 시뮬레이터 없이 개수만)"""
import glob, hashlib, os, re, subprocess, sys, tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
root = os.path.abspath(os.path.join(HERE, '..', '..', '..'))
DRY = '--dry' in sys.argv or '--list' in sys.argv
LIST = '--list' in sys.argv
EXTRA = {'rtl05_xadc_pwm': ['rtl04_pwm_led/src/pwm_core.v'], 'rtl07_buzzer_melody': ['rtl06_timer_led/src/switch_sync.v']}
CAP = 60
PASS_RE = re.compile(r'^PASS', re.M)

def mask(s):
    def blank(m): return re.sub(r'[^\n]', ' ', m.group(0))
    s = re.sub(r'/\*.*?\*/', blank, s, flags=re.S)
    s = re.sub(r'//[^\n]*', blank, s)
    return re.sub(r'"(?:[^"\\\n]|\\.)*"', blank, s)

def bump(m, d=1):
    base, digits = m.group(2).lower(), m.group(3)
    v = int(digits, 16 if base == 'h' else 10) + d
    if v < 0: return None
    return "%s'%s%s" % (m.group(1), m.group(2), format(v, 'x' if base == 'h' else 'd'))

OPS_A = [
    (r'==', '!='), (r'!=', '=='), (r'>=', '>'),
    (r'(?<![<>=!\-])>(?![=>])', '>='), (r'(?<![<>=!])<(?![=<])', '<='),
    (r'&&', '||'), (r'\|\|', '&&'),
    (r"1'b1", "1'b0"), (r"1'b0", "1'b1"),
    (r"\+\s*1'b1", "- 1'b1"), (r"-\s*1'b1", "+ 1'b1"),
    (r'~(?!=)', ''), (r'!(?!=)', ''),
]
OPS_B = [
    (r'(?<![+\-])\+(?![+=:])', '-'), (r'(?<![\-<>=])-(?![\-=>])', '+'),
    (r'(?<![&])&(?![&])', '|'), (r'(?<![|])\|(?![|])', '&'),
    (r'<<(?!<)', '>>'), (r'(?<!>)>>(?!>)', '<<'),
]
LIT = re.compile(r"(\d+)'([dDhH])([0-9a-fA-F_]+)")

def mutants(raw):
    m = mask(raw); A = []; B = []
    for pat, rep in OPS_A:
        for x in re.finditer(pat, m): A.append((x.start(), x.end(), rep))
    for x in LIT.finditer(m):
        if x.group(1) == '1': continue
        mm = re.match(r"(\d+)'([dDhH])([0-9a-fA-F]+)", x.group(0))
        if mm:
            r = bump(mm, 1)
            if r: A.append((x.start(), x.end(), r))
            r = bump(mm, -1)
            if r: B.append((x.start(), x.end(), r))
    for pat, rep in OPS_B:
        for x in re.finditer(pat, m): B.append((x.start(), x.end(), rep))
    return sorted(A)[:CAP], sorted(B)[:CAP]

def run(cmd, timeout):
    try: return subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired: return None

def evaluate(top, files, tb, td):
    """반환: ('compile'|'timeout'|'runfail'|'nopass'|'survive'|'baseline_ok', 설명)"""
    exe = os.path.join(td, 'a.vvp')
    c = run(['iverilog', '-g2012', '-s', top, '-o', exe] + files + [tb], 60)
    if c is None: return 'timeout', 'compile'
    if c.returncode != 0: return 'compile', c.stderr[:200]
    r = run(['vvp', exe], 60)
    if r is None: return 'timeout', 'vvp'
    if r.returncode != 0: return 'runfail', ''
    if not PASS_RE.search(r.stdout): return 'nopass', ''
    return 'survive', ''

equiv = {}
ef = os.path.join(HERE, 'equivalent.txt')
if os.path.exists(ef):
    for ln in open(ef, encoding='utf-8'):
        ln = ln.strip()
        if ln and not ln.startswith('#'):
            key, _, why = ln.partition('|'); equiv[key.strip()] = why.strip()

S = {k: {'n': 0, 'kill': 0, 'surv': 0, 'inv': 0, 'eq': 0} for k in 'AB'}
kinds = {'runfail': 0, 'timeout': 0, 'nopass': 0, 'compile': 0}
rows = []; surv_lines = []; bad_base = []
for tb in sorted(glob.glob(os.path.join(root, 'rtl', 'rtl*', 'tb', 'tb_*.v'))):
    d = os.path.dirname(os.path.dirname(tb)); ln = os.path.basename(d); top = os.path.basename(tb)[:-2]
    own = sorted(glob.glob(os.path.join(d, 'src', '*.v')))
    extra = [os.path.join(root, 'rtl', e) for e in EXTRA.get(ln, [])]
    if not DRY:
        with tempfile.TemporaryDirectory() as td:
            res, why = evaluate(top, own + extra, tb, td)
        if res != 'survive':
            bad_base.append('%s baseline=%s %s' % (ln, res, why)); rows.append((ln, 'BASELINE_FAIL')); continue
    cnt = {k: {'n': 0, 'kill': 0, 'surv': 0, 'inv': 0, 'eq': 0} for k in 'AB'}
    for f in own:
        raw = open(f, encoding='utf-8').read(); A, B = mutants(raw); seen = set()
        for tag, lst in (('A', A), ('B', B)):
            for (a, b, rep) in lst:
                new = raw[:a] + rep + raw[b:]
                h = hashlib.md5(new.encode()).hexdigest()
                if h in seen: continue
                seen.add(h)
                c = cnt[tag]; c['n'] += 1
                line = raw.count('\n', 0, a) + 1; col = a - raw.rfind('\n', 0, a)
                key = '%s %s:%d:%d %s->%s' % (ln, os.path.basename(f), line, col, raw[a:b].strip(), rep.strip() or '(제거)')
                if LIST: print('%s [%s] | %s' % (key, tag, raw.splitlines()[line - 1].strip()[:90]))
                if DRY: continue
                with tempfile.TemporaryDirectory() as td:
                    mf = os.path.join(td, os.path.basename(f)); open(mf, 'w', encoding='utf-8').write(new)
                    res, why = evaluate(top, [mf if x == f else x for x in own] + extra, tb, td)
                if res == 'compile': c['kill'] += 1; kinds['compile'] += 1   # 기준선은 컴파일되는데 변형은 거부 = 변형이 TB 의 파라미터 구성에서 무효(검출)
                elif res == 'survive':
                    if key in equiv: c['eq'] += 1
                    c['surv'] += 1
                    srcline = raw.splitlines()[line - 1].strip()[:90]
                    surv_lines.append('%s [%s]%s | %s' % (key, tag, ' EQUIV' if key in equiv else '', srcline))
                else: c['kill'] += 1; kinds[res] += 1
    rows.append((ln,) + tuple(cnt['A'][k] for k in ('n', 'kill', 'surv')) + tuple(cnt['B'][k] for k in ('n', 'kill', 'surv')))
    for t in 'AB':
        for k in S[t]: S[t][k] += cnt[t][k]

print('과제\tA변형\tA잡힘\tA생존\tB변형\tB잡힘\tB생존')
for r in rows: print('\t'.join(map(str, r)))
for t in 'AB':
    s = S[t]; valid = s['kill'] + s['surv']
    eff = valid - s['eq']
    print('집합 %s: 변형 %d 잡힘 %d 생존 %d (동치 판정 %d) 컴파일거부(검출로 계산) %d' % (t, s['n'], s['kill'], s['surv'], s['eq'], kinds['compile'] if t=='A' else 0))
    if valid: print('  검출률(잡힘/유효) = %.1f %%' % (100.0 * s['kill'] / valid) + ('   실질(동치 제외) = %.1f %%' % (100.0 * s['kill'] / eff) if eff else ''))
print('잡은 방식: 실행실패 %(runfail)d, 시간초과 %(timeout)d, PASS줄없음 %(nopass)d, 컴파일 거부 %(compile)d' % kinds)
if bad_base: print('기준선 실패: ' + '; '.join(bad_base))
outd = os.path.join(HERE, 'results'); os.makedirs(outd, exist_ok=True)
open(os.path.join(outd, 'mutation_survivors.txt'), 'w', encoding='utf-8').write('\n'.join(surv_lines) + '\n')
sys.exit(1 if bad_base else 0)
