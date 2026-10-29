/* AXI GPIO (PG144 v2.0) 드라이버 — 핀 단위 입출력, 최대 2채널(각 최대 32비트), 인터럽트 상태 확인·해제.
 * 근거: AMD PG144 AXI GPIO v2.0 (users.ece.utexas.edu 의 PDF 사본을 웹 가져오기가 요약한 본문, D1 요약 경유 — PDF 원문은 직접 열지 못함),
 *   독립 확인 D3: Linux drivers/gpio/gpio-xilinx.c (오프셋·TRI 극성·GIER 비트 31·ISR 읽은 값을 그대로 다시 써서 해제). 사실표 gpio/ref/axi_gpio_facts.txt 와 tools/crosscheck_pdf.py 로 대조.
 * 레지스터: GPIO_DATA 0x0000, GPIO_TRI 0x0004 (비트 1 = 입력, 0 = 출력), GPIO2_DATA 0x0008, GPIO2_TRI 0x000C (이중 채널일 때만),
 *   GIER 0x011C(비트 31), IP ISR 0x0120(쓰기로 토글: 1 을 쓴 비트가 뒤집힘), IP IER 0x0128 (인터럽트를 켠 구성일 때만).
 * 중요: PG144 는 출력으로 설정된 비트를 읽으면 항상 0 이라고 한다 → 출력 상태는 드라이버가 그림자(shadow)로 기억하고, 읽기는 입력 비트만 하드웨어 값을 쓴다.
 * 미확인: ① 입력으로 설정된 비트에 DATA 를 써도 값이 저장되는지(요약은 "쓰기는 효과 없음"이라고만 함). 그래서 출력 전환은 "DATA 쓰기 → TRI 변경 → DATA 다시 쓰기" 순서로 하여 어느 쪽이든 원하는 초기값이 나가게 했다.
 *   ② 리셋 직후 DATA/TRI 값: 레지스터 표는 0x0 으로, 동작 절은 C_TRI_DEFAULT(0xFFFFFFFF)로 적어 서로 어긋난다(문서가 해결하지 않음) → gpio_init 이 TRI 를 직접 쓴다.
 *   ③ 인터럽트가 레벨인지 에지인지(요약에 없음). 이 드라이버는 상태를 읽고 해제만 하며 극성은 가정하지 않는다.
 * 보드에서 실행해 보지 않았다(호스트 모의 레지스터 시험만). */
#ifndef AXI_GPIO_H
#define AXI_GPIO_H
#include <stdint.h>

#ifndef GPIO_RD
#define GPIO_RD(base, off)    (*(volatile uint32_t *)((uintptr_t)(base) + (off)))
#define GPIO_WR(base, off, v) (*(volatile uint32_t *)((uintptr_t)(base) + (off)) = (v))
#endif

#define GPIO_REG_DATA   0x0000
#define GPIO_REG_TRI    0x0004
#define GPIO_REG_DATA2  0x0008
#define GPIO_REG_TRI2   0x000C
#define GPIO_REG_GIER   0x011C
#define GPIO_REG_ISR    0x0120
#define GPIO_REG_IER    0x0128
#define GPIO_GIER_ENABLE (1u << 31)
#define GPIO_IRQ_CH1    0x1u
#define GPIO_IRQ_CH2    0x2u

#define GPIO_OK       0
#define GPIO_ERR_ARGS (-1)
#define GPIO_ERR_DIR  (-2)      /* 입력으로 설정된 핀에 쓰기 */

typedef struct {
    uintptr_t base;
    unsigned channels;          /* 1 또는 2 (IP 의 Enable Dual Channel 설정과 같아야 함) */
    uint32_t out[2];            /* 출력 그림자 */
    uint32_t in_mask[2];        /* TRI 그림자: 1 = 입력 */
    uint32_t ier;               /* IER 그림자 (비트 0 = 채널 1, 비트 1 = 채널 2) */
} gpio_t;

/* 모든 핀을 입력(고임피던스)으로 만들고 출력 값을 0 으로. IP 에 인터럽트 레지스터가 없어도 상관없다(init 은 건드리지 않는다) */
int gpio_init(gpio_t *g, uintptr_t base, unsigned channels);
/* ch 0/1, bit 0..31. out=1 이면 출력으로(초기 level 로), 0 이면 입력으로 */
int gpio_pin_mode(gpio_t *g, unsigned ch, unsigned bit, int out, int level);
int gpio_write(gpio_t *g, unsigned ch, unsigned bit, int level);      /* 출력 핀만, 아니면 GPIO_ERR_DIR */
int gpio_toggle(gpio_t *g, unsigned ch, unsigned bit);
int gpio_write_mask(gpio_t *g, unsigned ch, uint32_t mask, uint32_t value);   /* mask 의 모든 비트가 출력이어야 함(아니면 아무것도 쓰지 않고 GPIO_ERR_DIR) */
int gpio_read(gpio_t *g, unsigned ch, unsigned bit);                  /* 0/1, 오류는 음수. 출력 핀은 그림자 값 */
int gpio_read_port(gpio_t *g, unsigned ch, uint32_t *value);          /* 입력 비트는 하드웨어, 출력 비트는 그림자 */
/* 인터럽트 (IP 에 Enable Interrupt 설정이 있을 때만 레지스터가 존재). irq_mask 는 GPIO_IRQ_CH1|CH2 */
int gpio_irq_enable(gpio_t *g, unsigned irq_mask);     /* 남아 있는 상태를 먼저 해제 → IER → GIER 순 */
int gpio_irq_disable(gpio_t *g, unsigned irq_mask);    /* IER 해제, 켜진 채널이 하나도 없으면 GIER 도 끔 */
unsigned gpio_irq_pending(gpio_t *g);                  /* ISR 비트 0..1 */
int gpio_irq_ack(gpio_t *g, unsigned irq_mask);        /* ISR 은 쓰기로 토글되므로 실제로 세트된 비트에만 1 을 쓴다 */
#endif
