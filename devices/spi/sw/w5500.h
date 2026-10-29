/* W5500 유선 이더넷(TCP/IP 하드와이어) 드라이버. axi_qspi 위에서 동작. SPI 모드 0 또는 3, 설계 속도 80 MHz / 보장 최소 33.3 MHz.
 * 근거: WIZnet W5500 Datasheet Version 1.0.5 (NOV 2014) 본문 — 웹 가져오기(WebFetch)가 돌려준 본문 요약을 통해 읽음(원문 PDF 를 이 환경에서 직접 내려받지 못함).
 *   그래서 교차검증 스크립트는 "원문 텍스트"가 아니라 그 요약에서 옮겨 적은 사실 파일(w5500_facts.txt)과 Apache NuttX w5500.c(독립 소스, D3)를 대조한다. 등급 D1(요약 경유) + D3 일치.
 * 사용 범위: TCP 클라이언트/서버, UDP 송수신. MACRAW 는 쓰지 않음(데이터시트 쪽 값이 0x02/0x42 로 상충).
 * UDP 수신: RX 버퍼에 패킷마다 8바이트 헤더(상대 IP 4, 포트 2, 데이터 길이 2)가 앞선다 — WIZnet 문서 UDP 쪽(D1, 요약 경유)과 W5100 계열 Arduino 라이브러리 socket.cpp(D3: 포트·길이를 상위 바이트 먼저 조립)에서 확인.
 *   W5500 자체 코드로 바이트 순서를 확인한 것은 아님(W5500 문서에는 순서 표기 없음) → 빅엔디안은 D3 + 설계. w5500_recv 는 TCP 용 원시 바이트 읽기로 남김.
 * SPI 프레임은 가변 길이 데이터 모드(VDM, OM=00)만 쓴다. 보드에서 실행해 보지 않았고 호스트 모의 테스트만 있다. 리셋 핀·지연은 호출자가 콜백으로 제공한다. */
#ifndef W5500_H
#define W5500_H
#include "axi_qspi.h"

/* 제어 바이트 = BSB[7:3] | RWB[2] | OM[1:0]  (p.15~17). RWB: 0 읽기, 1 쓰기 */
#define W5500_RWB_READ  0x00u
#define W5500_RWB_WRITE 0x04u
#define W5500_OM_VDM    0x00u   /* 가변 길이: SCSn 이 프레임 길이를 정함 */
#define W5500_OM_FDM1   0x01u   /* 고정 1/2/4 바이트 (이 드라이버는 사용하지 않음) */
#define W5500_OM_FDM2   0x02u
#define W5500_OM_FDM4   0x03u
#define W5500_BSB_COMMON      0x00u
#define W5500_BSB_SOCK_REG(n) ((uint8_t)(4u * (n) + 1u))   /* 소켓 n 레지스터 */
#define W5500_BSB_SOCK_TX(n)  ((uint8_t)(4u * (n) + 2u))   /* 소켓 n TX 버퍼 */
#define W5500_BSB_SOCK_RX(n)  ((uint8_t)(4u * (n) + 3u))   /* 소켓 n RX 버퍼 */
#define W5500_CTRL(bsb, rwb, om) ((uint8_t)(((bsb) << 3) | (rwb) | (om)))
#define W5500_NSOCK 8

/* 공통 레지스터 오프셋 (Table 3, p.29) */
#define W5500_MR       0x0000u   /* 리셋 0x00 */
#define W5500_GAR      0x0001u   /* 4 */
#define W5500_SUBR     0x0005u   /* 4 */
#define W5500_SHAR     0x0009u   /* 6 */
#define W5500_SIPR     0x000Fu   /* 4 */
#define W5500_INTLEVEL 0x0013u   /* 2 */
#define W5500_IR       0x0015u
#define W5500_IMR      0x0016u
#define W5500_SIR      0x0017u
#define W5500_SIMR     0x0018u
#define W5500_RTR      0x0019u   /* 2, 단위 100 µs, 리셋 0x07D0 (200 ms) */
#define W5500_RCR      0x001Bu   /* 리셋 0x08 */
#define W5500_PTIMER   0x001Cu
#define W5500_PMAGIC   0x001Du
#define W5500_PHAR     0x001Eu
#define W5500_PSID     0x0024u
#define W5500_PMRU     0x0026u
#define W5500_UIPR     0x0028u
#define W5500_UPORTR   0x002Cu
#define W5500_PHYCFGR  0x002Eu   /* 리셋 0b10111XXX */
#define W5500_VERSIONR 0x0039u   /* 읽기 전용, 0x04 */
#define W5500_VERSION  0x04u
#define W5500_MR_RST   (1u << 7)
#define W5500_MR_PB    (1u << 4)
#define W5500_MR_FARP  (1u << 1)
#define W5500_PHY_LNK  (1u << 0)   /* 읽기 전용 */
#define W5500_PHY_SPD  (1u << 1)   /* 1 = 100 Mbps */
#define W5500_PHY_DPX  (1u << 2)   /* 1 = 전이중 */

