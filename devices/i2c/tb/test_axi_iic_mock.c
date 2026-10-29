/* AXI IIC 드라이버 호스트 테스트. PG090 의 예제 워드열과 비교한다.
 * 빌드: gcc -Wall -I sw -include tb/mock_iic.h tb/test_axi_iic_mock.c sw/axi_iic.c sw/i2c_sensors.c -o t && ./t
 * 모의 하드웨어는 FIFO 를 즉시 처리하는 단순 모델이며 실제 타이밍은 검증하지 않는다. */
#include <stdio.h>
#include <string.h>
#include "axi_iic.h"
#include "i2c_sensors.h"

#define MAXW 64
static uint32_t txlog[MAXW]; static int ntx;
static uint32_t crlog[MAXW]; static int ncr;
static uint8_t  rxq[256]; static int rxh, rxt;
static uint32_t isr; static int bb;
static enum { IDLE, WRITE, COUNT, DROP } st;
static uint8_t cur_addr, nack_addr = 0xFF;
static uint8_t script[256]; static int nscript, sp;   /* 읽기 응답 */

void iic_delay_ms(unsigned ms) { (void)ms; }

uint32_t mock_rd(uint32_t off)
{
    switch (off) {
    case IIC_SR: return SR_TX_EMPTY | (rxh == rxt ? SR_RX_EMPTY : 0) | (bb ? SR_BB : 0);
    case IIC_ISR: return isr;
    case IIC_RX_FIFO: return rxq[rxh++];
    default: return 0;
    }
}

void mock_wr(uint32_t off, uint32_t v)
{
    if (off == IIC_CR) { crlog[ncr++] = v; return; }
    if (off == IIC_ISR) { isr &= ~v; return; }
    if (off != IIC_TX_FIFO) return;
    if (v & TXF_START) {
        cur_addr = (v >> 1) & 0x7F; bb = 1;
        if (cur_addr == nack_addr) { isr |= ISR_TXERR; bb = 0; st = DROP; return; }
        txlog[ntx++] = v;
        st = (v & 1) ? COUNT : WRITE;
        return;
    }
    if (st == DROP) return;
    txlog[ntx++] = v;
    if (st == COUNT) {
        for (unsigned i = 0; i < (v & 0xFF); i++) rxq[rxt++] = script[sp++];
        isr |= ISR_TXERR;                              /* 수신 완료 표시 */
        st = WRITE;
    }
    if (v & TXF_STOP) { bb = 0; st = IDLE; }
}

static void reset_model(void)
{
    ntx = ncr = 0; rxh = rxt = 0; isr = 0; bb = 0; st = IDLE; nack_addr = 0xFF; sp = 0; nscript = 0;
}
static void feed(const uint8_t *b, int n) { memcpy(script + nscript, b, n); nscript += n; }

static int errors;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); errors++; } } while (0)

static void expect_log(const uint32_t *e, int n, const char *name)
{
    CHECK(ntx == n, "%s: 워드 수 %d != %d", name, ntx, n);
    for (int i = 0; i < n && i < ntx; i++)
        CHECK(txlog[i] == e[i], "%s[%d]: %03x != %03x", name, i, txlog[i], e[i]);
}

