# 1-Wire

<!-- nav -->[저장소 홈](../../README.md) › [devices](../README.md)

AMD IP 없음(열람 범위) → 전용 RTL. 구분 표기는 [../README.md](../README.md) 의 "근거 수준".

| 항목 | 위치 |
|---|---|
| DS18B20 온도 센서 사양 | [ds18b20.md](ds18b20.md) |
| 비트 엔진 + AXI4-Lite 레지스터 RTL (미시뮬레이션) | [rtl/README.md](rtl/README.md) |
| 테스트벤치 (미실행) | [tb/tb_onewire_engine.v](tb/tb_onewire_engine.v) |
| 드라이버 + 호스트 시험 | [sw/ds18b20.c](sw/ds18b20.c), [tb/test_ds18b20.c](tb/test_ds18b20.c) |

<!-- nav-foot -->← [gpio](../gpio/README.md) · [devices 목차](../README.md) · [i2c](../i2c/README.md) →
