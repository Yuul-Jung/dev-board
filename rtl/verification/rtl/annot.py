#!/usr/bin/env python3
"""파일을 GitHub 주석(::notice)으로 쪼개 출력(로그·artifact 를 못 내려받는 환경용).
GitHub 주석 본문은 약 4 KB 에서 잘리므로 한 덩어리를 3200자 이하로 나눈다. 한 step 당 notice 는 10개까지만 보인다.
사용: annot.py 파일 제목 [시작덩어리=0]   (10개 덩어리 출력)"""
import sys
path, title = sys.argv[1], sys.argv[2]; off = int(sys.argv[3]) if len(sys.argv) > 3 else 0
lines = open(path, encoding='utf-8').read().splitlines(); chunks = []; cur = ''
for l in lines:
    if len(cur) + len(l) + 1 > 3200: chunks.append(cur); cur = ''
    cur += l + '\n'
if cur: chunks.append(cur)
esc = lambda s: s.replace('%', '%25').replace('\r', '%0D').replace('\n', '%0A')
for i, c in enumerate(chunks[off:off + 10], start=off):
    print('::notice title=%s %d/%d::%s' % (title, i + 1, len(chunks), esc(c)))
if not chunks: print('::notice title=%s::(비어 있음)' % title)
