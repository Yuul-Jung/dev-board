# 16차시 — DS18B20와 거리 경고

[Arduino 목차](../../README.md) · [32차시 테크트리](../../ROADMAP.md)

## 개요
DS18B20와 거리 경고을 Arduino UNO R4의 입출력으로 구현한다. 기본 예제에서 확인한 입력·출력 개념을 이어서 사용한다.

## 준비 및 배선
- 보드: Arduino UNO R4 Minima 또는 WiFi. 실제 보드에 맞게 Arduino IDE 보드와 포트를 선택한다.
- 배선: DS18B20 DATA→D2, 4.7kΩ 풀업→센서 정격 VCC(일반적으로 5V 구성), GND 공통. HC-SR04 TRIG→D7 ECHO→D8, VCC→5V, GND 공통. 내장 LED, D9 수동 피에조 부저.
- 주의: OneWire 및 DallasTemperature 라이브러리 설치. 이 차시는 기능 결합용으로 DS18B20의 requestTemperatures()가 변환을 기다리는 블로킹 구현입니다.

## 전체 코드
[16_dual_sensor_alarm.ino](16_dual_sensor_alarm.ino) — 이 폴더의 스케치 원본

## 실행 절차
1. Arduino IDE에서 `16_dual_sensor_alarm.ino`를 열고 보드·포트를 선택한다.
2. 필요한 라이브러리를 설치하고 스케치를 검증(컴파일)한 뒤 업로드한다.
3. 배선을 확인하고 입력을 변화시키면서 출력 반응을 관찰한다.
4. Serial 출력이 있는 차시는 115200 baud로 시리얼 모니터를 연다.

## 예상 결과
온도와 거리 값이 출력되고 물체가 20cm 미만이면 LED와 부저가 켜집니다. 온도 센서 오류는 별도 표기합니다.

## 검증 상태
- 코드 작성: 완료
- Arduino IDE 컴파일: 미실행
- UNO R4 실물 시험: 미실행
- 실제 모듈별 전압·핀 배치 대조: 시험 전 확인 필요

**공통 모듈 자료**: [devices](../../../devices/README.md) (해당 자료의 RTL/C 코드는 Arduino 스케치가 아님).
