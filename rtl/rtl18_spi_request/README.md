# RTL18 — SPI 트랜잭션 요청

[RTL 목차](../README.md) · [Arduino 전용 과정](../../arduino/ROADMAP.md)

## 기능과 설계 범위
SPI 코어의 request/ready/done을 하나의 트랜잭션으로 관리. SPI 신호 타이밍을 새로운 미검증 PHY로 중복 구현하지 않는다.

## 소스
- [spi_request_controller.v](src/spi_request_controller.v): 합성 대상 기능 Controller
- [tb_spi_request_controller.v](tb/tb_spi_request_controller.v): 클럭 Smoke Test (DUT 기능 검증 테스트벤치로 확장 필요)

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
