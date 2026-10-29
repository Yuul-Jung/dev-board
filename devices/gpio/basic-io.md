# 기본 입출력: LED, 버튼, RGB, 비차단 타이머

<!-- nav -->[저장소 홈](../../README.md) › [devices](../README.md) › [gpio](README.md)

교재 6·7·9·10장(디지털 출력·입력, PWM, delay/millis)에 해당하는 내용을 AXI GPIO(PG144)와 AXI Timer(PG079) 위에서 하는 예제 묶음입니다. 코드: [`sw/axi_gpio.h`](sw/axi_gpio.h), [`sw/axi_gpio.c`](sw/axi_gpio.c), [`sw/basic_io.h`](sw/basic_io.h), [`sw/basic_io.c`](sw/basic_io.c). 시험: [`tb/test_axi_gpio.c`](tb/test_axi_gpio.c), [`tb/test_basic_io.c`](tb/test_basic_io.c). 사실표: [`ref/axi_gpio_facts.txt`](ref/axi_gpio_facts.txt), [`ref/basic_io_facts.txt`](ref/basic_io_facts.txt).

**근거와 한계**: AXI GPIO 값은 PG144 v2.0 PDF 사본을 웹 가져오기가 요약한 본문(D1, 요약 경유 — PDF 원문은 직접 열지 못함)과 Linux `gpio-xilinx.c`(D3)가 서로 일치합니다. AXI Timer 값은 PG079 PDF 텍스트를 직접 확인했습니다(`tools/crosscheck_pdf.py` 가 TCR0 08h 를 pg079 텍스트에서도 찾음). 보드에서 실행한 것은 없고 모두 자작 모델에 대한 호스트 시험입니다.

## AXI GPIO 레지스터 (PG144 v2.0, 요약 경유 D1 + 리눅스 드라이버 D3)
| 오프셋 | 이름 | 의미 |
|---|---|---|
| 0x0000 | GPIO_DATA | 채널 1 데이터 |
| 0x0004 | GPIO_TRI | 채널 1 방향, **1 = 입력, 0 = 출력** |
| 0x0008 / 0x000C | GPIO2_DATA / GPIO2_TRI | 채널 2 (이중 채널 구성일 때만) |
| 0x011C | GIER | 비트 31 = 전역 인터럽트 허용 |
| 0x0120 | IP ISR | 비트 0/1 = 채널 1/2 상태, **쓰기로 토글(1 을 쓴 비트가 뒤집힘)** |
| 0x0128 | IP IER | 비트 0/1 = 채널 1/2 허용 |

GIER/ISR/IER 는 IP 의 인터럽트 옵션을 켠 구성에서만 존재합니다. `C_TRI_DEFAULT` 기본값은 0xFFFFFFFF(전부 입력), `C_DOUT_DEFAULT` 는 0.

- **출력 비트를 읽으면 항상 0** 이라고 PG144 가 적었습니다 → 드라이버는 출력 값을 그림자(shadow)로 기억하고 읽기에서는 입력 비트만 하드웨어 값을 씁니다.
- 인터럽트 상태는 토글 쓰기이므로 `gpio_irq_ack` 는 **실제로 세트된 비트에만** 1 을 씁니다(아니면 0 이던 비트가 1 이 됨). 리눅스 드라이버도 읽은 값을 그대로 다시 써서 해제합니다.
- 미확인 ① 입력으로 설정된 비트에 DATA 를 쓰면 값이 저장되는지(요약은 "쓰기는 효과 없음"이라고만 함) → 출력 전환은 "DATA 쓰기 → TRI 변경 → DATA 다시 쓰기" 순서로 해서 어느 쪽이든 최종 값이 맞게 했습니다. 저장되지 않는다면 출력으로 바뀌는 순간 DATA 의 이전 값이 잠깐 나올 수 있습니다(시험의 `data_latch=0` 모델이 이를 보여 줌). ② 리셋 직후 값: 레지스터 표는 0x0, 동작 절은 C_TRI_DEFAULT 로 적어 서로 어긋납니다(문서가 해결하지 않음) → `gpio_init` 이 TRI 를 전부 입력으로 직접 씁니다. ③ 인터럽트가 레벨인지 에지인지 요약에 없음.

