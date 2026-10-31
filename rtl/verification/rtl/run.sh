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
  fi
  rm -f "$out"
  total=$((total+1))
done
echo "RTL testbenches=$total failures=$failed"
if [ "$total" -ne 32 ]; then echo "Expected 32 RTL tests"; exit 3; fi
[ "$failed" -eq 0 ]
