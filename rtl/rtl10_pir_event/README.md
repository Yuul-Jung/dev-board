# RTL10 — PIR 입력 이벤트 검출

[RTL 목차](../README.md) · [Arduino 32차시](../../arduino/ROADMAP.md)

## 회로 기능
비동기 PIR의 2FF 동기화, 상승·하강 이벤트 1클럭 펄스. 실제 PIR 전압과 예열 시간은 별도 확인.

- 소스: [pir_event.v](src/pir_event.v)
- 테스트벤치: [tb_pir_event.v](tb/tb_pir_event.v) — 현재는 Clock Smoke Test만 작성되어 있으며 기능 검증 미완료.
- 기존 `devices/`의 PHY/AXI 구현과 기능이 중복되지 않도록 **표시·판단·제어 RTL을 별도 구성**한다.
- 실제 버스·보드 연결과 XDC는 미설정. Clock/Reset, 비트 폭, 전압을 검토한 뒤 검증한다.

## 검증 상태
RTL 소스 작성. 기능 Testbench 미완성, RTL Simulation/합성/Timing/보드 검증 미실행. 완료로 간주하지 않는다.

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 하위 폴더

| 폴더 | 내용 |
|---|---|
| [src/](src/README.md) | src |
| [tb/](tb/README.md) | 호스트 테스트와 레지스터/장치 모델(mock) |

<!-- index:end -->
