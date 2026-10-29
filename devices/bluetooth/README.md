# Bluetooth

<!-- nav -->[저장소 홈](../../README.md) › [devices](../README.md)

블루투스 모듈은 모두 UART 로 연결되므로 문서와 코드는 [`uart/`](../uart/README.md) 에 있습니다.

| 모듈 | 종류 | 문서 | 코드 | 근거 |
|---|---|---|---|---|
| HC-05 | 클래식 SPP (줄 끝 있는 AT 명령) | [modules.md](../uart/modules.md) | [`hc05.c`](../uart/sw/hc05.c) | D3 |
| HC-06 | 클래식 SPP 슬레이브 (책 28장, 줄 끝 없는 AT 명령) | [modules.md](../uart/modules.md) | [`hc06.c`](../uart/sw/hc06.c) + [`btmod.c`](../uart/sw/btmod.c) | D3, 같은 저자 2건 |
| HM-10 계열 (CC2541) | 저전력 블루투스(BLE) UART 브리지 (책 29장 취지) | [modules.md](../uart/modules.md) | [`hm10.c`](../uart/sw/hm10.c) + [`btmod.c`](../uart/sw/btmod.c) | D1(요약 경유), 불일치 있음 |

책 29장이 쓰는 **ESP32-S3 의 BLE** 는 다루지 않았습니다(모듈 선정 전). 위 코드는 AT 명령으로 모듈을 설정·점검하는 부분이고, 연결된 뒤의 데이터 송수신은 UART Lite 로 바이트를 주고받는 것과 같아 별도 코드가 없습니다. BLE 의 GATT 서비스(0xFFE0/0xFFE1)나 스마트폰 쪽 앱은 범위 밖입니다.
모두 호스트 모의 시험까지만 했고 실제 모듈과 연결한 적이 없습니다.

<!-- nav-foot -->← [ethernet](../ethernet/README.md) · [devices 목차](../README.md) · [ps2](../ps2/README.md) →
