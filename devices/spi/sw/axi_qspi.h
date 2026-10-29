/* AXI Quad SPI (PG153) 마스터 드라이버, 표준 SPI 모드, 바이트 단위 전송.
 * 레지스터·비트는 PG153 표에서 확인. 전송 순서는 FreeBSD axi_quad_spi.c(D3, 2차 자료)를 따랐다:
 *   SRR=0xA → SPICR 로 FIFO 리셋 → DGIER=0 → SPICR=MASTER|MSS|SPE (inhibit 0) ;
 *   CS 는 SPISSR 비트를 0 으로(active-low) ; 바이트마다 DTR 쓰기 → 완료 대기 → DRR 읽기.
 * PG153 본문의 절차는 열지 못했다(미확인). 보드 미실행, 호스트 모의 테스트만. */
#ifndef AXI_QSPI_H
#define AXI_QSPI_H
#include <stdint.h>
#include <stddef.h>

#ifndef QSPI_RD
#define QSPI_RD(base, off)    (*(volatile uint32_t *)((uintptr_t)(base) + (off)))
#define QSPI_WR(base, off, v) (*(volatile uint32_t *)((uintptr_t)(base) + (off)) = (v))
#endif
#define QSPI_SRR   0x40
#define QSPI_CR    0x60
#define QSPI_SR    0x64
#define QSPI_DTR   0x68
#define QSPI_DRR   0x6C
#define QSPI_SSR   0x70
#define QSPI_DGIER 0x1C

#define QCR_LOOP   (1u << 0)
#define QCR_SPE    (1u << 1)
#define QCR_MASTER (1u << 2)
#define QCR_CPOL   (1u << 3)
#define QCR_CPHA   (1u << 4)
#define QCR_RST_TX (1u << 5)
#define QCR_RST_RX (1u << 6)
#define QCR_MSS    (1u << 7)   /* 수동 슬레이브 선택 */
#define QCR_INHIBIT (1u << 8)  /* 리셋 값 1 */
#define QCR_LSB    (1u << 9)
#define QSR_RX_EMPTY (1u << 0)
#define QSR_TX_EMPTY (1u << 2)
#define QSR_TX_FULL  (1u << 3)

#define QSPI_OK 0
#define QSPI_ERR_TIMEOUT -1
#define QSPI_ERR_ARGS -2

typedef struct { uintptr_t base; uint32_t cr; uint32_t ssr_idle; unsigned timeout; } qspi_t;

/* mode: 0~3 (bit1 = CPOL, bit0 = CPHA). 모드는 인스턴스마다 하나로 고정하는 것을 가정(실행 중 변경은 미확인) */
int  qspi_init(qspi_t *s, uintptr_t base, unsigned mode, int lsb_first, unsigned num_ss);
void qspi_cs(qspi_t *s, unsigned idx, int assert_cs);
int  qspi_xfer(qspi_t *s, const uint8_t *tx, uint8_t *rx, size_t len);   /* tx 또는 rx 는 NULL 가능(tx NULL 이면 0x00 송신) */
#endif
