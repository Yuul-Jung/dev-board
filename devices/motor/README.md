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

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 하위 폴더

| 폴더 | 내용 |
|---|---|
| [ref/](ref/README.md) | 근거 자료와 사실표(교차 검증 대상) |
| [rtl/](rtl/README.md) | Verilog RTL 과 RTL 테스트벤치(시뮬레이터 없음, 미실행) |
| [sw/](sw/README.md) | 드라이버 C 소스 |
| [tb/](tb/README.md) | 호스트 테스트와 레지스터/장치 모델(mock) |

### 파일

| 파일 | 설명 |
|---|---|
| [28byj-48.md](28byj-48.md) | 28BYJ-48 스테퍼 모터 + ULN2003 드라이버 보드 |
| [l298.md](l298.md) | L298 듀얼 풀 브리지 드라이버 |
| [sg90.md](sg90.md) | SG90 마이크로 서보 |
| [tb6612fng.md](tb6612fng.md) | TB6612FNG 듀얼 DC 모터 드라이버 |

<!-- index:end -->
