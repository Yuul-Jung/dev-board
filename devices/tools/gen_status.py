#!/usr/bin/env python3
"""data/module-status.csv 와 data/chapter-status.csv 로 STATUS.md 의 표를 만든다. 사용: python3 tools/gen_status.py > STATUS.md
CSV 의 「상태」는 사람이 저장소를 보고 적는다(판정 기준은 STATUS.md §1). 이 스크립트는 표 모양만 만든다."""
import csv, os, sys
from collections import Counter, OrderedDict
H = os.path.dirname(os.path.abspath(__file__))
M = list(csv.DictReader(open(os.path.join(H, '..', 'data', 'module-status.csv'), encoding='utf-8')))
C = list(csv.DictReader(open(os.path.join(H, '..', 'data', 'chapter-status.csv'), encoding='utf-8')))
lab = {'완비': '완비', '일부': '일부', '문서': '문서만', '없음': '없음', '해당 없음': '해당 없음'}
cnt = Counter(m['status'] for m in M)
o = []; a = o.append
q = lambda x: x.replace('|', '/')
a('# 아두이노 모듈 현황 (구조화·순서화)\n')
a('<!-- nav -->[저장소 홈](../README.md) › [devices](README.md)\n')
a('이 문서는 `tools/gen_status.py` 가 `data/module-status.csv`(모듈 %d종)와 `data/chapter-status.csv`(교재 장)에서 만든 표입니다. 두 CSV 는 대시보드와 같은 자료입니다. 상태는 사람이 저장소 파일 유무를 대조해 적었고(근거 등급: 계산), 아래 §1 기준을 씁니다.\n' % len(M))
a('## 1. 판정 기준 (위에서 아래 순서로 따짐)\n')
a('| 순서 | 상태 | 조건 |\n|---|---|---|')
a('| 1 | 완비 | 소자 문서 + 드라이버 C + 그 드라이버를 부르는 호스트 시험이 모두 있음 |')
a('| 2 | 일부 | 드라이버나 RTL 은 있으나 호스트 시험이 없음(드라이버가 없고 RTL 만 있는 경우 포함), 또는 시험은 있으나 변형 검사·사실표 대조를 아직 안 함 |')
a('| 3 | 문서만 | 문서(또는 조사)만 있고 전용 드라이버·시험 없음 |')
a('| 4 | 없음 | 문서도 드라이버도 없음 |\n')
a('**「완비」도 한계가 있습니다**: 호스트 시험은 직접 만든 모의 장치를 상대로 한 것이라 데이터시트 해석이 틀리면 시험도 같이 틀립니다. 보드 실행과 Verilog 시뮬레이션은 하나도 없고, 판정 기준에도 들어 있지 않습니다(RTL 이 있는 모듈은 메모에 「RTL 시뮬레이션 전」으로 적음). 「완비」는 "저장소 안에서 할 수 있는 검증을 마쳤다"는 뜻이지 "동작이 확인됐다"는 뜻이 아닙니다.\n')
a('## 2. 요약\n')
a('| 완비 | 일부 | 문서만 | 없음 | 합계 |\n|---|---|---|---|---|\n| %d | %d | %d | %d | %d |\n' % (cnt['완비'], cnt['일부'], cnt['문서'], cnt['없음'], len(M)))
a('## 3. 인터페이스 묶음별 (기술 트리 순서)\n')
g = OrderedDict()
for m in M: g.setdefault(m['group'], []).append(m)
for k, v in g.items():
    c = Counter(x['status'] for x in v)
    a('### %s — 완비 %d · 일부 %d · 문서만 %d · 없음 %d\n' % (k, c['완비'], c['일부'], c['문서'], c['없음']))
    a('| ID | 모듈 | 단계 | 교재 장 | 상태 | 경로 | 메모 |\n|---|---|---|---|---|---|---|')
    for x in v: a('| %s | %s | %s | %s | %s | %s | %s |' % (x['id'], x['name'], x['stage'], x['book_ch'], lab[x['status']], ('`%s`' % x['where']) if x['where'] else '', q(x['note'])))
    a('')
a('## 4. 교재 장별\n')
a('| 장 | 주제 | 상태 | 경로 | 메모 |\n|---|---|---|---|---|')
for x in C: a('| %s | %s | %s | %s | %s |' % (x['chapter'], x['topic'], lab[x['status']], ('`%s`' % x['where']) if x['where'] else '', q(x['note'])))
a('')
a('## 5. 빈틈을 메우는 순서\n')
a('교재 장이 있고 기초 단계인 것을 먼저, 그다음 교재 장이 있는 것, 그다음 기초 단계, 마지막으로 나머지 순입니다. 같은 점수에서는 「일부」 → 「문서만」 → 「없음」 순입니다.\n')
st = {'기초': 2, '응용': 1, '심화': 0}
gap = [m for m in M if m['status'] != '완비']
sc = lambda m: (3 if m['book_ch'] else 0) + st.get(m['stage'], 0)
gap.sort(key=lambda m: (-sc(m), {'일부': 0, '문서': 1, '없음': 2}[m['status']], m['id']))
a('| 순서 | ID | 모듈 | 상태 | 단계 | 교재 장 | 메모 |\n|---|---|---|---|---|---|---|')
for i, m in enumerate(gap, 1): a('| %d | %s | %s | %s | %s | %s | %s |' % (i, m['id'], m['name'], lab[m['status']], m['stage'], m['book_ch'], q(m['note'])))
a('')
a('## 6. 상태 판정을 바로잡은 이력\n')
a('1. 처음 정리본은 「완비」를 30종으로 셌습니다. 드라이버 C 가 시험 파일에서 한 번이라도 쓰이는지 대조하자 `ds18b20.c`·`dht22.c`·`ultrasonic.c` 에 호스트 시험이 없었고(Verilog tb 만 있고 한 번도 돌리지 못함), HC-SR501·릴레이는 전용 드라이버·시험이 없었으며, microSD·ENC28J60 은 조사 문서가 있는데 「없음」으로 세었습니다 → 완비 25종으로 내림 (VERIFICATION.md 행 36).')
a('2. 이어서 세 드라이버를 다시 쓰고 모의 장치 시험·변형 검사·사실표를 갖춰 「완비」로 올렸습니다 (행 37~39). 다시 쓰기 전 드라이버에는 결함이 12건 있었습니다(DS18B20 변환 대기 생략으로 +85 °C 를 읽음, CRC 미검사, 폴링 상한 없음 등).')
a('3. L298(P02) 드라이버를 다시 읽다가 RTL 의 DUTY 레지스터가 16비트인데 기본 주기(100000 클럭)에서 66 % 넘는 듀티가 잘려 나가는 결함을 찾았습니다(모의 레지스터가 32비트로 저장해 기존 시험이 놓침). RTL 을 32비트로 넓히고 드라이버에 읽어 확인하는 단계를 넣었습니다 → 완비로 올림 (행 40). 「일부」 정의에서 「시뮬레이션·보드 확인 전」 조항을 뺐습니다: 앞서 DS18B20·DHT22·HC-SR04 를 같은 조건으로 완비로 올렸으므로 기준이 서로 어긋나 있었습니다.\n4. SG90(P01) 드라이버와 기본 시험을 추가했으나 사용자 요청(「검증은 나중」)으로 변형 검사·사실표 대조를 하지 않아 「일부」로 둡니다(행 41).\n5. MCP2515 가 S11 과 F02 로 두 번 들어 있습니다(중복). F02 메모에 표시했고 개수는 그대로 둡니다.\n')
sys.stdout.write('\n'.join(o))
