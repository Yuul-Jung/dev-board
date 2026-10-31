#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../../.."
: "${UVM_HOME:?Set UVM_HOME to Accellera UVM src path}"
command -v verilator
mkdir -p rtl/verification/uvm/results
LOG="rtl/verification/uvm/results/rtl03-uvm.log"
set -o pipefail
# 실패하면 로그 앞·뒤를 ::error 주석으로 남긴다(로그 파일을 내려받지 못하는 환경에서도 원인 확인).
esc() { sed ':a;N;$!ba;s/%/%25/g;s/\r/%0D/g;s/\n/%0A/g'; }
trap 'rc=$?; if [ "$rc" -ne 0 ] && [ -f "$LOG" ]; then printf "::error title=UVM head::%s\n" "$(head -c 2500 "$LOG" | esc)"; printf "::error title=UVM tail::%s\n" "$(tail -c 2500 "$LOG" | esc)"; fi' EXIT
verilator --binary --timing --assert -Wno-fatal -j 2 \
  --top-module tb_top --Mdir rtl/verification/uvm/results/obj_dir \
  +incdir+"${UVM_HOME}" +define+UVM_NO_DPI \
  "${UVM_HOME}/uvm_pkg.sv" \
  rtl/verification/uvm/uart_led_if.sv \
  rtl/verification/uvm/uart_led_uvm_pkg.sv \
  rtl/rtl03_uart_led/src/uart_led_controller.v \
  rtl/verification/uvm/tb_top.sv 2>&1 | tee "$LOG"
rtl/verification/uvm/results/obj_dir/Vtb_top +UVM_TESTNAME=led_test 2>&1 | tee -a "$LOG"
grep -q "UVM_SCOREBOARD_PASS" "$LOG"
if grep -E 'UVM_ERROR\s*:\s*[1-9]|UVM_FATAL\s*:\s*[1-9]' "$LOG"; then exit 1; fi
