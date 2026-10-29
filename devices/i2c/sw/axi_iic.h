/* AXI IIC (PG090 v2.1) 동적 컨트롤러 드라이버. 마스터, 7비트 주소.
 * 레지스터·절차 출처: PG090 (레지스터 표, Dynamic Controller Logic Flow, 예제).
 * 실제 보드에서 실행해 보지 않았다(호스트 모의 테스트만). */
#ifndef AXI_IIC_H
#define AXI_IIC_H
#include <stdint.h>

#ifndef AXI_IIC_RD
#define AXI_IIC_RD(base, off)     (*(volatile uint32_t *)((uintptr_t)(base) + (off)))
#define AXI_IIC_WR(base, off, v)  (*(volatile uint32_t *)((uintptr_t)(base) + (off)) = (v))
#endif

/* 레지스터 오프셋 */
#define IIC_GIE        0x01C
#define IIC_ISR        0x020
#define IIC_IER        0x028
#define IIC_SOFTR      0x040
#define IIC_CR         0x100
#define IIC_SR         0x104
#define IIC_TX_FIFO    0x108
#define IIC_RX_FIFO    0x10C
#define IIC_TX_OCY     0x114
#define IIC_RX_OCY     0x118
#define IIC_RX_PIRQ    0x120

/* CR 비트 */
#define CR_EN          (1u << 0)
#define CR_TX_FIFO_RST (1u << 1)
/* SR 비트 */
#define SR_BB          (1u << 2)
#define SR_TX_FULL     (1u << 4)
#define SR_RX_EMPTY    (1u << 6)
#define SR_TX_EMPTY    (1u << 7)
/* ISR 비트 */
#define ISR_TXERR      (1u << 1)   /* 마스터 송신 중이면 NACK 등 오류 */
/* TX_FIFO 워드 */
#define TXF_START      (1u << 8)
#define TXF_STOP       (1u << 9)

#define IIC_OK          0
#define IIC_ERR_TIMEOUT (-1)
#define IIC_ERR_NACK    (-2)
#define IIC_ERR_ARGS    (-3)
#define IIC_ERR_BUSY    (-4)

typedef struct {
    uintptr_t base;
    uint32_t  timeout;   /* 폴링 횟수 한도 */
} axi_iic_t;

/* 호출하는 쪽이 제공: 밀리초 지연 */
void iic_delay_ms(unsigned ms);

int axi_iic_init(axi_iic_t *d, uintptr_t base);
void axi_iic_reset(axi_iic_t *d);   /* SOFTR=0xA. 타이밍 레지스터가 기본값으로 돌아감 */
int axi_iic_write(axi_iic_t *d, uint8_t addr7, const uint8_t *buf, unsigned len);
int axi_iic_read(axi_iic_t *d, uint8_t addr7, uint8_t *buf, unsigned len);          /* len 1..255 */
int axi_iic_write_read(axi_iic_t *d, uint8_t addr7, const uint8_t *wbuf, unsigned wlen,
                       uint8_t *rbuf, unsigned rlen);   /* 반복 시작 */
#endif
