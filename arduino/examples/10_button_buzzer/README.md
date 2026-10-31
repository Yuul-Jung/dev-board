# 10차시 — 버튼과 수동 부저

[Arduino 목차](../../README.md) · [32차시 테크트리](../../ROADMAP.md)

## 개요
버튼과 수동 부저를 Arduino UNO R4의 입출력으로 구현한다. 기본 예제에서 확인한 입력·출력 개념을 이어서 사용한다.

## 준비 및 배선
- 보드: Arduino UNO R4 Minima 또는 WiFi. 실제 보드에 맞게 Arduino IDE 보드와 포트를 선택한다.
- 배선: D2 버튼-GND, D9 수동 피에조 부저 신호, 부저 GND 공통
- 주의: 수동 피에조(소전류) 부저. 능동 부저는 tone 출력 목적에 맞지 않습니다.

## 전체 코드
[10_button_buzzer.ino](10_button_buzzer.ino) — 이 폴더의 스케치 원본

## 실행 절차
1. Arduino IDE에서 `10_button_buzzer.ino`를 열고 보드·포트를 선택한다.
2. 필요한 라이브러리를 설치하고 스케치를 검증(컴파일)한 뒤 업로드한다.
3. 배선을 확인하고 입력을 변화시키면서 출력 반응을 관찰한다.
4. Serial 출력이 있는 차시는 115200 baud로 시리얼 모니터를 연다.

## 예상 결과
버튼을 누르면 440Hz 음이 나고 놓으면 멈춥니다.

## 검증 상태
- 코드 작성: 완료
- Arduino IDE 컴파일: 미실행
- UNO R4 실물 시험: 미실행
- 실제 모듈별 전압·핀 배치 대조: 시험 전 확인 필요

## 개정 메모 (2026-10-31 검토)
- 32차시 표([ROADMAP](../../ROADMAP.md))는 이 차시를 「능동·수동 부저 구분 및 멜로디」로 적었으나 **현재 스케치는 440 Hz 한 음만 냅니다**(구분 설명·멜로디 없음). 부저 사양은 [devices/gpio/buzzer.md](../../../devices/gpio/buzzer.md)에 미확인으로 남아 있고, 멜로디는 [RTTTL 문서](../../../devices/gpio/rtttl.md)를 참고할 수 있습니다. 범위를 맞출지(스케치 확장 또는 ROADMAP 문구 수정)는 미결정.

**공통 모듈 자료**: [devices](../../../devices/README.md) (해당 자료의 RTL/C 코드는 Arduino 스케치가 아님).

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [10_button_buzzer.ino](10_button_buzzer.ino) |  |

<!-- index:end -->
