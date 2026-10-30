# tb

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [uart](../README.md)

호스트 테스트와 레지스터/장치 모델(mock)

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [fuzz_bt.c](fuzz_bt.c) | 적대적 시험: 임의 바이트·임의 크기 버퍼를 블루투스 모듈 드라이버(HC-06, HM-10)에 넣어 메모리 오류·행·불변식 위반을 찾는다(ASan/UBSan 아래 |
| [fuzz_nmea.c](fuzz_nmea.c) | 적대적 시험: 임의 바이트·변형된 RMC 문장을 NMEA 조립기/파서에 넣어 메모리 오류·행을 찾는다(ASan/UBSan 아래). |
| [mock_uart.h](mock_uart.h) | endif |
| [test_bt.c](test_bt.c) | 블루투스 UART 모듈(HC-06, HM-10) 시험: 공통 송수신(무음 구간 판단), 명령 만들기, 응답 해석, 모듈 시뮬레이터 위의 연산. |
| [test_rn_modules.c](test_rn_modules.c) | uart/tb/test_rn_modules.c — RN-42(Pmod BT2)·RN4871(Pmod BLE) 명령 만들기·응답 확인 시험. |
| [test_uart_mock.c](test_uart_mock.c) | 모의 UART Lite: rx 큐, tx 로그, 오류 비트는 STAT 읽기 시 지워짐 |

<!-- index:end -->
