# 11차시 — 74HC595와 7세그먼트

[Arduino 목차](../../README.md) · [32차시 테크트리](../../ROADMAP.md)

## 개요
74HC595와 7세그먼트를 Arduino UNO R4의 입출력으로 구현한다. 기본 예제에서 확인한 입력·출력 개념을 이어서 사용한다.

## 준비 및 배선
- 보드: Arduino UNO R4 Minima 또는 WiFi. 실제 보드에 맞게 Arduino IDE 보드와 포트를 선택한다.
- 배선: D2→74HC595 DS, D3→SHCP, D4→STCP; 74HC595 Q0~Q7→각 전류제한저항→7세그 a~g,dp; 공통 캐소드 GND. 74HC595 VCC=5V, GND 공통, /OE=GND, /MR=5V.
- 주의: 74HC595 출력 핀별·칩 전체 전류 정격을 확인합니다. 고휘도 디스플레이 또는 다자리 구동은 별도 드라이버가 필요합니다.

## 전체 코드
[11_hc595_7segment.ino](11_hc595_7segment.ino) — 이 폴더의 스케치 원본

## 실행 절차
1. Arduino IDE에서 `11_hc595_7segment.ino`를 열고 보드·포트를 선택한다.
2. 필요한 라이브러리를 설치하고 스케치를 검증(컴파일)한 뒤 업로드한다.
3. 배선을 확인하고 입력을 변화시키면서 출력 반응을 관찰한다.
4. Serial 출력이 있는 차시는 115200 baud로 시리얼 모니터를 연다.

## 예상 결과
0~9 숫자가 1초 간격으로 순환 표시됩니다.

## 검증 상태
- 코드 작성: 완료
- Arduino IDE 컴파일: 미실행
- UNO R4 실물 시험: 미실행
- 실제 모듈별 전압·핀 배치 대조: 시험 전 확인 필요

**공통 모듈 자료**: [devices](../../../devices/README.md) (해당 자료의 RTL/C 코드는 Arduino 스케치가 아님).

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [11_hc595_7segment.ino](11_hc595_7segment.ino) | Q0..Q6 -> a..g, Q7 -> dp, 공통 캐소드 기준 |

<!-- index:end -->
