#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
command -v iverilog >/dev/null || { echo "Install iverilog first"; exit 2; }
command -v vvp >/dev/null || { echo "Install vvp first"; exit 2; }
count=0
failed=0
for tb in rtl/rtl*/tb/tb_*.v; do
  dir="${tb%/tb/*}"
  files=( "${dir}"/src/*.v )
  # Reused RTL components are external to the current lesson.
  case "${dir}" in
    rtl/rtl05_xadc_pwm) files+=(rtl/rtl04_pwm_led/src/pwm_core.v) ;;
    rtl/rtl07_buzzer_melody) files+=(rtl/rtl06_timer_led/src/switch_sync.v) ;;
  esac
  top="$(basename "${tb%.v}")"
  exe="$(mktemp)"
  echo "== ${top} =="
  if iverilog -g2012 -Wall -s "${top}" -o "${exe}" "${files[@]}" "${tb}" && vvp "${exe}"; then
    echo "OK ${top}"
  else
    echo "FAIL ${top}"
    failed=$((failed+1))
  fi
  rm -f "${exe}"
  count=$((count+1))
done
echo "Total: ${count}, FAIL: ${failed}"
if (( failed > 0 )); then exit 1; fi
