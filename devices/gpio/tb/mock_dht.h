#ifndef MOCK_DHT_H
#define MOCK_DHT_H
#include <stdint.h>
uint32_t mock_dht_rd(uint32_t off);
void mock_dht_wr(uint32_t off, uint32_t v);
#define DHT_RD(base, off)    ((void)(base), mock_dht_rd((uint32_t)(off)))
#define DHT_WR(base, off, v) ((void)(base), mock_dht_wr((uint32_t)(off), (uint32_t)(v)))
#endif
