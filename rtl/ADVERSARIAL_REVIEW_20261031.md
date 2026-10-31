# RTL 32개 과제 적대적 검증 — 2026-10-31

[32개 과제](README.md) · [검증 상태 대장](VERIFICATION_STATUS.md) · [자동 테스트](run_rtl_tests.sh)

## 판정
**32개 과제 소스 등록은 완료. RTL 컴파일·실행 PASS, SoC/FPGA 통합, 보드 시험 완료는 확인할 수 없음.**

### 검증 방식과 한계
- GitHub 최신 `main`의 대표 RTL 코드, 테스트벤치, CI 스크립트, 파일 구조를 실제 열람해 검토함.
- 현재 검토 환경에서 `iverilog`, `verilator`, `yosys`가 설치되어 있지 않고 외부 GitHub로 직접 clone도 불가능함. **32개 TB 실행 결과는 수집되지 않음.**
- `get_commit_combined_status` 조회에서 커밋 상태 항목이 비어 있었음. 이것만으로 GitHub Actions 성공·실패 여부를 판정하지 않음.
- 전수 회로 시뮬레이션이나 정적 합성의 대체로 문서 리뷰를 인정하지 않음.

## 확인한 핵심 문제와 조치

| 우선순위 | 대상 | 발견 사항 | 조치 / 미해결 |
|---|---|---|---|
| P0 | 전체 32개 | 소스·테스트벤치 존재만으로 합성·보드 동작 보장 불가 | 미검증 유지; 실제 CI PASS 로그와 Vivado 보고서 필요 |
| P0 | RTL14/18/19/26/27/28 | I2C·SPI·SD·BT·RS485·네트워크의 물리 계층 및 통합 구현 미완료 | 모듈을 Controller로 명확히 구분; 실제 IP·PHY 통합 필요 |
| P0 | RTL05 | 12비트 XADC 샘플은 실제 XADC Wizard 연결이 아님. XADC 전압 입력 안전 검증 없음 | 실제 IP 설정/채널/전압/CDC 확인 전 보드 실습 금지 |
| P1 | RTL22 | 방향 신호는 FF로 저장되나 PWM gate의 enable은 원래 조합 경로에 직접 의존 → 제어 동작 시점 불일치 | enable을 `enabled_reg`에 저장하여 방향 저장과 같은 Clock 기준으로 정렬(커밋 수정) |
| P1 | RTL14 | `found`가 매 응답에서 ACK/NACK 값으로 덮여 이전 검색 성공이 사라짐 | `found <= found | reply_ack`로 고쳐 검색 중 한 번이라도 ACK가 있었음을 유지. 단, 검출 주소 목록 출력은 아직 없음 |
| P1 | RTL23 | 서보 목표 펄스 계산에서 16비트 중간 곱셈 Overflow 위험 | 기존에 32비트로 수정된 상태 확인 |
| P1 | RTL32 | 캡스톤 Controller 하나만 존재: 센서 2종·출력·통신 실제 연결 프로젝트 아님 | 보드용 Top/XDC와 기존 기능 모듈 통합 필요 |
| P1 | TB | 테스트벤치가 일부 입력만 검사하고 실제 주변장치 타이밍·오류 주입·경계값 검증 부족 | 기능 커버리지·Assertions 강화 필요 |
| P2 | 문서 | Arduino 32차시와 RTL 32개 과제는 번호가 1:1 동일하지 않음 | 대응표와 과제 실제 목적을 구분해 유지 |

## 과제별 특히 필요한 검증
- RTL01~02: 비동기 버튼, Reset, 입력 바운스, 여러 버튼의 동시 전이.
- RTL03: 실제 RX valid/ready FIFO 연동, 연속 Byte, Reset 중 패킷 처리.
- RTL04: PWM 0·100%, 주기 경계에서 Duty 변경, 초기 한 주기, CLK_DIV=1/2.
- RTL05: XADC 출력 포트·채널·CDC·핀 허용 전압.
- RTL06~07: Tick/NBA 1클럭 지연, 음 전환, 분주값 0 및 최저값.
- RTL08~09: SPI/Shift 데이터 비트 순서, CS와 Clock 에지, 주변장치 초기화.
- RTL10~13: 센서 유효 신호 및 Timeout, checksum, 실제 전기 조건.
- RTL14~21: 버스 ACK/NACK·에러/Timeout·ready/valid가 유지되는 시간·샘플 누락.
- RTL22~25: 모터 구동 안전, 드라이버 전원, 서보 펄스 폭과 실제 샤프트 동작.
- RTL26~29: 통신 오류, RS485 송수신 전환 시점, 큐 백프레셔.
- RTL30: FIFO empty/full, 동시 pop/push, 순서 보전, 랩어라운드.
- RTL31: Register Map 주소·권한·동시 읽기/쓰기.
- RTL32: 센서별 valid 신호 분리 및 통합 fail-safe, 외부 출력.

## 완료 게이트
1. `bash rtl/run_rtl_tests.sh` 실행과 로그 보존. TB 개수가 32인지 확인.
2. 실패한 RTL/TB를 **원본 보존 + 실제 오류 수정** 원칙으로 교정. TB PASS는 검증한 기능 범위만 주장.
3. Vivado Synthesis + Timing + XDC + DRC 검증.
4. 실제 Basys3·안전한 센서/모터 외부 연결 확인.
5. 캡스톤 전체 하드웨어 통합 완료 후 최종 완료 판정.
