#!/usr/bin/env python3
"""RTL 변형 검사: 각 과제의 src/*.v 를 한 곳씩 망가뜨려 보고(연산자 뒤집기, 상수 +1, 부정 제거 ...)
그 과제의 TB 가 실패하는지 본다. 살아남은 변형 = TB 가 못 잡는 결함 후보(동치 변형일 수도 있어 사람이 읽고 판정).
사용: python3 rtl/verification/rtl/mutate.py [--dry]   (--dry 는 시뮬레이터 없이 변형 개수만 센다)
성공 판정: vvp 종료코드 0 이고 출력에 PASS 가 있을 때만 「살아남음」."""
import glob, os, re, subprocess, sys, tempfile
root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
DRY = '--dry' in sys.argv
EXTRA = {'rtl05_xadc_pwm': ['rtl04_pwm_led/src/pwm_core.v'], 'rtl07_buzzer_melody': ['rtl06_timer_led/src/switch_sync.v']}
CAP = 60

def mask(s):  # 주석·문자열을 같은 길이의 공백으로(위치 보존)
    def blank(m): return re.sub(r'[^\n]', ' ', m.group(0))
    s = re.sub(r'/\*.*?\*/', blank, s, flags=re.S)
    s = re.sub(r'//[^\n]*', blank, s)
    return re.sub(r'"(?:[^"\\\n]|\\.)*"', blank, s)

def bump(m):
    base, digits = m.group(2).lower(), m.group(3)
    v = int(digits, 16 if base == 'h' else 10) + 1
    return "%s'%s%s" % (m.group(1), m.group(2), format(v, 'x' if base == 'h' else 'd'))

OPS = [
    (r'==', '!='), (r'!=', '=='), (r'>=', '>'),
    (r'(?<![<>=!\-])>(?![=>])', '>='), (r'(?<![<>=!])<(?![=<])', '<='),
    (r'&&', '||'), (r'\|\|', '&&'),
    (r"1'b1", "1'b0"), (r"1'b0", "1'b1"),
    (r"\+\s*1'b1", "- 1'b1"), (r"-\s*1'b1", "+ 1'b1"),
    (r'~(?!=)', ''), (r'!(?!=)', ''),
]
LIT = re.compile(r"(\d+)'([dDhH])([0-9a-fA-F_]+)")

def mutants(raw):
    m = mask(raw); out = []
    for pat, rep in OPS:
        for x in re.finditer(pat, m):
            out.append((x.start(), x.end(), rep, pat))
    for x in LIT.finditer(m):
        if x.group(1) == '1': continue
        try: out.append((x.start(), x.end(), bump(re.match(r"(\d+)'([dDhH])([0-9a-fA-F]+)", x.group(0))), 'lit+1'))
        except Exception: pass
    out.sort(key=lambda t: (t[0], t[1], t[2]))
    return out

def run(cmd, timeout):
    try: return subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired: return None

tot = {'mut': 0, 'kill': 0, 'surv': 0, 'inv': 0}
rows = []; surv_lines = []
for tb in sorted(glob.glob(os.path.join(root, 'rtl', 'rtl*', 'tb', 'tb_*.v'))):
    d = os.path.dirname(os.path.dirname(tb)); ln = os.path.basename(d); top = os.path.basename(tb)[:-2]
    own = sorted(glob.glob(os.path.join(d, 'src', '*.v')))
    extra = [os.path.join(root, 'rtl', e) for e in EXTRA.get(ln, [])]
    k = s = inv = n = 0
    for f in own:
        raw = open(f, encoding='utf-8').read(); ms = mutants(raw)[:CAP]
        for (a, b, rep, why) in ms:
            n += 1
            if DRY: continue
            line = raw.count('\n', 0, a) + 1
            new = raw[:a] + rep + raw[b:]
            with tempfile.TemporaryDirectory() as td:
                mf = os.path.join(td, os.path.basename(f)); open(mf, 'w', encoding='utf-8').write(new)
                files = [mf if x == f else x for x in own] + extra
                exe = os.path.join(td, 'a.vvp')
                c = run(['iverilog', '-g2012', '-s', top, '-o', exe] + files + [tb], 60)
                if c is None or c.returncode != 0: inv += 1; continue
                r = run(['vvp', exe], 60)
                if r is not None and r.returncode == 0 and 'PASS' in r.stdout: s += 1; surv_lines.append('%s %s:%d %s->%s' % (ln, os.path.basename(f), line, raw[a:b].strip() or '(제거)', rep or '(제거)'))
                else: k += 1
    rows.append((ln, n, k, s, inv)); tot['mut'] += n; tot['kill'] += k; tot['surv'] += s; tot['inv'] += inv
print('과제\t변형\t잡힘\t살아남음\t컴파일불가')
for r in rows: print('\t'.join(map(str, r)))
print('합계\t%d\t%d\t%d\t%d' % (tot['mut'], tot['kill'], tot['surv'], tot['inv']))
valid = tot['kill'] + tot['surv']
if valid: print('검출률(잡힘/유효 변형) = %.1f %%' % (100.0 * tot['kill'] / valid))
os.makedirs(os.path.join(root, 'rtl/verification/rtl/results'), exist_ok=True)
open(os.path.join(root, 'rtl/verification/rtl/results/mutation.tsv'), 'w', encoding='utf-8').write(
    '과제\t변형\t잡힘\t살아남음\t컴파일불가\n' + '\n'.join('\t'.join(map(str, r)) for r in rows) + '\n')
open(os.path.join(root, 'rtl/verification/rtl/results/mutation_survivors.txt'), 'w', encoding='utf-8').write('\n'.join(surv_lines) + '\n')
if surv_lines and not DRY:
    msg = '%0A'.join(l.replace('%', '%25') for l in surv_lines[:150])
    print('::warning title=변형 검사 살아남은 변형 %d건(앞 150건)::%s' % (len(surv_lines), msg[:60000]))
