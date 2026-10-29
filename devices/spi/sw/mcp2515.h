/* MCP2515 독립형 CAN 컨트롤러 (SPI). axi_qspi 위에서 동작. 모드 0,0 / 1,1, SCK ≤ 10 MHz.
 * 근거: Microchip DS20001801K (PDF 를 직접 읽음, D1) — 12장 Table 12-1 SPI 명령, Register 4-1~4-8 수신 버퍼,
 * Register 5-1~5-3 비트 타이밍, Register 7-1/7-2 인터럽트, Register 10-1/10-2 CANCTRL/CANSTAT, 5.5절 타이밍 제약, 8.1절 OST.
 * 보드에서 실행해 보지 않았다. */
#ifndef MCP2515_H
#define MCP2515_H
#include "axi_qspi.h"

/* SPI 명령 (Table 12-1) */
#define MCP_OP_RESET        0xC0
#define MCP_OP_READ         0x03
#define MCP_OP_READ_RXB     0x90   /* 1001 0nm0: nm = 00 RXB0SIDH(61h), 01 RXB0D0(66h), 10 RXB1SIDH(71h), 11 RXB1D0(76h). CS 가 올라가면 해당 RXnIF 자동 해제 */
#define MCP_OP_WRITE        0x02
#define MCP_OP_LOAD_TX      0x40   /* 0100 0abc: 000 TXB0SIDH(31h), 001 TXB0D0(36h), 010 TXB1SIDH(41h), 011 TXB1D0(46h), 100 TXB2SIDH(51h), 101 TXB2D0(56h) */
#define MCP_OP_RTS          0x80   /* 1000 0nnn: bit0 TXB0, bit1 TXB1, bit2 TXB2 */
#define MCP_OP_READ_STATUS  0xA0
#define MCP_OP_RX_STATUS    0xB0
#define MCP_OP_BIT_MODIFY   0x05   /* 이 명령을 지원하지 않는 레지스터에서는 마스크가 FFh 로 강제되어 바이트 쓰기가 됨 */

/* 레지스터 */
#define MCP_BFPCTRL 0x0C
#define MCP_TXRTSCTRL 0x0D
#define MCP_CANSTAT 0x0E   /* 상위 니블이 무엇이든 xE 는 CANSTAT, xF 는 CANCTRL (Table 11-1) */
#define MCP_CANCTRL 0x0F
#define MCP_TEC    0x1C
#define MCP_REC    0x1D
#define MCP_CNF3   0x28
#define MCP_CNF2   0x29
#define MCP_CNF1   0x2A
#define MCP_CANINTE 0x2B
#define MCP_CANINTF 0x2C
#define MCP_EFLG   0x2D
#define MCP_TXB_CTRL(n)  (0x30 + 0x10 * (n))   /* n = 0..2 */
#define MCP_TXB_SIDH(n)  (0x31 + 0x10 * (n))
#define MCP_RXB0CTRL 0x60
#define MCP_RXB1CTRL 0x70
#define MCP_TXREQ   (1u << 3)
#define MCP_RXM_ANY (3u << 5)            /* RXBnCTRL.RXM = 11: 마스크·필터 끔, 모든 메시지 수신 */
/* 모드 코드 (CANCTRL[7:5] = REQOP, CANSTAT[7:5] = OPMOD) */
#define MCP_MODE_NORMAL   0x00
#define MCP_MODE_SLEEP    0x20
#define MCP_MODE_LOOPBACK 0x40
#define MCP_MODE_LISTEN   0x60
#define MCP_MODE_CONFIG   0x80           /* 전원 투입·RESET 후 기본, CANCTRL 리셋값 87h */
#define MCP_MODE_MASK     0xE0
/* CANINTF 비트 */
#define MCP_RX0IF (1u << 0)
#define MCP_RX1IF (1u << 1)
#define MCP_TX0IF (1u << 2)
#define MCP_TX1IF (1u << 3)
#define MCP_TX2IF (1u << 4)
#define MCP_ERRIF (1u << 5)
#define MCP_WAKIF (1u << 6)
#define MCP_MERRF (1u << 7)

