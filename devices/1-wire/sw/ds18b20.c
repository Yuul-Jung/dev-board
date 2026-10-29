/* MicroBlaze V 용 DS18B20 드라이버 예시. onewire_axil_top 레지스터를 비트 단위로 사용한다.
 * 주의: 컴파일과 실제 보드 시험은 아직 하지 않았다. 스크래치패드 바이트 순서와 CRC는 데이터시트로 확인 필요. */
#include <stdint.h>

#define OW_BASE   0x44A00000u            /* Vivado 주소 편집기에서 정한 값으로 바꿀 것 */
#define OW_CMD    (*(volatile uint32_t *)(OW_BASE + 0x00))
#define OW_STAT   (*(volatile uint32_t *)(OW_BASE + 0x04))

#define OP_RESET  0u
#define OP_WRITE  1u
#define OP_READ   2u
#define ST_BUSY   (1u << 0)
#define ST_DONE   (1u << 1)
#define ST_PRES   (1u << 2)
#define ST_RBIT   (1u << 3)

static void ow_run(uint32_t cmd) {
    while (OW_STAT & ST_BUSY) { }
    OW_CMD = cmd;
    while (!(OW_STAT & ST_DONE)) { }
}
static int ow_reset(void) { ow_run(OP_RESET); return (OW_STAT & ST_PRES) != 0; }
static void ow_write_bit(int b) { ow_run(OP_WRITE | ((uint32_t)(b & 1) << 2)); }
static int ow_read_bit(void) { ow_run(OP_READ); return (OW_STAT & ST_RBIT) != 0; }

static void ow_write_byte(uint8_t v) { for (int i = 0; i < 8; i++) ow_write_bit((v >> i) & 1); }
static uint8_t ow_read_byte(void) {
    uint8_t v = 0;
    for (int i = 0; i < 8; i++) v |= (uint8_t)(ow_read_bit() << i);
    return v;
}

/* 소자 1개만 연결된 버스에서 온도(1/16 °C 단위)를 읽는다. 응답이 없으면 0x7FFFFFFF를 돌려준다 */
int32_t ds18b20_read_raw(void) {
    if (!ow_reset()) return 0x7FFFFFFF;
    ow_write_byte(0xCC);                 /* Skip ROM */
    ow_write_byte(0x44);                 /* Convert T */
    /* 12비트 변환은 최대 750 ms. 여기서는 타이머로 기다리는 부분을 생략했다. */
    if (!ow_reset()) return 0x7FFFFFFF;
    ow_write_byte(0xCC);
    ow_write_byte(0xBE);                 /* Read Scratchpad */
    uint8_t lo = ow_read_byte();
    uint8_t hi = ow_read_byte();
    return (int16_t)((hi << 8) | lo);    /* 12비트 분해능에서 1 LSB = 0.0625 °C */
}
