/* I2C I/O 확장기: PCF8574(8비트 준양방향), MCP23017(16비트).
 * 근거(D1): PCF8574 = TI SCPS068, MCP23017 = Microchip 데이터시트(farnell 사본, IOCON.BANK=0 레지스터 표).
 * PCF8574A(주소 0x38~0x3F)는 열어본 문서에 없어 미확인 — 지원하지 않는다.
 * 보드에서 실행해 보지 않았다. */
#ifndef IO_EXPANDER_H
#define IO_EXPANDER_H
#include <stdint.h>
#include "axi_iic.h"

/* ---------- PCF8574 : 7비트 주소 0x20 + (A2A1A0). 레지스터 없음, 바이트 하나가 포트 전체 ----------
 * 핀을 입력으로 쓰려면 먼저 그 비트에 1 을 써야 한다(준양방향). 전원 인가 시 모든 핀이 High.
 * 읽기는 핀의 실제 전압을 돌려주고, 출력 래치(shadow)와는 다를 수 있다. */
typedef struct { axi_iic_t *iic; uint8_t addr7; uint8_t shadow; } pcf8574_t;
int pcf8574_init(pcf8574_t *p, axi_iic_t *iic, uint8_t addr7);      /* 0x20~0x27, 모든 비트 1 을 씀 */
int pcf8574_write(pcf8574_t *p, uint8_t v);
int pcf8574_read(pcf8574_t *p, uint8_t *v);
int pcf8574_set_pin(pcf8574_t *p, unsigned pin, int level);          /* shadow 기준 한 비트 변경 */

/* ---------- MCP23017 : 7비트 주소 0x20 + (A2A1A0), IOCON.BANK=0 지도만 ---------- */
#define MCP_IODIRA 0x00
#define MCP_IPOLA  0x02
#define MCP_IOCON  0x0A
#define MCP_GPPUA  0x0C
#define MCP_GPIOA  0x12
#define MCP_OLATA  0x14
/* B 포트 = A 포트 + 1 (BANK=0 에서). */
#define MCP_IOCON_BANK   (1u << 7)
#define MCP_IOCON_MIRROR (1u << 6)
#define MCP_IOCON_SEQOP  (1u << 5)
#define MCP_IOCON_DISSLW (1u << 4)
#define MCP_IOCON_HAEN   (1u << 3)
#define MCP_IOCON_ODR    (1u << 2)
#define MCP_IOCON_INTPOL (1u << 1)

typedef struct { axi_iic_t *iic; uint8_t addr7; uint16_t olat; } mcp23017_t;
/* BANK=0, SEQOP=0(주소 자동 증가)으로 IOCON 을 쓴다. 장치가 BANK=1 로 남아 있었다면 먼저 RESET 핀을 쓸 것. */
int mcp23017_init(mcp23017_t *m, axi_iic_t *iic, uint8_t addr7);
int mcp23017_write_reg(mcp23017_t *m, uint8_t reg, uint8_t v);
int mcp23017_read_reg(mcp23017_t *m, uint8_t reg, uint8_t *v);
/* 16비트 접근: 비트 0~7 = A 포트(GPA0~7), 비트 8~15 = B 포트. 자동 증가로 A,B 를 한 트랜잭션에 처리 */
int mcp23017_set_dir(mcp23017_t *m, uint16_t input_mask);   /* 1 = 입력 */
int mcp23017_set_pullup(mcp23017_t *m, uint16_t mask);      /* 1 = 약 100 kΩ 풀업 */
int mcp23017_write(mcp23017_t *m, uint16_t v);              /* OLAT 에 씀 */
int mcp23017_read(mcp23017_t *m, uint16_t *v);              /* GPIO 읽기(핀 전압) */
int mcp23017_set_pin(mcp23017_t *m, unsigned pin, int level);
#endif
