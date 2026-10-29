/* MCP2515 독립형 CAN 컨트롤러 (SPI). axi_qspi 위에서 동작. 모드 0,0 / 1,1, SCK ≤ 10 MHz.
 *
 * 근거 수준이 항목마다 다르다. 이 헤더의 상수는 세 구역으로 나뉜다.
 *  [A] 데이터시트 본문에서 확인(D1): 비트 타이밍 CNF1/2/3, TXBn/RXBn 레지스터 주소와 필드, TXREQ, RXM, RX0IF/RX1IF 비트.
 *  [B] 2차 자료 확인(D3, Zephyr mcp2515 드라이버): SPI 옵코드 READ 03h, BIT MODIFY 05h, LOAD TX BUFFER 40h|abc,
 *      RTS 80h|nnn, READ STATUS A0h, RESET C0h.
 *  [C] 미확인(이 저장소에서 어떤 문서로도 확인 못함 — DS21801 12장 SPI 명령 표와 CANCTRL/CANSTAT 표 필요):
 *      WRITE, READ RX BUFFER 옵코드, CANCTRL/CANSTAT/CANINTE/CANINTF 주소, REQOP/OPMOD 코드, TX0IF 비트 위치.
 *      아래 [C] 값은 기억에 의존한 임시값이며 PDF 와 대조 전에는 쓰지 말 것.
 * 보드에서 실행해 보지 않았다. */
#ifndef MCP2515_H
#define MCP2515_H
#include "axi_qspi.h"

/* ---- [B] 옵코드 (D3) ---- */
#define MCP_OP_RESET        0xC0
#define MCP_OP_READ         0x03
#define MCP_OP_BIT_MODIFY   0x05
#define MCP_OP_LOAD_TX      0x40   /* | abc: 0 TXB0SIDH, 1 TXB0D0, 2 TXB1SIDH, 3 TXB1D0, 4 TXB2SIDH, 5 TXB2D0 (abc 의미는 미확인) */
#define MCP_OP_RTS          0x80   /* | nnn: 비트 0 = TXB0, 1 = TXB1, 2 = TXB2 */
#define MCP_OP_READ_STATUS  0xA0
/* ---- [C] 미확인 임시값 ---- */
#define MCP_OP_WRITE_UNVERIFIED     0x02
#define MCP_OP_READ_RXB_UNVERIFIED  0x90   /* | nm */
#define MCP_CANSTAT_UNVERIFIED      0x0E
#define MCP_CANCTRL_UNVERIFIED      0x0F
#define MCP_CANINTE_UNVERIFIED      0x2B   /* CNF1(2Ah)와 EFLG(2Dh) 사이의 두 칸이라는 점은 확인된 주소에서 도출(계산) */
#define MCP_CANINTF_UNVERIFIED      0x2C

/* ---- [A] 레지스터 (D1) ---- */
#define MCP_CNF3   0x28
#define MCP_CNF2   0x29
#define MCP_CNF1   0x2A
#define MCP_TEC    0x1C
#define MCP_REC    0x1D
#define MCP_EFLG   0x2D
#define MCP_TXB_CTRL(n)  (0x30 + 0x10 * (n))   /* n = 0..2 */
#define MCP_TXB_SIDH(n)  (0x31 + 0x10 * (n))
#define MCP_RXB0CTRL 0x60
#define MCP_RXB1CTRL 0x70
#define MCP_TXREQ   (1u << 3)
#define MCP_RXM_ANY (3u << 5)   /* RXM = 11: 마스크·필터 끔, 모든 메시지 수신 */

typedef struct { uint32_t id; uint8_t ext, rtr, dlc; uint8_t data[8]; } mcp_frame_t;

/* 비트 타이밍: Tq = 2×(BRP+1)/Fosc, 각 구간 길이 = (필드값+1)×Tq. 한 비트 = 동기 1 + PRSEG + PHSEG1 + PHSEG2.
 * BRP 0~63, PRSEG/PHSEG1 1~8, PHSEG2 2~8(CNF3 설명: 최소 2 Tq), SJW 1~4.
 * 설계 선택(문서 규정 아님): 한 비트 8~25 Tq, 표본 점 87.5% 에 가장 가까운 것. 정확히 나누어떨어지는 해가 없으면 실패 */
#define MCP_ERR_TIMING (-40)
int mcp2515_calc_timing(uint32_t fosc_hz, uint32_t bitrate, uint8_t *cnf1, uint8_t *cnf2, uint8_t *cnf3);

/* TX 버퍼 배치(D1): SIDH, SIDL(SID[2:0]<<5 | EXIDE<<3 | EID[17:16]), EID8, EID0, DLC(RTR bit6 | DLC[3:0]), 데이터. 반환: 바이트 수(5+dlc) */
int mcp2515_pack_tx(const mcp_frame_t *f, uint8_t out[13]);

/* SPI 명령(확인된 옵코드만 사용) */
int mcp2515_reset(qspi_t *s, unsigned cs);                                   /* C0h. 이후 발진기 안정 시간은 호출자가 대기 */
int mcp2515_read_regs(qspi_t *s, unsigned cs, uint8_t addr, uint8_t *buf, unsigned n);
int mcp2515_bit_modify(qspi_t *s, unsigned cs, uint8_t addr, uint8_t mask, uint8_t val);
int mcp2515_load_tx(qspi_t *s, unsigned cs, unsigned buf_idx, const mcp_frame_t *f);   /* abc = 2×idx (SIDH 부터) — [C] 해석 */
int mcp2515_rts(qspi_t *s, unsigned cs, unsigned buf_idx);
int mcp2515_read_status(qspi_t *s, unsigned cs, uint8_t *st);
#endif
