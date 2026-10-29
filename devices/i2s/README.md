# I2S 소자와 SoC 연결

구분: **확인** = 문서를 열어 확인, **계산**, **설계안**, **미확인**.

## AMD IP: I2S Transmitter / Receiver (PG308 v1.0, 2025-08-12) — 확인 (문서 본문 열람)
| 항목 | 값 |
|---|---|
| 디바이스 | 지원 계열에 **7 series** 포함 |
| 라이선스 | **추가 비용 없이 Vivado에 포함** (End User License) |
| 인터페이스 | AXI4-Lite(제어), AXI4-Stream(오디오), AXI4 |
| 채널·폭 | I2S 선 최대 4개, 최대 8채널, 데이터 16 또는 24비트 (20비트 미지원) |
| 형식 | 표준 I2S, 좌/우 정렬, 선택적 32비트 LRCLK |
| 모드 | **마스터만. 슬레이브 모드 미지원** (문서의 미지원 기능 목록) |
| 포트 (송신) | aud_mclk(입력, "보통 Fs의 배수"), aud_mrst, s_axis_aud_*, lrclk_out, sclk_out, sdata_0~3_out, irq |
| 포트 (수신) | aud_mclk, lrclk_out, sclk_out, sdata_0~3_in |
| SCLK 생성 | I2S Timing Control(0x20) 하위 8비트 divider: MCLK/SCLK = divider × 2. 일반적으로 SCLK = 2 × 24 × Fs (폭 16이면 2 × 16 × Fs) |
| LRCLK | 보통 SCLK / (2 × 폭). 좌/우 정렬 또는 32비트 LRCLK 선택 시 SCLK / (2×32) |
| 레지스터 | 0x00 Core Version · 0x04 Core Configuration · 0x08 Control · 0x0C Validity · 0x10 Interrupt Control · 0x14 Interrupt Status · 0x20 I2S Timing Control · 0x30/0x34/0x38/0x3C 채널 0/1~6/7 Control · 0x50~0x64 AES Channel Status |
| 디버깅 안내 | 소리가 안 나오면 ADC/DAC/코덱이 슬레이브인지 확인. IP는 마스터이며 16/24비트 I2S만 지원. 코덱 MCLK는 IP에 넣는 MCLK와 같아야 함 |

**결정**: AMD IP 사용(마스터). BCLK·LRCLK를 받는 슬레이브 소자(INMP441, MAX98357A)에 맞음. 마스터 코덱은 연결 불가. 전용 RTL 불필요.
INMP441은 64×fWS의 SCK와 채널당 32 클럭이 필요하므로 **24비트 + 32비트 LRCLK 모드**가 맞음(LRCLK = SCLK / 64, 문서 서술에서 도출. 이 조합의 동작은 실제 보드에서 확인 필요).

## 클럭 계산
Basys3 는 클럭 관리 타일 5개(RM, 확인). 오디오 클럭 aud_mclk 를 MMCM/클럭 위저드로 만들어 넣고, IP가 divider 로 SCLK 를 만듭니다(PG308 확인).
- INMP441: fSCK = 64 × fWS (확인). 48 kHz → SCLK 3.072 MHz(데이터시트 예시). MCLK 를 256×Fs = 12.288 MHz 로 잡으면 MCLK/SCLK = 4 = divider × 2 → divider 2 (계산, MCLK 선택은 설계안).
- 16 kHz 이면 SCLK 1.024 MHz, MCLK 256×Fs = 4.096 MHz → divider 2.
- 100 MHz → 12.288 MHz 를 MMCM 으로 정확히 만들 수 있는지(분수 분주 오차)는 이번에 계산하지 않았습니다(미확인).
- 100 MHz 를 단순 정수 분주해서는 3.072 MHz 를 만들 수 없음(100/3.072 ≈ 32.55, 계산).

## 소자
[devices.md](devices.md): INMP441(마이크), MAX98357A(앰프).

## 핀 (설계안)
소자당 3선(BCLK, LRCLK, 데이터) + 선택 핀(INMP441 L/R, MAX98357A SD_MODE·GAIN_SLOT). Pmod 한 포트(8신호)에 마이크 1 + 앰프 1 가능.
