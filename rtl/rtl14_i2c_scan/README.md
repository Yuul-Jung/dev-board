# RTL14 — I2C 주소 탐색 요청 컨트롤러

[RTL 목차](../README.md) · [Arduino 32차시](../../arduino/ROADMAP.md)

## 회로 기능
I2C 주소 0x08~0x77의 명령·응답 트랜잭션을 관리하며 실제 SDA/SCL PHY는 AXI IIC나 검증된 버스 코어에 연결.

- 소스: [i2c_scan_controller.v](src/i2c_scan_controller.v)
- 테스트벤치: [tb_i2c_scan_controller.v](tb/tb_i2c_scan_controller.v) — 현재는 Clock Smoke Test만 작성되어 있으며 기능 검증 미완료.
- 기존 `devices/`의 PHY/AXI 구현과 기능이 중복되지 않도록 **표시·판단·제어 RTL을 별도 구성**한다.
- 실제 버스·보드 연결과 XDC는 미설정. Clock/Reset, 비트 폭, 전압을 검토한 뒤 검증한다.

## 검증 상태
RTL 소스 작성. 기능 Testbench 미완성, RTL Simulation/합성/Timing/보드 검증 미실행. 완료로 간주하지 않는다.
