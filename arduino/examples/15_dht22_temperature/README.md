# 15차시 — DHT22 온습도 측정

[Arduino 목차](../../README.md) · [32차시 테크트리](../../ROADMAP.md)

## 개요
DHT22 온습도 측정을 Arduino UNO R4의 입출력으로 구현한다. 기본 예제에서 확인한 입력·출력 개념을 이어서 사용한다.

## 준비 및 배선
- 보드: Arduino UNO R4 Minima 또는 WiFi. 실제 보드에 맞게 Arduino IDE 보드와 포트를 선택한다.
- 배선: DHT22 모듈 VCC→정격에 따른 전원, GND 공통, DATA→D2; 베어 센서는 DATA 풀업 저항을 별도로 확인.
- 주의: Arduino IDE 라이브러리 매니저에서 'DHT sensor library by Adafruit' 및 종속 라이브러리 'Adafruit Unified Sensor' 설치. 모델별 핀 순서를 반드시 확인.

## 전체 코드
[15_dht22_temperature.ino](15_dht22_temperature.ino) — 이 폴더의 스케치 원본

## 실행 절차
1. Arduino IDE에서 `15_dht22_temperature.ino`를 열고 보드·포트를 선택한다.
2. 필요한 라이브러리를 설치하고 스케치를 검증(컴파일)한 뒤 업로드한다.
3. 배선을 확인하고 입력을 변화시키면서 출력 반응을 관찰한다.
4. Serial 출력이 있는 차시는 115200 baud로 시리얼 모니터를 연다.

## 예상 결과
약 2초마다 온도(°C)와 상대습도(%)가 출력됩니다.

## 검증 상태
- 코드 작성: 완료
- Arduino IDE 컴파일: 미실행
- UNO R4 실물 시험: 미실행
- 실제 모듈별 전압·핀 배치 대조: 시험 전 확인 필요

## 개정 메모 (2026-10-31 검토)
- 스케치의 `delay(2000)` 은 [DHT22 문서](../../../devices/gpio/dht22.md)의 「수집 주기 2 s 초과」와 맞습니다(읽기 시간까지 더해 실제 주기는 2 s 를 넘음).
- **미확인**: Adafruit DHT 라이브러리가 UNO R4(Renesas RA4M1)에서 정상 동작하는지는 확인하지 못했습니다. 베어 센서의 풀업 값도 데이터시트에 없습니다(위 devices 문서).

**공통 모듈 자료**: [devices](../../../devices/README.md) (해당 자료의 RTL/C 코드는 Arduino 스케치가 아님).

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [15_dht22_temperature.ino](15_dht22_temperature.ino) |  |

<!-- index:end -->
