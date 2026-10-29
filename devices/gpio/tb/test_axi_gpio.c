/* 모델의 레지스터 오프셋은 헤더 매크로가 아닌 PG144 요약의 숫자를 직접 적었다(헤더가 틀려도 모델이 같이 틀리지 않도록).
 * AXI GPIO 드라이버 시험 — PG144 요약을 옮긴 레지스터 모델(자작)에 대해서만 확인한다. 보드나 실제 IP 로 확인한 것이 아니다.
 * 모델이 강제하는 것: ① 출력으로 설정된 비트를 읽으면 0(PG144) ② 입력 비트의 쓰기 효과는 data_latch 로 두 가지를 모두 시험(문서가 불명확)
 *   ③ ISR 은 쓰기로 토글 ④ 없는 레지스터 접근과 2채널이 아닐 때의 채널 2 접근은 위반 ⑤ 출력 핀에 잘못된 레벨이 한 번이라도 나갔는지(글리치) 기록 */
#include <stdio.h>
#include <string.h>
#include "../sw/axi_gpio.h"

static uint32_t data[2], tri[2], pins_in[2], gier, ier, isr;
static int dual, intr_present, data_latch, violations;
static int nhist[2];                                             /* 레지스터 쓰기 횟수 */
static uint32_t watch_mask, watch_level; static int glitch;      /* watch_mask 비트가 구동되는 동안 watch_level 과 다른 값이 나가면 글리치 */
static uint32_t driven_mask(int ch) { return ~tri[ch]; }
static void snap(int ch) { nhist[ch]++; if (ch == 0 && (driven_mask(0) & watch_mask) && ((data[0] ^ watch_level) & driven_mask(0) & watch_mask)) glitch++; }
uint32_t mock_g_rd(uint32_t off)
{
    switch (off) {
    case 0x0000:  return (data[0] & tri[0] & 0) | (pins_in[0] & tri[0]);          /* 출력 비트는 0 */
    case 0x0004:   return tri[0];
    case 0x0008: if (!dual) { violations++; return 0; } return pins_in[1] & tri[1];
    case 0x000C:  if (!dual) { violations++; return 0; } return tri[1];
    case 0x011C:  if (!intr_present) { violations++; return 0; } return gier;
    case 0x0128:   if (!intr_present) { violations++; return 0; } return ier;
    case 0x0120:   if (!intr_present) { violations++; return 0; } return isr;
    default: violations++; return 0;
    }
}
void mock_g_wr(uint32_t off, uint32_t v)
{
    int ch;
    switch (off) {
    case 0x0000: case 0x0008:
        ch = off == 0x0008; if (ch && !dual) { violations++; return; }
        data[ch] = data_latch ? v : ((data[ch] & ~driven_mask(ch)) | (v & driven_mask(ch)));    /* data_latch=0: 입력 비트 쓰기 무시 */
        snap(ch); break;
    case 0x0004: case 0x000C:
        ch = off == 0x000C; if (ch && !dual) { violations++; return; }
        tri[ch] = v; snap(ch); break;
    case 0x011C: if (!intr_present) { violations++; return; } gier = v; break;
    case 0x0128:  if (!intr_present) { violations++; return; } ier = v; break;
    case 0x0120:  if (!intr_present) { violations++; return; } isr ^= (v & 3u); break;      /* 1 을 쓰면 뒤집힘 */
    default: violations++;
    }
}
static int fails;
#define CHK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
static void reset_model(int d, int ip, int latch)
{
    memset(data, 0, sizeof data); tri[0] = tri[1] = 0; pins_in[0] = pins_in[1] = 0; gier = ier = isr = 0;
    dual = d; intr_present = ip; data_latch = latch; violations = 0; nhist[0] = nhist[1] = 0; watch_mask = 0; glitch = 0;
}
/* 모델의 출력 구동 값 */
static uint32_t drv(int ch) { return data[ch] & driven_mask(ch); }

