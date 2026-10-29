/* quad_axil 엔코더 레지스터 접근 (rtl/quad_axil_regs.v 와 같은 맵). */
#ifndef QUAD_H
#define QUAD_H
#include <stdint.h>
#ifndef QD_RD
#define QD_RD(base, off)    (*(volatile uint32_t *)((uintptr_t)(base) + (off)))
#define QD_WR(base, off, v) (*(volatile uint32_t *)((uintptr_t)(base) + (off)) = (v))
#endif
#define QD_COUNT 0x00
#define QD_STAT  0x04
#define QD_CTRL  0x08
#define QD_ST_BTN   (1u << 0)
#define QD_ST_PRESS (1u << 1)
#define QD_ST_ERR   (1u << 2)
#define QD_ST_STEP  (1u << 3)
#define QD_ST_EVENTS (QD_ST_PRESS | QD_ST_ERR | QD_ST_STEP)
static inline int32_t quad_count(uintptr_t b) { return (int32_t)QD_RD(b, QD_COUNT); }
static inline void quad_clear(uintptr_t b) { QD_WR(b, QD_COUNT, 0); }
static inline void quad_set_invert(uintptr_t b, int inv) { QD_WR(b, QD_CTRL, inv ? 1u : 0u); }
/* 래치된 이벤트(press/err/step)를 읽고 지움. 반환: 읽은 STATUS 의 이벤트 비트 */
static inline uint32_t quad_take_events(uintptr_t b)
{ uint32_t e = QD_RD(b, QD_STAT) & QD_ST_EVENTS; if (e) QD_WR(b, QD_STAT, e); return e; }
/* 한 클릭(디텐트)당 카운트 수는 소자마다 달라 문서에서 확인되지 않음(ALPS 문서: 디텐트 30, 펄스 15 → 4배수로 하면 클릭당 2카운트일 가능성, 미확인) */
#endif