## 함수
| 기능 | 함수 |
|---|---|
| LED 등 출력 | `gpio_pin_mode(g, ch, bit, 1, 초기레벨)`, `gpio_write`, `gpio_toggle`, `gpio_write_mask` |
| 버튼 등 입력 | `gpio_pin_mode(g, ch, bit, 0, 0)`, `gpio_read`, `gpio_read_port` |
| 인터럽트 | `gpio_irq_enable/disable`, `gpio_irq_pending`, `gpio_irq_ack` |
| millis 식 시계 | `mclock_start`, `mclock_millis`, `mclock_micros`, `mclock_cycles` (AXI Timer 0 을 올림 자유 실행) |
| 비차단 주기 | `ticker_init`, `ticker_due` — `if (ticker_due(&t, mclock_millis(&m))) toggle()` |
| 버튼 디바운스·횟수 | `button_init(b, 디바운스ms, active_low)`, `button_update(b, 원시값, 현재ms)`, `button_down`, `button_held_ms`, `b.presses` |
| RGB LED | `rgb_init(r, 베이스 3개, clk, 주파수, 공통 애노드)`, `rgb_set(r, R, G, B)`, `rgb_from_hsv` |

## 설명
- **버튼**: 풀업 입력이면 `active_low=1`. 후보 상태가 `debounce_ms` 동안 바뀌지 않아야 확정되고, 확정된 눌림/놓음에서만 이벤트가 나옵니다. 짧은 글리치는 무시, 같은 눌림은 한 번만 셉니다. 첫 호출은 기준만 잡고 이벤트를 내지 않습니다(시작할 때 이미 눌려 있어도 1회 눌림으로 세지 않음). 시간은 부호 없는 뺄셈이라 32비트 되돌아감에 안전합니다. 디바운스 시간은 근거 자료가 없는 설계안이므로 사용하는 스위치에 맞춰 정하세요.
- **시계**: PG079 에서 올림 카운트(UDT=0)는 한 바퀴가 (MAX_COUNT − TLR + 2) 클럭이고, ARHT=0 이면 넘침에서 **멈춥니다** → 자유 실행을 위해 ARHT=1, TLR0=0 으로 쓰고, LOAD 는 ENT 를 켤 때 지웁니다. 한 바퀴가 2^32+1 클럭이라 넘침마다 1 클럭 어긋납니다(계산: 100 MHz 에서 43 초마다 10 ns). 카운터가 한 바퀴(100 MHz 에서 42.9 초) 안에 최소 한 번은 읽혀야 시간이 줄어들지 않습니다(시험이 한 바퀴 넘게 부르지 않은 경우를 "잃는다"로 고정해 둠).
- **비차단 타이머**: 기준을 한 주기씩 전진시켜 오차가 쌓이지 않습니다. 두 주기 이상 늦으면 밀린 만큼을 버리고 지금으로 다시 맞춥니다(몰아서 여러 번 실행하지 않음, 설계안).
- **RGB**: AXI Timer 인스턴스는 PWM 출력이 1개라서 색 3개에 인스턴스 3개가 필요합니다. 주기 256 클럭 이상(8비트 해상도). 공통 애노드는 핀이 낮을 때 켜지므로 듀티를 반전합니다. 선형 밝기이며 감마 보정은 하지 않습니다. 하이/로우 시간이 2 클럭 미만이 되지 않게 보정하는 것은 `tpwm_set_cycles` 의 제한(하이 ≥ 2)과 로우 쪽 보수적 가정입니다. 0 %/100 % 상수 출력은 기존 PWM 드라이버의 미확인 사항을 그대로 따릅니다([fan-pwm.md](fan-pwm.md) 참고).
- **HSV**: 정수 연산이며 실수 계산과 ±2 이내(시험 전 범위 확인).

## 시험
- `test_axi_gpio.c`: PG144 요약을 옮긴 레지스터 모델(출력 비트 읽기 0, ISR 토글, 없는 레지스터·단일 채널 접근 금지)로 두 가지 변형(입력 비트 쓰기 저장/무시) 모두 실행. 의도적으로 틀리게 만든 사본 40개가 모두 실패로 잡힘. 모델의 오프셋은 헤더 매크로가 아니라 숫자를 직접 적어 헤더 오류를 같이 따라가지 않게 했습니다.
- `test_basic_io.c`: AXI Timer 모델(LOAD 가 카운터를 멈춤, 레지스터 접근 추적)로 시계(넘침 건너기·비정수 클럭·한 바퀴 이상 방치), 주기 타이머, 버튼(채터링·글리치·경계·시간 되돌아감·시작 시 눌림), RGB(공통 애노드·반올림·짧은 시간 보정), HSV 전 범위. 틀리게 만든 사본 46개가 모두 실패로 잡힘.
- 모델은 내가 읽은 문서대로 내가 만든 것이라 해석이 틀리면 같이 틀립니다. 보드에서 실행하지 않았습니다.

<!-- nav-foot -->[gpio 목차](README.md)
