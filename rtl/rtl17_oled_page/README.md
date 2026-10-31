# RTL17 — OLED 페이지 선택

[RTL 목차](../README.md) · [Arduino 전용 과정](../../arduino/ROADMAP.md)

## 기능과 설계 범위
OLED의 프레임 버퍼 및 I2C 전송은 기존 IP 담당. 4개 페이지의 상태 레지스터와 1클럭 redraw를 구현.

## 소스
- [oled_page_controller.v](src/oled_page_controller.v): 합성 대상 기능 Controller
- [tb_oled_page_controller.v](tb/tb_oled_page_controller.v): 클럭 Smoke Test (DUT 기능 검증 테스트벤치로 확장 필요)

## 검증 기준
해당 파일은 **RTL 구현 초안**이며 컴파일·기능 시뮬레이션·Vivado 합성·Timing·보드 검증을 수행하지 않았다. 외부 PHY, 실제 IP, XDC, 전원 제약 조건을 검토한 뒤 실험한다.

## 교육 핵심
- 조합 논리의 즉시 반영과 순차 레지스터의 posedge 저장을 구분한다.
- 입출력 비트 폭, ready/valid 신호가 있다면 전달 클럭, 상태 갱신 조건을 검토한다.
- Top에는 모듈 인스턴스와 wire만 둔다.
