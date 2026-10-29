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
cc -I i2c/sw i2c/tb/test_hd44780.c i2c/sw/hd44780.c i2c/sw/hd44780_pcf8574.c -o "$T/hd44780" && run hd44780-model "$T/hd44780"
cc -I spi/sw spi/tb/test_mcp2515_model.c spi/sw/mcp2515.c -o "$T/mcm" && run mcp2515-model "$T/mcm"
cc -I spi/sw spi/tb/test_nrf24_model.c spi/sw/nrf24l01.c -o "$T/nrf" && run nrf24-model "$T/nrf"
cc -I spi/sw spi/tb/test_w5500_model.c spi/sw/w5500.c -o "$T/w55" && run w5500-model "$T/w55"
cc -I spi/sw -I ethernet/sw ethernet/tb/test_ntp.c ethernet/sw/ntp.c ethernet/sw/ntp_client.c spi/sw/w5500.c -o "$T/ntp" && run ntp-model "$T/ntp"
cc -I spi/sw -I ethernet/sw ethernet/tb/test_dns.c ethernet/sw/dns.c ethernet/sw/dns_client.c ethernet/sw/net_name.c ethernet/sw/ntp.c ethernet/sw/ntp_client.c ethernet/sw/http.c ethernet/sw/http_client.c spi/sw/w5500.c -o "$T/dns" && run dns-model "$T/dns"
cc -I spi/sw -I ethernet/sw ethernet/tb/test_http.c ethernet/sw/http.c ethernet/sw/http_client.c spi/sw/w5500.c -o "$T/http" && run http-model "$T/http"
cc -I spi/sw spi/tb/test_max7219_model.c spi/sw/max7219.c -o "$T/m72" && run max7219-model "$T/m72"
cc -I spi/sw spi/tb/test_mcp2515.c spi/sw/mcp2515.c -o "$T/mcp2515" && run mcp2515 "$T/mcp2515"
cc -I spi/sw spi/tb/test_hc595.c spi/sw/spi_devices.c -o "$T/hc595" && run hc595 "$T/hc595"
cc -I spi/sw spi/tb/test_spi_sensors.c spi/sw/spi_sensors.c -o "$T/spisen" && run spi-sensors "$T/spisen"
cc -include uart/tb/mock_uart.h -I uart/sw uart/tb/test_uart_mock.c uart/sw/uartlite.c uart/sw/nmea.c uart/sw/hc05.c -o "$T/uart" && run uart "$T/uart"
cc -include can/tb/mock_can.h -I can/sw can/tb/test_can_mock.c can/sw/axi_can.c -o "$T/can" && run can "$T/can"
cc -include spi/tb/mock_qspi.h -I spi/sw spi/tb/test_qspi_mock.c spi/sw/axi_qspi.c spi/sw/spi_devices.c -o "$T/qspi" && run qspi "$T/qspi"
cc -include spi/tb/mock_lcd.h -I spi/sw spi/tb/test_ili9341_sw.c spi/sw/ili9341.c -o "$T/lcd" && run ili9341 "$T/lcd"
cc -include gpio/tb/mock_timer.h -I gpio/sw gpio/tb/test_gpio_sw.c gpio/sw/axi_timer_pwm.c -o "$T/pwm" && run timer-pwm "$T/pwm"
cc -include gpio/tb/mock_timer.h -I gpio/sw gpio/tb/test_tach.c gpio/sw/axi_timer_tach.c -o "$T/tach" && run timer-tach "$T/tach"
cc -include gpio/tb/mock_gpio.h -I gpio/sw gpio/tb/test_axi_gpio.c gpio/sw/axi_gpio.c -o "$T/agpio" && run axi-gpio-model "$T/agpio"
cc -include gpio/tb/mock_timer_b.h -I gpio/sw gpio/tb/test_basic_io.c gpio/sw/basic_io.c gpio/sw/axi_timer_pwm.c -o "$T/bio" -lm && run basic-io-model "$T/bio"
cc -I gpio/sw gpio/tb/test_analog_sensors.c gpio/sw/analog_sensors.c -o "$T/anl" -lm && run analog-sensors "$T/anl"
cc -I ps2/sw ps2/tb/test_ps2_sw.c ps2/sw/ps2.c -o "$T/ps2" && run ps2-sw "$T/ps2"
run ps2-model python3 ps2/tb/model_ps2_rx.py
run quad-model python3 gpio/tb/model_quad.py
run links python3 tools/check_links.py
run crosscheck-facts bash -c 'python3 -I tools/crosscheck_pdf.py --facts-only >/dev/null && echo ALL PASS'
if [ "${FUZZ:-0}" = 1 ]; then
  [ "${SAN:-0}" = 1 ] || SANF="-g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all"
  cc -I i2c/sw i2c/tb/fuzz_bme280.c i2c/sw/bme280.c i2c/sw/axi_iic.c "$T/stub.c" -o "$T/f1" && run fuzz-bme280 "$T/f1"
  cc -I uart/sw uart/tb/fuzz_nmea.c uart/sw/nmea.c -o "$T/f2" && run fuzz-nmea "$T/f2"
  cc -I can/sw can/tb/exhaustive_timing.c can/sw/axi_can.c -o "$T/f3" && run can-timing "$T/f3"
  cc -I spi/sw -I ethernet/sw ethernet/tb/fuzz_http.c ethernet/sw/http.c -o "$T/f4" && run fuzz-http "$T/f4"
  cc -I spi/sw -I ethernet/sw ethernet/tb/fuzz_dns.c ethernet/sw/dns.c -o "$T/f5" && run fuzz-dns "$T/f5"
fi
exit $fail
