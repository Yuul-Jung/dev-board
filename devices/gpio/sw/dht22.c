/* MicroBlaze V 용 DHT22 드라이버 예시. 컴파일과 보드 시험은 아직 하지 않았다. */
#include <stdint.h>

#define DHT_BASE  0x44A20000u   /* Vivado 주소 편집기에서 정한 값으로 바꿀 것 */
#define DHT_CMD   (*(volatile uint32_t *)(DHT_BASE + 0x00))
#define DHT_STAT  (*(volatile uint32_t *)(DHT_BASE + 0x04))
#define DHT_RH    (*(volatile uint32_t *)(DHT_BASE + 0x08))
#define DHT_TEMP  (*(volatile uint32_t *)(DHT_BASE + 0x0C))

#define ST_BUSY  (1u << 0)
#define ST_DONE  (1u << 1)
#define ST_ERR   (1u << 2)
#define ST_CSUM  (1u << 3)

/* 성공하면 0. 읽기 간격은 2초보다 길게 두는 것은 호출하는 쪽 책임이다(데이터시트: 수집 주기 2초 초과). */
int dht22_read(uint16_t *rh_raw, uint16_t *temp_raw) {
    while (DHT_STAT & ST_BUSY) { }
    DHT_CMD = 1;
    while (!(DHT_STAT & ST_DONE)) { }
    uint32_t s = DHT_STAT;
    if (s & ST_ERR)     return -1;   /* 응답 없음 또는 시간 초과 */
    if (!(s & ST_CSUM)) return -2;   /* 체크섬 불일치 */
    *rh_raw = (uint16_t)DHT_RH;      /* 0.1 %RH 단위(분해능 0.1 %RH) */
    *temp_raw = (uint16_t)DHT_TEMP;  /* 0.1 °C 단위. 음수 표현 방식은 데이터시트 미확인 */
    return 0;
}
