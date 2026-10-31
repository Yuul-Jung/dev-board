# 16차시 — DS18B20와 거리 경고

[Arduino 목차](../../README.md) · [32차시 테크트리](../../ROADMAP.md)

## 개요
DS18B20와 거리 경고를 Arduino UNO R4의 입출력으로 구현한다. 기본 예제에서 확인한 입력·출력 개념을 이어서 사용한다.

## 준비 및 배선
- 보드: Arduino UNO R4 Minima 또는 WiFi. 실제 보드에 맞게 Arduino IDE 보드와 포트를 선택한다.
- 배선: DS18B20 DATA→D2, 4.7kΩ 풀업→센서 정격 VCC(일반적으로 5V 구성), GND 공통. HC-SR04 TRIG→D7 ECHO→D8, VCC→5V, GND 공통. 내장 LED, D9 수동 피에조 부저.
- 주의: OneWire 및 DallasTemperature 라이브러리 설치. 이 차시는 기능 결합용 예제이며, 변환 대기는 아래 개정 메모처럼 비차단으로 처리합니다.

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

## 개정 메모 (2026-10-31 검토)
- **지연 문제를 고쳤습니다**: 이전 스케치는 `requestTemperatures()` 가 변환을 기다려(12비트 최대 750 ms, [DS18B20 문서](../../../devices/1-wire/ds18b20.md)) 한 바퀴가 약 1.8 s 였고, 그동안 거리 경고가 멈췄습니다. 이제 `setWaitForConversion(false)` 와 millis 로 변환을 기다리지 않고 거리는 약 0.1 s 마다 갱신합니다.
- 2 cm 미만·400 cm 초과 값은 경고 판단에서 제외합니다(HC-SR04 규격 범위).
- 위 주의 문구의 「블로킹 구현」은 이전 버전 설명입니다. 개정한 스케치는 **스텁 헤더로 문법만 확인**했고 `setWaitForConversion` 등 라이브러리 API 는 실제 라이브러리로 컴파일하지 않았습니다(미확인).

**공통 모듈 자료**: [devices](../../../devices/README.md) (해당 자료의 RTL/C 코드는 Arduino 스케치가 아님).

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [16_dual_sensor_alarm.ino](16_dual_sensor_alarm.ino) | 변환 대기를 loop 를 막지 않고 millis 로 처리한다(거리 경고가 온도 변환 때문에 늦어지지 않게). |

<!-- index:end -->
