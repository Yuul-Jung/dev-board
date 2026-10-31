#!/usr/bin/env python3
"""data/session-map.csv(기술 트리 32차시 ↔ 모듈 ID)와 data/module-status.csv 로 SESSION-MAP.md 를 만든다.
사용: python3 tools/gen_sessions.py > SESSION-MAP.md   차시별 상태는 소속 모듈 중 가장 낮은 것."""
import csv, os
H = os.path.dirname(os.path.abspath(__file__))
rd = lambda n: list(csv.DictReader(open(os.path.join(H, '..', 'data', n), encoding='utf-8')))
M = {m['id']: m for m in rd('module-status.csv')}; S = rd('session-map.csv')
RANK = {'제외': 5, '완비': 4, '일부': 3, '문서': 2, '없음': 1}
LAB = {'완비': '완비', '일부': '일부', '문서': '문서만', '없음': '없음', '제외': '제외'}
q = lambda x: x.replace('|', '/')
o = []; a = o.append
a('# 기술 트리 32차시 ↔ 저장소 모듈 대응\n')
a('<!-- nav -->[저장소 홈](../README.md) › [devices](README.md)\n')
a('프로젝트 문서(`arduino_io_module_tech_tree_20261030_github_updated.md` §4·§10)의 32차시를 이 저장소의 모듈과 연결한 표입니다. 이 파일은 `tools/gen_sessions.py` 가 `data/session-map.csv` 와 `data/module-status.csv` 에서 만듭니다.\n')
a('- **대응(차시→모듈 ID)은 제목을 보고 맞춘 설계안**입니다. 문서가 모듈 ID 로 차시를 지정한 것이 아닙니다.')
a('- **차시 상태**는 소속 모듈 중 가장 낮은 상태입니다(계산). 「완비」도 호스트 모의 시험까지이고, 보드·RTL 시뮬레이션은 하나도 없습니다([STATUS.md](STATUS.md) §1).')
a('- AMD IP 열은 해당 차시에서 자체 RTL 대신 쓰는 AMD IP입니다(사용자 결정 2026-10-31, [TODO.md](TODO.md)). 14·19 차시는 모듈이 없고 AMD IP 만 씁니다.\n')
a('| 차시 | 트랙 | 실습 | 모듈 | AMD IP | 상태 | 메모 |\n|---|---|---|---|---|---|---|')
tot = {}
for s in S:
    ids = s['module_ids'].split()
    ms = [M[i] for i in ids]
    st = '—(AMD IP만)' if not ms else LAB[min((m['status'] for m in ms), key=lambda x: RANK[x])]
    if ms: tot[st] = tot.get(st, 0) + 1
    mods = ', '.join('%s %s(%s)' % (m['id'], q(m['name']), LAB[m['status']]) for m in ms) or '—'
    a('| %s | %s | %s | %s | %s | %s | %s |' % (s['session'], q(s['track']), q(s['title']), mods, q(s['amd_ip']) or '—', st, q(s['note'])))
a('\n## 요약\n')
a('| ' + ' | '.join(['완비', '일부', '문서만', '없음']) + ' | AMD IP만 |\n|---|---|---|---|---|')
a('| ' + ' | '.join(str(tot.get(k, 0)) for k in ['완비', '일부', '문서만', '없음']) + ' | %d |' % sum(1 for s in S if not s['module_ids'].split()))
a('\n<!-- nav-foot -->← [devices 목차](README.md)')
print('\n'.join(o))
