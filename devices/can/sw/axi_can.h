/* AXI CAN (PG096 v5.1) 드라이버. 레지스터·비트·메시지 구조는 PDF 표에서 확인한 값.
 * 호스트 모의 테스트만 수행, 보드 미검증. */
#ifndef AXI_CAN_H
#define AXI_CAN_H
#include <stdint.h>

#ifndef CAN_RD
#define CAN_RD(base, off)     (*(volatile uint32_t *)((uintptr_t)(base) + (off)))
#define CAN_WR(base, off, v)  (*(volatile uint32_t *)((uintptr_t)(base) + (off)) = (v))
#endif

#define CAN_SRR   0x000
#define CAN_MSR   0x004
#define CAN_BRPR  0x008
#define CAN_BTR   0x00C
#define CAN_SR    0x018
#define CAN_ISR   0x01C
#define CAN_IER   0x020
#define CAN_ICR   0x024
#define CAN_TXFIFO 0x030   /* ID, DLC, DW1, DW2 */
#define CAN_TXHPB  0x040
#define CAN_RXFIFO 0x050

#define CAN_SRR_SRST (1u << 0)
#define CAN_SRR_CEN  (1u << 1)
#define CAN_MSR_SLEEP (1u << 0)
#define CAN_MSR_LBACK (1u << 1)
#define CAN_SR_CONFIG (1u << 0)
#define CAN_SR_NORMAL (1u << 3)
#define CAN_SR_LBACK  (1u << 1)
#define CAN_SR_TXBFLL (1u << 9)
#define CAN_SR_TXFLL  (1u << 10)
/* ISR / ICR 비트 (표 기준; 본문 목록에는 RXNEMP 가 빠져 있고 표는 bit7 = RXNEMP, bit8 = ERROR) */
#define CAN_IS_ARBLST (1u << 0)
#define CAN_IS_TXOK   (1u << 1)
#define CAN_IS_RXOK   (1u << 4)
#define CAN_IS_RXUFLW (1u << 5)
#define CAN_IS_RXOFLW (1u << 6)
#define CAN_IS_RXNEMP (1u << 7)
#define CAN_IS_ERROR  (1u << 8)
#define CAN_IS_BSOFF  (1u << 9)

#define CAN_OK       0
#define CAN_ERR_ARGS -1
#define CAN_ERR_TIMING -2   /* 해당 클럭·속도로 정확한 비트 타이밍 없음 */
#define CAN_ERR_FULL -3     /* TX FIFO 가득 */
#define CAN_ERR_EMPTY -4    /* 수신 메시지 없음 */
#define CAN_ERR_MODE -5     /* 모드 전이 안 됨(CONFIG 비트 확인 실패) */

typedef struct {
    uint32_t id;       /* 표준: 11비트, 확장: 29비트 */
    uint8_t  ext;      /* 1 = 확장 프레임 */
    uint8_t  rtr;
    uint8_t  dlc;      /* 0~8 (데이터 길이는 8 로 제한) */
    uint8_t  data[8];
} can_msg_t;

typedef struct { uint8_t brpr; uint16_t btr; } can_timing_t; /* 레지스터에 쓸 값(실제값-1 적용 후) */

/* 정확한 분주만 허용. 샘플 포인트 87.5% 에 가장 가까운 조합 선택(설계안: 목표값은 PG096 이 아니라 일반 관례) */
int can_calc_timing(uint32_t clk_hz, uint32_t bitrate, can_timing_t *t);

/* 설정 모드에서 BRPR/BTR/MSR 기록 후 CEN=1. loopback!=0 이면 루프백. 반환 CAN_OK 또는 오류 */
int can_init(uintptr_t base, const can_timing_t *t, int loopback);
int can_send(uintptr_t base, const can_msg_t *m);       /* TX FIFO, 가득이면 CAN_ERR_FULL */
int can_send_hp(uintptr_t base, const can_msg_t *m);    /* High Priority Buffer */
int can_recv(uintptr_t base, can_msg_t *m);             /* 없으면 CAN_ERR_EMPTY */
#endif
