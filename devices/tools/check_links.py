#!/usr/bin/env python3
"""저장소 안 모든 .md 의 상대 링크가 실제 파일/폴더를 가리키는지 검사한다. 사용: python3 devices/tools/check_links.py"""
import os, re, sys
root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
bad = []; n = 0
for d, _, fs in os.walk(root):
    if '.git' in d.split(os.sep): continue
    for f in fs:
        if not f.endswith('.md'): continue
        p = os.path.join(d, f)
        for m in re.finditer(r'\[[^\]]*\]\(([^)\s#]+)(#[^)]*)?\)', open(p, encoding='utf-8').read()):
            u = m.group(1)
            if u.startswith(('http://', 'https://', 'mailto:')): continue
            n += 1
            if not os.path.exists(os.path.normpath(os.path.join(d, u))): bad.append((os.path.relpath(p, root), u))
for b in bad: print('깨진 링크:', *b)
print(f'링크 {n}개 검사, 깨진 것 {len(bad)}개')
if not bad: print('ALL PASS')
sys.exit(1 if bad else 0)
