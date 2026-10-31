#!/usr/bin/env python3
"""폴더마다 index.html (GitHub Pages 용 정적 페이지)을 만든다. README.md·데이터에서 읽기만 하고 .md 는 건드리지 않는다.
사용: python3 devices/tools/gen_pages.py [--check]   (먼저 gen_index.py 를 돌릴 필요는 없다)
.md/소스 링크는 github.com 절대 주소, 하위 폴더 링크는 상대 주소(index.html)."""
import os, re, sys, csv, html
sys.path.insert(0, os.path.dirname(__file__))
root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
GH = 'https://github.com/Yuul-Jung/dev-board'
SKIP = {'.git', '__pycache__', 'node_modules'}
ROLE = {'ref': '근거 자료와 사실표(교차 검증 대상)', 'rtl': 'Verilog RTL 과 테스트벤치 (시뮬레이터 없음, 미실행)',
        'sw': '드라이버 C 소스', 'tb': '호스트 테스트와 레지스터/장치 모델(mock)', 'data': '구조화 데이터(CSV·JSON)', 'tools': '검사·생성 스크립트'}
e = html.escape

def read(p):
    try: return open(p, encoding='utf-8', errors='replace').read()
    except OSError: return ''

def title(p):
    for l in read(p).splitlines():
        if l.startswith('# '): return l[2:].strip()
    return ''

def lede(p):
    out = []; seen = False
    for l in read(p).splitlines():
        if l.startswith('# '): seen = True; continue
        if not seen: continue
        s = l.strip()
        if not s or s.startswith(('<!--', '|', '#', '```')):
            if out: break
            continue
        out.append(s)
        if len(out) >= 3: break
    t = ' '.join(out)
    t = re.sub(r'\[([^\]]+)\]\([^)]*\)', r'\1', t)
    return re.sub(r'[`*]', '', t)

def first_comment(p):
    for i, l in enumerate(read(p).splitlines()):
        if i > 12: break
        s = l.strip()
        if s.startswith(('#include', '#ifndef', '#define', '#pragma', '#!')): continue
        m = re.match(r'(?:/\*+|//+|#|\*)\s*(.+?)\s*(?:\*+/)?$', s)
        if m and len(m.group(1)) > 3 and not m.group(1).startswith(('-', '=', '*')): return m.group(1)[:90]
        if s.startswith('"""'): return s.strip('"').strip()[:90]
    return ''

def desc(p): return title(p) if p.endswith('.md') else (title(p) if p.endswith('.html') else first_comment(p))

CSS = """:root{--bg:#fafaf7;--fg:#1c1c1a;--mut:#6b6b66;--line:#dcdcd4;--card:#fff;--acc:#1f5f8b;--ok:#2e7d4f;--part:#b7791f;--doc:#7a6bb0;--none:#8a8a84;--ex:#5b7f95}
@media(prefers-color-scheme:dark){:root:not([data-theme=light]){--bg:#161715;--fg:#e8e8e2;--mut:#9a9a92;--line:#33342f;--card:#1e1f1c;--acc:#6db3e0;--ok:#5cbf86;--part:#e0a948;--doc:#a597d8;--none:#8f8f88;--ex:#86a9be}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:16px/1.6 -apple-system,"Apple SD Gothic Neo","Noto Sans KR",system-ui,sans-serif}
main{max-width:960px;margin:0 auto;padding:20px 16px 60px}a{color:var(--acc);text-decoration:none}a:hover{text-decoration:underline}
nav.bc{font-size:14px;color:var(--mut);margin-bottom:12px}h1{font-size:28px;margin:4px 0 8px}h2{font-size:18px;margin:32px 0 10px;border-bottom:1px solid var(--line);padding-bottom:6px}
.lede{color:var(--mut);margin:0 0 8px}.gh{font-size:14px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(250px,1fr));gap:10px}
.card{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:12px 14px;display:block;color:var(--fg)}.card:hover{border-color:var(--acc);text-decoration:none}
.card b{display:block;color:var(--acc)}.card span{font-size:14px;color:var(--mut)}
table{width:100%;border-collapse:collapse;font-size:14px}th,td{text-align:left;padding:6px 8px;border-bottom:1px solid var(--line);vertical-align:top}th{color:var(--mut);font-weight:600}
td.f{word-break:break-all}.tw{overflow-x:auto}
.kpi{display:flex;flex-wrap:wrap;gap:8px;margin:8px 0}.kpi div{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:8px 14px;min-width:84px}.kpi b{font-size:22px;display:block}.kpi small{color:var(--mut)}
.s-완비{color:var(--ok)}.s-일부{color:var(--part)}.s-문서{color:var(--doc)}.s-없음{color:var(--none)}.s-제외{color:var(--ex)}
.dot{display:inline-block;width:9px;height:9px;border-radius:50%;margin-right:6px;background:currentColor}
footer{margin-top:40px;font-size:13px;color:var(--mut)}\npre.code{background:var(--card);border:1px solid var(--line);border-radius:9px;padding:14px;overflow-x:auto;font:13px/1.6 ui-monospace,SFMono-Regular,Consolas,monospace;tab-size:2}p,li{overflow-wrap:anywhere}code{font-family:ui-monospace,Consolas,monospace}"""

