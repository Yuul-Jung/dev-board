# tb

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [gpio](../README.md)

호스트 테스트와 레지스터/장치 모델(mock)

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [mock_dht.h](mock_dht.h) | endif |
| [mock_gpio.h](mock_gpio.h) | endif |
| [mock_timer.h](mock_timer.h) | endif |
| [mock_timer_b.h](mock_timer_b.h) | 기준 주소를 구분하는 AXI Timer 모의 (RGB 처럼 인스턴스가 여러 개인 시험용) |
| [mock_us.h](mock_us.h) | endif |
| [model_quad.py](model_quad.py) | rtl/quad_engine.v 의 전이 표(00->10->11->01->00 = +1)와 필터를 옮긴 파이썬 모델. Verilog 검증이 아님. |
| [tb_dht22_engine.v](tb_dht22_engine.v) | 엔진 단독 테스트. 소자 모델이 데이터시트 서술의 파형으로 응답한다. |
| [tb_ultrasonic.v](tb_ultrasonic.v) | 코어 단독 테스트. 트리거 뒤 에코를 만들어 넣어 거리 변환과 타임아웃을 확인한다. |
| [test_analog_sensors.c](test_analog_sensors.c) | 아날로그 센서 변환 시험: 손으로 계산한 값, 경계, 독립 계산(__int128 정수 반올림)과의 대조, 단조성·범위 불변식. 보드 측정이 아니다. |
| [test_axi_gpio.c](test_axi_gpio.c) | 모델의 레지스터 오프셋은 헤더 매크로가 아닌 PG144 요약의 숫자를 직접 적었다(헤더가 틀려도 모델이 같이 틀리지 않도록). |
| [test_basic_io.c](test_basic_io.c) | 기본 입출력 예제 묶음 시험: 자유 실행 시계, 비차단 타이머, 버튼 디바운스, RGB. 자작 AXI Timer 모의에 대한 시험이며 보드에서 확인한 것이 아니다 |
| [test_dht22.c](test_dht22.c) | DHT22 드라이버 호스트 시험. 모의 소자는 dht22_axil_regs.v 의 레지스터 동작(머리말 주석)을 흉내 낸다 — 비트 타이밍(RTL 엔진)은 시험하 |
| [test_gpio_sw.c](test_gpio_sw.c) |  |
| [test_rtttl.c](test_rtttl.c) | RTTTL 해석기·재생기 호스트 시험 (교재 17장). |
| [test_tach.c](test_tach.c) |  |
| [test_ultrasonic.c](test_ultrasonic.c) | HC-SR04 드라이버 호스트 시험. 모의 소자는 us_axil_regs.v 의 레지스터 동작(머리말 주석, 1 을 쓰면 지워지는 timeout·done)을 흉내 |

<!-- index:end -->
