# I2S 소자: INMP441, MAX98357A

<!-- nav -->[저장소 홈](../../README.md) › [devices](../README.md) › [i2s](README.md)

## INMP441 (I2S MEMS 마이크) — D1
| 항목 | 값 |
|---|---|
| 전원 | 표 1: 1.62~3.63 V, 핀 표/시험 조건: 1.8~3.3 V (문서 내 두 범위) |
| 데이터 | I2S 24비트 2의 보수, MSB 먼저, 채널당 32 SCK 사이클 |
| 클럭 | fSCK 0.5~3.2 MHz, fSCK = 64 × fWS, fWS 7.8~50 kHz |
| L/R 핀 | Low = 왼쪽 채널, High = 오른쪽 채널 |
| SD | 비구동 구간 3상태(권장 100 kΩ 풀다운) |
| 타이밍 | 각 하프 프레임 시작에서 MSB가 SCK 1사이클 지연 (표준 I2S) |
| 감도·SNR | -26 dBFS (94 dB SPL, 1 kHz), SNR 61 dBA 전형 |
| 시동 | 전원 후 218 SCK 클럭 동안 출력 0 (3.072 MHz에서 85 ms). 다른 절에는 217 사이클이라 적혀 문서 내 불일치 |

SoC (설계안): PG308 수신기(마스터)가 BCLK/LRCLK 를 공급, 마이크 SD 를 입력. 모노는 L/R 핀 고정. 24비트 중 상위 비트부터 쓰면 16비트 처리 가능(설계안).
시동 구간 출력 0 을 버리도록 소프트웨어가 첫 수십 ms 샘플을 무시.

## MAX98357A (I2S D급 앰프) — D1
| 항목 | 값 |
|---|---|
| 전원 | 2.5~5.5 V |
| 디지털 핀 | DIN, BCLK, LRCLK, SD_MODE, GAIN_SLOT. 외부 MCLK 불필요 |
| 샘플 속도 | 8 kHz~96 kHz |
| 형식 | I2S (좌 정렬은 MAX98357B). 비트 16/24/32 (TDM은 16/32) |
| SD_MODE | High=왼쪽, RSMALL 풀업=오른쪽, RLARGE 풀업=(L+R)/2 모노, Low=셧다운 |
| GAIN_SLOT (I2S 모드) | 100 kΩ→GND 15 dB · GND 12 dB · 개방 9 dB · VDD 6 dB · 100 kΩ→VDD 3 dB |
| 출력 | 4 Ω, 5 V에서 3.2 W (THD 10%), 1% THD에서 2.5 W |

SoC (설계안): PG308 송신기(마스터) → DIN. SD_MODE 는 GPIO(셧다운 제어) 또는 고정. 스피커 전원 5 V 는 FPGA 3.3 V 와 분리. 디지털 입력 레벨(VIH)은 열어본 부분에 없어 3.3 V 구동 적합성 미확인.
