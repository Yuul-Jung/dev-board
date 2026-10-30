#!/usr/bin/env python3
"""폴더별 README.md 에 자동 목록(하위 폴더·파일)을 넣는다. 없는 README 는 만든다.
사용: python3 devices/tools/gen_index.py [--check]   (--check: 변경이 필요하면 1 로 종료)
목록은 <!-- index:begin --> ~ <!-- index:end --> 사이만 다시 쓰고, 나머지는 건드리지 않는다."""
import os, re, sys
root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
SKIP = {'.git', '__pycache__', 'node_modules'}
B, E = '<!-- index:begin -->', '<!-- index:end -->'
ROLE = {'ref': '근거 자료와 사실표(교차 검증 대상)', 'rtl': 'Verilog RTL 과 RTL 테스트벤치(시뮬레이터 없음, 미실행)',
        'sw': '드라이버 C 소스', 'tb': '호스트 테스트와 레지스터/장치 모델(mock)',
        'data': '구조화 데이터(CSV·JSON)', 'tools': '검사·생성 스크립트'}
KIND = [('문서', ('.md', '.txt', '.html')), ('소스', ('.c', '.h', '.v', '.py', '.sh')), ('데이터', ('.csv', '.json'))]

def title(p):
    try:
        for l in open(p, encoding='utf-8', errors='replace'):
            if l.startswith('# '): return l[2:].strip()
    except OSError: pass
    return ''

def first_comment(p):
    ext = os.path.splitext(p)[1]
    try:
        for i, l in enumerate(open(p, encoding='utf-8', errors='replace')):
            if i > 12: break
            s = l.strip()
            if s.startswith(('#include', '#ifndef', '#define', '#pragma')): continue
            m = re.match(r'(?:/\*+|//+|#(?!!)|\*)\s*(.+?)\s*(?:\*+/)?$', s) if ext != '.md' else None
            if m and not m.group(1).startswith(('-', '=', '*')) and len(m.group(1)) > 3:
                return m.group(1)[:90].replace('|', '/')
            if ext == '.py' and s.startswith('"""'):
                return s.strip('"').strip()[:90].replace('|', '/')
    except OSError: pass
    return ''

def desc(p):
    return title(p) if p.endswith('.md') else first_comment(p)

def nav(d):
    rel = os.path.relpath(d, root)
    if rel == '.': return ''
    parts = rel.split(os.sep); out = []
    for i in range(len(parts)):
        up = '../' * (len(parts) - i)
        name = '저장소 홈' if i == 0 else parts[i - 1]
        out.append(f'[{name}]({up}README.md)')
    return '<!-- nav -->' + ' › '.join(out)

def block(d):
    dirs = sorted(x for x in os.listdir(d) if os.path.isdir(os.path.join(d, x)) and x not in SKIP and not x.startswith('.'))
    files = sorted(x for x in os.listdir(d) if os.path.isfile(os.path.join(d, x)) and x != 'README.md')
    L = [B, '<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->', '']
    if dirs:
        L += ['### 하위 폴더', '', '| 폴더 | 내용 |', '|---|---|']
        for x in dirs:
            r = os.path.join(d, x, 'README.md')
            t = title(r) if os.path.exists(r) else ''
            t = ROLE.get(x, t) if x in ROLE else t
            L.append(f'| [{x}/]({x}/README.md) | {t} |')
        L.append('')
    if files:
        L += ['### 파일', '', '| 파일 | 설명 |', '|---|---|']
        for x in files:
            L.append(f'| [{x}]({x}) | {desc(os.path.join(d, x))} |')
        L.append('')
    L.append(E)
    return '\n'.join(L)

def process(d):
    p = os.path.join(d, 'README.md')
    blk = block(d)
    if os.path.exists(p):
        s = open(p, encoding='utf-8').read()
        if B in s and E in s:
            new = re.sub(re.escape(B) + r'.*?' + re.escape(E), lambda m: blk, s, flags=re.S)
        else:
            new = s.rstrip('\n') + '\n\n## 이 폴더의 목록\n\n' + blk + '\n'
    else:
        name = os.path.basename(d)
        h = f'# {name}\n\n{nav(d)}\n\n{ROLE.get(name, "")}\n\n## 이 폴더의 목록\n\n'
        new = h + blk + '\n'
    old = open(p, encoding='utf-8').read() if os.path.exists(p) else None
    if old != new:
        if '--check' not in sys.argv: open(p, 'w', encoding='utf-8').write(new)
        return True
    return False

changed = []
for d, ds, _ in os.walk(root):
    ds[:] = sorted(x for x in ds if x not in SKIP and not x.startswith('.'))
    if process(d): changed.append(os.path.relpath(d, root))
print(f'README {len(changed)}개 갱신' + (' (--check, 미기록)' if '--check' in sys.argv else ''))
for c in changed: print(' ', c)
sys.exit(1 if changed and '--check' in sys.argv else 0)
