/* nRF24L01+ 2.4 GHz 송수신기 (SPI + CE + IRQ). axi_qspi 위에서 동작. SPI 모드 0,0 (그림 26~28 에서 읽음, 글로 명시된 문장은 없음), SCK ≤ 10 MHz (Cload 5 pF).
 * 근거: Nordic nRF24L01+ Product Specification v1.0 (PDF 를 직접 읽음, D1) — 6.1절 모드/타이밍(Table 15, 16), 8.3절 SPI 명령(Table 20)·타이밍(Table 22),
 * 9.1절 레지스터 맵(Table 28), 부록 A 송수신 절차. 보드에서 실행해 보지 않았다. CE 와 delay 는 호출자가 콜백으로 제공한다(GPIO·타이머는 이 파일 밖). */
#ifndef NRF24L01_H
#define NRF24L01_H
#include "axi_qspi.h"

/* SPI 명령 (Table 20) */
#define NRF_CMD_R_REGISTER   0x00   /* 000A AAAA */
#define NRF_CMD_W_REGISTER   0x20   /* 001A AAAA */
#define NRF_CMD_R_RX_PL_WID  0x60   /* FEATURE.EN_DPL 필요 */
#define NRF_CMD_R_RX_PAYLOAD 0x61
#define NRF_CMD_W_TX_PAYLOAD 0xA0
#define NRF_CMD_W_ACK_PAYLOAD 0xA8  /* 1010 1PPP, FEATURE.EN_ACK_PAY 필요 */
#define NRF_CMD_W_TX_NOACK   0xB0   /* FEATURE.EN_DYN_ACK 필요 */
#define NRF_CMD_FLUSH_TX     0xE1
#define NRF_CMD_FLUSH_RX     0xE2
#define NRF_CMD_REUSE_TX_PL  0xE3
#define NRF_CMD_NOP          0xFF

/* 레지스터 주소 (Table 28) */
#define NRF_REG_CONFIG 0x00
#define NRF_REG_EN_AA 0x01
#define NRF_REG_EN_RXADDR 0x02
#define NRF_REG_SETUP_AW 0x03
#define NRF_REG_SETUP_RETR 0x04
#define NRF_REG_RF_CH 0x05
#define NRF_REG_RF_SETUP 0x06
#define NRF_REG_STATUS 0x07
#define NRF_REG_OBSERVE_TX 0x08
#define NRF_REG_RPD 0x09
#define NRF_REG_RX_ADDR_P0 0x0A     /* P1=0B, P2..P5=0C..0F (LSByte 만, 상위는 P1 과 같음) */
#define NRF_REG_TX_ADDR 0x10
#define NRF_REG_RX_PW_P0 0x11       /* P1..P5 = 12..16 */
#define NRF_REG_FIFO_STATUS 0x17
#define NRF_REG_DYNPD 0x1C
#define NRF_REG_FEATURE 0x1D
/* CONFIG 비트 */
#define NRF_CFG_MASK_RX_DR (1u << 6)
#define NRF_CFG_MASK_TX_DS (1u << 5)
#define NRF_CFG_MASK_MAX_RT (1u << 4)
#define NRF_CFG_EN_CRC (1u << 3)
#define NRF_CFG_CRCO (1u << 2)
#define NRF_CFG_PWR_UP (1u << 1)
#define NRF_CFG_PRIM_RX (1u << 0)
/* STATUS 비트 (1 을 쓰면 해제) */
#define NRF_ST_RX_DR (1u << 6)
#define NRF_ST_TX_DS (1u << 5)
#define NRF_ST_MAX_RT (1u << 4)
#define NRF_ST_RX_P_NO(s) (((s) >> 1) & 7u)   /* 0..5 파이프, 7 = RX FIFO 비어 있음 */
#define NRF_ST_TX_FULL (1u << 0)
/* FIFO_STATUS 비트 */
#define NRF_FIFO_TX_REUSE (1u << 6)
#define NRF_FIFO_TX_FULL (1u << 5)
#define NRF_FIFO_TX_EMPTY (1u << 4)
#define NRF_FIFO_RX_FULL (1u << 1)
#define NRF_FIFO_RX_EMPTY (1u << 0)
/* FEATURE 비트 */
#define NRF_FEAT_EN_DPL (1u << 2)
#define NRF_FEAT_EN_ACK_PAY (1u << 1)
#define NRF_FEAT_EN_DYN_ACK (1u << 0)

/* 타이밍 (Table 16, 6.1.7절) */
#define NRF_T_PD2STBY_US 4500u   /* 외부 수정, Ls=90 mH 최악값. Ls<30 mH 이면 1500 */
#define NRF_T_STBY2A_US  130u
#define NRF_T_HCE_US     10u     /* CE High 최소 */
#define NRF_CE_PULSE_US  15u     /* 설계 여유: 최소 10 µs 에 5 µs 더함 */

enum { NRF_RATE_1M = 0, NRF_RATE_2M = 1, NRF_RATE_250K = 2 };
enum { NRF_PWR_M18 = 0, NRF_PWR_M12 = 1, NRF_PWR_M6 = 2, NRF_PWR_0 = 3 };

