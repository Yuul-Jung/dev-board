# 이더넷과 SoC 연결

구분: **확인** = 문서를 열어 확인, **계산**, **설계안**, **미확인**.

## 보드 사실 (Basys3 RM, 확인)
온보드 주변장치 목록: USB-UART, USB HID 호스트, 12비트 VGA, Pmod 3개 + XADC Pmod, 스위치 16, LED 16, 7세그 4자리, 시리얼 플래시, 푸시버튼 5, USB-JTAG. **이더넷 PHY와 RJ45는 없음.** 외부 모듈이 필요합니다.

## 경로 비교
| 경로 | 구성 | 핀 | TCP/IP | 근거 |
|---|---|---|---|---|
| A. W5500 (SPI) | 하드와이어 TCP/IP 칩 | SPI 버스 + CS + INT/RST | 칩 안(TCP, UDP, ICMP, IPv4, ARP, IGMP, PPPoE, 소켓 8개) | `../spi/comm-storage-io.md` (확인) |
| B. Pmod NIC100 (ENC424J600, SPI) | MAC+PHY 칩, 소프트웨어 스택 필요 | SPI 4선 + INT | 소프트웨어(lwIP 등) | Digilent 매뉴얼 (확인) |
| C. AXI Ethernet Lite + MII PHY | FPGA에 MAC IP, 외부 PHY | MII 다수 (아래) | 소프트웨어 | Xilinx 위키 (일부 확인) |

### 경로 B: Pmod NIC100 — D1 (Digilent 매뉴얼)
ENC424J600 10/100 이더넷 컨트롤러, 3.3 V 보드(외부 전원은 3~3.6 V 범위). J1 SPI 핀: 1 CS · 2 MOSI · 3 MISO · 4 SCLK · 7 ~INT/SPISEL · 5,11 GND · 6,12 VCC(3.3 V). INT/SPISEL 풀업이면 SPI 모드, 풀다운이면 병렬 슬레이브 포트 모드. 최대 SPI 클럭은 문서에 없음(미확인).

### 경로 C: AXI Ethernet Lite — D1/D3 부분
| 항목 | 값 | 구분 |
|---|---|---|
| 속도 | 10 Mb/s, 100 Mb/s | 확인 (위키) |
| PHY 연결 | IEEE 802.3 MII, MDIO 로 PHY 제어 | 확인 |
| 기능 | 내부 루프백, RX/TX 핑퐁 버퍼 | 확인 |
| 버스 | AXI4 또는 AXI4-Lite (PG135 v3.0) | 확인 |
| 드라이버 | emaclite, "최소 지원 모드이며 곧 폐기 예정" 표기, MicroBlaze에서만 검증 | 확인 |
| 레지스터 오프셋, 버퍼 크기, 클럭 주파수, 지원 디바이스 | 본문 못 열음 | **미확인** |

MII 신호 수는 일반 지식으로 약 16~18선(TXD 4, TX_EN, TX_CLK, RXD 4, RX_DV, RX_CLK, RX_ER, CRS, COL, MDC, MDIO)이라 Pmod 24신호 중 대부분을 차지합니다(계산, 신호 목록은 이번 문서에서 미확인). 핀 예산이 빠듯해 이 보드에서는 비현실적입니다.
소프트웨어 방향: MicroBlaze V 용 emaclite 드라이버가 MicroBlaze 계열 검증이라 MicroBlaze V 에서의 동작은 미확인.

## 권장 (설계안)
W5500(경로 A). 이유: 핀이 SPI 하나로 충분하고(`../spi/README.md` 버스 공유), TCP/IP 스택이 칩 안에 있어 MicroBlaze 소프트웨어가 단순합니다. 단, W5500 모듈의 PHY·자석(마그네틱)·RJ45 포함 여부는 모듈마다 다름(미확인).

## 전용 RTL
세 경로 모두 AMD IP 또는 외부 칩을 쓰므로 전용 RTL 불필요.

## Bluetooth
별도 IP 없이 UART 모듈(HC-05)로 처리합니다. `../uart/modules.md` 참고. `bluetooth/` 폴더는 HC-05 외 BLE 모듈이 정해질 때 채웁니다.