STAT = ['완비', '일부', '문서', '없음', '제외']
LABEL = {'완비': '완비', '일부': '일부', '문서': '문서만', '없음': '없음', '제외': '제외(AMD 중복)'}
MODS = list(csv.DictReader(open(os.path.join(root, 'devices/data/module-status.csv'), encoding='utf-8')))

def kpi(rows):
    c = {s: sum(1 for r in rows if r['status'] == s) for s in STAT}
    return '<div class="kpi">' + f'<div><b>{len(rows)}</b><small>전체 모듈</small></div>' + ''.join(
        f'<div class="s-{s}"><b>{c[s]}</b><small><span class="dot"></span>{LABEL[s]}</small></div>' for s in STAT) + '</div>'

def modtable(rows):
    h = '<div class="tw"><table><tr><th>ID</th><th>모듈</th><th>인터페이스</th><th>교재</th><th>상태</th><th>비고</th></tr>'
    for r in rows:
        w = r['where']; nm = e(r['name'])
        if w and os.path.exists(os.path.join(root, 'devices', w)): nm = f'<a href="{GH}/blob/main/devices/{e(w)}">{nm}</a>'
        h += f'<tr><td>{e(r["id"])}</td><td>{nm}</td><td>{e(r["interface"])}</td><td>{e(r["book_ch"])}</td><td class="s-{r["status"]}"><span class="dot"></span>{LABEL.get(r["status"], r["status"])}</td><td>{e(r["note"])}</td></tr>'
    return h + '</table></div>'

def lesson_content(d):
    """Arduino 실습 README 및 .ino를 정적 페이지 안에 직접 표시한다."""
    rd = read(os.path.join(d, 'README.md'))
    if not rd: return ''
    blocks = []; in_list = False
    for line in rd.splitlines():
        t = line.strip()
        if t.startswith('<!-- index:begin -->'): break
        if t.startswith('# '): continue
        if t.startswith('## '):
            if in_list: blocks.append('</ul>'); in_list = False
            blocks.append('<h2>' + e(t[3:]) + '</h2>'); continue
        if t.startswith(('- ', '1. ', '2. ', '3. ', '4. ')):
            if not in_list: blocks.append('<ul>'); in_list = True
            blocks.append('<li>' + inline_md(re.sub(r'^(?:- |[1-4]\\. )', '', t)) + '</li>')
            continue
        if in_list: blocks.append('</ul>'); in_list = False
        if t: blocks.append('<p>' + inline_md(t) + '</p>')
    if in_list: blocks.append('</ul>')
    for fn in sorted(os.listdir(d)):
        if fn.endswith('.ino'):
            code = read(os.path.join(d, fn))
            blocks.append('<h2>전체 스케치 — ' + e(fn) + '</h2>')
            blocks.append('<pre class="code"><code>' + e(code) + '</code></pre>')
            blocks.append('<p><a href="' + GH + '/blob/main/' +
                          e(os.path.relpath(os.path.join(d, fn), root).replace(os.sep, '/')) +
                          '">원본 소스 보기</a></p>')
    return '\\n'.join(blocks)

