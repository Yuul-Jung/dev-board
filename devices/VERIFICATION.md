# 적대적 검증 결과

## 2026-11-03 AHT20 신규 I13 등록 후 P0/P1 검토

- 조사: 기존 72개 CSV에 AHT20 항목 없음 확인. 기존 I12 VL53L0X와 중복되지 않는 온습도 I²C 후보 1종. 신규 등록 1종, 기존 갱신 0종. 제조사 데이터시트 V1.0.03과 Grove 기판/IC 판매 자료 비교; 특정 무명 기판 회로는 미확인.
- 갱신: `data/module-status.csv` I13 문서 상태, `STATUS.md` 73개 집계, `i2c/README.md`, `REFERENCES.md` 반영. 기존 32차시 및 RTL/C 코드 보존.
- P0: FPGA SDA/SCL은 3.3V 풀업만 허용. 보드의 전원·풀업·핀 배열을 실물 확인하기 전 직접 연결 미승인. XADC 0~1V 및 HC-SR04 5V ECHO는 별도 경로.
- P1: AHT20 I²C 0x38, 초기화·busy·20비트 조립·CRC·측정 타이밍은 드라이버 미구현이므로 미검증. 기존 AXI RTL/C 기능에 신규 변경 없음. 전체 회귀·HTML 생성 일치 및 CI는 별도 실행 결과 전까지 미검증.
- 출처: PDF 원본 버전 명시, Git SHA 없는 웹 원본은 라이선스 미확인; 코드·이미지 복제 없음.


## 2026-11-03 I12 후속 상태표 정합성 감사

- **조사:** 기존 PR #16의 ST VL53L0X IC·Pololu #2490·Adafruit breakout 2종을 재대조했다. 신규 등록 후보 0종, 새 모듈 ID 0건, 기존 I12 문서 상태 갱신 1건. Keyestudio/Freenove/Seeed·국내/AliExpress 특정 기판의 원본 회로도는 확인하지 못했으므로 호환성 추정 금지.
- **갱신:** `data/module-status.csv` I12를 `없음`에서 `문서`로, 경로를 `i2c/vl53l0x.md`로 수정. `STATUS.md`의 I12 행·집계(완비 29, 일부 8, 문서만 8, 없음 25, 제외 2; 합계 72) 동기화. VL53L1X는 미검증이며 별도 모듈 ID를 생성하지 않음.
- **P0/P1 적대적 검증:** VIN=5V일 때 Pololu I²C HIGH 레일이 5V가 될 수 있으므로 Basys3 직접 연결 불승인. XSHUT/GPIO1 비레벨변환, 보드별 풀업/핀맵 미확인 유지. XADC 0~1V 및 HC-SR04 5V ECHO는 별도 경로. 해당 센서 전용 RTL/AXI/C 코드가 없어 비트폭·FSM·타이밍·핸드셰이크 시험 PASS 주장 불가.
- **검증 증거/한계:** CSV I12 원문 단일 행 변경 및 STATUS 집계·행의 정합성 확인. GitHub Actions, 생성 HTML 일치 검사, 전체 상대 링크 검사, 실물 측정은 **이번 후속 커밋에서 미실행**. 생성 HTML 및 전체 CI가 확인되기 전 PR #16 병합/게시 완료로 표시하지 않음.
- **저작권:** Pololu·Adafruit·ST 공식 원본 링크만 인용. 고정 Git SHA 없는 웹 문서는 원본 개정/라이선스 미확인. 그림·소스 재배포 없음.


## 2026-11-03 SG90 서보 무효 설정 fail-safe 후속 검증

