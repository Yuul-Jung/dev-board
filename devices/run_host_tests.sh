#!/usr/bin/env bash
# 호스트(gcc, python3)에서 모의 레지스터로 실행되는 테스트를 모두 돌린다. Verilog 는 포함하지 않는다(시뮬레이터 필요).
# SAN=1 : ASan+UBSan 으로 빌드.  FUZZ=1 : 퍼저·전수 시험 추가(수 초~수십 초).
set -u
cd "$(dirname "$0")"
fail=0
run() { local name="$1"; shift; if out=$("$@" 2>&1) && grep -q "ALL PASS" <<<"$out"; then echo "PASS  $name"; else echo "FAIL  $name"; echo "$out" | tail -5; fail=1; fi; }
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
SANF=""; [ "${SAN:-0}" = 1 ] && SANF="-g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all"
cc() { gcc -Wall -Wextra $SANF "$@" ; }
printf 'void iic_delay_ms(unsigned ms){(void)ms;}\n' > "$T/stub.c"

cc -include i2c/tb/mock_iic.h -I i2c/sw i2c/tb/test_axi_iic_mock.c i2c/sw/axi_iic.c i2c/sw/i2c_sensors.c -o "$T/i2c" && run i2c "$T/i2c"
cc -I i2c/sw i2c/tb/test_bme280.c i2c/sw/bme280.c i2c/sw/axi_iic.c "$T/stub.c" -o "$T/bme" && run bme280 "$T/bme"
cc -I i2c/sw i2c/tb/test_rtc_eeprom.c i2c/sw/rtc_eeprom.c -o "$T/rtce" && run rtc-eeprom "$T/rtce"
cc -I i2c/sw i2c/tb/test_io_expander.c i2c/sw/io_expander.c -o "$T/ioe" && run io-expander "$T/ioe"
cc -I i2c/sw i2c/tb/test_ads1115.c i2c/sw/ads1115.c -o "$T/ads" && run ads1115 "$T/ads"
cc -I i2c/sw i2c/tb/test_ssd1306.c i2c/sw/ssd1306.c -o "$T/oled" && run ssd1306 "$T/oled"
cc -I spi/sw spi/tb/test_spi_sensors.c spi/sw/spi_sensors.c -o "$T/spisen" && run spi-sensors "$T/spisen"
cc -include uart/tb/mock_uart.h -I uart/sw uart/tb/test_uart_mock.c uart/sw/uartlite.c uart/sw/nmea.c uart/sw/hc05.c -o "$T/uart" && run uart "$T/uart"
cc -include can/tb/mock_can.h -I can/sw can/tb/test_can_mock.c can/sw/axi_can.c -o "$T/can" && run can "$T/can"
cc -include spi/tb/mock_qspi.h -I spi/sw spi/tb/test_qspi_mock.c spi/sw/axi_qspi.c spi/sw/spi_devices.c -o "$T/qspi" && run qspi "$T/qspi"
cc -include spi/tb/mock_lcd.h -I spi/sw spi/tb/test_ili9341_sw.c spi/sw/ili9341.c -o "$T/lcd" && run ili9341 "$T/lcd"
cc -include gpio/tb/mock_timer.h -I gpio/sw gpio/tb/test_gpio_sw.c gpio/sw/axi_timer_pwm.c -o "$T/pwm" && run timer-pwm "$T/pwm"
cc -include gpio/tb/mock_timer.h -I gpio/sw gpio/tb/test_tach.c gpio/sw/axi_timer_tach.c -o "$T/tach" && run timer-tach "$T/tach"
cc -I ps2/sw ps2/tb/test_ps2_sw.c ps2/sw/ps2.c -o "$T/ps2" && run ps2-sw "$T/ps2"
run ps2-model python3 ps2/tb/model_ps2_rx.py
run quad-model python3 gpio/tb/model_quad.py
run links python3 tools/check_links.py
if [ "${FUZZ:-0}" = 1 ]; then
  [ "${SAN:-0}" = 1 ] || SANF="-g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all"
  cc -I i2c/sw i2c/tb/fuzz_bme280.c i2c/sw/bme280.c i2c/sw/axi_iic.c "$T/stub.c" -o "$T/f1" && run fuzz-bme280 "$T/f1"
  cc -I uart/sw uart/tb/fuzz_nmea.c uart/sw/nmea.c -o "$T/f2" && run fuzz-nmea "$T/f2"
  cc -I can/sw can/tb/exhaustive_timing.c can/sw/axi_can.c -o "$T/f3" && run can-timing "$T/f3"
fi
exit $fail
