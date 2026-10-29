# I2S 소자와 SoC 연결

구분: **확인** = 문서를 열어 확인, **계산**, **설계안**, **미확인**.

## AMD IP: I2S Transmitter / Receiver (PG308) — 일부 확인
이전 문서들의 "열람한 AMD IP 없음"은 틀렸습니다. 이번에 PG308 페이지를 열어 IP가 존재함을 확인했고 해당 문구를 고쳤습니다.

| 항목 | 값 | 구분 |
|---|---|---|
| 인터페이스 | AXI4-Stream, 레지스터는 별도(AXI4-Lite 추정, 문서에 명시 없음) | 부분 확인 |
| 채널 | I2S 선 최대 4개, 오디오 채널 최대 8 | 확인 |
| 데이터 폭 | 16비트 또는 24비트 | 확인 |
| 형식 | 표준 I2S, 좌/우 정렬 | 확인 |
| 모드 | **마스터 I2S 모드**(슬레이브는 언급 없음) | 확인 |
| 기타 | FIFO 깊이 설정, AES 채널 상태, 선택적 32비트 LRCLK | 확인 |
| 샘플링 속도 | "어떤 샘플링 속도든 지원" (구체 범위 없음) | 확인(문구) |
| 레지스터 오프셋 | 0x00 Core Version · 0x04 Core Configuration · 0x08 Control · 0x0C Validity · 0x10 Interrupt Control · 0x14 Interrupt Status · 0x20 I2S Timing Control · 0x30/0x34/0x38/0x3C 채널 0/1~6/7 Control · 0x50~0x64 AES Channel Status | 확인(목차의 이름·오프셋만, 비트 필드는 미확인) |
| 지원 디바이스 계열(Artix-7 포함 여부), 라이선스, 포트 이름, 자원 사용량, 클럭 구조 | 문서 본문 못 열음 | **미확인** |

**결정**: 사용자 규칙(AMD IP 있으면 IP)에 따라 IP를 우선합니다. 단 Artix-7 지원과 라이선스는 Vivado IP 카탈로그에서 확인해야 합니다. 카탈로그에 없거나 쓸 수 없으면 그때 전용 수신 RTL(INMP441용)을 작성합니다. 지금은 RTL을 쓰지 않았습니다.
마스터 전용이므로 INMP441, MAX98357A 처럼 BCLK/LRCLK 를 받는 슬레이브 소자에 맞습니다. 마스터인 코덱과는 맞지 않습니다.

## 클럭 계산
Basys3 는 클럭 관리 타일 5개(RM, 확인). BCLK 는 100 MHz 를 정수로 나눠 만들 수 없으므로 MMCM/클럭 위저드가 필요합니다(계산).
- INMP441: fSCK = 64 × fWS (확인). 48 kHz → 3.072 MHz(문서 예시), 16 kHz → 1.024 MHz. 100 MHz / 3.072 MHz ≈ 32.55 이므로 정수 분주 불가(계산).
- IP가 BCLK 를 자체 생성하는지 외부에서 받는지는 미확인.

## 소자
[devices.md](devices.md): INMP441(마이크), MAX98357A(앰프).

## 핀 (설계안)
소자당 3선(BCLK, LRCLK, 데이터) + 선택 핀(INMP441 L/R, MAX98357A SD_MODE·GAIN_SLOT). Pmod 한 포트(8신호)에 마이크 1 + 앰프 1 가능.
