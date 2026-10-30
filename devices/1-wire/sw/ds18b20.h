/* DS18B20 드라이버 (AXI4-Lite 1-Wire 마스터 `rtl/onewire_axil_*` 위에서 비트 단위로 동작).
 * 근거: DS18B20 데이터시트(웹 가져오기 요약 경유, D1 요약 경유): 스크래치패드 9바이트(0,1 온도 LSB/MSB, 2 TH, 3 TL, 4 설정,
 *   5~7 예약, 8 CRC), 설정 바이트 비트 6:5 = R1R0 (9/10/11/12비트 = 1Fh/3Fh/5Fh/7Fh), 온도는 부호 확장 2의 보수 16비트,
 *   9/10/11비트일 때 하위 3/2/1비트 미정의, 전원 투입 값 +85 °C(0550h), 변환 시간 93.75/187.5/375/750 ms,
 *   CRC 다항식 x^8+x^5+x^4+1 (스크래치패드 0~7 바이트), 외부 전원일 때 Convert T 뒤 읽기 슬롯이 0 이면 변환 중, 1 이면 끝.
 *   명령: Skip ROM CCh, Convert T 44h, Read Scratchpad BEh, Write Scratchpad 4Eh.
 * 레지스터는 `rtl/README.md`(CMD 0x00: [1:0] 0=리셋 1=쓰기 2=읽기, [2] 쓸 비트 / STATUS 0x04: busy, done, presence, rbit).
 * 설계안: 변환 대기를 읽기 슬롯 폴링 횟수(max_polls)로 제한한다(슬롯 약 80 µs, 750 ms ≈ 9400 회). 기생 전원은 지원하지 않는다
 *   (강한 풀업 제어가 RTL 에 없음 — 기생 전원이면 변환 상태 비트를 읽을 수 없어 이 드라이버는 시간 초과로 끝난다).
 * 미확인: 예약 바이트 5~7 값(문서 안에서 "모두 1"과 FFh,0Ch,10h 로 갈림 → 이 드라이버는 값을 보지 않음).
 * 보드에서 실행해 보지 않았다. 호스트 모의 소자(비트 단위 슬레이브) 시험만. */
#ifndef DS18B20_H
#define DS18B20_H
#include <stdint.h>

#ifndef OW_RD
#define OW_RD(base, off)    (*(volatile uint32_t *)((uintptr_t)(base) + (off)))
#define OW_WR(base, off, v) (*(volatile uint32_t *)((uintptr_t)(base) + (off)) = (v))
#endif

#define OW_REG_CMD  0x00
#define OW_REG_STAT 0x04
#define OW_OP_RESET 0u
#define OW_OP_WRITE 1u
#define OW_OP_READ  2u
#define OW_ST_BUSY  (1u << 0)
#define OW_ST_DONE  (1u << 1)
#define OW_ST_PRES  (1u << 2)
#define OW_ST_RBIT  (1u << 3)

#define DS_OK           0
#define DS_ERR_ARGS    (-1)
#define DS_ERR_BUS     (-2)    /* 마스터 IP 가 busy/done 을 끝내지 않음 */
#define DS_ERR_NODEV   (-3)    /* 리셋 뒤 presence 없음 */
#define DS_ERR_CRC     (-4)
#define DS_ERR_TIMEOUT (-5)    /* 변환이 max_polls 안에 끝나지 않음(기생 전원 포함) */

#define DS_BUS_SPINS   100000u /* busy/done 폴링 상한 */
#define DS_CRC_POLY_REFLECTED 0x8C   /* x^8+x^5+x^4+1 의 비트 반사형 */
#define DS_CFG_BASE      0x1F        /* 설정 바이트 비트 4:0 은 1, 비트 7 은 0 */
#define DS_CFG_RES_SHIFT 5           /* R1R0 = 비트 6:5 */
#define DS_POWERON_RAW   0x0550      /* 전원 투입 직후 온도 레지스터 = +85 °C */
#define DS_TCONV_MS_9    94          /* 93.75 ms 를 올림 */
#define DS_TCONV_MS_12   750
#define DS_CMD_SKIP_ROM 0xCC
#define DS_CMD_CONVERT  0x44
#define DS_CMD_READ_SP  0xBE
#define DS_CMD_WRITE_SP 0x4E

uint8_t ds18b20_crc8(const uint8_t *p, unsigned n);                     /* x^8+x^5+x^4+1, 초기값 0 */
unsigned ds18b20_resolution_bits(uint8_t config);                       /* 설정 바이트 비트 6:5 → 9..12 */
int32_t ds18b20_raw_to_uc(int16_t raw, unsigned bits);                  /* 1/1000000 °C(1/16 °C = 62500 µ°C 라 정확). 미정의 하위 비트는 0 으로 보고 계산 */
int ds18b20_read_scratchpad(uintptr_t base, uint8_t sp[9]);             /* 리셋+Skip ROM+BEh+9바이트+CRC 검사 */
int ds18b20_set_resolution(uintptr_t base, unsigned bits, int8_t th, int8_t tl);   /* 4Eh. 9..12 */
uint32_t ds18b20_tconv_ms(unsigned bits);                               /* 최대 변환 시간(ms, 올림). 범위 밖이면 12비트 값 */
int ds18b20_convert(uintptr_t base, uint32_t max_polls);                /* 44h 후 읽기 슬롯이 1 이 될 때까지 */
int ds18b20_read_uc(uintptr_t base, uint32_t max_polls, int32_t *uc);   /* 변환 → 읽기 → µ°C */
#endif
