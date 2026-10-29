/* 아날로그 센서 변환 모음 (교재 8·16장 대응): LM35 온도, CdS(광저항) 분압, 가변저항·조이스틱.
 * ADC 에 의존하지 않는 순수 함수다. 전압(mV 또는 µV)을 받아 물리량을 돌려준다. 전압은 ADS1115(ads1115_raw_to_uv) 또는 MCP3008(mcp3008_to_mv)에서 얻는다.
 * 근거: LM35 — TI LM35 데이터시트(웹 가져오기가 돌려준 본문 요약, D1 요약 경유): 출력 10 mV/°C 선형, 전원 4~30 V, 형식별 측정 범위(LM35/LM35A −55~150, LM35C/CA −40~110, LM35D 0~100 °C),
 *   단독으로 50 pF 까지 구동, 용량성 부하에는 75 Ω 직렬 + 0.2 µF 또는 1 µF 댐퍼. 상수는 gpio/ref/analog_sensors_facts.txt 와 tools/crosscheck_pdf.py 로 대조.
 *   CdS·가변저항·조이스틱은 전압 분배식(회로 이론)과 설계안뿐이다. 조도(lux) 환산은 셀마다 다르며 데이터시트 값 없이는 정해지지 않는다(미확인).
 * 한계: LM35 의 정확도 수치는 요약에서 확인하지 못해 쓰지 않았다. 음의 온도는 전원·풀다운 구성에 달려 있어 이 함수는 부호 있는 값을 그대로 환산할 뿐 측정 가능 여부는 호출자가 구성으로 보장해야 한다.
 *   보드에서 실행해 보지 않았다. */
#ifndef ANALOG_SENSORS_H
#define ANALOG_SENSORS_H
#include <stdint.h>

#define AN_OK         0
#define AN_ERR_ARGS   (-1)
#define AN_ERR_RANGE  (-2)      /* 센서 범위 밖이거나 분압이 성립하지 않음 */

/* LM35: 출력 10 mV/°C → 1 m°C 당 10 µV */
#define AN_LM35_UV_PER_MDEGC 10
#define AN_LM35_VS_MIN_MV    4000
#define AN_LM35_VS_MAX_MV    30000
enum { AN_LM35_FULL = 0, AN_LM35_C, AN_LM35_D };          /* LM35/LM35A, LM35C/LM35CA, LM35D */
#define AN_LM35_FULL_MIN_MDEGC (-55000)
#define AN_LM35_FULL_MAX_MDEGC 150000
#define AN_LM35_C_MIN_MDEGC    (-40000)
#define AN_LM35_C_MAX_MDEGC    110000
#define AN_LM35_D_MIN_MDEGC    0
#define AN_LM35_D_MAX_MDEGC    100000

int32_t lm35_uv_to_mdegc(int32_t uv);                       /* 가장 가까운 m°C (반올림, 부호 대칭) */
int32_t lm35_mv_to_mdegc(int32_t mv);
int lm35_in_range(int variant, int32_t mdegc);              /* 1 이면 그 형식의 측정 범위 안, 아니면 0, variant 오류는 AN_ERR_ARGS */
/* 한 번에: 범위 검사까지 하고 m°C 를 *mdegc 에 쓴다. 범위 밖이면 값은 쓰되 AN_ERR_RANGE */
int lm35_read_uv(int variant, int32_t uv, int32_t *mdegc);

/* CdS(광저항) 분압: ADC 전압 v_mv 에서 광저항 값(Ω)을 구한다.
 *   ldr_top = 0: Vcc — R_fixed — (ADC) — LDR — GND      → R_ldr = R_fixed · v / (Vcc − v)
 *   ldr_top = 1: Vcc — LDR — (ADC) — R_fixed — GND      → R_ldr = R_fixed · (Vcc − v) / v
 * v 가 0 이하이거나 Vcc 이상이면 분압식이 성립하지 않아 AN_ERR_RANGE (한쪽 끝에 붙은 값은 저항 무한대/0 에 해당) */
int ldr_ohms(int32_t v_mv, int32_t vcc_mv, uint32_t r_fixed_ohm, int ldr_top, uint32_t *r_ldr_ohm);
#ifndef AN_NO_FLOAT
/* 거듭제곱 모형 R = R10 · (10 lux / E)^γ 를 쓰는 lux 환산. r10_ohm 과 gamma 는 사용하는 셀의 데이터시트 값(이 저장소는 값을 정하지 않음). 모형 자체가 근사다 */
int ldr_lux(uint32_t r_ldr_ohm, uint32_t r10_ohm, double gamma, double *lux);
#endif

/* 가변저항·조이스틱 */
int32_t an_map(int32_t x, int32_t in_lo, int32_t in_hi, int32_t out_lo, int32_t out_hi);   /* 아두이노 map() 과 같은 선형 사상이지만 반올림하고 in_lo == in_hi 를 오류로 취급(오류면 out_lo) */
int an_percent(uint32_t raw, uint32_t raw_max);             /* 0..100, 반올림, raw 가 raw_max 를 넘으면 100 으로 자름. raw_max 가 0 이면 AN_ERR_ARGS */
/* 조이스틱 한 축: 중심 center ± deadzone 안은 0, 바깥은 양 끝(0, raw_max)까지 −100..+100 으로 펴 준다(중심이 정확히 가운데가 아니어도 양쪽이 각각 끝까지 간다) */
int joy_axis(uint32_t raw, uint32_t center, uint32_t deadzone, uint32_t raw_max);
#endif
