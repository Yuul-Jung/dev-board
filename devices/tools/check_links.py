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
markdown_bad = list(bad)
for b in markdown_bad: print('깨진 링크:', *b)
print(f'Markdown 링크 {n}개 검사, 깨진 것 {len(markdown_bad)}개')
# (추가) 공개 메인/주변장치/캡스톤 HTML의 실제 상대 링크를 검사한다.
# 전체 HTML 재귀 검사는 별도 작업으로 남겨 두고 공개 핵심 탐색 경로를 회귀 검증한다.
from html.parser import HTMLParser
from urllib.parse import urlsplit, unquote

class HTMLLinks(HTMLParser):
    def __init__(self):
        super().__init__()
        self.links = []

    def handle_starttag(self, tag, attrs):
        if tag not in ('a', 'img', 'script', 'link'):
            return
        for key, value in attrs:
            if key in ('href', 'src') and value:
                self.links.append(value)

site_pages = ('index.html', 'devices/index.html', 'devices/capstone/index.html',
              'devices/capstone/capstone-02-sg90-position.html')
site_count = 0
for rel in site_pages:
    source = os.path.join(root, rel)
    if not os.path.isfile(source):
        bad.append((rel, 'HTML 파일 없음'))
        continue
    parser = HTMLLinks()
    with open(source, encoding='utf-8') as stream:
        parser.feed(stream.read())
    for raw in parser.links:
        parsed = urlsplit(raw)
        if parsed.scheme or raw.startswith('//') or not parsed.path:
            continue
        site_count += 1
        target = unquote(parsed.path)
        if target.startswith('/'):
            bad.append((rel, raw + ' (프로젝트 Pages의 루트 상대경로 금지)'))
            continue
        full = os.path.normpath(os.path.join(os.path.dirname(source), target))
        if os.path.commonpath((root, full)) != root:
            bad.append((rel, raw + ' (저장소 밖 경로)'))
            continue
        if os.path.isdir(full):
            full = os.path.join(full, 'index.html')
        if not os.path.isfile(full):
            bad.append((rel, raw))

with open(os.path.join(root, 'index.html'), encoding='utf-8') as stream:
    home = stream.read()
for dest in ('arduino/index.html', 'devices/index.html', 'rtl/index.html',
             'soc-map/index.html', 'atlas/index.html', 'catalog/index.html',
             'devices/capstone/index.html',
             'devices/capstone/capstone-02-sg90-position.html'):
    if 'href="' + dest + '"' not in home:
        bad.append(('index.html', '메인 탐색 누락: ' + dest))

for item in bad:
    # Markdown 오류는 위에서 출력했고 HTML 오류를 함께 출력해 CI에서 추적한다.
    if item not in markdown_bad:
        print('깨진 HTML/탐색 링크:', *item)
print(f'핵심 HTML 상대 링크 {site_count}개 검사, 총 오류 {len(bad)}개')
if not bad:
    print('ALL PASS (Markdown + 핵심 HTML)')
sys.exit(1 if bad else 0)
