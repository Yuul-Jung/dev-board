#!/usr/bin/env python3
"""간이 폭 검사(시뮬레이터 없이 소스만 읽음). 사용: python3 rtl/verification/lint_widths.py [루트]
잡는 것: (1) reg 폭보다 큰 리터럴을 대입(상태 번호 잘림 등), (2) ANSI 포트 목록에서 앞 [N:0] 폭이 뒤 이름들에 이어지는 경우.
한계: 문자열 매칭이라 식·파라미터가 들어간 폭은 건너뜀. 통과가 정상 동작을 뜻하지는 않음."""
import re, sys, glob, os
root = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..')
def strip(s):
    s = re.sub(r'/\*.*?\*/', '', s, flags=re.S); return re.sub(r'//[^\n]*', '', s)
bad = 0
for f in sorted(glob.glob(os.path.join(root, 'rtl', 'rtl*', 'src', '*.v'))):
    s = strip(open(f, encoding='utf-8').read()); rel = os.path.relpath(f, root)
    width = {}
    for m in re.finditer(r'\breg\s*(?:signed\s*)?(?:\[(\d+)\s*:\s*(\d+)\])?\s*([^;]+);', s):
        w = (abs(int(m.group(1)) - int(m.group(2))) + 1) if m.group(1) else 1
        for n in re.findall(r'\b([A-Za-z_]\w*)\b(?:\s*=\s*[^,;]+)?', re.sub(r'\[[^\]]*\]', '', m.group(3))):
            if n not in ('reg', 'wire', 'signed'): width[n] = w
    for m in re.finditer(r'\b([A-Za-z_]\w*)\s*(?:<=|=)\s*(?:(\d+)\'[dD]\s*)?(\d+)\s*[;\s]', s):
        n = m.group(1)
        if n not in width or m.group(2): continue
        v = int(m.group(3))
        if v >= 2 ** width[n]:
            print(f'{rel}: {n} 는 {width[n]}비트인데 {v} 를 대입(잘림)'); bad += 1
    for m in re.finditer(r'(?:input|output)(?:\s+(?:wire|reg|signed))*\s*\[[^\]]+\]\s*\w+(?:\s*,\s*(?!(?:input|output|inout)\b)\w+)+', s):
        names = re.findall(r'\w+', re.sub(r'\[[^\]]*\]', '', m.group(0)))[1:]
        names = [n for n in names if n not in ('wire', 'reg', 'signed')]
        if len(names) > 1:
            print(f'참고 {rel}: 폭이 이어지는 포트 목록(의도 확인): {", ".join(names)}')
print('검사 끝, 지적', bad)
sys.exit(1 if bad else 0)
