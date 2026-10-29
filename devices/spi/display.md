# SPI 표시장치: ILI9341, SSD1306, MAX7219

구분 표기는 [README.md](README.md) 와 같습니다.

## ILI9341 (TFT 컨트롤러) — D1 일부
| 항목 | 값 | 구분 |
|---|---|---|
| 해상도 | 240(RGB) × 320 | 확인 |
| 색 | 262,144색 | 확인 |
| 4선 SPI 핀 | SCL, SDA, D/CX(데이터/명령), CSX(active-low) — IM[3:0]=0110 또는 1110 | 확인 |
| SDO | 인터페이스 II(IM[3:0]=1110)에서 추가 출력 | 확인 |
| 명령 | 01h Software Reset · 11h Sleep Out · 29h Display ON · 2Ah Column Address · 2Bh Page Address · 2Ch Memory Write · 36h Memory Access Control · 3Ah COLMOD(Pixel Format) | 확인 |
| 최소 SCL 쓰기 주기, 리셋 후/Sleep Out 후 대기, 리셋 Low 폭 | 열어본 문서에서 값 없음 | 미확인 |
| 프레임 전송량 | 240×320×16비트 = 1,228,800비트 | 계산 (16비트 색 가정) |

사용자 노션 (ILI9341 TFT, SPI 이론 페이지): Mode 0, MSB first, 컨트롤러 구조 `ili9341 startup banner`. 기준 값은 노션이며 데이터시트에서 재확인하지 않았습니다.
SoC: AXI Quad SPI(SPI1, 빠른 버스) + GPIO 3선(D/C, RESET, 백라이트). 한 프레임 시간은 SCK를 모르면 계산 불가 — 예: SCK 25 MHz 가정이면 약 49 ms (계산, 클럭 한계 미확인).
초기화 순서는 데이터시트의 파워 시퀀스를 열어 확인하기 전에는 작성하지 않습니다.

## SSD1306 (OLED, 128×64) — D1 일부
| 항목 | 값 | 구분 |
|---|---|---|
| 해상도 | 128 × 64 | 확인 |
| 전원 | VDD 1.65~3.3 V (로직), VCC 7~15 V (패널) | 확인 |
| 4선 SPI | D0=SCLK, D1=SDIN, D/C#, CS# | 확인 |
| 명령 | 81h 대비(00h~FFh 256단계) · AEh 끔 · AFh 켬 · 20h 주소 모드(수평/수직/페이지, 리셋 기본 페이지) · 21h 열 주소(3바이트) · 22h 페이지 주소(3바이트) | 확인 |
| 8Dh 차지 펌프 | 열어본 문서에 없음 | 미확인 |
| SCLK 최소 주기 | 표 13-4가 있으나 값 없음 | 미확인 |
| 프레임 | 128×64/8 = 1,024바이트 | 계산 |

SoC: AXI Quad SPI + GPIO(D/C, RES). 프레임 버퍼 1 KB 는 MicroBlaze 메모리에서 충분.

## MAX7219 (8자리 7세그/8×8 LED 드라이버) — D1
| 항목 | 값 |
|---|---|
| 전원 | V+ 4.0~5.5 V (3.3 V 불가) |
| 패킷 | 16비트: D15~D12 무시, D11~D8 주소, D7~D0 데이터 |
| 클럭 | CLK 최대 10 MHz (최소 주기 100 ns), DIN은 CLK 상승 에지에서 시프트 |
| 래치 | LOAD(MAX7219) 상승 에지 — MAX7221은 CS |
| 자릿수 | 8 (DIG0~7), Scan limit 으로 1~8 |
| 레지스터 | 00h No-Op · 01h~08h Digit0~7 · 09h Decode mode · 0Ah Intensity · 0Bh Scan limit · 0Ch Shutdown · 0Fh Display test |
| 직렬 연결 | DIN→DOUT(16.5클럭 지연) 체인, LOAD 공통. 4개 체인에서 마지막 칩에 쓰려면 뒤에 No-Op 워드 3개 |

SoC (설계안): AXI Quad SPI, LOAD 를 CS 로 사용. 전원이 5 V 이므로 FPGA 3.3 V 신호의 입력 하이 레벨 적합성은 **미확인** — 확인 전에는 레벨 변환기 사용을 가정.

## Basys3 온보드 표시 (참고)
7세그먼트와 VGA 12비트는 Basys3 RM 기준 온보드이며 SPI 가 아닌 GPIO/전용 RTL 경로입니다 (`../CATEGORIES.md` 참고).