**1단계 갱신:** 기존 [TowerPro SG90 Analog](https://towerpro.com.tw/product/sg90-analog/) 제조사 사양(4.8V 외부 전원, 신호 HIGH 3.3V 인식 임계값 미표기)과 [Arduino Modulino Motors](https://docs.arduino.cc/hardware/modulino-motors)(MAX22211, Qwiic I²C 기반, 5~24V 모터 전원)를 다시 대조했다. 둘은 제어 인터페이스가 달라 같은 RTL/핀맵으로 대체할 수 없다. 기존 Keyestudio·Seeed·Freenove 및 국내 판매처의 SG90 변형·핀맵·라이선스 근거는 `motor/sg90.md`에 유지하며 확인되지 않은 사양을 승격하지 않는다. 제조사 웹페이지는 고정 Git SHA와 원본 재배포 라이선스가 없으므로 링크로만 인용한다.

**2단계 적대적 검증:** `servo_axil_regs.v`는 C 드라이버의 `servo_init()` 유효성 검사를 우회하여 PERIOD=0 등 무효 설정을 수락한다. 기존 `servo_engine.v`는 PERIOD=0일 때 마이크로초 카운터가 매 tick 0으로 돌아가고 `w_clamped > 0`이면 PWM이 계속 HIGH가 될 수 있었다. **P1 기능·구동 안전 문제**로 판정한다.

- **수정:** `servo_engine.v`에서 `0 < MIN <= MAX < PERIOD`일 때만 PWM을 생성하고, 위반하면 PWM LOW 및 APPLIED=0으로 만든다. 정상 설정의 클램프·주기 계산은 유지한다.
- **회귀:** `tb_servo_engine.v`에 PERIOD=0, MIN=0, MIN>MAX, MAX>=PERIOD, 정상 복귀 시나리오를 추가하고 `devices-check.yml`에서 실행한다. CI 통과 여부는 PR/최종 main Actions 기록으로 확인한다.
- **P0 유지:** SG90 전원은 FPGA GPIO가 아닌 적합한 외부 4.8V 공급원으로 공급하고 GND를 공유한다. 3.3V 제어 HIGH 인식은 실제 보유품에서 미검증이다. XADC 0~1V, HC-SR04 5V ECHO, I²C 풀업 3.3V 기준도 기존 게이트 유지.
- **미해결:** RTL에서 PWM LOW는 물리적 모터 무전원·기계적 정지를 뜻하지 않는다. 동적 설정 변경 중 글리치 없는 주기 경계 반영, AXI VIP 전체 검증, Vivado 합성·타이밍, 실물 부하·전압 측정은 미실시.


## 2026-11-03 SG90 서보 자료 갱신 → AXI4-Lite 적대적 검증

**1단계 자료 갱신:** [TowerPro SG90 Analog](https://towerpro.com.tw/product/sg90-analog/)·[SG90 360°](https://towerpro.com.tw/product/sg90-360-degree-continuous-rotation-servo/), [Keyestudio KS0194](https://docs.keyestudio.com/projects/KS0194/en/latest/docs/KS0194.html), [Seeed Grove Servo](https://wiki.seeedstudio.com/Grove-Servo/), [Arduino Servo API](https://github.com/arduino-libraries/Servo/blob/440b319ec11ea2664df8ce201c271c935de28a2d/docs/api.md), [Freenove Ultimate Starter Kit](https://github.com/Freenove/Freenove_Ultimate_Starter_Kit/tree/5315f61497eea49553aa1b70721751fb16a497e5), 국내 [디바이스마트 SG90 Analog](https://www.devicemart.co.kr/goods/view?no=12503474)를 교차 확인. **위치 제어형 SG90 Analog와 360° 연속 회전형을 구분**하고, Keyestudio 갈색 GND/빨강 VCC/주황 신호, Grove 커넥터 차이, 외부 서보 전원과 공통 GND, 3.3 V 신호 HIGH 미보증을 `motor/sg90.md`에 기록했습니다. Arduino Servo 기준 커밋 `440b319ec11ea2664df8ce201c271c935de28a2d`(LGPL-2.1), Freenove 기준 커밋 `5315f61497eea49553aa1b70721751fb16a497e5`(CC BY-NC-SA 3.0). 웹 문서의 Git SHA·재배포 라이선스는 미확인; 원본 그림·코드 복제 없음.

**2단계 직후 적대적 검증:** `motor/rtl/servo_axil_regs.v`·`servo_axil_top.v`, `servo_engine.v`, `motor/sw/servo.[ch]`를 확인하고 DC 모터·초음파 AXI 수정 패턴과 대조했습니다.

| 등급 | 발견 사항 | 조치 |
|---|---|---|
| **P0 예방** | 서보 모터 전원 4.8~5 V를 FPGA 3.3 V GPIO/Pmod 신호와 혼동하거나 서보 전류를 I/O 핀에서 공급할 위험 | 서보 별도 전원·공통 GND·전원 용량·실물 핀 순서 확인 전 연결 금지 |
| **P0 미확인** | 보유 SG90의 3.3 V 입력 HIGH 인식 임계값은 제조사 페이지에서 확인되지 않음 | 직접 연결 동작 보증하지 않음. 필요 시 적합한 버퍼 사용 |
| **P1 수정** | `awvalid && wvalid`가 같은 클록에 도착해야만 쓰기 수락 | AW 주소·W 데이터/스트로브 독립 1단 버퍼, 양쪽 수락 뒤 1회 커밋 |
| **P1 수정** | WSTRB 입력 부재, CTRL 및 WIDTH/PERIOD/MIN/MAX에서 부분 바이트 쓰기 불가 | Top WSTRB 전달, CTRL 하위 바이트·16비트 레지스터 하위 2바이트만 선택 쓰기 |
| **P1 지정 검증** | B/R 응답 백프레셔, APPLIED 읽기 전용, 리셋 중 미완료 W 처리 | `tb_servo_axil_regs.v`에 시나리오 추가, CI 연결 |
| **정상 유지** | `servo_engine.v` 펄스 생성, `servo.c`의 정상 입력 검증·각도 변환 | 기능 코드 변경하지 않음. C 헤더 인터페이스 설명만 갱신 |
| **미해결** | AXI 레지스터에 직접 PERIOD=0 등 무효 설정 시 C 드라이버 검증을 우회함 | `servo_init()`의 검증은 유지. HW 레지스터 범위 제한 정책·엔진 fail-safe는 추후 검토 |
| **기존 P0 유지** | XADC 0~1 V, HC-SR04 5 V ECHO, I²C 3.3 V 풀업 | SG90 자체의 인터페이스와 혼동하지 않으며 기존 게이트 유지 |

**검증 범위:** GitHub Actions에서 Python·JSON 파싱, Markdown 상대 링크, HTML 재생성 일치, C 호스트 모의시험, 기존 AXI RTL 시험 및 신규 서보 AXI Top 컴파일·지정 시나리오 실행을 확인한다. **PR #13 지정 검증 성공:** [GitHub Actions 실행 41551779497](https://github.com/Yuul-Jung/dev-board/actions/runs/41551779497)에서 Python·JSON, Markdown/HTML 링크, HTML 재생성 일치, C 호스트 시험, 기존 초음파·1-Wire·DHT22·엔코더·DC 모터 AXI 시험 및 **신규 서보 AXI Top 컴파일/지정 시나리오** 단계가 성공했다. 이는 PR 브랜치 검증이며 병합 후 main 최종 SHA와 CI는 별도로 확인한다. AXI VIP 전수 검증·Vivado 합성·타이밍·Basys3 실물 서보 구동/전압 측정은 수행하지 않았다.


## 2026-11-03 DC 모터 자료 갱신 → AXI4-Lite 적대적 검증

**1단계 자료 갱신:** ST [L298 공식](https://www.st.com/en/motor-drivers/l298.html), [Keyestudio KS0063](https://docs.keyestudio.com/projects/KS0063/en/latest/docs/KS0063.html), [Seeed Grove I2C Motor Driver v1.2](https://wiki.seeedstudio.com/Grove-I2C_Motor_Driver_V1.2/), Arduino [Motor Shield Rev3](https://docs.arduino.cc/hardware/motor-shield-rev3)·[Modulino Motors](https://docs.arduino.cc/hardware/modulino-motors), [Freenove 키트](https://github.com/Freenove/Freenove_Ultimate_Starter_Kit/tree/5315f61497eea49553aa1b70721751fb16a497e5), 국내 [디바이스마트](https://www.devicemart.co.kr/goods/view?no=1278835)를 교차 확인했습니다. 칩/직접 GPIO·PWM 기판/ATmega8L 경유 I²C/Arduino Shield/MAX22211 Qwiic I²C를 구분하고, 모터·로직·I²C 전원과 점퍼·풀업·핀맵을 분리했습니다. `motor/l298.md`, `REFERENCES.md` 갱신. Arduino_Modulino 기준 커밋 `24d08a94ac4e8f83aed211df0fa6f38e9994b164`(MPL-2.0 확인), Freenove 기준 커밋 `5315f61497eea49553aa1b70721751fb16a497e5`(라이선스 NOASSERTION, 재배포 미승인). 제조사 웹 문서의 SHA는 미확인으로 기록하고 원본 그림·코드는 복제하지 않았습니다.

**2단계 직후 적대적 검증:** `motor/rtl/dcmotor_axil_regs.v`·`dcmotor_axil_top.v`와 `motor/sw/dcmotor.[ch]`를 검토하고, 이전 검증된 AXI AW/W·WSTRB 구조와 비교했습니다.

| 등급 | 발견 사항 | 조치 |
|---|---|---|
| **P0 예방** | L298N ENA 점퍼가 5 V 레일에 묶인 상태에서 FPGA PWM 구동 위험 | 구매한 기판의 점퍼 제거·5 V 결선 확인 전 연결 금지 명시 |
| **P0 예방** | Grove I²C Motor Driver v1.2는 L298N이지만 I²C 버스 5 V | 3.3 V Basys3 SDA/SCL 직결 금지, 양방향 레벨 변환·풀업 전원 확인 |
| **P1 근거 충돌** | Seeed v1.2 문서의 2 A 설명과 0.5 A/채널 사양표 상충 | 2 A 연속 구동으로 확정하지 않고 미확인 처리 |
| **P1 수정** | `awvalid && wvalid` 같은 클록 동시 도착만 쓰기 수락 | AW/W 각각 1단 버퍼, 양쪽 수락 후 1회 커밋 |
| **P1 수정** | `WSTRB` 포트 부재로 바이트 부분 쓰기 무시 | Top WSTRB 연결, CTRL은 하위 바이트, DUTY/PERIOD는 4바이트 선택 쓰기 |
| **P1 검증** | B/R 응답 백프레셔 및 STATUS 읽기 전용 유지 | `tb_dcmotor_axil_regs.v`에서 순서별 AW/W·WSTRB·동시 수락·리셋·B/R 유지 검증 |
| **정상 유지** | `dcmotor_engine.v`, `dcmotor.c`의 PWM·모드·듀티 기능 | 기능 코드는 수정하지 않고 C 헤더의 AXI 설명만 갱신 |
| **미해결** | `servo_axil_regs.v`, PS/2·ILI9341 등 다른 AXI 래퍼 | 이번 회차 범위 밖. 다음 회차에서 별도 재현 후 처리 |
| **기존 기준 유지** | XADC 0~1 V, HC-SR04 5 V ECHO, I²C 풀업 전원 | 이번 모터 전용 회로와 혼동하지 않고 기존 P0 게이트 유지 |

**검증:** GitHub Actions [devices-check 실행 41536284637](https://github.com/Yuul-Jung/dev-board/actions/runs/41536284637)의 `Python syntax and structured-data parse`, `Markdown links and HTML regeneration consistency`, `C driver host tests`, 기존 초음파·1-Wire·DHT22·엔코더 RTL 시험, 신규 `AXI4-Lite DC motor RTL directed simulation` 단계가 **모두 성공**했습니다. 신규 RTL은 `iverilog -g2012 -Wall`로 Top 컴파일·레지스터 지정 시나리오를 수행했습니다. 실제 AXI VIP 전수 검증, Vivado 합성/타이밍, 보드·모터 실측은 수행하지 않았습니다. 테스트 통과가 실제 모터 구동 승인을 뜻하지 않습니다.


## 2026-11-03 DHT22·로터리 엔코더 자료 갱신 → AXI4-Lite 적대적 검증

**1단계 자료 갱신:** [Arduino Modulino Thermo](https://docs.arduino.cc/hardware/modulino-thermo), [Modulino Knob](https://docs.arduino.cc/tutorials/modulino-knob/how-knob/), [Keyestudio KS0430](https://docs.keyestudio.com/projects/KS0430/en/latest/docs/KS0430.html)·[KS0431](https://docs.keyestudio.com/projects/KS0431/en/latest/docs/KS0431.html)·[KS0013](https://docs.keyestudio.com/projects/KS0013/en/latest/docs/KS0013.html), [Seeed Grove DHT22](https://wiki.seeedstudio.com/Grove-Temperature_and_Humidity_Sensor_Pro/)·[Grove Encoder](https://wiki.seeedstudio.com/Grove-Encoder/), [국내 DHT22 3핀 판매처](https://eduiot.kr/product/디지털-온습도센서-dht22-고정밀-pcb-모듈-am2302-3pin/), [KY-040 국내 판매처](https://www.11st.co.kr/products/2227891483)를 대조하여 `gpio/dht22.md`, `gpio/rotary-encoder.md`, `REFERENCES.md`를 보완했습니다. 공식 Arduino_Modulino 커밋 `24d08a94ac4e8f83aed211df0fa6f38e9994b164`/MPL-2.0, Seeed Grove DHT 라이브러리 `826ce8f2593f810a6063cb1cbc788fc6519dc25e`/MIT는 LICENSE 파일로 확인했습니다. 원본 이미지·예제코드는 복제하지 않았습니다.

**2단계 직후 적대적 검증:** `gpio/rtl/dht22_axil_regs.v`, `quad_axil_regs.v`와 두 Top, `gpio/sw/dht22.[ch]`, `gpio/sw/quad.h`를 기존 수정된 `gpio/rtl/us_axil_regs.v` 및 [Arm AXI4](https://developer.arm.com/-/media/Arm%20Developer%20Community/PDF/Learn%20the%20Architecture/102202_0100_01_Introduction_to_AMBA_AXI.pdf) 규칙과 비교했습니다.

| 등급 | 발견 사항 | 조치 |
|---|---|---|
| **P0 예방** | DHT22 단품·3핀 PCB·RJ11·Grove의 VCC와 DATA 풀업 전압/핀배열을 동일하게 취급할 위험 | 5 V DATA→3.3 V FPGA 직접 연결 금지, 실물 저항·핀맵·레벨 확인 요구 |
| **P0 예방** | KS0013(5 V)·Grove Encoder(4.5~5.5 V)를 Basys3 GPIO에 직접 연결할 위험 | 전원·A/B/SW HIGH 실측 또는 회로 확인 전 직결 미승인 |
| **P1 분류** | Modulino Thermo(HS3003 I²C), Knob(STM32 경유 I²C)를 DHT22/직접 GPIO 엔코더와 혼동 | 인터페이스·실제 IC·I²C 주소·풀업 전원을 분리해 기록 |
| **P1 수정** | DHT22와 Quad AXI4-Lite의 AWVALID && WVALID 동시 도착 요구 | AW/W 독립 1단 버퍼, 단일 커밋 및 B 응답 백프레셔 구현 |
| **P1 수정** | DHT22/Quad의 WSTRB 포트 누락 및 부작용 무조건 실행 | WSTRB[0] 적용, 0/상위 바이트 쓰기는 레지스터 상태·펄스 무변경 |
| **P1 유지** | Quad STATUS W1C와 새 이벤트의 우선순위 | 새 이벤트가 우선하는 기존 동작 유지 |
| **P1 수정** | C `quad_set_invert()`가 CTRL[1] IRQ_EN까지 지우는 문제 | 읽기-수정-쓰기로 IRQ_EN 보존, 호스트 회귀시험 2건 추가 |
| **미확인** | XADC 0~1 V, 5 V ECHO, I²C 풀업 등은 이번 직접 GPIO 모듈과 다른 경로 | 혼용하지 않음. 기존 HC-SR04·ADS1115 감사 결과 유지 |

**검증 명령(자동 CI 설정):** `python3 devices/tools/check_links.py`, `python3 devices/tools/gen_pages.py --check`, `bash devices/run_host_tests.sh`, `iverilog -g2012 -Wall -s tb_dht22_axil_regs ...` 및 `tb_quad_axil_regs` 지정 시나리오, Top 컴파일. `tb_dht22_axil_regs.v`·`tb_quad_axil_regs.v`를 추가하고 `.github/workflows/devices-check.yml`에 등록했습니다.

**검증 상태:** [GitHub Actions 실행 41518512735](https://github.com/Yuul-Jung/dev-board/actions/runs/41518512735)에서 Python/JSON, Markdown 상대 링크·HTML 재생성 일치, C 드라이버 호스트 시험, 초음파·1-Wire 기존 RTL, **신규 DHT22·Quad AXI4-Lite Top 컴파일 및 지정 시나리오 시뮬레이션이 모두 SUCCESS**로 기록되었습니다(검증한 커밋 `edd90f39225cacc165ec1aa6559ec8ff1e0b9171`). 이후 문서 목록 변경은 기능 코드에 영향을 주지 않지만 최종 커밋 CI도 별도 확인합니다. AXI VIP 전수 검증·Vivado 합성/타이밍·실물 Basys3 연결은 미실시입니다. DHT22/Quad 엔진 기능 코드는 변경하지 않았으며, 엔진의 단독 동작·실물 센서 응답은 별도로 검증해야 합니다.


<!-- nav -->[저장소 홈](../README.md) › [devices](README.md)

작성한 자료를 "틀렸다고 가정하고" 깨뜨려 본 결과입니다. 통과한 항목뿐 아니라 **발견해서 고친 결함**과 **검증하지 못한 것**을 함께 적습니다. 재현 명령은 맨 아래.

## 2026-11-03 DS18B20 모듈 자료 갱신 → 동일 실행 AXI4-Lite 적대적 검증

**① 자료 갱신:** [Analog Devices DS18B20 Rev.6 원문](https://www.analog.com/media/en/technical-documentation/data-sheets/ds18b20.pdf), [Arduino 1-Wire](https://github.com/arduino/docs-content/blob/475b308dd2c9d7e6783b3dbdc9626b848a454ecd/content/learn/05.communication/06.one-wire/one-wire.md), [Keyestudio KS0329](https://docs.keyestudio.com/projects/KS0329/en/latest/docs/KS0329.html), [KS0124](https://docs.keyestudio.com/projects/KS0124/en/latest/docs/KS0124.html), [Seeed Grove DS18B20](https://wiki.seeedstudio.com/One-Wire-Temperature-Sensor-DS18B20/), [국내 판매처](https://www.bliiot.co.kr/product/programmable-resolution-temperature-sensor-ds18b20.html)를 대조하고 `1-wire/ds18b20.md`와 [출처 대장](REFERENCES.md)에 칩/모듈·전원·핀맵·풀업·기생 전원·라이선스 한계를 반영했습니다. Arduino OneWire 기준 커밋 `800f26f3ee6eb446a72e013785cac3700e54cc13`이며, 소스 헤더에 MIT형 고지가 있으나 독립 LICENSE 파일은 확인되지 않았습니다. 원본 코드는 복제하지 않았습니다.

**② 직후 적대적 검증:** `1-wire/rtl/onewire_axil_regs.v`·`onewire_axil_top.v`와 C 드라이버 `1-wire/sw/ds18b20.[ch]`를 기존 수정된 `gpio/rtl/us_axil_regs.v` 및 [Arm AXI4](https://developer.arm.com/-/media/Arm%20Developer%20Community/PDF/Learn%20the%20Architecture/102202_0100_01_Introduction_to_AMBA_AXI.pdf)·[AMD PG267](https://docs.amd.com/r/en-US/pg267-axi-vip/AXI-Protocol-Checks-and-Descriptions)와 비교했습니다.

| 우선순위 | 실제 발견·판정 | 처리 |
|---|---|---|
| **P0 예방** | DS18B20 5 V 동작 지원을 5 V DQ→Basys3 3.3 V GPIO 직접 연결 허용으로 오해 | 3.3 V 외부 VDD·3.3 V DQ 풀업 기준 및 실물 기판·저항 확인 요구. 5 V 풀업 직결 금지 |
| **P1 출처 정정** | Freenove Ultimate Starter Kit의 온도 예제를 DS18B20 1-Wire 구현으로 잘못 인용할 위험 | 원본 Sketch 12.1.1은 A0 서미스터, 22.1.1은 DHT11임을 소스 직접 확인. 1-Wire 근거로 사용하지 않고 CC BY-NC-SA 3.0 라이선스 기록 |
| **P0 예방** | TO-92 칩·Keyestudio 방수 프로브·Grove 기판·EASY plug RJ11을 동일 핀맵으로 오해 | 핀 번호/케이블 색/기판·커넥터 분리, 실물 확인 전 연결 미승인. XADC 0~1 V·I²C 풀업과 1-Wire 구별 |
| **P0 예방** | 기생 전원 DS18B20을 약한 풀업·기존 RTL만으로 지원한다고 오해 | 강한 풀업(변환 명령 후 10 µs 이내) 미구현 명시. 외부 VDD 모드만 설계 범위 |
| **P1 수정** | 1-Wire AXI4-Lite `AWVALID && WVALID` 동시 요구 | AW/W 독립 1단 버퍼·양 채널 수락 후 단일 커밋·BREADY 백프레셔로 수정 |
| **P1 수정** | `s_axi_wstrb` 포트 누락·CMD/IRQ_EN 쓰기에서 WSTRB 무시 | Top→Regs WSTRB[3:0] 전달, 하위 바이트 스트로브만 부작용 허용 |
| **P1 수정(회귀시험 발견)** | ARREADY를 `rstn && !rvalid`로 바꾼 뒤, 읽기 데이터 래치가 ARVALID 없이 ARREADY만 검사하여 임의 읽기 응답을 발생시킴 | 새 테스트벤치가 ARREADY/read value 실패 12건 재현. `take_ar = arvalid && arready`로 읽기 래치 조건 수정. CI 재검증 필요 |
| **P1 확인** | `CMD` 실행 중 busy, `STATUS.done`·`IRQ_EN`·`B/R` 응답 | 기존 명령/IRQ 정책 유지. 새 지향 테스트벤치 `1-wire/tb/tb_onewire_axil_regs.v`와 CI 등록 |
| **유지** | `onewire_bit_engine.v` 슬롯 타이밍, `ds18b20.c` CRC·변환·폴링, 기존 C 호스트 시험 | 기능 소스 수정 없음. C 헤더의 원문 근거·WSTRB 설명만 정정 |
| **P1 추가 발견(미해결)** | `gpio/rtl/dht22_axil_regs.v`·`quad_axil_regs.v`에도 AW/W 동시 요구와 WSTRB 누락이 남음 | **이번 범위 외**. 개별 테스트벤치와 프로토콜 검증 전 수정·검증 완료로 표시하지 않음 |
| **미검증** | 실제 DS18B20 전압·3.3 V 풀업·배선, 1-Wire 아날로그 파형, AXI VIP, Vivado 합성·타이밍·실물 보드 | 실제 측정/통합 검증 전 완료 주장 금지 |

**검증 결과(2026-11-03):** [GitHub Actions #41499100802](https://github.com/Yuul-Jung/dev-board/actions/runs/41499100802) **success** (커밋 `22117f3856463dfef00fdf8c841999cedf95d015`). Python 구문·JSON 파싱 PASS, Markdown 상대 링크 **1,838개 오류 0**, 핵심 HTML 상대 링크 **44개 오류 0**, `gen_pages.py --check` **0개 변경**, C 호스트 모의시험 전체 PASS, 초음파 AXI RTL 회귀시험 **PASS**, 1-Wire Top Icarus Verilog 컴파일 및 **`PASS tb_onewire_axil_regs`** 확인. AW 선행/W 선행/동시 도착, WSTRB=0/상위 바이트, busy 중 명령 무시, done/IRQ, B/R 백프레셔·동시 읽기, ARVALID 없는 상태의 임의 RVALID 방지, 리셋을 검사했습니다. 이 결과는 지정된 RTL 시뮬레이션 범위에만 적용되며 **AXI VIP·Vivado 합성·Timing·실물 센서/보드 검증은 미실시**입니다. 이후 문서 커밋은 별도 CI 확인 대상입니다.

## 2026-11-03 초음파 AXI4-Lite·Modulino 자료 갱신 → 같은 실행 적대적 검증

**① 자료 갱신:** Arduino [Modulino Distance 공식 하드웨어](https://docs.arduino.cc/hardware/modulino-distance)·[ABX00102 데이터시트](https://docs.arduino.cc/resources/datasheets/ABX00102-datasheet.pdf)에서 VL53L4CDV0DH/1, 3.3 V Qwiic, 센서 공급 2.6~3.5 V를 확인했습니다. [Arduino_Modulino](https://github.com/arduino-libraries/Arduino_Modulino) 소스 `24d08a94ac4e8f83aed211df0fa6f38e9994b164`(v0.10.0, MPL-2.0)의 `ModulinoDistance`가 주소 0x29와 VL53L4CD→VL53L4ED 폴백을 구현한 사실을 확인하되, **호환 코드가 실제 실장 IC를 증명하지는 않는다**고 구분했습니다. [일반 HC-SR04 5 V](https://www.sparkfun.com/ultrasonic-distance-sensor-hc-sr04.html), [HC-SR04-33](https://www.sparkfun.com/ultrasonic-distance-sensor-3-3v-hc-sr04.html), [Seeed Grove 1선 SIG](https://wiki.seeedstudio.com/Grove-Ultrasonic_Ranger/), [Adafruit ECHO 5 V](https://learn.adafruit.com/ultrasonic-sonar-distance-sensors/pinouts) 및 기존 Keyestudio/Freenove·국내 판매처의 모델별 차이·저작권 상태를 [출처 대장](REFERENCES.md)과 [HC-SR04 문서](gpio/hc-sr04.md)에서 재확인했습니다.

**② 즉시 적대적 검증:** [Arm AXI4](https://developer.arm.com/-/media/Arm%20Developer%20Community/PDF/Learn%20the%20Architecture/102202_0100_01_Introduction_to_AMBA_AXI.pdf), [AMD PG138](https://docs.amd.com/r/en-US/pg138-axi-ethernet/AXI4-Lite-Interface), [PG267](https://docs.amd.com/r/en-US/pg267-axi-vip/AXI-Protocol-Checks-and-Descriptions)와 실제 `gpio/rtl/us_axil_regs.v`·`us_axil_top.v`·`gpio/sw/ultrasonic.[ch]`·`gpio/tb/test_ultrasonic.c`를 대조했습니다.

| 등급 | 발견·검증 항목 | 실제 조치·상태 |
|---|---|---|
| **P0 예방** | 일반 HC-SR04 ECHO 5 V → Basys3 3.3 V GPIO 직접 연결 | 기존 레벨 변환·공통 GND·실물 전압 확인 기준 유지. XADC 0~1 V와 GPIO 3.3 V 혼동 금지. **실물 연결 승인하지 않음** |
| **P0 예방** | Arduino Modulino Distance Qwiic 3.3 V와 일반 HC-SR04 5 V 혼동 | 실제 IC·3.3 V 전원·I²C SDA/SCL·0x29를 명시. 5 V I²C 풀업은 FPGA에 직결 금지, 실제 기판 풀업·핀 배열 미확인 |
| **P1 수정** | `us_axil_top.v`에 WSTRB 누락 | `s_axi_wstrb[3:0]` 포트와 레지스터 연결 추가 |
| **P1 수정** | `us_axil_regs.v`에서 AWVALID와 WVALID의 동시 유효 요구 | AW/W 독립 1단 수락 버퍼, 양쪽 수락 후 한 번만 쓰기/응답, BREADY 백프레셔 처리 |
| **P1 수정** | 부분 바이트 쓰기 및 STATUS W1C가 WSTRB를 무시 | `WSTRB[0]`이 선택된 경우에만 하위 바이트 갱신, WSTRB=0/상위 바이트만 선택 시 레지스터 부작용 없음 |
| **P1 수정** | C 헤더·센서 문서에 'enable 재활성화 동작은 코어를 읽지 못해 미확인' | 실제 `ultrasonic_distance_meter.v` 소스와 대조해 enable=0 상태 초기화·재활성화 후 첫 1 µs tick 동작으로 정정. **보드 동작은 여전히 미검증** |
| **유지** | 초음파 거리 코어, C 드라이버 함수·오프셋, 기존 호스트 시험 | 정상 기능은 그대로 유지. `ultrasonic_distance_meter.v`, `ultrasonic.c`, `test_ultrasonic.c` 변경 없음 |
| **미해결** | Vivado IP Integrator/AXI VIP, 합성·Timing, 실물 전압/핀·온도·반사 성능 | 미검증. AXI 프로토콜의 전수 형식 검증을 주장하지 않음. 다른 AXI 래퍼에도 유사 패턴이 있는지는 별도 감사 필요 |

**추가 테스트:** `gpio/tb/tb_us_axil_regs.v`는 AW 먼저/W 먼저/동시 도착, WSTRB 0·상위 바이트, W1C·IRQ, B/R 백프레셔, B 응답 대기 중 독립 읽기를 검사합니다. `.github/workflows/devices-check.yml`에서 Icarus Verilog로 `us_axil_top` 컴파일과 레지스터 시뮬레이션을 실행합니다.

**자동시험 확인(2026-11-03, UTC 2026-11-02):** [GitHub Actions #41479846093](https://github.com/Yuul-Jung/dev-board/actions/runs/41479846093) **success** (헤드 SHA `600a816c7877bb7b769e36bfe2f4b34d8f723066`): Python 구문·JSON 검사 PASS, Markdown 링크 **1,828개 오류 0**, 핵심 HTML 상대 링크 **43개 오류 0**, HTML 생성 `--check` **0개 변경**, C 드라이버 호스트 모의시험 **45/45 PASS**, `us_axil_top` 컴파일 PASS, **`PASS tb_us_axil_regs`**. 새 문서만 추가한 후속 커밋은 최종 CI를 별도로 확인합니다. 호스트 시험/RTL 시뮬레이션을 FPGA 합성·실물 보드 검증으로 표기하지 않습니다.

## 2026-11-02 ADS1115 외부 ADC 자료 갱신 → 즉시 적대적 검증 (3차)

**자료 갱신 후 검증 순서:** TI SBAS444E §5.1/§5.3/§7.3.3·Arduino UNO R4·Seeed Grove ADS1115·DFRobot DFR0553·국내 판매 자료 조사 → [ADS1115 기술자료](i2c/ads1115.md) 및 [JSON](data/devices.json)·[출처 대장](REFERENCES.md) 갱신 → 관련 C 드라이버·호스트 모델·I²C 전원·GPIO/XADC 안전 문서 정적 대조 → 실제 결함만 수정. 2026-11-02 원본 웹 문서 확인, 외부 사진/코드 복제 없음.

| 우선순위 | 검증 대상 | 발견·판정·수정 |
|---|---|---|
| **P0 수정** | 기존 `ads1115.md`의 “3.3 V 공급 시 3.6 V까지 입력 허용” | TI **정상 동작 GND~VDD**, **절대최대 GND−0.3~VDD+0.3** 구분. 3.6 V는 **절대최대 스트레스 정격**이지 정상 측정 허용 전압이 아님. 문서·구조화 데이터·C 헤더 주석 수정 |
| **P0 예방** | ADS1115 PGA ±6.144 V를 실제 AIN 6.144 V 입력 가능으로 오인 | PGA는 ADC 출력 코드 스케일. VDD 3.3 V에서 AIN 0~3.3 V 권장. **Basys3 내장 XADC 0~1 V와 ADS1115 외부 AIN을 분리** |
| **P0 예방** | 5 V I²C 풀업 → FPGA 3.3 V SDA/SCL 과전압 | 원본 칩·Seeed Grove·DFRobot DFR0553·국내 판매 모듈을 분리. **DFR0553의 3.3 V HIGH는 제조사 보드 특정 사양**, Seeed/범용 모듈의 풀업 레일은 실물/회로도 미확인. 직결 승인 플래그 false |
| **P1 수정** | `gpio/analog-sensors.md`의 ADS1115 링크가 일반 `i2c/sensors.md`로 연결 | 실제 ADC 설명 `i2c/ads1115.md`로 수정. 링크 자체는 존재했지만 대상 문서가 잘못됨 |
| **P1 수정** | `sw/ads1115.c` 공개 레지스터 API가 NULL 장치/버스/출력 포인터를 역참조 | `ads1115_write_reg`·`ads1115_read_reg`에 `IIC_ERR_ARGS` 가드 추가. `tb/test_ads1115.c`에 5개 회귀 검사 추가. 정상 변환·Config·폴링 경로 유지 |
| **P1 확인** | ADS1115 7비트 주소 0x48~0x4B, Config 비트, MSB 순서, OS 폴링, timeout | `ads1115.c`·`axi_iic.c`·`test_ads1115.c` 정적 대조. 주소 변환(`addr7 << 1`), 비트필드, 원시값 환산은 정상으로 판단해 유지 |
| **P1 미해결** | AXI IIC 실제 FIFO·Repeated START·400 kHz 풀업 타이밍, 외부 ADS1115 실물 연결 | 기존 호스트 모델은 실제 I²C 버스 전기적 특성·실제 IP FIFO·FPGA 연결을 검증하지 못함. 실물·로직애널라이저 시험 전 완료 표시 금지 |
| **P1 기존 미해결** | `gpio/rtl/us_axil_top.v` WSTRB·독립 AW/W 처리 | 이번 ADS1115 수정 범위와 무관. 기존 미해결 유지; 테스트벤치 없이 구조 변경 금지 |
| **저작권·근거** | Seeed/DFRobot GitHub 원본 | Seeed 커밋 `00beed48708bae7cce1f78ad05448e1af132e0a3`, DFRobot 커밋 `6d3df78d29693bf9e245a223afe0443406bd2980`; 양쪽 **MIT LICENSE 직접 확인**, 코드는 복제하지 않음 |

**정적 검증 (이 실행에서 실제 수행):** GitHub 원본 파일의 최신 브랜치 내용을 다시 읽고 JSON 49개 항목 파싱, P0/P1 가드·출처 SHA·주소·PGA 필드·MSB·OS·200회 timeout·5 V ECHO·XADC 0~1 V·I²C 풀업·기존 WSTRB 누락을 코드/문서 문자열 및 구조로 교차 검사(12개 조건 확인). ADS1115 문서 상대 링크 6개를 추출하여 경로 구조 대조. **문자열/구조 정적 확인은 C 컴파일·호스트 모의시험·RTL 시뮬레이션·보드 검증과 다름**.

**자동시험 결과 (2026-11-02, 실제 CI 확인):** [GitHub Actions 41443931037](https://github.com/Yuul-Jung/dev-board/actions/runs/41443931037) **success** — Python 구문·JSON 파싱 PASS, Markdown 상대 링크 **1,827개 오류 0**, 핵심 HTML 상대 링크 **43개 오류 0**, `gen_pages.py --check` **변경 0**, `bash devices/run_host_tests.sh` **45/45 PASS**(ADS1115 NULL 포인터 회귀 포함). **이 결과는 호스트 모의시험이며 RTL 시뮬레이션·합성·실물 ADS1115 전압/풀업·XADC 연결 검증을 의미하지 않음.**

## 2026-11-02 PIR 모듈 갱신 → 같은 실행 적대적 검증 (후속 회차)

**자료 갱신 대상:** [HC-SR501](gpio/hc-sr501.md), [devices.json](data/devices.json), [출처 대장](REFERENCES.md), [RTL10 문서](../rtl/rtl10_pir_event/README.md). 기존 HC-SR04 문서·코드와 AXI 드라이버는 변경하지 않음.

| 등급 | 적대적 대조 | 판정 / 처리 |
|---|---|---|
| **P0 예방** | 'HC-SR501 OUT HIGH 3.3 V → FPGA 직결 가능'이라는 **제품 변형을 무시한 단정** | Joy-IT 제품은 3 V, Espruino 일반형은 3.3 V이나 일부 오픈드레인 또는 VCC 레벨 출력 변형이 있다고 명시. **실제 OUT 전압·풀업·기판 확인 전 직결 승인 금지**로 수정 |
| **P1 확인** | 동작전류 **65 mA**와 **<50 µA** 자료 충돌 | HSHL 65 mA(동작)/<50 µA(대기), Espruino <50 µA(동작)로 상충. 원인 미확인, 오타로 단정하지 않고 전원 예산 미승인 |
| **P1 수정** | 국내 에듀이노 HC-SR501 상품 설명의 '적외선 송신·반사량 측정' | PIR은 **수동 적외선 감지** 방식이므로 해당 판매처의 능동 IR 설명은 기술 근거에서 제외. 가치창조기술의 65 mA 수치도 독립 측정 자료가 없어 상충 상태 유지 |
| **P1 수정** | HC-SR501 3핀과 Seeed Grove PIR 4핀, Arduino Modulino Movement I²C/6축을 혼동할 위험 | 실제 VCC·OUT/SIG·핀 수·전원·프로토콜·소비전류를 제품별 분리. Keyestudio Project 47 DIY PIR도 동일 PCB로 단정하지 않음 |
| **P1 수정** | 구조화 JSON의 `out_high_v: 3.3`만으로 보드 연결을 승인할 위험 | 기존 숫자형 필드는 호환성을 위해 보존하되 **공칭/미검증 표시**와 `fpga_direct_connect_approved: false`를 추가 |
| **P1 수정** | RTL10 README의 'Clock Smoke Test만 작성/기능 TB 미완성'과 실제 `tb_pir_event.v`의 기대값 비교 코드가 불일치 | 테스트벤치가 2-FF 지연·상승/하강·리셋·임의 입력을 검사하도록 **작성된 사실**로 문서 수정. 이번 실행에서 시뮬레이션 통과를 주장하지 않음 |
| **유지** | RTL `pir_event.v`의 비동기 입력 2-FF·펄스·FSM/타이밍, HC-SR04 5 V ECHO, XADC 0~1 V, I²C 5 V 풀업 | RTL10 소스/테스트벤치, HC-SR04, I²C 드라이버 및 C 코드는 변경하지 않음. 기존 P0/P1 안전 경고 유지 |

**정적 검사:** `devices.json` JSON 파싱 성공(49개 항목), 수정 Markdown 3개에서 상대 링크 19건의 경로를 추출해 기존 저장소 파일과 대조(새 링크 대상 존재 확인). `rtl/rtl10_pir_event/src/pir_event.v`와 `tb/tb_pir_event.v`의 포트·기대값을 눈으로 대조. **Icarus Verilog 시뮬레이션·C 드라이버 호스트 시험·합성·실물 전압 측정은 이 회차에서 직접 수행하지 않았음**. GitHub Actions 결과는 별도 확인한다.

**출처/저작권:** Joy-IT·Seeed·Arduino 공식 웹 자료는 URL·제품명을 기록(웹 페이지는 커밋 SHA 해당 없음), Espruino/HSHL은 2차 자료로 구분. Keyestudio/Freenove의 고정 커밋·라이선스 정보는 기존 대장 유지. 원본 사진·PDF·소스의 복제는 없음.

## 2026-11-02 주변모듈 갱신 직후 적대적 검증

**범위:** [HC-SR04](gpio/hc-sr04.md), [구조화 데이터](data/devices.json), [출처·라이선스 대장](REFERENCES.md), `gpio/rtl/ultrasonic_distance_meter.v`, `gpio/rtl/us_axil_regs.v`, `gpio/rtl/us_axil_top.v`, `gpio/sw/ultrasonic.[ch]`, `gpio/tb/tb_ultrasonic.v`, `gpio/tb/test_ultrasonic.c`, `tools/gen_pages.py`와 기존 I²C LCD·XADC 안전 문서.

| 등급 | 대조 항목 | 판정 / 조치 |
|---|---|---|
| **P0 예방** | 일반 HC-SR04 5 V ECHO → Basys3 3.3 V GPIO | Adafruit 공식 핀 설명과 SparkFun 5 V 제품을 대조. **직결 금지**, 분압 예(10 kΩ/10 kΩ → 2.5 V)와 전원·GND·핀 확인을 [센서 문서](gpio/hc-sr04.md)에 명시. **실물 전압·VIH 여유는 미검증** |
| **P0 예방** | XADC에 분압된 ECHO 2.5 V 연결 위험 | XADC는 일반 단극성 0~1 V(AMD UG480)이며 **디지털 ECHO는 GPIO**라고 명시. CdS·조이스틱 문서의 XADC 주의와 교차 확인 |
| **P1 수정** | 일반 HC-SR04 vs HC-SR04-33 vs Grove SIG vs Modulino Distance 혼동 | 제조사 문서로 **전원·신호 핀·프로토콜이 서로 다름**을 표로 분리. Grove 1선 양방향 및 Modulino I²C를 기존 TRIG/ECHO RTL의 대체품으로 분류하지 않음 |
| **P1 확인** | 국내 HC-SR04 판매처의 3.3 V 호환성 표기 충돌 | 아이씨팩토리 HC-SR04+는 3~5.5 V로 표기하나 ECHO HIGH는 미확인. EKU 상품은 제목의 3~5.5 V와 상세의 DC5V/HIGH5V가 서로 모순. **제품 제목만으로 FPGA 직결 승인 금지**, [센서 문서](gpio/hc-sr04.md)에 기록 |
| **P1 수정** | RTL 코어 주석의 `CTRL[2]` vs 실제 `us_axil_regs.v`의 `CTRL[0]` | 실제 enable 포트·레지스터 주소 대조 후 **주석만 `CTRL[0]`으로 정정**, 기능 코드 변경 없음 |
| **P1 수정** | `tools/gen_pages.py`가 수작업 `devices/capstone/index.html`을 자동 덮어씀 | `MANUAL_PAGES`에 등록하여 덮어쓰기 방지, 페이지 누락은 `--check`에서 검출하도록 변경 |
| **P1 미해결** | `us_axil_top.v`에 WSTRB 포트가 없음 | [RTL README](gpio/rtl/README.md)에 **완전한 AXI4-Lite IP 검증 전**이라고 명시. 표준 채널·WSTRB·독립 AW/W 타이밍·응답·백프레셔를 시뮬레이션 후 IP화해야 함. 코드 무검증 재설계 금지 |
| **P1 확인** | Keyestudio/Freenove 저작권·원본 커밋 | Keyestudio 원본 루트에 LICENSE 없음(재배포 권한 미확인), Freenove README/라이선스는 **CC BY-NC-SA 3.0**. 각 GitHub SHA·출처 링크를 [대장](REFERENCES.md)에 기록. 외부 사진·코드 복제 없음 |
| **유지·실행 통과** | HC-SR04 C 드라이버·호스트 모의시험 | 기존 `us_poll`/W1C 플래그 처리·거리 범위·헤더 상수와 RTL 주소를 정적으로 대조. 드라이버 기능 변경 없음. 아래 CI의 `ultrasonic` 호스트 모의시험 **PASS** |
| **P1 수정·재현** | 기존 DNS→HTTP 호스트 모의시험에서 HTTP 본문 sink가 `memcpy()` 뒤 `strlen()`을 다시 호출 | 종료 NUL이 아직 없는 새 데이터에서 버퍼 밖 읽기가 발생할 수 있는 **테스트 코드 결함**. `devices/ethernet/tb/test_dns.c`의 복사 전 길이를 저장해 정확한 끝에 NUL을 쓰도록 수정하고 조각·버퍼 센티넬 회귀 시험 추가. **C 드라이버 자체는 변경 없음** |
| **유지** | I²C PCF8574 LCD 5 V 풀업 위험 | [lcd1602.md](i2c/lcd1602.md)의 SDA/SCL 풀업 전원 미확인 경고 유지. 실제 백팩 회로가 없으므로 안전한 직결로 승격하지 않음 |

**자동 검증 결과 (2026-11-02):** [GitHub Actions 실행 41413151486](https://github.com/Yuul-Jung/dev-board/actions/runs/41413151486) **성공** — Python 구문·`devices.json` JSON 파싱 통과, Markdown 상대 링크 **1,806개 중 오류 0개**, `gen_pages.py --check` 변경 대상 **0개**, C 드라이버 **호스트 모의시험 45개 PASS** (`dns-model`, `ultrasonic` 포함). **ASan/UBSan 및 실제 Verilog 시뮬레이션은 이번 CI에 포함하지 않음.**

**검증 수준 구분:** 위의 정적 원본·소스 대조는 보드 동작 확인이 아닙니다. RTL 32과제의 GitHub Actions 시뮬레이션 기록과 `devices/gpio/rtl`의 초음파 AXI 래퍼 시뮬레이션은 **서로 다른 검증 범위**입니다. 후자는 본 작업에서 합성·타이밍·실물 검증하지 않았습니다. 자동 검사 결과는 CI 실행 후 별도 기록해야 합니다.

## 1. 발견해서 고친 결함
| # | 발견 방법 | 결함 | 조치 |
|---|---|---|---|
| 1 | UBSan (BME280 단위 테스트) | 보정식에서 음수 값의 좌측 시프트(C 에서 미정의 동작) | 곱셈(`x * 2^n`)으로 바꿈 |
| 2 | UBSan + 임의 보정 계수·원시값 200만 회 | 습도식 32비트 중간값 부호 있는 정수 넘침(극단 원시값) | int64 로 계산. 넘침 없는 입력 1,997,142 개에서 32비트 기준식과 결과 일치, 습도 상한(102400 = 100 %RH) 초과 0 |
| 3 | NMEA 퍼저 100만 문장 | 주소 필드 `G,RMC,...` 같은 비정상 문장을 RMC 로 받아들임 | talker 는 대문자 2자 + `RMC,` 만 허용 |
| 4 | 같은 퍼저 | 위도 `23519.00`(235도)이 int32 넘침으로 음수 값이 됨 | 위도 ≤ 90°, 경도 ≤ 180°, 분 < 60, 자릿수 ddmm/dddmm 만 허용. 시·분·초·일·월 범위도 검사 |
| 5 | PG153 원문 대조 | SPI 전송 순서를 2차 자료(FreeBSD)로 썼는데 원문 절차(inhibit 설정 → DTR 채움 → SS → inhibit 해제)와 달랐음 | 원문 p.77 순서로 재작성 |
| 6 | PG153 원문 대조 | SCK 비율 "2, 4, 8, 16×N" 이 틀림(본문이 둘로 갈림), "비율 80" 은 허용 여부 불확실 | 비율 128(781 kHz)로 정정, 선택지는 Vivado 에서 확인하도록 표기 |
| 7 | 문서 모순 검색 | "다룰 예정", "드라이버 아직 작성하지 않았습니다", "SCK 전송 절차 미확인" 등 이미 해결된 문장 | 문서와 soc_map 갱신 |
| 8 | 링크 검사 | 저장소에 문서 간 링크가 거의 없고 폴더 4곳에 README 없음 | README 추가, 모든 문서에 상·하위 이동 링크 |
| 9 | 변형 시험(드라이버·헤더를 sed 로 틀리게 만든 사본, §3) | `an_map` 의 중간 계산이 32비트라 `x - in_lo`·`out_hi - out_lo` 가 넘칠 수 있었고 결과 포화도 없었음 | 64비트로 계산하고 int32 로 포화, 시험 추가 |
| 10 | 같은 변형 시험 | 처음에는 살아남은 변형이 있었음: GPIO 마스크 쓰기의 마스킹 빠짐, ISR 해제가 세트되지 않은 비트에 쓰는 토글, HSV 구간 1·2·4, 시계의 LOAD·TLR0 쓰기, HD44780 입력 모드 코드, 글자 쓰기 뒤 DDRAM 복귀 등 | 시험이 약했던 것이라 해당 경우를 시험에 추가해 모두 잡히게 함(드라이버 결함은 아님) |
| 11 | 변형 하네스 점검 | 하네스가 변형 헤더를 시험에 적용하지 못하고 있었음(시험이 `../sw/x.h` 로 포함해서 원본 헤더가 쓰임) → 헤더 변형이 전부 "살아남음" 으로 나올 뻔함 | 하네스가 `sw/`·`tb/` 구조를 복제해 변형 헤더가 실제로 쓰이게 수정(저장소에는 없는 임시 도구) |
| 12 | W5500 모델 첫 실행 | 시험이 한 번에 통과해 오히려 의심 → 모델이 약했음 | 소켓 명령이 `Sn_CR` 폴링 후에야 효력, SEND_MAC 전제, 쓰기 누락 주입, 닫기 실패 상태 등을 모델에 추가해 변형 86개가 모두 잡히게 함 |
| 13 | UDP 수신(`w5500_udp_recvfrom`) 변형 시험 | 처음에 살아남은 변형: 데이터가 덜 온 패킷 검사에서 헤더 8바이트를 빼먹어도 통과(남은 양이 길이와 같거나 크면 거르지 못함) | "20 중 16 도착(헤더 포함 24 ≥ 20)" 경우를 시험에 추가. 나머지 생존 변형 2개는 동등(헤더 미만 검사가 뒤의 `avail < 8 + len` 에 포함, 길이 0 읽기는 프레임 0개) → 제외 |
| 14 | HTTP 변형 시험 | ① 접속 실패·송신 실패 때 소켓을 닫지 않아도 통과(모델이 SR 을 스스로 0 으로 둬서) → `proto` 해제·DISCON 횟수까지 검사하도록 시험 보강. ② 상태줄 길이 경계를 틀려도 통과 → 앞 줄이 line 버퍼에 남긴 글자를 읽는 경우를 만드는 시험 추가. ③ 코드에 중복 검사 2개(줄 이어쓰기 검사는 이름 공백 검사에 포함, 상태 확인은 `w5500_tcp_disconnect` 가 이미 함) → 동등 변형이라 코드를 단순화 | 시험 보강·코드 단순화 |
| 15 | HTTP 대기 시간 의미 점검 | `http_get` 의 수신 대기 제한이 "총 소요 시간"처럼 동작해, 조각이 천천히 계속 오는 정상 응답이 제한 때문에 실패할 수 있었음(느리게 오는 시험을 만들다 발견) | 데이터가 올 때마다 0 부터 다시 세는 "멈춘 시간" 제한으로 바꾸고 문서에 명시 |
| 16 | 시험 데이터 손계산 | HTTP 시험의 Content-Length(7→8)·청크 크기(8→6→7) 계산 실수 3건 — 해석기 결함이 아니라 시험 문자열 오류 | 시험 수정(해석기는 그대로) |
| 17 | WIZnet 문서 대조 | UDP 페이지 본문은 `Sn_RX_RD` 를 "길이 + 헤더 크기"만큼 올리라는데 같은 페이지 예제 코드는 길이만 더함(서로 상충) | 본문을 따르고 `!val`(알려진 불일치)로 기록, 칩 동작은 미확인 |
| 18 | DNS 시험 작성 중 | 시험 데이터 손계산 오류(NTP 응답의 전송 시각 16진수 0xE9358A80 → 정답 0xE93C7F00)와 시험 상태 누적(와이어 기록이 호출 사이에 안 지워짐), 시험 문자열이 의도와 달라 기대값이 틀린 곳 몇 군데 — 해석기 결함이 아니라 시험 쪽 오류 | 시험 수정 |
| 19 | DNS 변형 시험 | 처음 한 번에 109개가 모두 잡혀서 오히려 의심 → 변형이 실제로 적용됐는지(`NOCHANGE`·컴파일 실패 없음), 무한 루프 변형(점프 횟수 제한 제거 등)이 시간 초과로 잡힌 것인지 확인. 변형 목록의 동치 후보(용량 검사 등)는 제외 | 결과 109/109, 한계는 "내가 떠올린 변형 범위" |
| 20 | DHCP 시험 작성 중 | 시험 기대값 오류 2건(T1 3000 은 기본 T2 3150 보다 작아 유효한데 기본값으로 돌아간다고 적음, 쓰기 누락 시험에서 이미 같은 값이 들어 있어 검출 안 됨 → 먼저 다른 값으로 채움)과 FRAME 시험이 남긴 RX 상태가 다음 시험을 오염(모델이 CLOSE 때 수신 버퍼를 비우는지 모름 → 시험이 직접 비움) — 클라이언트 결함이 아니라 시험 쪽 | 시험 수정 |
| 21 | DHCP 변형 시험 | 처음 87/101 → 살아남은 14개를 하나씩 분석: 반복 옵션(첫 값)·T1/T2/DNS 길이·종류 32 이상(시프트 넘침으로 OFFER 로 오인)·수신 오류(FRAME)·IR 정리·서버 ID 없는 NAK·대기 시간 경계가 시험에 없었음 → 시험 추가로 101/101. 한 바이트 넘어 읽는 변형은 일반 빌드에서는 결과가 같아 ASan 빌드에서만 잡힘 | 시험 보강, 한계는 "내가 떠올린 변형 범위" |
| 22 | JSON 시험 작성 중 | 시험 쪽 오류 5건: 깊이 시험 문자열 길이를 잘못 셈(33 이어야 하는데 34), `"000"` 은 JSON 이 아닌데 숫자 변환 입력으로 씀, 범위 시험에서 `5`×10^9 가 이미 범위 밖, 위조 조각 시험에 닫는 따옴표가 없어 의도와 다른 경로, 서러게이트 쌍 U+2FA1D 의 UTF-8 을 손으로 잘못 계산(F0 AF A0 9D → 파이썬으로 다시 구한 F0 AF A8 9D). 경로 문법 오류가 "키 없음"에 가려지던 것은 **실제 결함**이라 코드 수정(경로 문법을 먼저 전부 확인) | 시험 수정 + 코드 1건 수정 |
| 23 | JSON 변형 시험·독립 비교 | 처음 90/115 → 살아남은 25개를 하나씩 분석: 시험 부족 14건(대문자 F, 리터럴 마지막 글자, 작은따옴표/등호 구분자, BOM 바이트, `\u` 3자리, 4바이트 인코딩 비트, 문자열 최소 길이, 지수 방향, 긴 지수 상쇄, 래퍼 cap 등)은 시험 추가, 동치 11건(유효숫자 17/18, 곱셈·버림 루프 경계 등 결과가 같고 오버플로 방지용인 것)은 이유를 확인하고 제외. 별도로 변형 문서 3만 건을 파이썬 `json` 과 비교해 판정 불일치 0건 | 104/104, 한계는 "내가 떠올린 변형 범위", 파이썬 비교는 유효/무효 판정만 |
| 24 | 블루투스 퍼저(임의 바이트 60만 회) | `hm10_parse` 가 응답 안의 NUL·제어·비ASCII 바이트를 걸러내지 않아 `OK+AB\0CD` 같은 입력이 성공으로 해석되고 태그가 C 문자열로 잘려 바이트 수와 어긋남. 보레이트가 어긋났을 때 나오는 쓰레기 바이트가 성공 응답으로 읽힐 수도 있음 | 응답 본문은 0x20~0x7E 만 허용(설계안), 아니면 `BT_ERR_REPLY`. `hc06_reply_version` 에도 같은 검사. 퍼저에 독립 기대값 추가 |
| 25 | 블루투스 시험 작성 중 | 시험 쪽 오류: `AT+NAMEBLE-1`(12자)·`AT+PASS`+6자리(13자) 길이를 11·12 로 잘못 셈 — 드라이버 결함 아님 | 시험 수정 |
| 26 | 블루투스 변형 시험 | 처음 114/129 → 살아남은 15개를 하나씩 분석: 동치 7개(예: PIN 길이를 `!=` 대신 `>` 로 바꿔도 뒤의 숫자 검사가 NUL 을 거절해 결과가 같음) 제외, 하네스 문제 2건(sed 구분자 `/` 충돌로 변형이 적용되지 않음, 의미 없는 줄), 시험 부족 7건 보강(응답 안의 0x00 바이트, PIN 의 '9', 버전 출력 버퍼 경계, `Set` 이 아닌 응답, 빈 `want`, 한 글자 응답). 한 바이트 넘어 읽기 변형은 `exact()` 가 NUL 자리 1바이트를 더 잡아 ASan 이 못 잡았음 → `malloc(n)` 로 정확한 크기 사용 | 시험 보강 후 비동치 121개 모두 잡힘(ASan 빌드 기준) |
| 27 | 모듈 보드 대조(디바이스마트·에듀이노·GitHub·분해 글) — L298N | `l298.md` 의 "ENA 점퍼가 꽂혀 있으면 PWM 이 안 될 수 있음"은 위험을 줄여 말한 것: 점퍼가 ENA 를 모듈 +5 V 레일에 묶으므로 FPGA 가 구동하면 5 V 와 맞붙음(근거 4건 일치). 환류 다이오드·감지 저항 "기록하지 않음"이던 공백도 확인: 다이오드 8개, SENSE 는 GND 직결(전류 감지 불가, 분해 글 1건) | 문서에 "점퍼를 빼야 한다"로 정정, 모듈 보드 표 추가(모터 전원 한계는 칩 46 V 가 아니라 모듈 35 V, 78M05 쓰려면 약 7 V 이상 등) |
| 28 | 같은 대조 — HM-10 전원 | 문서/헤더의 "모듈 보드는 3.3 V 로 켤 수 있어 Pmod 전원과 맞다"는 칩 모듈 단품에만 해당. 아두이노용 브레이크아웃은 3.3 V 레귤레이터가 있고 VCC 정격이 3.6~6 V(Martyn Currey)라 Pmod 3.3 V 로는 못 켤 수 있음. 문서끼리도 칩 전압 범위가 다름(2.5~3.3 V / 2.0~3.6 V, 최대 3.9 V) | `uart/modules.md`·`hm10.h` 정정, devices.json 에 `breakout_vcc_v` 추가 |
| 29 | 같은 대조 — HC-06·HD44780 백팩 | HC-06: 보레이트 범위 양 끝(1200·1382400)과 기본값 9600 이 판매사 슬라이드와 일치 → 사실표 3항목에 두 번째 근거 추가(그전엔 같은 저자의 글 두 건뿐이라 독립 근거 없음). 백팩: P0=RS·P2=E 배치는 코드 1건과 일치하지만 포럼 글 2건은 서로·코드와 다르게 적음 → 배치가 보편이 아님 | `i2c/lcd1602.md` 에 "근거가 갈림"과 5 V 전원/풀업 안전 문제 추가 |
| 30 | 선정 기준 적용(아두이노→디질런트) — 디질런트 대응품 조사 | ① Pmod BT2 회로도를 읽어 보니 매뉴얼 핀표의 "VCC 3.3V/5V"는 틀리고 6·12번 핀이 모두 VCC3V3(레귤레이터 없음) — 본문 "3.0~3.6 V"가 맞음. ② Pmod HB5 매뉴얼의 "Enable High 중 Direction 변경 금지"가 저장소 엔진의 방향 전환 보호와 방향이 일치. ③ HB5 는 DIR/EN 2핀·브레이크 없음이라 같은 RTL 에 그대로 못 꽂힘. ④ Pmod CLS 는 ATmega48 시리얼 모듈이라 `hd44780_*` 와 접근이 다름. ⑤ 아두이노 쉴드 회로도 PDF 는 allowlist 로 열지 못함 | [SELECTION.md](SELECTION.md) 신설, 열지 못한 것은 미확인으로 표기 |
| 31 | SELECTION.md 나머지 분류 조사(2026-10-30) — 18개 분류 Pmod 대응 | ① Pmod GPS(MT3329 UART 9600) — 저장소 NEO-6M 과 UART/NMEA 인터페이스 동일, NMEA 파서 재사용 가능(설계안). ② Pmod ALS — SPI ADC, lux 환산식 없음(저장소 BH1750 I2C 와 인터페이스·방식 다름). ③ Pmod AMP3 — SSM2518 I2S+I2C, 12핀 Pmod(D1 요약 경유). ④ Pmod CAN — MCP2515(저장소와 동일 칩), TJA1050 3.3 V 적합성은 미확인. ⑤ Pmod NIC100(ENC28J60) — 하드웨어 TCP/IP 스택 없음, W5500 드라이버와 완전히 다른 구조. ⑥ Pmod OLED — SSD1306 SPI 128×32, 저장소 I2C 드라이버와 초기화 순서가 다름(설계안). ⑦ Pmod MEM 칩 종류·판매 여부 미확인. ⑧ TMP2·GYRO·RTCC·I2S2·CAN·AD1/AD2/Da3/DA4 참조 매뉴얼 미열람(D3). | [SELECTION.md](SELECTION.md) 대응표에 18개 분류·서술 절 추가 |
| 32 | 28BYJ-48 스테퍼(ch.27) 구현 + 소스 변형 검사(2026-10-30) | ① 시험 자체의 오류: 하프 스텝 15 rpm 은 스텝 600 Hz 를 넘어 드라이버가 거절하는 것이 맞는데 시험이 허용을 기대했음 → 시험을 고침(8 rpm 허용, 15 rpm 거절을 명시). ② 문서가 근거 없이 "책의 rpm"을 단정 → "책이 쓰는 속도는 모름"으로 정정. ③ 변형 294개 중 1차 61개가 살아남아 경계 시험이 약함이 드러남: 속도 한계 경계(간격×한계 주파수 ≥ 1e6)·±3,600,000 도 경계·정확히 한 간격 늦은 poll 의 재동기·재초기화 시 상태 지우기를 겨냥한 `t_edges` 추가 → 53개. 남은 53개를 읽어서 분류: 반환 상수 0 == STEPPER_OK 라 동치, 오류 코드·`STEPPER_HZ_OPERATING` 등 기호로만 쓰이는 상수, `PAT[8]` 크기, 하위 계층(gpio)이 이미 거르는 중복 인자 검사(채널·shift), 도달 불가 방어 코드(`n > 0x7FFFFFFF`, `iv > 0xFFFFFFFF`, `mrpm > MRPM_MAX`는 최대 한계 1000 Hz 때문에 항상 먼저 걸림), `dir==0`·`remaining==0` 이 앞에서 걸러져 `>`와 `>=`가 같은 곳, GPIO 쓰기 실패 전파(모의는 실패하지 않음). 형식상 증명한 것이 아니라 코드를 읽고 판단한 분류다 | `stepper.c` 변경 없음, 시험 `test_stepper.c` 에 `t_edges`, 사실표 27항목 + crosscheck 등록 |
| 33 | SSD1306 그리기(ch.23) 구현 중 기존 코드 강화 | 기존 `oled_hline`/`oled_rect`/`oled_text` 가 극단 좌표(INT_MIN/INT_MAX)에서 정수 넘침·수십억 번 반복 가능했음(60k 회 극단 무작위 호출 시험에서 확인) → 64비트 범위 계산과 클리핑으로 고침, 글꼴 배율 8 제한. 시험 쪽: `logb` 기호 충돌(→`txlog`), 'A' 글자 기대 그림이 틀렸음(렌더링으로 확인해 수정), `|| 1` 자리표시 검사 제거 | `ssd1306.c` 수정, `test_oled_gfx.c` 신설(성질 기반) |
| 34 | SSD1306 소스 변형 검사 | 변형 1688개 중 **1082개가 살아남음**(1차). 원인: ① 5×7 글꼴 표 475칸 ±1 변형 약 800개 — 시험이 7글자 모양만 봤음. ② 기존 3×5 숫자 글꼴(`oled_text`)의 모양·배율 맞춤을 아무도 시험하지 않았음. ③ 전송 계층(대비·표시·반전 바이트, 프레임버퍼가 페이지 순서 그대로 나가는지, `oled_clear`, `oled_init` 인자 검사·flip=1 바이트·단계별 실패)이 기존 시험에 없었음. ④ **비트맵 바이트 색인 `i>>3`→`i>>4` 가 살아남음**(폭 9 시험은 우연히 같은 값) — 폭 1~40 무작위 데이터를 점마다 독립 계산과 비교하도록 보강해 잡힘. 보강 후 글꼴 표를 뺀 792개 중 136개 생존(표 변형은 FNV 고정값으로 죽는 것을 손으로 2개 확인, 재실행은 안 함; ④는 재실행 없이 변형 하나만 다시 확인) | `test_ssd1306.c`·`test_oled_gfx.c` 보강. 남은 136개를 읽고 분류: `oled_pixel` 이 다시 거르는 중복 클리핑(hline/rect/bitmap/print 의 `>=`↔`>` 경계, 약 70개), 배율 1·8 경계에서 `<`↔`<=`(값이 같아 동치), 선·삼각형 부호 비교의 동치 경계, 컴파일 시 크기 상수 `buf[16]`·`g[][5]`, 도달 불가한 방어 검사. **형식상 증명이 아니라 코드를 읽고 한 판단**이며, 5×7 글꼴 고정값은 모양이 맞다는 근거가 아니라 회귀 방지용 |
| 35 | RTTTL(ch.17) 구현 + 소스 변형 검사 | ① **결함**: `rtttl_set_gap_pct` 를 재생 시작 뒤에 부르면 지금 나는 음에 적용되지 않았음(시험 중 발견) → 지금 음의 끝 시각을 다시 계산하도록 고침. ② 시험 자체 오류 2건(끝 쉼표를 오류로 기대, `c5,,d` 가 정상 음표라는 점을 놓침)과 모의 PWM 레지스터 값 계산 실수(주기 = TLR0+2, 하이 = TLR1+2)를 고침. ③ 변형 421개 중 1차 98개 → 시험 보강(공백 종류·대문자 키·키 경계 a/z·값 없는 키·4자리 숫자·`x::c` 이후 음표·변화 없는 소리는 레지스터를 다시 쓰지 않음·쉼표로 시작해도 부저가 켜져 있으면 끔·간격 즉시 적용·간격 상한 49/50/51·곡 도중 PWM 실패) 후 75개. 남은 75개 분류: 반환 상수 0 == RTTTL_OK 동치, 플래그 값 1→2 처럼 참/거짓만 쓰는 곳, 주파수 표 ±1/64 Hz(정수 Hz 반올림에 영향 없음, 22개), 헤더의 쓰이지 않는 상수(`RTTTL_NAME_MAX`, `RTTTL_A4_HZ`), 동치인 중복 검사(`*p==0` 은 다음 줄 키 검사가 거름), **ASan 빌드에서만 잡히는 2개**(NUL 을 넘어 읽는 변형 — ASan 으로 직접 확인함). 기준 변형 도구는 임시 도구이며 저장소에 없다 | `rtttl.c` 수정, `test_rtttl.c`, 사실표 + 알려진 불일치 3건(`!val`) |
| 36 | 현황 정리(`STATUS.md`) 중 상태 판정 대조(2026-10-30) | **판정 오류 7건**: ① 드라이버 C 3개(`ds18b20.c`·`dht22.c`·`ultrasonic.c`)를 부르는 호스트 시험이 하나도 없는데 「완비」로 셌음(Verilog tb 는 있으나 시뮬레이터가 없어 미실행) ② HC-SR501·릴레이는 전용 드라이버·시험이 없는데 「완비」로 셌음 ③ microSD·ENC28J60 은 조사 문서가 있는데 「없음」으로 셌음. MCP2515 가 두 번 들어 있음(중복) | 「완비」 30→25종. 세 드라이버는 「일부」, 둘은 「문서만」으로 내리고 둘은 올림. 세 드라이버의 호스트 시험은 이어서 작성함(행 37~39) → 「완비」 28종 |
| 37 | DS18B20(`1-wire/sw/ds18b20.[ch]`) 호스트 시험 작성 — 기존 예시 드라이버를 적대적으로 읽음(2026-10-30) | **결함 5건**: ① 변환 대기를 "생략"해 두어(코드 주석에도 적혀 있었음) Convert T 직후 스크래치패드를 읽으면 전원 투입 값 +85 °C 또는 이전 값이 나옴 ② CRC 를 검사하지 않음 ③ 설정 바이트의 해상도·미정의 하위 비트를 무시 ④ busy/done 폴링에 상한이 없어 IP 가 멈추면 영원히 돎 ⑤ 헤더·기준 주소 인자가 없어 시험할 수 없는 구조. 시험 쪽 오류 2건(긴 나눗셈 CRC 모델의 항 위치, 버스 사망 시험의 경계) | 드라이버를 다시 씀(CRC·µ°C 변환·변환 폴링·상한·오류 코드). 변형 검사: **1차의 「0개 생존」은 무효** — 하네스가 출력의 "ALL PASS"를 요구하는데 시험이 그 문구를 내지 않았음. 문구를 넣자 52/250 생존 → 원인: 모의 레지스터가 헤더 매크로를 그대로 써서 상수를 바꿔도 같이 바뀜(리터럴로 교체), busy 중 명령 쓰기·버스 도중 사망·폴링 횟수 시험 없음(추가) → 19/270 생존, 읽어서 분류: 동치/미사용 상수 13(리턴 0=DS_OK, `bits>12`↔`>=12`, 비트 0 시프트, 선언 삭제 등), 배열 크기·`<=` 변형 6은 ASan 빌드에서만 잡힐 것으로 **읽어서 분류했을 뿐 증명하지 않음** |
| 38 | DHT22(`gpio/sw/dht22.[ch]`) 호스트 시험 작성(2026-10-30) | **결함 4건**: ① 폴링 상한 없음 ② 온도를 원시값으로만 돌려주고 부호 처리 없음(음수 표현은 문서에 "미확인"이라 적혀 있었음) ③ 센서 범위 검사 없음 ④ 2 s 간격을 호출자 책임으로만 둠. 근거: 온도 부호·체크섬은 데이터시트 사본에 없어 RIOT-OS 코드 한 곳(D3)으로만 확인 — 독립 근거 아님. 시험 쪽 오류 2건(무응답 시 STAT 읽기 횟수, 32비트 시계 넘침 경계 계산) | 드라이버를 다시 씀(`dht22_read_raw`·`dht22_temp_x10`·범위 검사·`dht22_read_paced`). 변형 8/115 생존, 전부 동치(DHT_OK=0 리턴, 초기화에서 쓰지 않는 값, 선언 삭제)이거나 드라이버가 쓰지 않는 레지스터 상수(IRQEN, 비트 0 시프트 — 사실표가 대조). 한계: 모의 레지스터는 RTL 머리말 주석을 읽고 만든 것이라 RTL 과 어긋나도 모름 |
| 39 | HC-SR04(`gpio/sw/ultrasonic.[ch]`) 호스트 시험 작성(2026-10-30) | **결함 3건**: ① `us_read_cm` 에 상한이 없어 enable 이 꺼져 있으면 영원히 돎 ② 거리 범위(2~400 cm) 검사 없음 — 코어는 최대 999 cm 까지 내므로 반사 오류 값이 그대로 나감 ③ 비차단 읽기·정지 함수 없음. 미확인: enable 을 껐다 켜면 측정이 새로 시작하는지(코어 소스를 읽지 못함) | 드라이버를 다시 씀(`us_init/start/stop/poll/read_cm`). 변형 15/75 생존: 동치 2(리턴 0, 인자 검사 중복), 사용하지 않는 레지스터·비트 상수 13(사실표가 대조) |
| 40 | DC 모터 드라이버(`motor/sw/dcmotor.[ch]`, L298·TB6612) 다시 읽기(2026-10-30) | **결함 4건**: ① **RTL 의 DUTY 레지스터가 16비트**인데 기본 주기가 100000 클럭이라 듀티 66 % 넘는 값이 잘렸다(100 % → 34464 = 약 34 %, 75 % → 약 9.5 %). 기존 시험은 모의 레지스터가 32비트로 저장해 통과했다 — 모의가 실제를 가린 사례 ② `period * percent` 가 32비트라 주기 4290만 클럭 넘으면(100 MHz 에서 약 2.3 Hz 이하) 넘침 ③ mode 가 0~3 밖이면 `mode << 1` 이 enable·다른 비트를 건드림 ④ 쓰기 실패를 알 방법이 없음(void) | RTL 을 DUTY 32비트로 넓힘(regs·top·engine·tb, **시뮬레이션 못 함**). 드라이버: 64비트 곱셈, mode 검증, DUTY 를 쓴 뒤 다시 읽어 다르면 CTRL=0 으로 멈추고 `MOT_ERR_DUTY` — 예전 16비트 RTL 비트스트림에서도 조용히 틀린 속도로 돌지 않음. 시험의 모의 레지스터는 리터럴 오프셋 + DUTY 폭(16/32) 선택 가능. 변형 5/50 생존, 전부 동치(`<`↔`<=` 경계, 같은 값으로 클램프, 리턴 0) |
| 41 | SG90 서보 드라이버(`motor/sw/servo.[ch]`, 교재 26장) 신규 작성(2026-10-30) | 시험이 한 번에 통과(실패 0) — **통과를 믿을 근거가 아직 없다**: 변형 검사와 사실표 대조를 사용자 요청(「검증은 나중」)으로 하지 않았음. 미확인: SG90 방향·펄스 폭 범위(2차 자료), 레지스터 맵은 RTL 주석 기준 | 드라이버·모의 레지스터 시험·문서 추가, 상태는 「일부」로 둠. 다음 검증 때 변형 검사와 대조를 먼저 할 것 |
| 42 | CdS 상대 밝기·슈미트 스위치(`gpio/sw/analog_sensors.[ch]`에 추가, 2026-10-30) | 사용자 결정으로 lux 환산 대신 상대 밝기(원소스, 감도는 외부 R_fixed)로 감. 시험은 한 번에 통과(실패 0) — **변형 검사 미실시라 통과를 믿을 근거가 아직 없음**(사용자 요청으로 뒤로 미룸). 미확인: R_fixed 선택 기준은 셀 자료·실측 필요 | 함수 3개 추가, 문서 갱신. 상태는 「일부」 |
| 43 | 프로젝트 문서(기술 트리 2026-10-30)와 저장소 대조(2026-10-31) | **문서가 오래된 곳 4건**: ① `SELECTION.md` 가 28BYJ-48 을 「문서만」으로 적음(실제는 드라이버·모델 시험 있음) ② `README.md` 가 선정 기준 조사를 「모터·블루투스·문자 LCD 만」으로 적음(전 분류 완료) ③ `CATEGORIES.md` 가 「4개 분류만」으로 적음 ④ CSV 메모의 「이번 세션」은 시점이 사라지는 표현 | 문장 수정, STATUS.md 재생성. 이 4건은 자동 검사가 못 잡는 문장이라 앞으로도 상태가 바뀔 때 `grep "문서만\|드라이버 없음"` 로 SELECTION·CATEGORIES 를 같이 봐야 함 |
| 44 | 같은 대조: 프로젝트 문서 §6 하드웨어 점검 11항목과 소자 문서 대조 | **경고 누락 3건**: ① CdS(`gpio/analog-sensors.md`)에 XADC 0~1.0 V 입력 범위 주의 없음 ② `i2c/README.md` 에 풀업이 5 V 인 모듈의 SDA/SCL 5 V 문제 없음 ③ MFRC522(`spi/comm-storage-io.md`)에 UID 단독 인증의 보안 한계 없음 | 세 곳에 추가. 출처는 프로젝트 문서(설계안·미확인)이며 UG480 원문은 열람하지 못함 |
| 45 | `arduino/` 01~16차시 링크·구조 점검(2026-10-31) | **구조 결함 2건**: ① 02~08차시와 01차시 README 가 루트 `examples/` 에 잘못 놓여 `arduino/README.md` 링크 8개와 01차시 내부 링크가 깨짐(링크 검사 25건 실패) ② 09~16차시 개요 조사 오류 7곳 | `arduino/examples/` 로 이동, 조사 수정. 링크 검사 통과 |
| 46 | `arduino/` 14·16차시 스케치를 `devices/` 값과 대조(2026-10-31) | **결함 2건**: ① 14차시가 HC-SR04 규격 범위(2~400 cm) 밖 값을 그대로 출력 ② 16차시가 `requestTemperatures()` 로 최대 750 ms 를 기다려 한 바퀴 약 1.8 s, 거리 경고가 그만큼 늦음 | 범위 검사 추가, 16차시는 비차단(millis). **스텁 헤더로 문법만 확인, 실제 컴파일·보드 시험 없음** |
| 47 | 같은 대조: 번호 체계 | 저장소에 「32차시」가 두 종류(Arduino 트랙, 프로젝트 문서의 SoC 트랙)이고 번호가 달라 혼동 가능(09차시가 millis 타이머 대 서보) | SESSION-MAP.md·arduino/REVIEW.md 에 명시 |
| 48 | `rtl/` 32과제를 GitHub Actions(Icarus)에서 처음 실행(2026-10-31, 런 41189056034) | **결함 1건(2과제)**: RTL08·RTL09 의 `reg [1:0] state` 에 상태 4·5 를 대입해 0·1 로 잘림 → `done` 에 도달 못해 TB 타임아웃. 손으로 32개 TB 를 추적했을 때는 놓침 | 3비트로 수정(248f08e), 폭 검사 `lint_widths.py` 를 CI 에 추가(변형 검사 통과). 수정 뒤 32/32 PASS |
| 49 | RTL03 UVM 첫 실행(런 41189224402) | **결함 1건**: `led_test` 가 없는 `env.agent.scoreboard` 를 참조해 컴파일 실패(스코어보드는 `led_env` 소속) | `env.scoreboard` 로 수정(b6b6111), 런 41189772574 PASS. 로그를 못 내려받아 `::error` 주석으로 원인 확인 |
| 50 | `rtl/` TB 검출력: 변형 검사 `mutate.py`(런 41190240306) | 변형 222개 중 **114개만 잡힘(51.4 %)**, 108개 살아남음. 특히 RTL10·11·13·15·25 는 잡힌 변형 0, RTL08·09 는 5개 중 1개. 일부는 동치 변형(리셋값 등)이라 사람 판독 필요 | 미수정. TB 보강이 남은 과제. 결과 기록만 함 |
| 51 | `devices/tools/gen_pages.py`(ChatGPT 수정분) 실행 | **결함 2건**: ① 한 줄에 리터럴 `\n` 이 들어가 SyntaxError ② 정규식·join 의 역슬래시가 이중이라 `re.PatternError` → 페이지 생성기가 아예 돌지 않았음(= `--check` 도 못 돎) | 수정 후 인덱스·페이지(165개) 재생성, `--check` 통과 |
| 52 | 최신 main(d6bd7e8) 독립 대조: 런 ID↔커밋 SHA↔RESULTS.md | ChatGPT 의 TB 보강(dcc6410) 뒤 변형 검출 51.4 → 54.5 %. 그러나 RTL08·09 TB 는 에지 수만 봐 변형 5개 중 4개 생존(데이터 순서·래치·주기 미검사) | TB 보강(3d259fa), 런 41191680387 PASS, 변형 57.2 %(127/222). 생존 95건 동치 판정은 미실시 |
| 53 | 변형 검사 도구 v2 자체 검증(2026-10-31, 런 41195043505·41195453934) | 기준선 가드가 TB 결함 3건을 걸러냄: RTL02(진리표 00 이 입력 변화 없이 X), RTL14(루프 클럭 진행 누락), RTL12(임의값 편향 커버리지 미달) — 모두 **내 TB 의 결함**이며 RTL 소스는 정상 | TB 만 수정(898ac8b, 4ae45f8), 이후 directed 32/32 PASS |
| 54 | ChatGPT 4-a 동치 판정 대조 | `CW=(X>1)?…:1` 의 `>`→`>=` 는 RTL01·02·04·06 에서 **비동치**(X=1 이면 `$clog2(1)=0` → 길이 0 복제 컴파일 오류, 컴파일 거부로 검출). RTL07 만 동치. RTL04 `256→257` 은 비동치 후보가 아니라 pwm_out 기준 **동치** | MUTATION_CLASSIFICATION.md §4 에 근거 기록 |
| 55 | TB 2차 보강(RTL01·02·04·06·07·12·14·16·18·19·21·22·23·26·27·29·30·31·32) | 변형 검출률 A 57.2 → 95.9 %, B 63.9 → 84.7 %. 최종 생존 20건은 전부 동치 판정(근거 `equivalent.txt`) → 실질 100 %. **RTL 소스 결함은 새로 발견되지 않음** | 런 41195651507. 연산자 집합 한정·동치는 논리 추론이라는 한계 명시 |
| 56 | Verilator lint(5.032) 첫 실행(런 41194273028, 41195043489) | LATCH·MULTIDRIVEN 0건. 스타일 경고 20건(WIDTHEXPAND 13, 조건식 WIDTHTRUNC 6, RTL29 `near` 예약어 1) | 대입 잘림(RTL08·09 유형)은 CI 실패 조건으로 추가 |

## 2. 원문 대조 (코드의 상수·레지스터 ↔ 근거)
`tools/crosscheck_pdf.py`: 두 가지 방식이 섞여 있습니다.
- **PDF 텍스트 검색**: PG079·PG090·PG096·PG142·PG153(AMD)과 MCP2515(DS20001801K)·nRF24L01+(Product Specification v1.0) 텍스트에서 코드 헤더의 오프셋·비트 이름·옵코드를 찾는다 — AMD 86 + MCP2515 33 + nRF24L01+ 89 = 208항목 전부 일치(MCP2515·nRF24L01+ 는 `mcp2515.txt`·`nrf24.txt` 가 있을 때만 검사).
- **사실표 대조**(PDF 를 직접 열지 못한 소자): 저장소 안의 사실표(`*/ref/*_facts.txt`)에 데이터시트 본문 요약에서 옮긴 값을 적고 헤더의 `#define` 과 대조한다. W5500 149(UDP 헤더 5 추가, +알려진 불일치 4, 실패로 세지 않음), MAX7219 21, AXI GPIO 22, AXI Timer TCR0 1, HD44780 44, 아날로그 센서(LM35 등) 9, SNTP 19, DNS 26, 그 밖 소자, HC-06 20(알려진 불일치 2 포함, 근거 둘 일치 3 포함), HM-10 22(알려진 불일치 4 포함)항목. 사실표 블록만 돌린 총합은 **378항목 중 자동 대조 실패 0**(이번 작업 전 336 → +42). PDF 텍스트까지 넣은 총합은 이번에 PDF 텍스트가 없어 다시 돌리지 못했고, 직전 기록 544 에 42 를 더한 586 은 계산값이다.
- 음성 대조: 오프셋을 틀리게(0x68→0x6A) 또는 비트를 어긋나게(CPOL 3→5, inhibit 8→9) 바꾼 사본, 그리고 이번에 추가한 사실표 소자의 헤더 상수(MAX7219 SHUTDOWN 주소, HD44780 대기 하한, AXI GPIO ISR 주소, LM35 범위)를 하나씩 틀리게 한 사본은 모두 실패로 잡힘.
- **한계(중요)**: ① PDF 텍스트 검색은 "숫자와 이름이 같은 줄/가까운 줄에 있다"만 본다. 비트의 의미(방향, 리셋값)는 사람이 PDF 를 읽어 확인한 것이고 자동 검사가 아님. ② **사실표는 PDF 원문이 아니라 웹 가져오기(WebFetch) 요약을 옮긴 것**이다. 요약은 모델이 가공한 텍스트이므로 원문과 다를 수 있고, 두 번째 독립 출처가 없는 항목(MAX7219, LM35, HD44780 의 초기화 대기·버스 타이밍)은 요약 자체의 오류를 잡지 못한다. 두 출처가 있는 항목(W5500 ↔ NuttX, AXI GPIO ↔ Linux, HD44780 명령 ↔ Wikipedia)만 서로 일치하는지 확인했다. ③ PG135(Ethernet Lite)·PG308(I2S)는 코드가 없어 대조 대상 아님.

## 3. 코드 시험
- 호스트 테스트 43개(링크 검사·사실표 대조 포함, `FUZZ=1` 이면 53개; 이번에 ds18b20·dht22·ultrasonic 3개 추가) 전부 통과, ASan+UBSan 빌드에서도 통과(수정 후). 이번에 추가: w5500-model, max7219-model, axi-gpio-model, basic-io-model, analog-sensors, hd44780-model, ntp-model, http-model, dns-model, dhcp-model, json-model, bt-modules, fuzz-http, fuzz-dns, fuzz-dhcp, fuzz-json, fuzz-bt, json-vs-python. **Task 3 추가**: rn-modules(RN-42/RN4871 명령·응답 모의), hb5(Pmod HB5 DC 모터 방향·PWM 안전 순서), pmod-cls(Pmod CLS SPI LCD 이스케이프 시퀀스).
- **변형 시험**(임시 도구, 저장소에 없음): 드라이버·헤더를 sed 로 의도적으로 틀리게 만든 사본에 대해 시험이 실패하는지 본다. 결과: W5500 86/86(+UDP 수신 23/23), SNTP 60/60, HTTP 85/85, DNS 109/109, DHCP 101/101(일반 빌드 100/101, 한 바이트 넘어 읽기 변형은 ASan 빌드에서만), JSON 104/104(동치 11개 제외, 일반 빌드 102/104), 블루투스(btmod·HC-06·HM-10) 121/121(동치 7개 제외, ASan 빌드), MAX7219 44/44, AXI GPIO 40/40, 기본 입출력(시계·타이머·버튼·RGB) 46/46, 아날로그 센서 47/47, HD44780 75/75 모두 잡힘(동작이 같은 동등 변형은 목록에서 제외하고 이유를 확인함). 이 수치는 "시험이 그 변형을 잡는다"일 뿐 드라이버가 옳다는 증명이 아님.
- CAN: 임의 메시지 20만 개 왕복(표준/확장, RTR, DLC 0~8) 일치. 비트 타이밍은 클럭 8종 × 속도 10종에서 모든 해가 정확한 비트레이트·필드 범위를 지키고, 해가 없다고 한 11개는 독립 전수 탐색(파이썬)과 일치.
- BME280 온도 25.08 °C 는 BMP280 데이터시트 예제와 일치. 압력은 0.02 Pa 차이(기억에 의존한 기준값이라 ±8 LSB 허용).

## 4. 검증하지 못한 것 (믿지 마세요)
- **`devices/` 전용 Verilog RTL**: 초음파·DHT22·PS/2·엔코더 등의 실제 RTL 시뮬레이션·합성·보드 시험은 이 문서의 호스트 모의시험 범위 밖입니다. 별도 `rtl/` 32과제는 GitHub Actions의 Icarus/Verilator 검증 기록(위 48~56행)이 있으므로 '모든 Verilog 미실행'으로 일반화하지 않습니다.
- **요약 경유 근거**: W5500, MAX7219, AXI GPIO(PG144), LM35, HD44780 은 PDF 원문이 아니라 웹 가져오기 요약에서 값을 옮겼다(각 소자 문서에 표기). 표 머리글이 깨졌다고 요약이 밝힌 부분(HD44780 전압 열)은 추정이다.
- **새 드라이버 전체가 실제 소자·보드에서 실행된 적 없음**: W5500(TCP/UDP), DNS·NTP·HTTP 응용, MAX7219, AXI GPIO·버튼·RGB·시계, HD44780(+PCF8574 백팩). 특히 HD44780 은 실제 모듈의 핀 배치·전원·대비, MAX7219 는 CLK 휴지 레벨과 자릿수 방향, AXI GPIO 는 입력 비트 쓰기 효과와 리셋 값 불일치가 미확인.
- **모의 장치 순환 논리**: 드라이버 테스트의 모의 IP 는 내가 읽은 문서대로 내가 만든 것이라, 문서 해석이 틀리면 테스트도 같이 틀린다. 실제 보드·Vivado 시뮬레이션이 필요.
- 2차 자료(D3) 기반 항목: HC-05, HC-06(같은 저자의 글 2건), HC-SR501, 릴레이, PS/2 프로토콜, TTP223, 엔코더. 목록은 [TODO.md](TODO.md).
- 원문이 없는 값: ILI9341 타이밍·초기화, PWM 0 %/100 %, NMEA XOR 범위, GGA 필드, CAN RXNEMP 재세트, MCP2515 필터·슬립·오류 처리(드라이버 미작성) 등. MCP2515 SPI 명령·모드 레지스터는 PDF 로 확인 완료.
- BME280 보정식은 PDF 추출 텍스트(습도 괄호 복원)이고 Bosch 원본 소스와 직접 대조하지 않음.
- **이번 추가분(스테퍼·OLED 그리기·RTTTL)**: 모두 호스트 모의로만 시험. 스테퍼는 IN1..IN4 ↔ 코일 A..D 대응·회전 방향·부하 시 한계 주파수·감속비(64 대 63.68395) 중 실제 값이 미확인. OLED 는 5×7 글꼴 표의 출처 문서가 없고(기억 의존, 눈으로 확인), 최대 SCL 과 하드웨어 스크롤이 미확인·미작성. RTTTL 은 명세 자료 셋이 서로 충돌(기본 옥타브 6 대 4, 1박의 기준, 옥타브 4 의 A)하고 내가 고른 해석으로 기대값을 만든 순환이 있으며, 부저 사양을 몰라 실제 소리는 확인하지 못했다.
- 인터럽트 구동, 실제 타이밍, FIFO 넘침, 동시 접근은 어디서도 시험하지 않음.
- **모듈 보드 회로 근거의 한계(행 27~29)**: 디바이스마트·에듀이노 상품 페이지에는 회로도가 없어 근거가 되지 못했고, GitHub(`arc12/Bluetooth-Modules-Eagle`)은 사용자 모듈이 아니라 직접 설계한 HM-10 브레이크아웃이라 모듈 사실로 쓰지 않았다. L298N 회로 연결은 분해 글 1건(작성자가 오류 가능성을 적음)에 기대고, 값은 모두 웹 가져오기 요약 경유다. **"스케메틱이 확정된" 사용자의 실제 모듈 회로도는 아직 받지 못했으므로**, 위 표는 일반 시판 보드에 대한 것이며 실제 모듈로 다시 대조해야 한다.
- **블루투스 모듈**: 모듈 시뮬레이터가 같은 문서 요약에서 옮긴 규칙이라(순환) 실제 모듈이 문서와 다르면 못 잡는다. 응답 지연·조각 간격·무음 구간 적정값, HM-10 의 서로 다른 응답 접두, HC-06 펌웨어별 명령 차이, I/O 레벨은 미확인. 무음 구간이 응답 조각 사이 간격보다 짧으면 응답이 앞 조각에서 끊긴다(시험으로 확인한 한계). 송신(`uartlite_write`)은 TX FIFO 가 계속 가득이면 영원히 기다린다(UART Lite 자체의 동작이라 모듈이 죽어도 비워지긴 하지만 시간 제한은 없음).

## 5. 재현
```
cd devices
./run_host_tests.sh                    # 기본 40개(링크·사실표 대조 포함)
SAN=1 FUZZ=1 ./run_host_tests.sh       # ASan/UBSan + 퍼저·전수
python3 tools/check_links.py           # 문서 링크
python3 tools/gen_status.py > STATUS.md; python3 tools/gen_sessions.py > SESSION-MAP.md; python3 tools/gen_crosswalk.py > TREE-CROSSWALK.md   # 현황표·차시 대응(CSV 에서 생성)
python3 tools/gen_index.py --check; python3 tools/gen_pages.py --check                      # 폴더 인덱스가 최신인지
pdftotext -layout pgNNN.pdf pgNNN.txt  # 각 PG 문서를 pg079.txt pg090.txt pg096.txt pg142.txt pg153.txt (그리고 mcp2515.txt, nrf24.txt) 로 변환해 한 폴더에 두고
python3 -I tools/crosscheck_pdf.py <폴더>              # PDF 텍스트 블록 + 사실표 블록(PDF 포함 총합은 이번에 못 돌림, 계산상 586)
python3 -I tools/crosscheck_pdf.py --facts-only         # 사실표 블록만(524항목, PDF 텍스트 없이 저장소의 ref/ 만으로 돎, run_host_tests.sh 에도 포함)
```


## 2026-11-03 I12 VL53L0X 자료 조사 → 문서 갱신 → 적대적 검증

- **조사:** 기존 I12 후보(VL53L0X/L1X) 중 VL53L0X에 대해 Pololu #2490·Adafruit 기판과 ST IC를 분리했다. 신규 후보로 검토한 VL53L0X는 기존 상태표에 이미 있어 **새 모듈 ID 0건**, 신규 기술 문서 1건, 기존 모듈 1건 자료 보강으로 집계한다. Keyestudio/Freenove/Seeed·국내·AliExpress 특정 PCB의 공식 회로 및 라이선스는 미확인.
- **갱신:** [i2c/vl53l0x.md](i2c/vl53l0x.md) 신규, REFERENCES 출처·버전·라이선스 상태 추가. 기존 32차시·RTL·C는 수정하지 않음.
- **P0 적대적 검증:** Pololu 기판 SDA/SCL은 VIN 레일로 레벨 변환되므로 VIN=5V에서 Basys3 직접 연결 불가. Adafruit GPIO와 Pololu XSHUT/GPIO1은 I²C 레벨 시프터를 경유하지 않는다. 무명 보드 전원/풀업/핀맵 미검증 → 실물 연결 미승인. XADC 0~1V 및 HC-SR04 ECHO 5V는 해당 I²C 센서와 구분.
- **P1 적대적 검증:** 기본 주소 0x29, 다중 센서 주소 재설정 시 XSHUT 필요. VL53L0X 초기화/API, RTL·AXI·C 핸드셰이크·비트폭·FSM·타이밍·회귀시험은 구현·검증되지 않았으므로 PASS로 표시하지 않음.
- **출처/저작권:** 원본 링크만 인용. 고정 Git SHA가 없는 제조사 웹 문서는 열람일을 기록하고 라이선스 미확인으로 표시. 원본 코드/그림 복제 없음.
- **검증 상태:** Markdown 상대 링크·CI·Vivado 합성·실물 측정은 이 기록 작성 시점에 아직 실행되지 않음. 새 문서의 상대 링크 경로는 저장소 기준 확인 필요. 게시 여부 미확인.
- **다음 작업:** module-status.csv의 I12를 문서만으로 변경하고 생성된 STATUS.md/HTML을 일괄 재생성·CI 확인, 무명 기판 실물 회로 검증, I²C 소프트웨어/API 범위 확정.