int main(void)
{
    axi_iic_t d; axi_iic_init(&d, 0); d.timeout = 1000;
    CHECK(crlog[0] == CR_TX_FIFO_RST && crlog[1] == CR_EN, "init CR 순서");

    /* PG090 예제의 "주소 0x34" 는 R/W 비트 자리를 포함한 8비트 주소 바이트이므로 7비트 주소는 0x1A */
    /* 쓰기: 0x134, 0x33, 0x89, 0xAB, 0xCD, 0x2EF */
    reset_model();
    { uint8_t w[] = {0x33, 0x89, 0xAB, 0xCD, 0xEF};
      CHECK(axi_iic_write(&d, 0x1A, w, 5) == IIC_OK, "write");
      uint32_t e[] = {0x134, 0x33, 0x89, 0xAB, 0xCD, 0x2EF}; expect_log(e, 6, "pg090-write"); }

    /* PG090 예제: 주소 0x34 에서 4바이트 읽기 → 0x135, 0x204 */
    reset_model();
    { uint8_t in[] = {1, 2, 3, 4}, out[4]; feed(in, 4);
      CHECK(axi_iic_read(&d, 0x1A, out, 4) == IIC_OK && !memcmp(in, out, 4), "read");
      uint32_t e[] = {0x135, 0x204}; expect_log(e, 2, "pg090-read"); }

    /* 반복 시작: 0x68 레지스터 0x75 읽기 */
    reset_model();
    { uint8_t reg = 0x75, id, in = 0x68; feed(&in, 1);
      CHECK(axi_iic_write_read(&d, 0x68, &reg, 1, &id, 1) == IIC_OK && id == 0x68, "wr");
      uint32_t e[] = {0x1D0, 0x75, 0x1D1, 0x201}; expect_log(e, 4, "write-read"); }

    /* NACK */
    reset_model(); nack_addr = 0x50; ncr = 0;
    { uint8_t w = 0; CHECK(axi_iic_write(&d, 0x50, &w, 1) == IIC_ERR_NACK, "nack");
      CHECK(ncr == 2 && crlog[0] == (CR_EN | CR_TX_FIFO_RST) && crlog[1] == CR_EN, "nack 복구 CR");
      CHECK(isr == 0, "nack ISR 지움"); }

    /* 인자 검사 */
    reset_model();
    { uint8_t b; CHECK(axi_iic_read(&d, 0x1A, &b, 0) == IIC_ERR_ARGS, "len 0");
      CHECK(axi_iic_read(&d, 0x1A, &b, 256) == IIC_ERR_ARGS, "len 256"); }

    /* 버스 사용 중이면 거부 */
    reset_model(); bb = 1;
    { uint8_t w = 0; CHECK(axi_iic_write(&d, 0x1A, &w, 1) == IIC_ERR_BUSY, "busy"); }

    /* MPU-6050 초기화: WHO_AM_I 읽기 → sleep 해제 쓰기 */
    reset_model();
    { uint8_t in = 0x68; feed(&in, 1);
      CHECK(mpu6050_init(&d, 0x69) == IIC_OK, "mpu init");
      uint32_t e[] = {0x1D2, 0x75, 0x1D3, 0x201, 0x1D2, 0x6B, 0x200}; expect_log(e, 7, "mpu-init"); }
    CHECK(mpu6050_temp_mc(-521) == 35000, "mpu 온도 변환");
    CHECK(mpu6050_acc_mg(16384) == 1000, "1 g");
    CHECK(mpu6050_gyro_dps10(131) == 100, "1 deg/s");

    /* SHT31: CRC 검사값 CRC(0xBEEF)=0x92 (데이터시트), 변환 */
    { uint8_t t[] = {0xBE, 0xEF}; CHECK(sht3x_crc8(t, 2) == 0x92, "crc 0xBEEF"); }
    reset_model();
    { uint8_t th[2] = {0x66, 0x66}, rh[2] = {0x80, 0x00}, in[6];
      memcpy(in, th, 2); in[2] = sht3x_crc8(th, 2); memcpy(in + 3, rh, 2); in[5] = sht3x_crc8(rh, 2);
      feed(in, 6);
      int32_t t, h; CHECK(sht31_read(&d, 0x44, &t, &h) == IIC_OK, "sht31");
      CHECK(t >= 24990 && t <= 25000, "T=%d", (int)t);
      CHECK(h >= 50000 && h <= 50001, "RH=%d", (int)h);
      uint32_t e[] = {0x188, 0x24, 0x200, 0x189, 0x206}; expect_log(e, 5, "sht31"); }
    /* CRC 오류 */
    reset_model();
    { uint8_t in[6] = {0x66, 0x66, 0x00, 0x80, 0x00, 0x00}; feed(in, 6);
      int32_t t, h; CHECK(sht31_read(&d, 0x44, &t, &h) == -11, "crc 오류 감지"); }

    /* BH1750: raw 0x0120 = 288 → 240 lx */
    reset_model();
    { uint8_t in[2] = {0x01, 0x20}; feed(in, 2);
      uint32_t lux;
      CHECK(bh1750_start(&d, 0x23) == IIC_OK, "bh start");
      CHECK(bh1750_read_lux10(&d, 0x23, &lux) == IIC_OK && lux == 2400, "lux x10=%u", (unsigned)lux);
      uint32_t e[] = {0x146, 0x201, 0x146, 0x210, 0x147, 0x202}; expect_log(e, 6, "bh1750"); }

    printf(errors ? "FAILED %d\n" : "ALL PASS\n", errors);
    return errors != 0;
}
