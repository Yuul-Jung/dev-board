# RTL32 — 캡스톤 모듈 통합 제어

[RTL 목차](../README.md) · [Arduino 전용 과정](../../arduino/ROADMAP.md)

## 기능과 설계 범위
센서 샘플별 안전 제어 및 이벤트 스트로브. 센서/통신/출력은 독립 모듈로 연결하는 최종 구조의 기능 Controller.

## 소스
- [capstone_controller.v](src/capstone_controller.v): 합성 대상 기능 Controller
- [tb_capstone_controller.v](tb/tb_capstone_controller.v): 클럭 Smoke Test (DUT 기능 검증 테스트벤치로 확장 필요)

## 검증 기준
해당 파일은 **RTL 구현 초안**이며 컴파일·기능 시뮬레이션·Vivado 합성·Timing·보드 검증을 수행하지 않았다. 외부 PHY, 실제 IP, XDC, 전원 제약 조건을 검토한 뒤 실험한다.

## 교육 핵심
- 조합 논리의 즉시 반영과 순차 레지스터의 posedge 저장을 구분한다.
- 입출력 비트 폭, ready/valid 신호가 있다면 전달 클럭, 상태 갱신 조건을 검토한다.
- Top에는 모듈 인스턴스와 wire만 둔다.
