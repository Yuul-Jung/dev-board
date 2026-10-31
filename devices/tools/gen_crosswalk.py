#!/usr/bin/env python3
"""data/crosswalk.csv(주제별: 교재 장 ↔ Arduino 차시 ↔ 테크트리 차시 ↔ 모듈 ID)로 TREE-CROSSWALK.md 를 만든다.
사용: python3 tools/gen_crosswalk.py > TREE-CROSSWALK.md   검사: 세 체계의 모든 번호가 한 행 이상에 나오는지 확인하고, 빠지면 비0 종료."""
import csv, os, sys
H = os.path.dirname(os.path.abspath(__file__))
rd = lambda n: list(csv.DictReader(open(os.path.join(H, '..', 'data', n), encoding='utf-8')))
M = {m['id']: m for m in rd('module-status.csv')}; X = rd('crosswalk.csv'); CH = rd('chapter-status.csv')
LAB = {'완비': '완비', '일부': '일부', '문서': '문서만', '없음': '없음', '제외': '제외'}; RANK = {'제외': 5, '완비': 4, '일부': 3, '문서': 2, '없음': 1}
q = lambda x: x.replace('|', '/')
need = {'book': {c['chapter'] for c in CH}, 'ard': {'%02d' % i for i in range(1, 33)}, 'tt': {'%02d' % i for i in range(1, 33)}}
got = {'book': set(), 'ard': set(), 'tt': set()}
for r in X:
    got['book'] |= set(r['book_ch'].split()); got['ard'] |= set(r['arduino_lesson'].split()); got['tt'] |= set(r['techtree_session'].split())
bad = [(k, sorted(need[k] - got[k])) for k in need if need[k] - got[k]] + [('unknown', sorted(got[k] - need[k])) for k in got if got[k] - need[k]]
o = []; a = o.append
a('# 세 체계 대조: 교재 목차 ↔ Arduino 32차시 ↔ 테크트리 32차시\n')
a('<!-- nav -->[저장소 홈](../README.md) › [devices](README.md)\n')
a('아두이노 교재 목차(31장)와 [Arduino 32차시](../arduino/ROADMAP.md)를 뼈대로 삼아, 프로젝트 문서의 기존 테크트리(FPGA/SoC 32차시, [SESSION-MAP.md](SESSION-MAP.md))와 **주제 단위로** 맞춘 표입니다. `tools/gen_crosswalk.py` 가 `data/crosswalk.csv` 에서 만들고, 세 체계의 모든 번호가 한 행 이상에 들어 있는지 검사합니다.\n')
a('- 세 체계의 번호는 서로 다릅니다. 이 표만이 대응을 보여 줍니다(예: Arduino 09차시 = 테크트리 대응 없음, 테크트리 09차시 = 서보).')
a('- **주제 묶기는 제목을 보고 한 설계안**입니다. 교재 본문은 보지 못했습니다([BOOK-MAP](BOOK-MAP.md)).')
a('- 모듈 상태는 [STATUS.md](STATUS.md) 기준(호스트 모의 시험까지, 보드·RTL 시뮬레이션 0건)이며 소속 모듈 중 가장 낮은 값입니다.\n')
a('## 1. 주제별 대조\n')
a('| 주제 | 교재 장 | Arduino 차시 | 테크트리 차시 | 모듈 | 상태 | 메모 |\n|---|---|---|---|---|---|---|')
for r in X:
    ids = r['module_ids'].split(); ms = [M[i] for i in ids]
    st = '—' if not ms else LAB[min((m['status'] for m in ms), key=lambda s: RANK[s])]
    f = lambda v: v.replace(' ', ', ') or '—'
    a('| %s | %s | %s | %s | %s | %s | %s |' % (q(r['topic']), f(r['book_ch']), f(r['arduino_lesson']), f(r['techtree_session']),
      ', '.join(q(m['name']) for m in ms) or '—', st, q(r['note'])))
def only(cond, title):
    rows = [r for r in X if cond(r)]
    a('\n### %s (%d)\n' % (title, len(rows)))
    for r in rows:
        ids = r['module_ids'].split(); st = ', '.join('%s %s' % (i, LAB[M[i]['status']]) for i in ids) or '모듈 없음'
        a('- %s — %s%s' % (q(r['topic']), st, (' · ' + q(r['note'])) if r['note'] else ''))
a('\n## 2. 한쪽에만 있는 것\n')
only(lambda r: r['techtree_session'] and not r['arduino_lesson'], '테크트리에만 있는 주제 (Arduino 쪽에 대응 차시가 없음)')
only(lambda r: r['arduino_lesson'] and not r['techtree_session'], 'Arduino 쪽에만 있는 주제 (테크트리에 대응 차시가 없음)')
only(lambda r: r['book_ch'] and not r['arduino_lesson'] and not r['techtree_session'], '교재에만 있는 주제 (둘 다 없음)')
a('\n## 3. 읽는 법\n')
a('- 「테크트리에만 있음」은 Arduino 로 먼저 익힐 차시가 없다는 뜻이라 RTL 실습 전에 참조 동작을 얻을 곳이 없습니다. Arduino 차시 추가 또는 공통 자료 보강 후보입니다.')
a('- 「Arduino 쪽에만 있음」은 RTL 로 옮길 대상이 테크트리에 없다는 뜻입니다. 필요하면 테크트리에 차시를 더하거나, Arduino 전용으로 둘지 결정해야 합니다(미결).')
a('\n<!-- nav-foot -->← [devices 목차](README.md)')
print('\n'.join(o))
if bad: sys.stderr.write('번호 누락/미상: %s\n' % bad); sys.exit(1)