/* 소켓 레지스터 오프셋 (Table 4, p.30) */
#define W5500_Sn_MR        0x0000u
#define W5500_Sn_CR        0x0001u
#define W5500_Sn_IR        0x0002u
#define W5500_Sn_SR        0x0003u
#define W5500_Sn_PORT      0x0004u   /* 2 */
#define W5500_Sn_DHAR      0x0006u   /* 6 */
#define W5500_Sn_DIPR      0x000Cu   /* 4 */
#define W5500_Sn_DPORT     0x0010u   /* 2 */
#define W5500_Sn_MSSR      0x0012u
#define W5500_Sn_TOS       0x0015u
#define W5500_Sn_TTL       0x0016u
#define W5500_Sn_RXBUF_SIZE 0x001Eu  /* 리셋 0x02 (2 KB) */
#define W5500_Sn_TXBUF_SIZE 0x001Fu
#define W5500_Sn_TX_FSR    0x0020u   /* 2 */
#define W5500_Sn_TX_RD     0x0022u
#define W5500_Sn_TX_WR     0x0024u
#define W5500_Sn_RX_RSR    0x0026u
#define W5500_Sn_RX_RD     0x0028u
#define W5500_Sn_RX_WR     0x002Au
#define W5500_Sn_IMR       0x002Cu   /* 리셋 0xFF */

#define W5500_PROTO_TCP 0x01u        /* Sn_MR P3:P0 (p.46) */
#define W5500_PROTO_UDP 0x02u

/* Sn_CR 명령 (p.47~49). 받아들여지면 칩이 0x00 으로 되돌림 */
#define W5500_CR_OPEN  0x01u
#define W5500_CR_LISTEN 0x02u
#define W5500_CR_CONNECT 0x04u
#define W5500_CR_DISCON 0x08u
#define W5500_CR_CLOSE 0x10u
#define W5500_CR_SEND  0x20u
#define W5500_CR_SEND_MAC 0x21u
#define W5500_CR_SEND_KEEP 0x22u
#define W5500_CR_RECV  0x40u

/* Sn_IR 비트(1 을 써서 해제, p.49) */
#define W5500_IR_CON      (1u << 0)
#define W5500_IR_DISCON   (1u << 1)
#define W5500_IR_RECV     (1u << 2)
#define W5500_IR_TIMEOUT  (1u << 3)
#define W5500_IR_SEND_OK  (1u << 4)
#define W5500_IR_ALL      0x1Fu      /* 상위 3비트는 예약 */

/* Sn_SR 값 (p.50~51) */
#define W5500_SR_CLOSED      0x00u
#define W5500_SR_INIT        0x13u
#define W5500_SR_LISTEN      0x14u
#define W5500_SR_SYNSENT     0x15u
#define W5500_SR_SYNRECV     0x16u
#define W5500_SR_ESTABLISHED 0x17u
#define W5500_SR_FIN_WAIT    0x18u
#define W5500_SR_CLOSING     0x1Au
#define W5500_SR_TIME_WAIT   0x1Bu
#define W5500_SR_CLOSE_WAIT  0x1Cu
#define W5500_SR_LAST_ACK    0x1Du
#define W5500_SR_UDP         0x22u

/* 타이밍 (p.11, p.63) */
#define W5500_T_RC_MIN_US  500u    /* RSTn Low 최소 */
#define W5500_T_PL_MAX_US  1000u   /* RSTn 해제 → 내부 PLL 잠금 최대 */
#define W5500_RESET_LOW_US 1000u   /* 설계: 최소의 2배 */
#define W5500_RESET_WAIT_US 2000u  /* 설계: 최대의 2배 */

#define W5500_ERR_VERSION (-60)    /* VERSIONR != 0x04 (칩 없음·배선 불량·SPI 모드 오류 의심) */
#define W5500_ERR_VERIFY  (-61)    /* 쓴 값을 다시 읽으니 다름, 또는 16비트 값이 안정되지 않음 */
#define W5500_ERR_TIMEOUT (-62)
#define W5500_ERR_STATE   (-63)    /* 소켓 상태가 이 동작에 맞지 않음 */
#define W5500_ERR_NOSPACE (-64)    /* TX 버퍼가 가득 차 시간 내 비지 않음 */
#define W5500_ERR_PEER    (-65)    /* Sn_IR.TIMEOUT(ARP/TCP 재전송 한도) 또는 연결 거부·종료 */
#define W5500_ERR_NOPKT   (-66)    /* UDP 수신: 완전한 패킷이 아직 없음(헤더 8바이트 미만이거나 데이터가 덜 들어옴) */
#define W5500_ERR_FRAME   (-67)    /* UDP 수신: 헤더의 길이가 RX 버퍼에 들어갈 수 없음 → 소켓을 닫고 다시 열어야 함 */
#define W5500_UDP_HDR     8u       /* 패킷 앞 헤더 바이트 수 */

