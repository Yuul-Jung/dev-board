# sw

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [uart](../README.md)

드라이버 C 소스

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [btmod.c](btmod.c) |  |
| [btmod.h](btmod.h) | 블루투스 UART 모듈(HC-06, HM-10 계열) 공통 송수신. 이 모듈들의 AT 명령은 줄 끝 문자 없이 보내고 응답도 줄 끝이 없다 → 응답의 끝은 "무음 |
| [hc05.c](hc05.c) |  |
| [hc05.h](hc05.h) | HC-05 AT 명령 도우미. 명령 문자열은 2차 자료(D3) 기준, 응답 문자열 'OK' 외에는 미확인. |
| [hc06.c](hc06.c) |  |
| [hc06.h](hc06.h) | HC-06(ZS-040 등, 클래식 블루투스 SPP 슬레이브) AT 명령. 명령은 대문자, 줄 끝 없음. |
| [hm10.c](hm10.c) |  |
| [hm10.h](hm10.h) | HM-10 계열(CC2541) BLE UART 모듈 AT 명령. 명령은 대문자, 줄 끝 없음. 응답은 "OK" 또는 "OK+<태그>[:<값>]" (줄 끝 없음). |
| [nmea.c](nmea.c) |  |
| [nmea.h](nmea.h) | NMEA 줄 조립 + RMC 파서. |
| [rn42.c](rn42.c) | Pmod BT2 — RN-42 드라이버 구현. |
| [rn42.h](rn42.h) | Pmod BT2 — RN-42 (클래식 블루투스 2.1+EDR, SPP) 명령 층. |
| [rn4871.c](rn4871.c) | Pmod BLE — RN4871 드라이버 구현. |
| [rn4871.h](rn4871.h) | Pmod BLE — RN4871 (Bluetooth Low Energy 4.2, GATT/SPP) 명령 층. |
| [uartlite.c](uartlite.c) | STAT 읽기는 오류 비트(5~7)를 지우므로 읽을 때마다 누적한다. |
| [uartlite.h](uartlite.h) | AXI UART Lite (PG142 v2.0) 드라이버. 레지스터·비트는 PDF 에서 확인한 값. |

<!-- index:end -->