typedef struct { uint32_t id; uint8_t ext, rtr, dlc; uint8_t data[8]; } mcp_frame_t;

#define MCP_ERR_TIMING (-40)
#define MCP_ERR_MODE   (-41)   /* 요청한 모드로 바뀌지 않음 */
#define MCP_ERR_NODATA (-42)
/* 비트 타이밍: Tq = 2×(BRP+1)/Fosc (Register 5-1), 구간 = (필드+1)×Tq. 한 비트 = 동기 1 + PropSeg + PS1 + PS2.
 * 범위(5.2절): PropSeg 1~8, PS1 1~8, PS2 2~8, SJW 1~4. 규칙(5.5절): PropSeg+PS1 ≥ PS2, PropSeg+PS1 ≥ TDELAY(1~2 Tq), PS2 ≥ SJW.
 * 설계 선택(문서 규정 아님): 한 비트 8~25 Tq, 표본 점 87.5% 에 가장 가까운 것. BTLMODE=1, SAM=0. 정확한 해가 없으면 실패 */
int mcp2515_calc_timing(uint32_t fosc_hz, uint32_t bitrate, uint8_t *cnf1, uint8_t *cnf2, uint8_t *cnf3);

/* TX 버퍼 배치: SIDH, SIDL(SID[2:0]<<5 | EXIDE<<3 | EID[17:16]), EID8, EID0, DLC(RTR bit6 | DLC[3:0]), 데이터. 반환: 바이트 수(5+dlc, RTR 이면 5) */
int mcp2515_pack_tx(const mcp_frame_t *f, uint8_t out[13]);
/* RX 버퍼 배치: SIDH, SIDL(SID[2:0], SRR bit4, IDE bit3, EID[17:16]), EID8, EID0, DLC(RTR bit6, DLC[3:0]) */
int mcp2515_unpack_rx(const uint8_t b[5], mcp_frame_t *f);

/* SPI 명령 */
int mcp2515_reset(qspi_t *s, unsigned cs);   /* C0h → 구성 모드. 이후 OST(128 OSC1 클럭) 동안 SPI 접근 금지: 호출자가 대기 */
int mcp2515_read_regs(qspi_t *s, unsigned cs, uint8_t addr, uint8_t *buf, unsigned n);
int mcp2515_write_regs(qspi_t *s, unsigned cs, uint8_t addr, const uint8_t *buf, unsigned n);
int mcp2515_bit_modify(qspi_t *s, unsigned cs, uint8_t addr, uint8_t mask, uint8_t val);
int mcp2515_load_tx(qspi_t *s, unsigned cs, unsigned buf_idx, const mcp_frame_t *f);
int mcp2515_rts(qspi_t *s, unsigned cs, unsigned buf_idx);
int mcp2515_read_status(qspi_t *s, unsigned cs, uint8_t *st);

/* 고수준: 모드 변경은 CANCTRL 쓰기 후 CANSTAT 로 확인(최대 tries 회 읽기) */
int mcp2515_set_mode(qspi_t *s, unsigned cs, uint8_t mode, unsigned tries);
/* 구성 모드에서 CNF1/2/3 쓰기, 두 수신 버퍼 모두 필터 끔(RXM=11), 인터럽트 끔, 요청한 모드로 전환 */
int mcp2515_init(qspi_t *s, unsigned cs, uint32_t fosc_hz, uint32_t bitrate, uint8_t final_mode);
#define MCP_ERR_TXFULL (-43)
int mcp2515_send(qspi_t *s, unsigned cs, const mcp_frame_t *f);          /* TXREQ 가 0 인 TXB 를 찾아 적재 후 RTS. 셋 다 사용 중이면 MCP_ERR_TXFULL */
int mcp2515_recv(qspi_t *s, unsigned cs, mcp_frame_t *f);                /* 수신 버퍼 0 우선. 없으면 MCP_ERR_NODATA */
#endif
