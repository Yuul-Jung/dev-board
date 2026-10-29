# UART 소자와 SoC 연결

<!-- nav -->[저장소 홈](../../README.md) › [devices](../README.md)

구분: **확인** = 이번 작업에서 문서를 열어 확인, **설계안**, **미확인**. 소자: [modules.md](modules.md).

## AXI UART Lite (PG142 v2.0, 2017-04-05) — 확인 (문서 본문 열람)
| 항목 | 값 |
|---|---|
| 기능 | 전이중, AXI4-Lite, 데이터 5~8비트, 패리티 홀/짝/없음, 16엔트리 송수신 FIFO |
| 보레이트 | **Vivado IDE 파라미터(설계 시점)**. 지원값: 110, 300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 128000, 230400, 460800, 921600. GUI가 AXI 클럭 대비 오차를 계산해 유효한 값만 허용. 런타임 변경 레지스터 없음 |
| 7 series | 최대 주파수 표(AXI4-Lite)에 Artix-7: -1 120 MHz, -2 140 MHz, -3 160 MHz |
| 레지스터 | 0x00 Rx FIFO · 0x04 Tx FIFO · 0x08 STAT_REG · 0x0C CTRL_REG |
| STAT_REG | 0 Rx 유효 · 1 Rx 가득 · 2 Tx 빔(리셋 1) · 3 Tx 가득 · 4 인터럽트 활성 · 5 오버런 · 6 프레임 오류 · 7 패리티 오류. 5~7은 읽으면 지워짐 |
| CTRL_REG | 0 Tx FIFO 리셋 · 1 Rx FIFO 리셋 · 4 인터럽트 활성 (모두 쓰기) |
| 주의 | 빈 Rx FIFO를 읽으면 버스 오류(SLVERR)와 정의되지 않은 값. 프레임 오류 문자와 오버런 문자는 FIFO에 쓰이지 않음. 패리티 오류 문자는 FIFO에 쓰임 |

따라서 **소자마다 보레이트가 다르면 UART Lite 인스턴스를 나눠야 함**(확인). 이전의 "합성 시점 고정은 추정"은 이 문서로 확정되었습니다.
Basys3 USB-UART 콘솔과 NEO-6M(9600)·HC-05(38400)는 각각 별도 인스턴스입니다.

## 포트 계획 (설계안)
| 연결 | 보레이트 | 포맷 | 근거 |
|---|---|---|---|
| Basys3 USB-UART (TXD B18, RXD A18, RM 확인) | 임의 | - | 디버그 콘솔 |
| NEO-6M GPS | 9600 (기본) | 8N1 | modules.md |
| HC-05 | 38400 (기본) | 8N1 | modules.md, 2차 자료 |
| MAX485 | 용도별 | - | 최대 2.5 Mbps |
UART Lite 인스턴스 소자당 1개, Pmod 핀은 TX/RX 2선. 보레이트가 합성 시 고정이면 소자별 인스턴스가 필요합니다(미확인이라 인스턴스를 나누는 쪽이 안전).

## 전용 RTL 여부
UART Lite 가 있으므로 소자 접근은 IP. 전용 RTL 후보는 RS-485 방향 제어(DE/RE) 자동화뿐이며, GPIO 한 비트를 소프트웨어가 제어하면 충분하므로 이번에는 작성하지 않았습니다.

## 드라이버 (devices/uart/sw, 설계안·호스트 모의 테스트만)
| 파일 | 내용 | 근거 |
|---|---|---|
| uartlite.h/.c | PG142 레지스터 접근. Rx 가 비었으면 읽지 않음(빈 FIFO 읽기 = SLVERR), STAT 오류 비트(5~7)는 읽으면 지워지므로 읽을 때마다 누적 | 확인 (PG142) |
| nmea.h/.c | 줄 조립, XOR 체크섬, RMC 파서 (위경도 1e-7도, 속도 0.01노트) | NMEA_Format_v0.1 (NavSpark) — 형식·XOR·RMC 필드 순서 확인 |
| hc05.h/.c | AT 명령 문자열 생성과 `OK` 응답 확인 | D3 (2차 자료) |

미확인: 체크섬 XOR 범위에 `$`·`*` 포함 여부(문서가 명시 안 함, 관례대로 둘 사이로 계산). GGA 필드 순서(문서에서 확인하지 못해 GGA 파서 없음). RMC 위도 표기가 문서 안에서 ddmm/dddmm 으로 상충해 두 형식 모두 처리. NavSpark 문서이므로 NEO-6M 의 NMEA 2.3 출력과 같다는 보장은 없음. HC-05 의 AT 모드 보레이트·응답 형식.
테스트: `cd devices && gcc -Wall -include uart/tb/mock_uart.h -I uart/sw uart/tb/test_uart_mock.c uart/sw/*.c -o t && ./t` → ALL PASS (모의 레지스터 기준이며 보드 동작 검증 아님).

<!-- nav-foot -->← [spi](../spi/README.md) · [devices 목차](../README.md) · [i2s](../i2s/README.md) →
