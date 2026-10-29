/* MicroBlaze V 용 HC-SR04 거리계 드라이버 예시. 컴파일과 보드 시험은 아직 하지 않았다. */
#include <stdint.h>

#define US_BASE   0x44A10000u   /* Vivado 주소 편집기에서 정한 값으로 바꿀 것 */
#define US_CTRL   (*(volatile uint32_t *)(US_BASE + 0x00))
#define US_STAT   (*(volatile uint32_t *)(US_BASE + 0x04))
#define US_DIST   (*(volatile uint32_t *)(US_BASE + 0x08))
#define US_IRQEN  (*(volatile uint32_t *)(US_BASE + 0x10))

#define ST_BUSY (1u << 0)
#define ST_VALID (1u << 1)
#define ST_TIMEOUT (1u << 2)
#define ST_DONE (1u << 3)

void us_start(void) { US_STAT = ST_TIMEOUT | ST_DONE; US_CTRL = 1; }

/* 측정이 끝나기를 기다렸다가 cm를 돌려준다. 타임아웃이면 -1. 측정 간격이 60 ms라 최대 그만큼 걸린다. */
int us_read_cm(void) {
    for (;;) {
        uint32_t s = US_STAT;
        if (s & ST_TIMEOUT) { US_STAT = ST_TIMEOUT; return -1; }
        if (s & ST_DONE)    { US_STAT = ST_DONE; return (int)(US_DIST & 0x3FF); }
    }
}
