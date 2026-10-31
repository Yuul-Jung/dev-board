#!/usr/bin/env bash
# Verilator 정적 검사(--lint-only -Wall) — 과제별로 src 만 검사, 경고 종류별 개수를 모은다.
# 실패(종료 1): LATCH, MULTIDRIVEN, UNOPTFLAT, BLKANDNBLK, COMBDLY, 그리고 조건식 아닌 WIDTHTRUNC(대입 잘림). 그 밖의 경고(WIDTH 등)는 보고만 한다.
set -uo pipefail
cd "$(dirname "$0")/../.."
mkdir -p rtl/verification/results_lint
out=rtl/verification/results_lint/lint.txt; : > "$out"
sev=0; : > /tmp/all_warn.txt
for d in rtl/rtl*/; do
  n=$(basename "$d"); files=("$d"src/*.v)
  case "$n" in rtl05_xadc_pwm) files+=(rtl/rtl04_pwm_led/src/pwm_core.v);; rtl07_buzzer_melody) files+=(rtl/rtl06_timer_led/src/switch_sync.v);; esac
  verilator --lint-only -Wall -Wno-fatal -Wno-MULTITOP -Wno-DECLFILENAME -Wno-TIMESCALEMOD -Wno-UNUSEDSIGNAL -Wno-UNUSEDPARAM -Wno-PINCONNECTEMPTY "${files[@]}" > /tmp/l.txt 2>&1
  grep -E '^%(Warning|Error)' /tmp/l.txt | sed "s|^|$n |" >> /tmp/all_warn.txt
done
echo "== 경고 종류별 개수 ==" >> "$out"
sed -E 's/^[^ ]+ %(Warning|Error)-?([A-Z0-9_]*).*/\2/' /tmp/all_warn.txt | sort | uniq -c | sort -rn >> "$out"
echo "== 중대 경고(실패 대상) ==" >> "$out"
grep -E '%(Warning|Error)-(LATCH|MULTIDRIVEN|UNOPTFLAT|BLKANDNBLK|COMBDLY)' /tmp/all_warn.txt >> "$out" && sev=1
# 대입 잘림(WIDTHTRUNC) 중 「조건식 폭」 경고(파라미터를 조건으로 쓴 것)는 제외하고 실패 처리 - RTL08/09 의 상태 폭 결함 유형
grep -E '%Warning-WIDTHTRUNC' /tmp/all_warn.txt | grep -v 'Conditional Test' >> "$out" && sev=1
grep -E '%Error' /tmp/all_warn.txt | head -5 >> "$out"
echo "== 전체 경고 ==" >> "$out"; cat /tmp/all_warn.txt >> "$out"
exit $sev
