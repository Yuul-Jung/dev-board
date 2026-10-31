# RTL 32개 과제 검증 상태 대장

[RTL 목차](README.md) · [원래 Arduino 테크트리](../arduino/ROADMAP.md)

## 완료 판정 게이트
1. **코드 작성**: 실제 Verilog 모듈, 포트, Bit Width 명확
2. **RTL 시뮬레이션**: DUT를 인스턴스한 Self-checking Testbench PASS
3. **Vivado 합성**: 경고 검토, 합성 가능 확인
4. **Timing 검증**: 보드 XDC와 Clock/IO 경로 확인
5. **Board 검증**: 전기 사양·배선·실물 동작 확인

작성된 모듈 파일만으로 **실습 완료** 또는 **시뮬레이션 통과**로 표시하지 않는다.

| 범위 | 작성 수준 | 남은 핵심 작업 |
|---|---|---|
| RTL01~RTL07 | 기능 RTL, 전용 TB 작성 | 실제 시뮬레이션과 문제 수정, IP 실물 연결 |
| RTL08~RTL32 | 기능 RTL 초안과 DUT 인스턴스 자체검사 TB | 기능 시뮬레이션 실행·고장 수리·통합과제 Top·XDC 작성 필요 |

## 자동화된 실행
- `bash rtl/run_rtl_tests.sh`: Icarus Verilog로 각 차시 TB를 컴파일하고 실행.
- GitHub Actions `.github/workflows/rtl-tests.yml`: 변경 시 동일한 검사 수행.
- 해당 자동화는 **등록된 시험 시나리오만** 검증한다. IP 미연결·전기적 안전·실물 기능은 보장하지 않는다.
- 워크플로 실행 성공 증거는 아직 수집하지 않았다.

## 기술적 주의
- RTL14, RTL18, RTL19, RTL26, RTL27, RTL28은 통신 PHY/프로토콜을 완성한 것이 아니라 **트랜잭션 제어기**이다.
- RTL23은 서보 펄스 목표값만 생성한다. 실제 PWM 파형 발생기는 별도 모듈.
- RTL30 이벤트 FIFO는 FIFO 경계 조건과 동시 push/pop를 기능 시뮬레이션으로 반드시 검사한다.
- RTL32은 센서 판단 기능 Controller이며 다양한 하위 모듈을 통합한 보드 프로젝트는 아니다.
- Arduino 32개 차시, 순수 RTL 32개 과제, MicroBlaze V SoC 41장은 상이한 과정이다.
