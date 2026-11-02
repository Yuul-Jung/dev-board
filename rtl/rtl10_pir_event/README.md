# RTL10 — PIR 입력 이벤트 검출

[RTL 목차](../README.md) · [Arduino 32차시](../../arduino/ROADMAP.md)

## 회로 기능
비동기 PIR의 2FF 동기화, 상승·하강 이벤트 1클럭 펄스. 실제 PIR 전압과 예열 시간은 별도 확인.

- 소스: [pir_event.v](src/pir_event.v)
- 테스트벤치: [tb_pir_event.v](tb/tb_pir_event.v) — **자체 기대값 비교**(2-FF 지연·상승/하강 펄스·리셋·임의 입력 100회) 코드가 이미 작성되어 있다. 단순 Clock Smoke Test라는 기존 설명은 코드와 불일치했다.
- 기존 `devices/`의 PHY/AXI 구현과 기능이 중복되지 않도록 **표시·판단·제어 RTL을 별도 구성**한다.
- 실제 버스·보드 연결과 XDC는 미설정. Clock/Reset, 비트 폭, 전압을 검토한 뒤 검증한다.

## 검증 상태
RTL 소스와 자체 판정 테스트벤치 **작성됨**. 실행 경로는 `rtl/verification/rtl/run.sh` 및 GitHub Actions `rtl-tests.yml`이며, **이 문서 변경에서 새 RTL 시뮬레이션 통과를 주장하지 않는다**. 합성/Timing/실물 보드 검증은 미확인. 전기적 연결은 [HC-SR501 자료](../../devices/gpio/hc-sr501.md)의 OUT 전압·출력단 확인 이후에만 승인한다.

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 하위 폴더

| 폴더 | 내용 |
|---|---|
| [src/](src/README.md) | src |
| [tb/](tb/README.md) | 호스트 테스트와 레지스터/장치 모델(mock) |

<!-- index:end -->
