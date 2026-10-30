/* DHT22(AM2302) 드라이버 — `rtl/dht22_axil_*` 레지스터 위에서 동작. 비트 타이밍은 RTL 이 하고 이 드라이버는 결과 레지스터만 읽는다.
 * 근거: 프레임 40비트 = 습도 16 + 온도 16 + 체크섬 8, 수집 주기 2 s 초과, 습도 0.1 %RH·온도 0.1 °C 단위, 범위 0~100 %RH·-40~80 °C
 *   (gpio/dht22.md, D1 스파크펀 호스팅 데이터시트). 온도 부호와 체크섬 규칙은 그 사본에 없어 RIOT-OS drivers/dht/dht.c 의 코드로 확인:
 *   온도 비트 15 가 1 이면 음수이고 크기는 하위 15비트(부호-크기 표현), 체크섬은 앞 4바이트 합의 하위 8비트 (D3, 웹 가져오기 요약 경유).
 *   두 근거가 서로 독립인지는 모름(RIOT 코드가 데이터시트를 보고 쓰였을 것).
 * 레지스터는 `rtl/dht22_axil_regs.v` 머리말: CMD 0x00 [0]=1 읽기 시작(busy 중이면 무시), STATUS 0x04 busy/done/err/csum_ok,
 *   RH 0x08, TEMP 0x0C, CHK 0x10, IRQ_EN 0x14. done 은 새 명령을 줄 때 지워진다.
 * 설계안: 폴링 상한(DHT_SPINS), 범위 밖 값 거부, 최소 간격을 지키는 dht22_read_paced. DHT11 은 형식이 달라 지원하지 않는다.
 * 보드에서 실행해 보지 않았다. 호스트 모의 레지스터 시험만. */
#ifndef DHT22_H
#define DHT22_H
#include <stdint.h>

#ifndef DHT_RD
#define DHT_RD(base, off)    (*(volatile uint32_t *)((uintptr_t)(base) + (off)))
#define DHT_WR(base, off, v) (*(volatile uint32_t *)((uintptr_t)(base) + (off)) = (v))
#endif

#define DHT_REG_CMD    0x00
#define DHT_REG_STAT   0x04
#define DHT_REG_RH     0x08
#define DHT_REG_TEMP   0x0C
#define DHT_REG_CHK    0x10
#define DHT_REG_IRQEN  0x14
#define DHT_ST_BUSY    (1u << 0)
#define DHT_ST_DONE    (1u << 1)
#define DHT_ST_ERR     (1u << 2)
#define DHT_ST_CSUM    (1u << 3)

#define DHT_MIN_PERIOD_MS 2000     /* 수집 주기 2 s 초과 */
#define DHT_SIGN_BIT      0x8000   /* 온도 비트 15 = 음수 */
#define DHT_SPINS         1000000u /* 폴링 상한(IP 가 멈춘 경우) */
#define DHT_RH_MAX_X10    1000
#define DHT_TEMP_MAX_X10  800
#define DHT_TEMP_MIN_X10  (-400)

#define DHT_OK            0
#define DHT_ERR_NORESP   (-1)      /* RTL err: 응답 없음 또는 시간 초과 */
#define DHT_ERR_CSUM     (-2)
#define DHT_ERR_ARGS     (-3)
#define DHT_ERR_BUS      (-4)      /* IP 가 busy/done 을 끝내지 않음 */
#define DHT_ERR_RANGE    (-5)      /* 체크섬은 맞는데 값이 센서 범위 밖 */
#define DHT_ERR_TOOSOON  (-6)      /* 지난 읽기 후 2 s 가 안 지남(dht22_read_paced) */

typedef struct { uintptr_t base; uint32_t last_ms; int has_last; } dht22_t;

int dht22_read_raw(uintptr_t base, uint16_t *rh_raw, uint16_t *temp_raw, uint8_t *chk);   /* RTL 결과 그대로. 체크섬 오류면 DHT_ERR_CSUM */
int32_t dht22_temp_x10(uint16_t raw);                                  /* 0.1 °C 단위 부호 있는 값 */
int dht22_read(uintptr_t base, int32_t *rh_x10, int32_t *temp_x10);    /* 변환 + 범위 검사 */
void dht22_init(dht22_t *d, uintptr_t base);
int dht22_read_paced(dht22_t *d, uint32_t now_ms, int32_t *rh_x10, int32_t *temp_x10);   /* 지난 읽기(성공·실패 무관)로부터 2 s 초과일 때만 */
#endif
