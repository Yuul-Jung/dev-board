# RTL25 — 거리 기반 모터 안전 정지

[RTL 목차](../README.md) · [Arduino 전용 과정](../../arduino/ROADMAP.md)

## 기능과 설계 범위
유효 거리 측정이 충분할 때만 모터 허가. 실제 측정 타임아웃·통신 오류에는 상위 안전정책을 추가해야 한다.

## 소스
- [distance_motor_guard.v](src/distance_motor_guard.v): 합성 대상 기능 Controller
- [tb_distance_motor_guard.v](tb/tb_distance_motor_guard.v): 클럭 Smoke Test (DUT 기능 검증 테스트벤치로 확장 필요)

## 검증 기준
해당 파일은 **RTL 구현 초안**이며 컴파일·기능 시뮬레이션·Vivado 합성·Timing·보드 검증을 수행하지 않았다. 외부 PHY, 실제 IP, XDC, 전원 제약 조건을 검토한 뒤 실험한다.

## 교육 핵심
- 조합 논리의 즉시 반영과 순차 레지스터의 posedge 저장을 구분한다.
- 입출력 비트 폭, ready/valid 신호가 있다면 전달 클럭, 상태 갱신 조건을 검토한다.
- Top에는 모듈 인스턴스와 wire만 둔다.

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 하위 폴더

| 폴더 | 내용 |
|---|---|
| [src/](src/README.md) | src |
| [tb/](tb/README.md) | 호스트 테스트와 레지스터/장치 모델(mock) |

<!-- index:end -->
