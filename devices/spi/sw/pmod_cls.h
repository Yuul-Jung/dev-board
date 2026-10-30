/* Pmod CLS — SPI 문자 LCD 16×2 드라이버.
 *
 * 장치 개요 (D1 Digilent Pmod CLS Reference):
 *   - 컨트롤러: PIC16F723A + 직렬(SPI/UART/I2C) 인터페이스.
 *   - SPI 모드 (점퍼 JP1=off, JP2=off 기본): Mode 0 (CPOL=0, CPHA=0), 최대 1 MHz, MSB 먼저.
 *   - CS 는 전송 시작 전 Low, 전송 완료 후 High.
 *   - 각 문자/이스케이프 바이트를 개별 1바이트 SPI 전송으로 보낸다.
 *   - 열 0~15, 행 0~1.
 *
 * 이스케이프 시퀀스 요약 (D1):
 *   \x1B[j             화면 지우기 + 커서 홈.
 *   \x1B[<r>;<c>H      커서 이동 (r='0'/'1', c='0'..'F' 16진 2자리 열번호 아님, 0~15 ASCII 숫자 또는 문자).
 *                      ※ CLS 행·열 순서: Pmod CLS Reference 의 예제를 따름.
 *   \x1B[nA/B          커서 위/아래 n 행.
 *   \x1B[nC/D          커서 오른쪽/왼쪽 n 열.
 *
 * 이 드라이버가 구현하는 기능:
 *   - cls_clear:        화면 지우기.
 *   - cls_home:         커서를 (0,0) 으로.
 *   - cls_cursor:       지정 위치로.
 *   - cls_write:        현재 위치에서 문자열 출력 (자동 줄넘김 없음).
 *   - cls_puts:         커서 위치 지정 후 문자열 출력.
 *   - cls_clear_write:  지우기 + 2행 문자열 출력 편의 함수.
 *
 * 한계:
 *   - 각 바이트를 개별 qspi_xfer 로 전송 (FIF0 깊이 0 가정이 안전).
 *   - 커서 이동 이스케이프 시퀀스의 행·열 숫자 형식은 D1 예제를 따름;
 *     공식 컨트롤러 데이터시트 원문 열람 실패(D3/미확인).
 *   - 초기화 지연(전원 투입 후 최소 대기) 미구현 — 호출자가 대기해야 함.
 *   - 실제 보드에서 시험하지 않음 (호스트 모의 시험만).
 */
#ifndef PMOD_CLS_H
#define PMOD_CLS_H
#include <stdint.h>
#include <stddef.h>
#include "axi_qspi.h"

#define CLS_COLS   16u
#define CLS_ROWS    2u

#define CLS_OK         0
#define CLS_ERR_ARGS  (-1)
#define CLS_ERR_RANGE (-2)
#define CLS_ERR_SPI   (-3)

/* SPI 초기화는 호출자가 수행; cls_t 는 이미 qspi_init 된 qspi_t 를 참조한다. */
typedef struct {
    qspi_t *spi;
    unsigned cs_idx;  /* qspi_cs() 에 전달할 SS 인덱스 */
} cls_t;

/* 드라이버 초기화 (SPI 는 이미 초기화 완료여야 함) */
int cls_init(cls_t *c, qspi_t *spi, unsigned cs_idx);

/* 화면 지우기 + 커서를 (0,0) 으로 */
int cls_clear(cls_t *c);

/* 커서를 (0,0) 으로 */
int cls_home(cls_t *c);

/* 커서 이동: row 0..1, col 0..15 */
int cls_cursor(cls_t *c, unsigned row, unsigned col);

/* 현재 커서 위치에서 len 바이트 출력 (NUL 종결 문자열: NULL 이면 strlen 사용) */
int cls_write(cls_t *c, const char *text, size_t len);

/* 커서 이동 후 문자열 출력 편의 함수 */
int cls_puts(cls_t *c, unsigned row, unsigned col, const char *text);

/* 화면 지우기 후 최대 2행 출력 (line1 = 행 0, line2 = 행 1; NULL 이면 빈 행) */
int cls_clear_write(cls_t *c, const char *line1, const char *line2);

#endif /* PMOD_CLS_H */
