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

### 경로 C: AXI Ethernet Lite (PG135 v3.0, 2021-11-02) — 확인 (문서 본문 열람)
| 항목 | 값 |
|---|---|
| 속도·PHY | 10/100 Mb/s, MII, 옵션 MDIO 관리 모듈, 내부 루프백 |
| 버퍼 | TX·RX 각 2 KB 듀얼포트 메모리(프레임 1개), 옵션 핑퐁(TX·RX 각 4 KB), 송수신 인터럽트 |
| 디바이스 | Artix-7 최대 주파수 표·자원 표 있음 (AXI4-Lite 기준 -1 120 / -2 140 / -3 160 MHz) |
| 라이선스 | **추가 비용 없이 Vivado에 포함**, Xilinx End User License |
| 드라이버 | emaclite (최소 지원 모드, 폐기 예정 표기, MicroBlaze 검증 — Xilinx 위키) |

**MII 신호 (확인, 18개)**: phy_tx_clk, phy_rx_clk, phy_rx_data[3:0], phy_tx_data[3:0], phy_dv, phy_rx_er, phy_tx_en, phy_crs, phy_col, phy_rst_n, phy_mdc, phy_mdio(i/o/t, 3상태는 IP Integrator가 자동 삽입). 합계 2+4+4+5+3 = 18 → Pmod 24신호 + JXADC 8신호 중 절반 이상을 차지합니다(계산).
레지스터 (표 2-7): 07E4h MDIOADDR · 07E8h MDIOWR · 07ECh MDIORD · 07F0h MDIOCTRL · 07F4h TX Ping Length · 07F8h GIE · 07FCh TX Ping Control · 0FF4h TX Pong Length · 0FFCh TX Pong Control · 17FCh RX Ping Control · 1FFCh RX Pong Control. MDIO 레지스터는 MII 관리 모듈 활성 시에만, Pong 레지스터는 버퍼 옵션 활성 시에만 존재. TX 버퍼는 0x0000부터, RX 버퍼는 0x1000부터인 구조는 표에서 직접 확인하지 못함(미확인).
미확인: 사용할 PHY Pmod 제품과 25 MHz 클럭 요구, MicroBlaze V 에서 emaclite 동작.

## 권장 (설계안)
W5500(경로 A). 이유: 핀이 SPI 하나로 충분하고(`../spi/README.md` 버스 공유), TCP/IP 스택이 칩 안에 있어 MicroBlaze 소프트웨어가 단순합니다. 단, W5500 모듈의 PHY·자석(마그네틱)·RJ45 포함 여부는 모듈마다 다름(미확인).

## 전용 RTL
세 경로 모두 AMD IP 또는 외부 칩을 쓰므로 전용 RTL 불필요.

## Bluetooth
별도 IP 없이 UART 모듈(HC-05)로 처리합니다. `../uart/modules.md` 참고. `bluetooth/` 폴더는 HC-05 외 BLE 모듈이 정해질 때 채웁니다.