int main(void)
{
    for (int latch = 0; latch <= 1; latch++) {
        gpio_t g;
        reset_model(1, 1, latch);
        tri[0] = 0; tri[1] = 0; data[0] = data[1] = 0xFFFFFFFFu;       /* 리셋 직후 상태가 문서마다 달라 일부러 어긋나게 둠 */
        CHK(gpio_init(&g, 0x40000000, 2) == GPIO_OK);
        CHK(tri[0] == 0xFFFFFFFFu && tri[1] == 0xFFFFFFFFu && drv(0) == 0 && drv(1) == 0);
        if (latch) CHK(data[0] == 0 && data[1] == 0);                    /* 저장되는 모델에서는 DATA 에도 실제로 0 이 쓰였어야 함 */
        /* 출력 핀 만들기: 초기 레벨이 1 이면 최종적으로 1 이어야 한다 */
        if (!latch) data[0] = 0;                                        /* 쓰기가 무시되는 모델에서는 init 의 DATA=0 도 무시됐으므로, 리셋 값 0(레지스터 표)을 가정 */
        watch_mask = 1u << 3; watch_level = 1u << 3;
        CHK(gpio_pin_mode(&g, 0, 3, 1, 1) == GPIO_OK);
        CHK((tri[0] & (1u << 3)) == 0 && drv(0) == (1u << 3));
        if (latch) CHK(glitch == 0);                                    /* 입력 쓰기가 저장되는 모델이면 출력이 되는 순간부터 계속 1 이어야 함 */
        else       CHK(glitch == 1);                                    /* 저장 안 되는 모델이면 한 번 0 이 나가는 것을 피할 수 없다(미확인 ① 의 근거) */
        watch_mask = 0;
        CHK(gpio_pin_mode(&g, 0, 5, 1, 0) == GPIO_OK && drv(0) == (1u << 3) && !(tri[0] & (1u << 5)));
        CHK(gpio_write(&g, 0, 5, 1) == GPIO_OK && drv(0) == ((1u << 3) | (1u << 5)));
        CHK(gpio_write(&g, 0, 3, 0) == GPIO_OK && drv(0) == (1u << 5));
        CHK(gpio_toggle(&g, 0, 3) == GPIO_OK && drv(0) == ((1u << 3) | (1u << 5)));
        CHK(gpio_toggle(&g, 0, 3) == GPIO_OK && drv(0) == (1u << 5));
        /* 입력 핀: 쓰기는 거부, 읽기는 핀 값 */
        CHK(gpio_write(&g, 0, 7, 1) == GPIO_ERR_DIR && drv(0) == (1u << 5));
        pins_in[0] = (1u << 7) | (1u << 5) | (1u << 3);                  /* 출력 비트 쪽 핀 값은 읽기에 영향이 없어야 함 */
        CHK(gpio_read(&g, 0, 7) == 1 && gpio_read(&g, 0, 6) == 0);
        CHK(gpio_read(&g, 0, 5) == 1);                                   /* 출력 핀은 그림자(=1) */
        CHK(gpio_read(&g, 0, 3) == 0);                                   /* 그림자(=0), 하드웨어 읽기는 0 이지만 핀이 1 인 것은 무관 */
        uint32_t v; CHK(gpio_read_port(&g, 0, &v) == GPIO_OK && v == ((1u << 7) | (1u << 5)));
        /* 마스크 쓰기 */
        CHK(gpio_pin_mode(&g, 0, 0, 1, 0) == GPIO_OK && gpio_pin_mode(&g, 0, 1, 1, 0) == GPIO_OK);
        CHK(gpio_write_mask(&g, 0, 0x3, 0xFA) == GPIO_OK && drv(0) == ((1u << 5) | 2));   /* 마스크 밖 비트(3)는 값에 1 이 있어도 안 바뀜 */
        CHK(gpio_write_mask(&g, 0, 0x3, 0x1) == GPIO_OK && drv(0) == ((1u << 5) | 1));     /* 마스크 안의 기존 1 은 지워짐 */
        CHK(gpio_write_mask(&g, 0, 0x3, 0x2) == GPIO_OK && (drv(0) & 3) == 2);
        uint32_t before = drv(0); int nh = nhist[0];
        CHK(gpio_write_mask(&g, 0, 0x3 | (1u << 7), 0xFF) == GPIO_ERR_DIR && drv(0) == before && nhist[0] == nh);   /* 입력 비트가 섞이면 아무것도 쓰지 않음 */
        /* 입력으로 되돌리기 */
        CHK(gpio_pin_mode(&g, 0, 3, 0, 0) == GPIO_OK && (tri[0] & (1u << 3)));
        CHK(gpio_write(&g, 0, 3, 1) == GPIO_ERR_DIR);
        /* 채널 2 와 인자 오류 */
        CHK(gpio_pin_mode(&g, 1, 31, 1, 1) == GPIO_OK && (drv(1) == 0x80000000u) && tri[0] != 0 && tri[1] == 0x7FFFFFFFu);
        pins_in[1] = 0x7FFFFFFFu; CHK(gpio_read(&g, 1, 31) == 1 && gpio_read(&g, 1, 0) == 1);
        CHK(gpio_pin_mode(&g, 2, 0, 1, 0) == GPIO_ERR_ARGS && gpio_pin_mode(&g, 0, 32, 1, 0) == GPIO_ERR_ARGS);
        CHK(gpio_read(&g, 0, 32) == GPIO_ERR_ARGS && gpio_read(&g, 3, 0) == GPIO_ERR_ARGS);
        CHK(gpio_write(&g, 0, 32, 1) == GPIO_ERR_ARGS && gpio_toggle(&g, 5, 0) == GPIO_ERR_ARGS);
        CHK(gpio_read_port(&g, 0, 0) == GPIO_ERR_ARGS && gpio_write_mask(&g, 9, 1, 1) == GPIO_ERR_ARGS);
        CHK(gpio_init(&g, 0, 0) == GPIO_ERR_ARGS && gpio_init(&g, 0, 3) == GPIO_ERR_ARGS && gpio_init(0, 0, 1) == GPIO_ERR_ARGS);
        CHK(violations == 0);
        /* 단일 채널 IP: 채널 2 레지스터에 접근하지 않음 */
        reset_model(0, 0, latch);
        CHK(gpio_init(&g, 0, 1) == GPIO_OK && gpio_pin_mode(&g, 0, 2, 1, 1) == GPIO_OK && gpio_write(&g, 0, 2, 0) == GPIO_OK);
        CHK(gpio_pin_mode(&g, 1, 0, 1, 0) == GPIO_ERR_ARGS && violations == 0);
    }
    /* 인터럽트 */
    {
        gpio_t g; reset_model(1, 1, 1);
        CHK(gpio_init(&g, 0, 2) == GPIO_OK);
        isr = 1;                                                           /* 켜기 전에 남아 있던 채널 1 상태 */
        CHK(gpio_irq_enable(&g, GPIO_IRQ_CH1) == GPIO_OK);
        CHK(isr == 0 && ier == 1 && gier == GPIO_GIER_ENABLE);             /* 남은 상태를 먼저 해제하고 IER, GIER */
        isr = 3; CHK(gpio_irq_pending(&g) == 3);
        isr = 1; CHK(gpio_irq_ack(&g, GPIO_IRQ_CH1 | GPIO_IRQ_CH2) == GPIO_OK && isr == 0);   /* 세트된 비트에만 1 을 써야 둘 다 0 */
        isr = 3;
        CHK(gpio_irq_ack(&g, GPIO_IRQ_CH1) == GPIO_OK && isr == 2);        /* 채널 1 만 해제 */
        CHK(gpio_irq_ack(&g, GPIO_IRQ_CH1) == GPIO_OK && isr == 2);        /* 이미 0 인 비트에 1 을 쓰면 다시 켜지므로 쓰지 않아야 함 */
        CHK(gpio_irq_enable(&g, GPIO_IRQ_CH2) == GPIO_OK && ier == 3 && isr == 0);
        CHK(gpio_irq_disable(&g, GPIO_IRQ_CH1) == GPIO_OK && ier == 2 && gier == GPIO_GIER_ENABLE);
        CHK(gpio_irq_disable(&g, GPIO_IRQ_CH2) == GPIO_OK && ier == 0 && gier == 0);
        CHK(gpio_irq_enable(&g, 0) == GPIO_ERR_ARGS && gpio_irq_enable(&g, 4) == GPIO_ERR_ARGS && gpio_irq_ack(&g, 4) == GPIO_ERR_ARGS && gpio_irq_disable(&g, 8) == GPIO_ERR_ARGS);
        CHK(violations == 0);
        reset_model(0, 1, 1); CHK(gpio_init(&g, 0, 1) == GPIO_OK);
        CHK(gpio_irq_enable(&g, GPIO_IRQ_CH2) == GPIO_ERR_ARGS && ier == 0);   /* 단일 채널에서 채널 2 인터럽트 */
        CHK(gpio_irq_enable(&g, GPIO_IRQ_CH1) == GPIO_OK && ier == 1 && violations == 0);
        CHK(GPIO_GIER_ENABLE == 0x80000000u);
    }
    printf(fails ? "FAILED %d\n" : "ALL PASS (axi_gpio model)\n", fails);
    return fails != 0;
}