def inline_md(s):
    s = e(s)
    s = re.sub(r'\\[([^\\]]+)\\]\\(([^)]+)\\)', lambda m:
               '<a href="' + e(m.group(2), quote=True) + '">' + m.group(1) + '</a>', s)
    s = re.sub(r'`([^`]+)`', r'<code>\\1</code>', s)
    s = re.sub(r'\\*\\*([^*]+)\\*\\*', r'<strong>\\1</strong>', s)
    return s

def page(d):
    rel = os.path.relpath(d, root); rel = '' if rel == '.' else rel
    parts = rel.split(os.sep) if rel else []
    rd = os.path.join(d, 'README.md')
    name = title(rd) or (parts[-1] if parts else 'dev-board')
    if not rel: name = 'dev-board'
    sub = sorted(x for x in os.listdir(d) if os.path.isdir(os.path.join(d, x)) and x not in SKIP and not x.startswith('.'))
    files = sorted(x for x in os.listdir(d) if os.path.isfile(os.path.join(d, x)) and x not in ('index.html', '.nojekyll'))
    bc = []
    for i in range(len(parts)):
        up = '../' * (len(parts) - i)
        bc.append(f'<a href="{up}index.html">{"홈" if i == 0 else e(parts[i-1])}</a>')
    bc.append(e(parts[-1]) if parts else '홈')
    o = [f'<!doctype html><html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>{e(name)} · dev-board</title><style>{CSS}</style></head><body><main>']
    if parts: o.append('<nav class="bc">' + ' › '.join(bc) + '</nav>')
    o.append(f'<h1>{e(name)}</h1>')
    L = lede(rd)
    if L: o.append(f'<p class="lede">{e(L)}</p>')
    o.append(f'<p class="gh"><a href="{GH}/tree/main/{rel}">GitHub 에서 이 폴더 보기</a> · <a href="{GH}/blob/main/{rel + "/" if rel else ""}README.md">README</a></p>')
    if rel in ('', 'devices'): o.append('<h2>모듈 현황</h2>' + kpi(MODS) + ('<p class="lede">상태 기준은 <a href="%s/blob/main/devices/STATUS.md">STATUS.md</a>. 코드는 호스트 모의 테스트까지만, Verilog 는 시뮬레이션 전입니다.</p>' % GH))
    if sub:
        o.append('<h2>하위 폴더</h2><div class="grid">')
        for x in sub:
            r = os.path.join(d, x, 'README.md')
            t = title(r); t = ROLE.get(x) or t or ''
            o.append(f'<a class="card" href="{e(x)}/index.html"><b>{e(x)}/</b><span>{e(t)}</span></a>')
        o.append('</div>')
    if rel.startswith('arduino/examples/') and len(parts) == 3:\n        o.append(lesson_content(d))\n    if rel == 'devices':\n        o.append('<h2>모듈 전체 목록</h2>' + modtable(MODS))
    if files:
        o.append('<h2>파일</h2><div class="tw"><table><tr><th>파일</th><th>설명</th></tr>')
        for x in files:
            href = f'{GH}/blob/main/{rel + "/" if rel else ""}{x}'
            if x.endswith('.html'): href = e(x)
            o.append(f'<tr><td class="f"><a href="{href}">{e(x)}</a></td><td>{e(desc(os.path.join(d, x)))}</td></tr>')
        o.append('</table></div>')
    o.append('<footer>자동 생성 페이지(devices/tools/gen_pages.py). 근거 수준과 한계는 각 문서에 적혀 있습니다.</footer></main></body></html>')
    return '\n'.join(o) + '\n'

changed = []
nj = os.path.join(root, '.nojekyll')
if not os.path.exists(nj):
    if '--check' not in sys.argv: open(nj, 'w').close()
    changed.append('.nojekyll')
for d, ds, _ in os.walk(root):
    ds[:] = sorted(x for x in ds if x not in SKIP and not x.startswith('.'))
    p = os.path.join(d, 'index.html'); new = page(d)
    if read(p) != new:
        if '--check' not in sys.argv: open(p, 'w', encoding='utf-8').write(new)
        changed.append(os.path.relpath(p, root))
print(f'페이지 {len(changed)}개 갱신' + (' (--check, 미기록)' if '--check' in sys.argv else ''))
sys.exit(1 if changed and '--check' in sys.argv else 0)
