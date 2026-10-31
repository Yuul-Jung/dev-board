#!/usr/bin/env bash
set -uo pipefail
cd "$(dirname "$0")/../../.."
mkdir -p rtl/verification/rtl/results
summary="rtl/verification/rtl/results/rtl-summary.tsv"
printf 'test\tstatus\tlog\n' > "$summary"
failed=0
total=0
for tb in rtl/rtl*/tb/tb_*.v; do
  dir="${tb%/tb/*}"
  top="$(basename "${tb%.v}")"
  files=("${dir}"/src/*.v)
  case "$dir" in
    rtl/rtl05_xadc_pwm) files+=(rtl/rtl04_pwm_led/src/pwm_core.v) ;;
    rtl/rtl07_buzzer_melody) files+=(rtl/rtl06_timer_led/src/switch_sync.v) ;;
  esac
  log="rtl/verification/rtl/results/${top}.log"
  out="rtl/verification/rtl/results/${top}.vvp"
  if iverilog -g2012 -Wall -s "$top" -o "$out" "${files[@]}" "$tb" >"$log" 2>&1 && vvp "$out" >>"$log" 2>&1; then
    printf '%s\tPASS\t%s\n' "$top" "$log" >> "$summary"
  else
    printf '%s\tFAIL\t%s\n' "$top" "$log" >> "$summary"
    failed=$((failed+1))
    # 로그 파일을 못 내려받는 환경에서도 원인을 읽을 수 있게 실패 로그 앞부분을 주석(annotation)으로 남긴다.
    if [ "$failed" -le 10 ]; then
      msg="$(head -c 1800 "$log" | sed ':a;N;$!ba;s/%/%25/g;s/\r/%0D/g;s/\n/%0A/g')"
      printf '::error title=FAIL %s::%s\n' "$top" "$msg"
    fi
  fi
  rm -f "$out"
  total=$((total+1))
done
echo "RTL testbenches=$total failures=$failed"
if [ "$total" -ne 32 ]; then echo "Expected 32 RTL tests"; exit 3; fi
[ "$failed" -eq 0 ]
