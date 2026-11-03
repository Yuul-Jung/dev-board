# AHT20 온습도 센서 — 신규 후보 조사 (미검증)

**상태:** 신규 후보 기술 조사. 기존 72개 상태표의 정식 모듈 ID 부여 전; RTL/C 드라이버, 실물 검증 없음. 기존 32차시 변경 없음.

## IC 및 모듈 차이

- Aosong/ASAIR AHT20 원본 IC: I²C(7비트 주소 0x38) 온습도 센서. 제조사 데이터시트 버전 V1.0.03 원문을 기준으로 상세 명령·타이밍을 재검증해야 한다.
- Seeed Grove AHT20(101990644): Grove 커넥터를 사용하는 별도 기판. 원본 IC와 보드 전원·풀업·커넥터 핀 배열은 별개로 검증한다.
- 무명 Arduino 호환/AliExpress 기판: 실크의 AHT20 표기만으로 레귤레이터, 풀업 전원, 핀 순서, 실제 실장 IC를 확정할 수 없다.

## Basys3 연결 안전 게이트

| 항목 | 판단 |
|---|---|
| 전원 | IC 공급 가능 전압과 FPGA I/O 허용 전압은 다르다. 보드별 전원 확인 전 직결 금지 |
| SDA/SCL | 오픈드레인, 3.3V 풀업만 FPGA에 연결. 5V 풀업 금지; 병렬 저항 확인 |
| I²C 주소 | 0x38, 동일 주소 복수 장치는 별도 버스/멀티플렉서 검토 |
| XADC | 아날로그 0~1V 입력이 아닌 I²C 디지털 센서 |
| 드라이버 | 초기화, busy, 측정 대기, 20비트 데이터 조립, CRC 유무는 실제 데이터시트 리비전별 확인 전 구현 금지 |

## 출처·버전·권리

- [ASAIR AHT20 데이터시트 V1.0.03](https://asairsensors.com/wp-content/uploads/2021/09/Data-Sheet-AHT20-Humidity-and-Temperature-Sensor-ASAIR-V1.0.03.pdf) — 원본 PDF 링크; 저작권/재배포 허가 미확인, 복제하지 않음. 조회 2026-11-03.
- [Seeed Grove AHT20 판매 정보](https://www.kiwi-electronics.com/en/grove-aht20-i2c-industrial-grade-temperature-humidity-sensor-10016) — 판매처의 제품 번호와 보드 변형 참고; 원본 회로도 및 리비전 미확인.
- [Aosong AHT20 부품 판매 정보](https://www.lcsc.com/product-detail/Temperature-and-Humidity-Sensor_Aosong-Guangzhou-Elec-AHT20_C2757850.html) — IC 정보 교차 참조.
- Git SHA: 위 제조사·판매처 웹/PDF에는 해당 없음. 공개 GitHub 라이브러리 커밋·라이선스는 미확인; 코드를 복사하지 않음.
- Keyestudio/Freenove/Arduino Hardware Docs/국내·AliExpress의 **동일 기판 특정 모델** 및 회로는 이번 회차 확인하지 못함.

## 적대적 검증 (2026-11-03)

- **P0:** 5V 풀업 기판의 Basys3 직결 금지. 3.3V 전원/핀맵/풀업 레일을 실물에서 확인할 때까지 미승인.
- **P1:** 주소 충돌, 20비트 데이터 조립과 CRC 처리, 명령 타이밍·FSM은 구현 및 시험 미실시.
- **문서:** 원본 링크 명시, 외부 그림·코드 미복제. 신규 파일 자체 상대 링크 없음.
- **미실시:** 저장소 전체 링크 검사, CI, Vivado 합성·타이밍, 실제 보드 시험. 정식 상태표/기계 판독 데이터/HTML 반영은 보류.
