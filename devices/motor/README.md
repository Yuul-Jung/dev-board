# 모터·서보·스테퍼

<!-- nav -->[저장소 홈](../../README.md) › [devices](../README.md)

| 소자 | 문서 | 비고 |
|---|---|---|
| L298 | [l298.md](l298.md) | DC 모터 드라이버 |
| TB6612FNG | [tb6612fng.md](tb6612fng.md) | DC 모터 드라이버 |
| SG90 서보 | [sg90.md](sg90.md) | 2차 자료(출처마다 값이 다름) |
| 28BYJ-48 + ULN2003 | [28byj-48.md](28byj-48.md) | 스테퍼, GPIO 4핀 — 드라이버 [stepper.c](sw/stepper.c)(풀·하프·웨이브, 비차단 이동, 각도), 호스트 시험 |

구현: [rtl/README.md](rtl/README.md) (DC 모터, 서보 PWM 전용 RTL — 미시뮬레이션), 테스트벤치 [tb/](tb/), 드라이버 [sw/](sw/). 서보는 AXI Timer PWM 대안도 있음([../gpio/fan-pwm.md](../gpio/fan-pwm.md) 의 PG079 요약). 구분 표기는 [../README.md](../README.md).

<!-- nav-foot -->← [ps2](../ps2/README.md) · [devices 목차](../README.md)
