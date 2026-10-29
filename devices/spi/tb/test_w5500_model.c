/* W5500 드라이버 시험. 장치 모델은 w5500_model.h. 규칙·한계는 그 파일 머리말 참고. */
#include "w5500_model.h"

/* ---------- 시험 ---------- */
static void t_init_stream(void)
{
    printf("[init 바이트열]\n");
    reset_model(); make_cfg();
    cfg.tx_kb[0] = 16;
    for (int i = 0; i < 4; i++) cfg.rx_kb[i] = 4;
    memset(&W, 0, sizeof W); W.s = &Q; W.delay_us = model_delay; W.rst = model_rst;
    CHECK(w5500_init(&W, &cfg) == 0, "init 성공");
    CHECK(viol == 0, "init 중 위반 없음");
    CHECK(was_reset, "하드웨어 리셋을 수행");
    /* 손으로 계산한 기대 프레임: (bsb, rw, off, 길이) */
    struct { uint8_t bsb, rw; uint16_t off; int n; uint8_t d[6]; } E[] = {
        { 0, 0, 0x0039, 1, { 0 } },
        { 0, 1, 0x0001, 4, { 192, 168, 0, 1 } },   { 0, 0, 0x0001, 4, { 192, 168, 0, 1 } },
        { 0, 1, 0x0005, 4, { 255, 255, 255, 0 } }, { 0, 0, 0x0005, 4, { 255, 255, 255, 0 } },
        { 0, 1, 0x0009, 6, { 0x00, 0x08, 0xDC, 0x01, 0x02, 0x03 } }, { 0, 0, 0x0009, 6, { 0x00, 0x08, 0xDC, 0x01, 0x02, 0x03 } },
        { 0, 1, 0x000F, 4, { 192, 168, 0, 11 } },  { 0, 0, 0x000F, 4, { 192, 168, 0, 11 } },
    };
    int ne = (int)(sizeof E / sizeof E[0]);
    CHECK(nflog >= ne, "프레임 수 충분");
    for (int i = 0; i < ne && i < nflog; i++) {
        int ok = flog[i].bsb == E[i].bsb && flog[i].rw == E[i].rw && flog[i].off == E[i].off && flog[i].n == E[i].n;
        if (ok && i > 0) ok = memcmp(flog[i].d, E[i].d, (size_t)E[i].n) == 0;
        if (!ok) printf("  프레임 %d 불일치: bsb=%u rw=%u off=%04X n=%d\n", i, flog[i].bsb, flog[i].rw, flog[i].off, flog[i].n);
        CHECK(ok, "init 앞부분 프레임이 손 계산과 같음");
    }
    /* 버퍼 크기 쓰기 순서: 1차(전부 2 KB 이하로), 2차(늘림). 합계는 도중에도 16 KB 이하(모델이 검사) */
    static const uint8_t RX0[8] = { 2, 2, 2, 2, 0, 0, 0, 0 }, TX0[8] = { 2, 0, 0, 0, 0, 0, 0, 0 };
    static const uint8_t RX1[4] = { 4, 4, 4, 4 }, TX1[4] = { 16, 0, 0, 0 };
    struct { int n; uint16_t off; uint8_t v; } X[8 * 2 + 4 * 2]; int nx = 0;
    for (int i = 0; i < 8; i++) { X[nx].n = i; X[nx].off = 0x1E; X[nx++].v = RX0[i]; X[nx].n = i; X[nx].off = 0x1F; X[nx++].v = TX0[i]; }
    for (int i = 0; i < 4; i++) { X[nx].n = i; X[nx].off = 0x1E; X[nx++].v = RX1[i]; X[nx].n = i; X[nx].off = 0x1F; X[nx++].v = TX1[i]; }
    int k = 0;
    for (int i = 0; i < nflog; i++) {
        if (flog[i].rw != 1 || flog[i].bsb == 0 || (flog[i].bsb - 1) % 4 != 0) continue;
        if (flog[i].off != 0x1E && flog[i].off != 0x1F) continue;
        if (k < nx) {
            int ok = (flog[i].bsb - 1) / 4 == X[k].n && flog[i].off == X[k].off && flog[i].d[0] == X[k].v;
            if (!ok) printf("  버퍼 크기 쓰기 %d 불일치: 소켓 %d off %02X 값 %u (기대 %d %02X %u)\n", k, (flog[i].bsb - 1) / 4, flog[i].off, flog[i].d[0], X[k].n, X[k].off, X[k].v);
            CHECK(ok, "버퍼 크기 쓰기 순서·값");
        }
        k++;
    }
    CHECK(k == nx, "버퍼 크기 쓰기 횟수");
    CHECK(W.txsz[0] == 16384 && W.rxsz[3] == 4096 && W.txsz[1] == 0, "드라이버가 소켓 버퍼 크기를 기억");
    CHECK(S[0][0x1F] == 16 && S[2][0x1E] == 4, "칩 레지스터 값");
}
static void t_init_variants(void)
{
    printf("[init 변형·오류]\n");
    CHECK(start(1) == 0, "기본 설정(크기 0 = 리셋 값 유지)");
    CHECK(W.txsz[5] == 2048 && W.rxsz[7] == 2048, "리셋 값 2 KB 유지");
    for (int i = 0; i < nflog; i++) CHECK(!((flog[i].bsb % 4) == 1 && flog[i].rw == 1 && (flog[i].off == 0x1E || flog[i].off == 0x1F)), "크기 0 이면 버퍼 크기 레지스터를 쓰지 않음");
    CHECK(viol == 0, "위반 없음");

    reset_model(); make_cfg(); memset(&W, 0, sizeof W); W.s = &Q; W.delay_us = model_delay; W.rst = model_rst; dead = 1;
    CHECK(w5500_init(&W, &cfg) == W5500_ERR_VERSION, "칩 응답 없음(0xFF) → ERR_VERSION");

    /* 잘못된 설정: SPI 통신 없이 거절 */
    int bad = 0;
    for (int c = 0; c < 4; c++) {
        reset_model(); make_cfg(); memset(&W, 0, sizeof W); W.s = &Q; W.delay_us = model_delay; W.rst = model_rst;
        if (c == 0) cfg.tx_kb[0] = 3;
        if (c == 1) cfg.rx_kb[2] = 5;
        if (c == 2) { cfg.tx_kb[0] = 16; cfg.tx_kb[1] = 1; }
        if (c == 3) { for (int i = 0; i < 8; i++) cfg.rx_kb[i] = 4; }
        int e = w5500_init(&W, &cfg);
        if (e != QSPI_ERR_ARGS || nflog != 0) bad++;
    }
    CHECK(bad == 0, "잘못된 크기 설정 4종은 ARGS, SPI 통신 없음");
    CHECK(w5500_init(0, &cfg) == QSPI_ERR_ARGS && w5500_init(&W, 0) == QSPI_ERR_ARGS, "NULL 인자");

    /* 쓴 값이 읽혀 오지 않으면 ERR_VERIFY */
    { struct { uint8_t bsb; uint16_t off; } D[] = { { 0, 0x0001 }, { 0, 0x0005 }, { 0, 0x000A }, { 0, 0x000F }, { 1, 0x001E }, { 1, 0x001F } };
      int ok = 0;
      for (unsigned i = 0; i < sizeof D / sizeof D[0]; i++) {
          reset_model(); make_cfg(); cfg.tx_kb[0] = 4; cfg.rx_kb[0] = 4; memset(&W, 0, sizeof W); W.s = &Q; W.delay_us = model_delay; W.rst = model_rst;
          drop_bsb = D[i].bsb; drop_off = D[i].off;
          if (w5500_init(&W, &cfg) == W5500_ERR_VERIFY) ok++;
      }
      CHECK(ok == 6, "GAR/SUBR/SHAR/SIPR/RXBUF/TXBUF 쓰기가 반영 안 되면 ERR_VERIFY"); }

    /* 소프트 리셋 경로 */
    reset_model(); make_cfg(); memset(&W, 0, sizeof W); W.s = &Q; W.delay_us = model_delay; mr_rst_polls = 3;
    CHECK(w5500_init(&W, &cfg) == 0, "소프트 리셋: MR.RST 자동 해제를 기다림");
    CHECK(viol == 0, "소프트 리셋 위반 없음");
    reset_model(); make_cfg(); memset(&W, 0, sizeof W); W.s = &Q; W.delay_us = model_delay; mr_rst_polls = 1000000;
    CHECK(w5500_init(&W, &cfg) == W5500_ERR_TIMEOUT, "MR.RST 가 안 풀리면 TIMEOUT");

    /* rtr/rcr */
    reset_model(); make_cfg(); cfg.rtr = 4000; cfg.rcr = 5; memset(&W, 0, sizeof W); W.s = &Q; W.delay_us = model_delay; W.rst = model_rst;
    CHECK(w5500_init(&W, &cfg) == 0 && C[0x19] == 0x0F && C[0x1A] == 0xA0 && C[0x1B] == 5, "RTR = 4000(0x0FA0) 상위 먼저, RCR");
}
static void t_link(void)
{
    printf("[링크]\n");
    int up, sp, fu;
    CHECK(start(1) == 0, "init");
    link_up = 1; full_reset(); C[0x39] = 4;
    CHECK(w5500_link(&W, &up, &sp, &fu) == 0 && up == 1 && sp == 1 && fu == 1, "링크 업·100M·전이중 (LNK=bit0, SPD=bit1, DPX=bit2)");
    C[0x2E] = 0xB8 | 0x02;
    CHECK(w5500_link(&W, &up, &sp, &fu) == 0 && up == 0 && sp == 1 && fu == 0, "링크 다운");
    C[0x2E] = 0xB8 | 0x01;
    CHECK(w5500_link(&W, &up, &sp, 0) == 0 && up == 1 && sp == 0, "10M");
}
static void t_open(void)
{
    printf("[소켓 열기·TCP 연결]\n");
    CHECK(start(1) == 0, "init");
    CHECK(w5500_sock_open(&W, 0, W5500_PROTO_TCP, 0) == QSPI_ERR_ARGS, "포트 0 거절");
    CHECK(w5500_sock_open(&W, 0, 3, 80) == QSPI_ERR_ARGS && w5500_sock_open(&W, 8, W5500_PROTO_TCP, 80) == QSPI_ERR_ARGS, "프로토콜·소켓 번호 범위");
    CHECK(w5500_sock_open(&W, 0, W5500_PROTO_TCP, 5000) == 0 && S[0][3] == 0x13, "TCP OPEN → INIT");
    CHECK(S[0][4] == 0x13 && S[0][5] == 0x88, "Sn_PORT = 5000 → 0x13, 0x88 (p.53 예제)");
    CHECK(w5500_sock_open(&W, 0, W5500_PROTO_TCP, 5000) == W5500_ERR_STATE, "이미 열린 소켓");
    CHECK(w5500_sock_open(&W, 1, W5500_PROTO_UDP, 6000) == 0 && S[1][3] == 0x22, "UDP OPEN → UDP");
    open_fail = 1; CHECK(w5500_sock_open(&W, 2, W5500_PROTO_TCP, 80) == W5500_ERR_STATE, "OPEN 후 상태가 기대와 다르면 오류"); open_fail = 0;
    CHECK(S[2][3] == 0 && n_close >= 1, "실패 후 CLOSE 명령으로 CLOSED 로 정리");
    CHECK(viol == 0, "위반 없음");

    /* TCP 연결 */
    CHECK(start(1) == 0, "init");
    cr_lat = 2; connect_polls = 3;
    CHECK(open_tcp_established(0, 5000) == 0 && S[0][3] == 0x17, "연결 성공 (SR 폴링, Sn_CR 해제 대기)");
    CHECK(S[0][0x0C] == 192 && S[0][0x0F] == 20 && S[0][0x10] == 0x13 && S[0][0x11] == 0x88, "DIPR 192.168.0.20, DPORT 5000");
    CHECK((S[0][2] & 1) == 0, "성공 후 Sn_IR.CON 해제");
    CHECK(viol == 0, "위반 없음");

    connect_mode = CONN_REFUSE; CHECK(start(1) == 0, "init"); connect_mode = CONN_REFUSE;
    CHECK(open_tcp_established(0, 5000) == W5500_ERR_PEER, "연결 거부(CLOSED) → ERR_PEER");
    connect_mode = CONN_TIMEOUT; CHECK(start(1) == 0, "init"); connect_mode = CONN_TIMEOUT;
    CHECK(open_tcp_established(0, 5000) == W5500_ERR_PEER && (S[0][2] & 0x08) == 0, "Sn_IR.TIMEOUT → ERR_PEER, 플래그 해제");
    CHECK(start(1) == 0, "init"); connect_mode = CONN_NEVER;
    CHECK(open_tcp_established(0, 5000) == W5500_ERR_TIMEOUT, "응답 없음 → ERR_TIMEOUT");
    CHECK(start(1) == 0, "init");
    CHECK(w5500_tcp_connect(&W, 0, PEER, 80, 1000) == W5500_ERR_STATE, "INIT 아니면 CONNECT 거절");
    CHECK(w5500_sock_open(&W, 0, W5500_PROTO_UDP, 80) == 0 && w5500_tcp_connect(&W, 0, PEER, 80, 1000) == W5500_ERR_STATE, "UDP 소켓에 CONNECT 거절");
    CHECK(w5500_tcp_listen(&W, 0) == W5500_ERR_STATE, "UDP 소켓에 LISTEN 거절");
    CHECK(w5500_sock_open(&W, 1, W5500_PROTO_TCP, 80) == 0 && w5500_tcp_listen(&W, 1) == 0 && S[1][3] == 0x14, "LISTEN");
    CHECK(w5500_tcp_listen(&W, 1) == W5500_ERR_STATE, "이미 LISTEN 인 소켓에 LISTEN 거절");
    CHECK(w5500_sock_open(&W, 2, W5500_PROTO_TCP, 81) == 0 && w5500_tcp_connect(&W, 2, PEER, 81, 10000) == 0, "소켓 2 연결");
    CHECK(w5500_tcp_connect(&W, 2, PEER, 81, 1000) == W5500_ERR_STATE && w5500_tcp_listen(&W, 2) == W5500_ERR_STATE, "연결된(INIT 아닌) TCP 소켓에 CONNECT/LISTEN 거절");
    CHECK(viol == 0, "위반 없음");
}
static void t_send(void)
{
    printf("[송신]\n");
    CHECK(start(1) == 0, "init");
    CHECK(open_tcp_established(0, 5000) == 0, "연결");
    uint8_t d[40]; for (int i = 0; i < 40; i++) d[i] = (uint8_t)(0x30 + i);
    CHECK(w5500_send(&W, 0, d, 10, 5000) == 10, "10바이트 송신");
    CHECK(nwire == 1 && wire[0].len == 10 && memcmp(wire[0].d, d, 10) == 0 && wire[0].sock == 0, "상대에게 간 바이트가 같음");
    CHECK(rd16s(0, 0x24) == 10 && rd16s(0, 0x22) == 10, "TX_WR 전진, TX_RD 따라감");
    CHECK((S[0][2] & 0x10) == 0, "SEND_OK 해제");
    CHECK(n_cmds >= 3, "명령 발행");

    /* 버퍼 경계(2 KB): 0x07F0 에서 40바이트 → 16 + 24 로 나뉨 */
    force_tx_ptr(0, 0x07F0); nwire = 0;
    CHECK(w5500_send(&W, 0, d, 40, 5000) == 40, "경계 걸친 송신");
    CHECK(nwire == 1 && wire[0].len == 40 && memcmp(wire[0].d, d, 40) == 0, "경계에서 나눠 써도 순서대로 도착");
    CHECK(rd16s(0, 0x24) == 0x0818, "TX_WR = 0x07F0 + 40");
    { int cross = 0, a = 0, b = 0; for (int i = 0; i < nflog; i++) if (flog[i].bsb == 2 && flog[i].rw == 1) { cross++; if (flog[i].off == 0x07F0 && flog[i].n == 16) a = 1; if (flog[i].off == 0x0800 && flog[i].n == 24) b = 1; }
      CHECK(a && b, "TX 버퍼 쓰기 프레임: (0x07F0,16) 와 (0x0800,24)"); CHECK(cross >= 2, "프레임이 둘 이상"); }
    /* 16비트 포인터 올림: 0xFFF0 + 32 → 0x0010 */
    force_tx_ptr(0, 0xFFF0); nwire = 0;
    CHECK(w5500_send(&W, 0, d, 32, 5000) == 32 && rd16s(0, 0x24) == 0x0010, "16비트 올림 버림 (p.57)");
    CHECK(nwire == 1 && memcmp(wire[0].d, d, 32) == 0, "올림 경계 송신 데이터");

    { static uint8_t big[2048]; for (int i = 0; i < 2048; i++) big[i] = (uint8_t)(i ^ 0x5A); force_tx_ptr(0, 0x0123); nwire = 0;
      CHECK(w5500_send(&W, 0, big, 2048, 5000) == 2048, "버퍼 크기와 같은 길이(2048) 송신 (여유 == 길이)");
      CHECK(nwire == 1 && wire[0].len == 2048 && memcmp(wire[0].d, big, 2048) == 0, "2048바이트가 순서대로 도착"); }
    CHECK(w5500_send(&W, 0, d, 0, 100) == QSPI_ERR_ARGS && w5500_send(&W, 0, d, 2049, 100) == QSPI_ERR_ARGS, "길이 0, 버퍼보다 큼 거절");
    CHECK(w5500_send(&W, 0, 0, 5, 100) == QSPI_ERR_ARGS && w5500_send(&W, 9, d, 5, 100) == QSPI_ERR_ARGS, "NULL·소켓 번호");
    CHECK(w5500_send(&W, 3, d, 5, 100) == W5500_ERR_STATE, "열리지 않은 소켓");

    /* 여유 공간 대기 */
    tx_busy_left[0] = 5;
    CHECK(w5500_send(&W, 0, d, 8, 5000) == 8, "TX_FSR 이 잠시 0 이어도 기다렸다 보냄");
    tx_busy_left[0] = 100000;
    CHECK(w5500_send(&W, 0, d, 8, 500) == W5500_ERR_NOSPACE, "계속 가득이면 ERR_NOSPACE");
    tx_busy_left[0] = 0;

    /* 전송 완료 지연·실패 */
    nwire = 0; send_lat = 4;
    CHECK(w5500_send(&W, 0, d, 8, 5000) == 8, "SEND_OK 가 늦게 와도 기다림");
    send_lat = 100000;
    CHECK(w5500_send(&W, 0, d, 8, 300) == W5500_ERR_TIMEOUT, "SEND_OK 가 안 오면 ERR_TIMEOUT");
    CHECK(start(1) == 0 && open_tcp_established(0, 5000) == 0, "재시작");
    peer_dead = 1;
    CHECK(w5500_send(&W, 0, d, 8, 5000) == W5500_ERR_PEER && (S[0][2] & 0x08) == 0, "Sn_IR.TIMEOUT → ERR_PEER, 플래그 해제");
    CHECK(w5500_send(&W, 0, d, 8, 500) == W5500_ERR_STATE, "끊어진 뒤에는 STATE");

    /* TX_FSR 첫 읽기가 틀려도 두 번 읽기로 바로잡음 */
    CHECK(start(1) == 0 && open_tcp_established(0, 5000) == 0, "재시작");
    glitch = 1;
    CHECK(w5500_send(&W, 0, d, 8, 5000) == 8, "TX_FSR 읽기 오류(첫 값이 다름)를 넘김");
    CHECK(viol == 0, "위반 없음");
}
static void t_udp(void)
{
    printf("[UDP]\n");
    CHECK(start(1) == 0, "init");
    CHECK(w5500_sock_open(&W, 2, W5500_PROTO_UDP, 6000) == 0, "UDP 열기");
    uint8_t d[20]; for (int i = 0; i < 20; i++) d[i] = (uint8_t)(0xA0 + i);
    CHECK(w5500_udp_sendto(&W, 2, PEER, 7000, d, 20, 5000) == 20, "UDP 송신");
    CHECK(nwire == 1 && wire[0].proto == 2 && wire[0].port == 7000 && memcmp(wire[0].ip, PEER, 4) == 0 && memcmp(wire[0].d, d, 20) == 0, "목적지 IP·포트·데이터");
    CHECK(w5500_udp_sendto(&W, 2, PEER, 0, d, 20, 5000) == QSPI_ERR_ARGS, "포트 0 거절");
    CHECK(open_tcp_established(0, 80) == 0 && w5500_udp_sendto(&W, 0, PEER, 7000, d, 20, 5000) == W5500_ERR_STATE, "연결된 TCP 소켓에 sendto 거절");
    CHECK(viol == 0, "위반 없음");
}
static void t_recv(void)
{
    printf("[수신]\n");
    CHECK(start(1) == 0, "init");
    CHECK(open_tcp_established(0, 5000) == 0, "연결");
    uint8_t in[100], out[100]; for (int i = 0; i < 100; i++) in[i] = (uint8_t)(i * 3 + 1);
    uint16_t av;
    CHECK(w5500_rx_avail(&W, 0, &av) == 0 && av == 0, "처음엔 없음");
    int rc0 = n_recv_cmd;
    CHECK(w5500_recv(&W, 0, out, 10) == 0 && n_recv_cmd == rc0, "없으면 0 반환, RECV 명령 안 보냄");
    inject_rx(0, in, 100);
    CHECK(w5500_rx_avail(&W, 0, &av) == 0 && av == 100, "수신량 100");
    CHECK(w5500_recv(&W, 0, out, 30) == 30 && memcmp(out, in, 30) == 0, "30바이트");
    CHECK(rd16s(0, 0x28) == 30 && n_recv_cmd == rc0 + 1, "RX_RD 전진 후 RECV 명령");
    CHECK(w5500_recv(&W, 0, out, 30) == 30 && memcmp(out, in + 30, 30) == 0, "다음 30바이트");
    CHECK(w5500_recv(&W, 0, out, 100) == 40 && memcmp(out, in + 60, 40) == 0, "남은 40바이트만 반환");
    CHECK(w5500_recv(&W, 0, out, 100) == 0, "다 읽음");
    CHECK((S[0][2] & 0x04) != 0, "Sn_IR.RECV 는 건드리지 않음(호출자가 해제)");
    CHECK(w5500_clear_ir(&W, 0, W5500_IR_RECV) == 0 && (S[0][2] & 0x04) == 0, "clear_ir");

    /* 버퍼 경계(2 KB) 걸침 + 16비트 올림 */
    force_rx_ptr(0, 0x07F8); inject_rx(0, in, 20);
    CHECK(w5500_recv(&W, 0, out, 20) == 20 && memcmp(out, in, 20) == 0, "경계 걸친 수신");
    CHECK(rd16s(0, 0x28) == 0x080C, "RX_RD = 0x07F8 + 20");
    force_rx_ptr(0, 0xFFF8); inject_rx(0, in, 20);
    CHECK(w5500_recv(&W, 0, out, 20) == 20 && memcmp(out, in, 20) == 0 && rd16s(0, 0x28) == 0x000C, "16비트 올림 경계 수신");

    glitch = 1; inject_rx(0, in, 12);
    CHECK(w5500_recv(&W, 0, out, 100) == 12 && memcmp(out, in, 12) == 0, "RX_RSR 첫 읽기가 틀려도 두 번 읽기로 바로잡음");
    CHECK(w5500_recv(&W, 0, 0, 5) == QSPI_ERR_ARGS && w5500_recv(&W, 0, out, 0) == QSPI_ERR_ARGS && w5500_recv(&W, 8, out, 5) == QSPI_ERR_ARGS, "인자 검사");
    CHECK(viol == 0, "위반 없음");
}
static void t_udp_recv(void)
{
    printf("[UDP 수신]\n");
    CHECK(start(1) == 0, "init");
    CHECK(w5500_sock_open(&W, 2, W5500_PROTO_UDP, 123) == 0, "UDP 열기");
    static uint8_t in[2100], out[2100]; for (int i = 0; i < 2100; i++) in[i] = (uint8_t)(i * 7 + 3);
    const uint8_t SRC[4] = {10, 20, 30, 40}, SRC2[4] = {172, 16, 5, 9};
    uint8_t ip[4]; uint16_t port;
    int rc0 = n_recv_cmd;
    CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 100) == W5500_ERR_NOPKT && n_recv_cmd == rc0, "없으면 NOPKT, RECV 안 보냄");
    inject_udp(2, SRC, 0x1234, in, 300);
    CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 1000) == 300 && memcmp(out, in, 300) == 0, "300바이트 패킷(길이 상·하위 바이트가 다름)");
    CHECK(memcmp(ip, SRC, 4) == 0 && port == 0x1234, "상대 IP·포트 (포트 0x1234)");
    CHECK(rd16s(2, 0x28) == 308 && n_recv_cmd == rc0 + 1, "RX_RD = 8 + 300, RECV 1회");
    CHECK(rsr(2) == 0, "다 소비");
    /* 두 패킷 연속 */
    inject_udp(2, SRC, 111, in, 10); inject_udp(2, SRC2, 222, in + 50, 20);
    CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 100) == 10 && memcmp(out, in, 10) == 0 && port == 111 && memcmp(ip, SRC, 4) == 0, "첫 패킷");
    CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 100) == 20 && memcmp(out, in + 50, 20) == 0 && port == 222 && memcmp(ip, SRC2, 4) == 0, "두 번째 패킷");
    CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 100) == W5500_ERR_NOPKT, "다 읽음");
    /* 버퍼보다 큰 maxlen 이 작을 때: 잘라내고 나머지는 버림 */
    inject_udp(2, SRC, 5, in, 40); inject_udp(2, SRC2, 6, in + 100, 8);
    memset(out, 0xEE, 64);
    CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 16) == 16 && memcmp(out, in, 16) == 0 && out[16] == 0xEE, "maxlen 16 → 16바이트만 복사, 더 쓰지 않음");
    CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 16) == 8 && memcmp(out, in + 100, 8) == 0 && port == 6, "잘린 패킷의 나머지는 버려지고 다음 패킷으로");
    /* 길이 0 패킷 */
    inject_udp(2, SRC, 77, in, 0);
    CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 16) == 0 && port == 77 && rsr(2) == 0, "데이터 0 바이트 패킷도 소비하고 0 반환");
    /* 아직 덜 들어온 패킷 */
    {
        uint8_t h[8] = {1, 2, 3, 4, 0, 9, 0, 20};
        inject_rx(2, h, 5);
        CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 100) == W5500_ERR_NOPKT && rsr(2) == 5, "헤더 8바이트 미만 → NOPKT, 소비 없음");
        inject_rx(2, h + 5, 3); inject_rx(2, in, 10);
        int c0 = n_recv_cmd; uint16_t rd0 = rd16s(2, 0x28);
        CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 100) == W5500_ERR_NOPKT && rd16s(2, 0x28) == rd0 && n_recv_cmd == c0, "데이터가 덜 옴(20 중 10) → NOPKT, RX_RD 그대로, RECV 안 보냄");
        inject_rx(2, in + 10, 6);                         /* 20 중 16 도착: 남은 양(24)이 길이(20)보다 커도 헤더 8 을 더해야 아직 덜 온 것 */
        CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 100) == W5500_ERR_NOPKT && rd16s(2, 0x28) == rd0, "20 중 16 도착(헤더 포함 24 ≥ 길이 20) → 여전히 NOPKT");
        inject_rx(2, in + 16, 4);
        pkt_end[2][npkt[2]++] = rd16s(2, 0x2A);
        CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 100) == 20 && memcmp(out, in, 20) == 0 && port == 9, "나머지가 오면 정상 수신");
    }
    /* 블록 경계·16비트 올림 */
    force_rx_ptr(2, 0x07F8); inject_udp(2, SRC, 99, in, 30);
    CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 100) == 30 && memcmp(out, in, 30) == 0 && port == 99 && memcmp(ip, SRC, 4) == 0 && rd16s(2, 0x28) == (uint16_t)(0x07F8 + 38), "버퍼 경계(헤더 안에서) 걸침");
    force_rx_ptr(2, 0xFFFA); inject_udp(2, SRC2, 98, in, 30);
    CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 100) == 30 && memcmp(out, in, 30) == 0 && port == 98 && memcmp(ip, SRC2, 4) == 0 && rd16s(2, 0x28) == (uint16_t)(0xFFFA + 38), "16비트 올림 경계");
    force_rx_ptr(2, 0x0803); inject_udp(2, SRC, 97, in, 1500);
    CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 2000) == 1500 && memcmp(out, in, 1500) == 0, "1500바이트 패킷, 경계 걸침");
    /* RSR 읽기 오류 */
    glitch = 1; inject_udp(2, SRC, 55, in, 12);
    CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 100) == 12 && port == 55, "RX_RSR 첫 읽기가 틀려도 처리");
    /* 비정상 길이: RX 버퍼(2 KB)보다 큰 길이 */
    {
        uint8_t h[8] = {1, 2, 3, 4, 0, 9, 0x0B, 0xB8};   /* 3000 */
        force_rx_ptr(2, 0); inject_rx(2, h, 8); inject_rx(2, in, 100);
        uint16_t rd0 = rd16s(2, 0x28); int c0 = n_recv_cmd;
        CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 100) == W5500_ERR_FRAME && rd16s(2, 0x28) == rd0 && n_recv_cmd == c0, "길이 3000 > 2 KB → FRAME, 소비 없음");
    }
    /* FRAME 경계: 8 + 2040 = 2048 은 받고 8 + 2041 은 거절 */
    {
        force_rx_ptr(2, 0); inject_udp(2, SRC, 1, in, 2040);
        CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 2000) == 2000 && memcmp(out, in, 2000) == 0 && rd16s(2, 0x28) == 2048, "8 + 2040 = RX 버퍼 크기: 받음(maxlen 2000 으로 잘림)");
        uint8_t h[8] = {1, 2, 3, 4, 0, 9, 0x07, 0xF9};   /* 2041 */
        force_rx_ptr(2, 0); inject_rx(2, h, 8); inject_rx(2, in, 100);
        CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 100) == W5500_ERR_FRAME, "8 + 2041 > 2048 → FRAME");
    }
    /* 인자·상태 */
    CHECK(w5500_udp_recvfrom(&W, 2, 0, &port, out, 5) == QSPI_ERR_ARGS && w5500_udp_recvfrom(&W, 2, ip, 0, out, 5) == QSPI_ERR_ARGS && w5500_udp_recvfrom(&W, 2, ip, &port, 0, 5) == QSPI_ERR_ARGS, "인자 검사");
    CHECK(w5500_udp_recvfrom(&W, 2, ip, &port, out, 0) == QSPI_ERR_ARGS && w5500_udp_recvfrom(&W, 8, ip, &port, out, 5) == QSPI_ERR_ARGS && w5500_udp_recvfrom(NULL, 2, ip, &port, out, 5) == QSPI_ERR_ARGS, "인자 검사 2");
    CHECK(open_tcp_established(0, 80) == 0 && w5500_udp_recvfrom(&W, 0, ip, &port, out, 5) == W5500_ERR_STATE, "TCP 소켓에는 STATE");
    CHECK(viol == 0, "위반 없음");
}
static void t_close(void)
{
    printf("[연결 종료]\n");
    CHECK(start(1) == 0 && open_tcp_established(0, 5000) == 0, "연결");
    CHECK(w5500_tcp_disconnect(&W, 0, 2000) == 0 && S[0][3] == 0 && W.proto[0] == 0, "DISCON → CLOSED");
    CHECK(n_discon == 1, "DISCON 명령을 보냄");
    CHECK(w5500_send(&W, 0, (const uint8_t *)"x", 1, 100) == W5500_ERR_STATE, "닫힌 뒤 송신 거절");
    CHECK(start(1) == 0 && open_tcp_established(0, 5000) == 0, "재연결");
    discon_hang = 1;
    CHECK(w5500_tcp_disconnect(&W, 0, 1000) == 0 && S[0][3] == 0, "DISCON 이 안 끝나면 CLOSE 로 정리");
    CHECK(n_discon == 1 && n_close >= 1, "DISCON 후 CLOSE 명령");
    CHECK(start(1) == 0 && w5500_sock_open(&W, 1, W5500_PROTO_UDP, 9) == 0, "UDP 소켓");
    S[1][2] = 0x1F;
    CHECK(w5500_close(&W, 1) == 0 && S[1][3] == 0 && S[1][2] == 0, "CLOSE 후 IR 해제");
    CHECK(w5500_tcp_disconnect(&W, 1, 100) == W5500_ERR_STATE, "UDP 소켓에 DISCON 거절");
    CHECK(viol == 0, "위반 없음");
}
static void t_frames(void)
{
    printf("[프레임 형식]\n");
    CHECK(start(1) == 0, "init");
    nflog = 0;
    uint8_t v; uint8_t b[3];
    CHECK(w5500_rd8(&W, W5500_BSB_COMMON, W5500_VERSIONR, &v) == 0 && v == 4, "VERSIONR = 0x04");
    CHECK(nflog == 1 && flog[0].bsb == 0 && flog[0].rw == 0 && flog[0].off == 0x0039, "읽기 프레임: BSB 0, RWB 0, 주소 0x0039");
    CHECK(w5500_wr8(&W, W5500_BSB_SOCK_REG(3), W5500_Sn_TTL, 0x55) == 0 && S[3][0x16] == 0x55, "소켓 3 레지스터 쓰기");
    CHECK(flog[1].bsb == 13 && flog[1].rw == 1, "소켓 3 레지스터 BSB = 4×3+1 = 13, RWB 1");
    CHECK(W5500_CTRL(W5500_BSB_SOCK_REG(0), W5500_RWB_WRITE, 0) == 0x0C && W5500_CTRL(W5500_BSB_SOCK_TX(0), W5500_RWB_WRITE, 0) == 0x14 && W5500_CTRL(W5500_BSB_SOCK_RX(0), W5500_RWB_READ, 0) == 0x18, "제어 바이트: 소켓 0 레지스터 쓰기 0x0C, TX 쓰기 0x14, RX 읽기 0x18");
    CHECK(W5500_CTRL(W5500_BSB_SOCK_REG(7), W5500_RWB_WRITE, 0) == 0xEC && W5500_CTRL(W5500_BSB_SOCK_TX(7), W5500_RWB_WRITE, 0) == 0xF4 && W5500_CTRL(W5500_BSB_SOCK_RX(7), W5500_RWB_READ, 0) == 0xF8, "소켓 7: 0xEC, 0xF4, 0xF8");
    CHECK(w5500_read(&W, 0, 0, b, 0) == QSPI_ERR_ARGS && w5500_read(&W, 32, 0, b, 1) == QSPI_ERR_ARGS && w5500_write(&W, 0, 0, 0, 1) == QSPI_ERR_ARGS, "인자 검사");
    CHECK(viol == 0, "위반 없음");
}

int main(void)
{
    t_init_stream(); t_init_variants(); t_link(); t_open(); t_send(); t_udp(); t_recv(); t_udp_recv(); t_close(); t_frames();
    if (fails) { printf("FAIL: %d\n", fails); return 1; }
    printf("ALL PASS (w5500 model)\n");
    return 0;
}
