# 14차시 — HC-SR04 초음파 거리

[Arduino 목차](../../README.md) · [32차시 테크트리](../../ROADMAP.md)

## 개요
HC-SR04 초음파 거리을 Arduino UNO R4의 입출력으로 구현한다. 기본 예제에서 확인한 입력·출력 개념을 이어서 사용한다.

## 준비 및 배선
- 보드: Arduino UNO R4 Minima 또는 WiFi. 실제 보드에 맞게 Arduino IDE 보드와 포트를 선택한다.
- 배선: HC-SR04 VCC→5V, GND 공통, TRIG→D7, ECHO→D8(UNO R4 입력 규격 및 실제 센서 모듈 확인).
- 주의: pulseIn은 최대 30ms 동안 블로킹될 수 있으므로 비차단 구조로 확장할 때 별도 상태 기계가 필요합니다.

## 전체 코드
[14_hcsr04_distance.ino](14_hcsr04_distance.ino) — 이 폴더의 스케치 원본

## 실행 절차
1. Arduino IDE에서 `14_hcsr04_distance.ino`를 열고 보드·포트를 선택한다.
2. 필요한 라이브러리를 설치하고 스케치를 검증(컴파일)한 뒤 업로드한다.
3. 배선을 확인하고 입력을 변화시키면서 출력 반응을 관찰한다.
4. Serial 출력이 있는 차시는 115200 baud로 시리얼 모니터를 연다.

## 예상 결과
물체를 움직이면 센티미터 거리 값이 시리얼 모니터에 출력됩니다. 무응답은 TIMEOUT으로 표시됩니다.

## 검증 상태
- 코드 작성: 완료
- Arduino IDE 컴파일: 미실행
- UNO R4 실물 시험: 미실행
- 실제 모듈별 전압·핀 배치 대조: 시험 전 확인 필요

**공통 모듈 자료**: [devices](../../../devices/README.md) (해당 자료의 RTL/C 코드는 Arduino 스케치가 아님).
