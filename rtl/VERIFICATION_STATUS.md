# RTL 32개 과제 검증 상태

[RTL 목차](README.md)

## 검증 단계
1. Verilog 소스 작성
2. DUT 인스턴스를 포함한 자체검사 테스트벤치 작성
3. Icarus Verilog 시뮬레이션 PASS
4. Vivado Synthesis 및 Timing Report 확인
5. XDC와 전압·실물 보드 동작 확인

## 현재 상황
- RTL01~RTL32: 32개 폴더·기능 RTL 작성
- RTL01~RTL32: DUT 기반 기본 테스트벤치 작성. 시나리오 커버리지는 과제별로 서로 다르며 추가 검증 필요.
- 전체 시뮬레이션 자동 실행 스크립트: [run_rtl_tests.sh](run_rtl_tests.sh)
- GitHub Actions: [rtl-tests.yml](../.github/workflows/rtl-tests.yml)
- 2026-10-31 GitHub Actions 에서 Icarus 32/32 PASS(런 41189772658 등), RTL03 UVM PASS(런 41189772574) 확인. 단 최초 실행에서 RTL08·RTL09 가 상태 레지스터 폭 부족(2비트에 상태 4·5)으로 실패했고 3비트로 수정함. 폭 검사 `rtl/verification/lint_widths.py` 를 CI 에 추가.
- 변형 검사(`rtl/verification/rtl/mutate.py`, 런 41190240306): 222개 변형 중 114개만 TB 가 잡음(51.4 %). PASS 는 「좁은 지향 시험 통과」이지 기능 완전성이 아니다.
- Vivado 합성·Timing·Basys3 실물 검증: 미실시.

## 통합 전 확인
- RTL03 UART PHY, RTL05 XADC Wizard, RTL14 I2C, RTL18 SPI, RTL19 SD, RTL26 Bluetooth UART, RTL27 RS485, RTL28 Ethernet의 외부 IP/PHY는 모듈 인터페이스만 정의됐다.
- RTL23 Servo는 32비트 중간 곱셈을 사용하도록 수정했지만 보드 PWM 코어 연결 전이다.
- RTL32 캡스톤은 기능 Controller이며 모든 실물 장치가 연결된 통합 시스템은 아니다.

## 실행
저장소 루트에서 `bash rtl/run_rtl_tests.sh` 실행. 보드가 없이 실행 가능한 범위의 RTL 검증이며 사용 중인 툴 버전 및 결과 로그를 보관해야 한다.