typedef struct {
    uint8_t mac[6], ip[4], gw[4], mask[4];
    uint8_t tx_kb[W5500_NSOCK], rx_kb[W5500_NSOCK];   /* 각 0,1,2,4,8,16. 합계는 TX·RX 각각 16 이하. 모두 0 이면 리셋 값(소켓당 2 KB) 유지 */
    uint16_t rtr;      /* 0 이면 리셋 값 유지 (단위 100 µs) */
    uint8_t rcr;       /* 0 이면 리셋 값 유지 */
} w5500_cfg_t;

typedef struct {
    qspi_t *s; unsigned cs;
    void *ctx;
    void (*rst)(void *ctx, int level);          /* RSTn 핀(0 = 리셋). NULL 이면 소프트 리셋 */
    void (*delay_us)(void *ctx, uint32_t us);
    uint16_t txsz[W5500_NSOCK], rxsz[W5500_NSOCK];   /* 바이트 (init 이 설정) */
    uint8_t proto[W5500_NSOCK];                       /* open 이 설정 */
} w5500_t;

/* 저수준 */
int w5500_read(w5500_t *w, uint8_t bsb, uint16_t off, uint8_t *buf, size_t len);
int w5500_write(w5500_t *w, uint8_t bsb, uint16_t off, const uint8_t *buf, size_t len);
int w5500_rd8(w5500_t *w, uint8_t bsb, uint16_t off, uint8_t *v);
int w5500_wr8(w5500_t *w, uint8_t bsb, uint16_t off, uint8_t v);
int w5500_rd16(w5500_t *w, uint8_t bsb, uint16_t off, uint16_t *v);   /* 상위 바이트가 낮은 주소. 두 번 연속 같을 때까지 읽음(최대 8번) */
int w5500_wr16(w5500_t *w, uint8_t bsb, uint16_t off, uint16_t v);

/* 고수준 */
int w5500_init(w5500_t *w, const w5500_cfg_t *c);   /* 리셋 → VERSIONR 확인 → 네트워크 설정·소켓 버퍼 크기 쓰기와 읽어 검증 */
int w5500_link(w5500_t *w, int *up, int *speed100, int *full);
int w5500_sock_state(w5500_t *w, unsigned n, uint8_t *sr);
int w5500_sock_open(w5500_t *w, unsigned n, uint8_t proto, uint16_t port);   /* 소켓은 CLOSED 여야 함. TCP → INIT, UDP → UDP 상태 확인 */
int w5500_tcp_listen(w5500_t *w, unsigned n);
int w5500_tcp_connect(w5500_t *w, unsigned n, const uint8_t ip[4], uint16_t port, uint32_t timeout_us);
int w5500_tcp_disconnect(w5500_t *w, unsigned n, uint32_t timeout_us);   /* DISCON, 시간 안에 CLOSED 가 안 되면 CLOSE */
int w5500_close(w5500_t *w, unsigned n);
int w5500_clear_ir(w5500_t *w, unsigned n, uint8_t mask);
int w5500_send(w5500_t *w, unsigned n, const uint8_t *data, unsigned len, uint32_t timeout_us);   /* TCP(ESTABLISHED) 또는 UDP. 성공하면 len */
int w5500_udp_sendto(w5500_t *w, unsigned n, const uint8_t ip[4], uint16_t port, const uint8_t *data, unsigned len, uint32_t timeout_us);
int w5500_rx_avail(w5500_t *w, unsigned n, uint16_t *avail);
int w5500_recv(w5500_t *w, unsigned n, uint8_t *buf, unsigned maxlen);   /* 원시 바이트. 읽은 수(0 이면 없음). Sn_IR.RECV 는 건드리지 않음 */
int w5500_udp_recvfrom(w5500_t *w, unsigned n, uint8_t ip[4], uint16_t *port, uint8_t *buf, unsigned maxlen);
/* UDP 소켓에서 패킷 1개를 꺼낸다. 성공하면 buf 에 복사한 바이트 수(0 포함; 패킷이 maxlen 보다 길면 maxlen 만 복사하고 나머지는 버림), ip·port 는 보낸 쪽.
 * 완전한 패킷이 없으면 W5500_ERR_NOPKT(아무것도 소비하지 않음). Sn_RX_RD 는 항상 패킷 전체(8 + 데이터 길이)만큼 전진시키고 RECV 를 보낸다.
 * Sn_IR.RECV 는 건드리지 않음(호출자가 w5500_clear_ir). */
#endif