#define NRF_ERR_VERIFY  (-50)   /* 쓴 값을 다시 읽으니 다름 (칩 없음·배선 불량 의심) */
#define NRF_ERR_MAXRT   (-51)   /* 재전송 한도까지 ACK 없음 */
#define NRF_ERR_TIMEOUT (-52)
#define NRF_ERR_NODATA  (-53)
#define NRF_ERR_BADLEN  (-54)   /* RX 폭이 0 또는 32 초과: FLUSH_RX 로 버림 */
#define NRF_ERR_TXFULL  (-55)

typedef struct {
    uint8_t channel;      /* RF_CH 0..125 (f = 2400 + ch MHz, 상한 2525 MHz) */
    uint8_t rate, power;  /* NRF_RATE_*, NRF_PWR_* */
    uint8_t crc_bytes;    /* 0 = CRC 끔, 1, 2. 자동 ACK 를 쓰면 CRC 필수 (EN_CRC 는 EN_AA 가 켜지면 강제 1) */
    uint8_t addr_width;   /* 3..5 */
    uint8_t ard, arc;     /* SETUP_RETR: 지연 (ard+1)×250 µs, 재전송 횟수 0..15 */
    uint8_t auto_ack;     /* 모든 파이프 EN_AA */
    uint8_t dyn_payload;  /* FEATURE.EN_DPL + DYNPD 전 파이프. auto_ack 필요 */
    uint8_t ack_payload;  /* FEATURE.EN_ACK_PAY. dyn_payload 필요 */
    uint8_t dyn_ack;      /* FEATURE.EN_DYN_ACK */
} nrf24_cfg_t;

typedef struct {
    qspi_t *s; unsigned cs;
    void *ctx;
    void (*ce)(void *ctx, int level);          /* CE 핀 */
    void (*delay_us)(void *ctx, uint32_t us);
    uint8_t config, feature, aw;               /* 내부 상태 (init 이 설정) */
} nrf24_t;

/* 저수준 명령. 모두 STATUS 를 st 로 돌려준다(NULL 가능) */
int nrf24_nop(nrf24_t *n, uint8_t *st);
int nrf24_read_reg(nrf24_t *n, uint8_t reg, uint8_t *buf, unsigned len);          /* len 1..5, LSByte 먼저 */
int nrf24_write_reg(nrf24_t *n, uint8_t reg, const uint8_t *buf, unsigned len);
int nrf24_write_reg8(nrf24_t *n, uint8_t reg, uint8_t v);
int nrf24_read_reg8(nrf24_t *n, uint8_t reg, uint8_t *v);
int nrf24_flush_tx(nrf24_t *n);
int nrf24_flush_rx(nrf24_t *n);
int nrf24_clear_irq(nrf24_t *n, uint8_t mask);   /* STATUS 에 1 쓰기 */

/* 고수준 */
int nrf24_init(nrf24_t *n, const nrf24_cfg_t *c);   /* 전원 상승까지 수행(NRF_T_PD2STBY_US 대기), 쓴 레지스터 읽어 검증. 끝나면 대기-I, PTX */
int nrf24_set_tx_addr(nrf24_t *n, const uint8_t *addr);   /* addr_width 바이트, LSByte 먼저. TX_ADDR 와 RX_ADDR_P0 에 같은 값 (ACK 수신용) */
int nrf24_set_rx_pipe(nrf24_t *n, unsigned pipe, const uint8_t *addr, unsigned payload_len);
                                                    /* pipe 0,1: addr_width 바이트. pipe 2..5: addr[0] 한 바이트만(상위는 P1). payload_len 1..32, dyn_payload 이면 무시 */
int nrf24_rx_start(nrf24_t *n);   /* PRIM_RX=1, CE=1, 130 µs 대기 */
int nrf24_rx_stop(nrf24_t *n);    /* CE=0 (대기-I) */
int nrf24_send(nrf24_t *n, const uint8_t *data, unsigned len, int noack, uint32_t timeout_us);
         /* PTX 로 전환 후 W_TX_PAYLOAD, CE 펄스, TX_DS/MAX_RT 대기. MAX_RT 이면 TX FIFO 를 비우고 NRF_ERR_MAXRT.
          * noack=1 은 W_TX_PAYLOAD_NOACK(dyn_ack 필요). 이때 TX_DS 가 언제 서는지는 문서에 없음 → 미확인 */
int nrf24_recv(nrf24_t *n, uint8_t buf[32], unsigned *len, unsigned *pipe);   /* 없으면 NRF_ERR_NODATA. 읽은 뒤 RX_DR 해제. 더 있는지는 다시 호출 */
int nrf24_write_ack_payload(nrf24_t *n, unsigned pipe, const uint8_t *data, unsigned len);
int nrf24_read_rpd(nrf24_t *n, int *rpd);   /* RX 모드에서 NRF_T_STBY2A_US+40 µs 뒤에 유효 (6.4절). -64 dBm 초과 신호 */
#endif
