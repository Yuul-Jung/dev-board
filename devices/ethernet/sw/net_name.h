/* 이름(호스트 이름)으로 NTP·HTTP 를 부르는 얇은 함수. 점 십진 IPv4 문자열이면 DNS 를 건너뛰고 그대로 씀. DNS 소켓과 응용 소켓은 같은 W5500 소켓 번호를 차례로 쓴다(DNS 가 끝나 닫힌 뒤 응용이 연다). */
#ifndef NET_NAME_H
#define NET_NAME_H
#include "dns.h"
#include "ntp.h"
#include "http.h"

#define NET_DNS_TRIES 2u

int ntp_query_name(w5500_t *w, unsigned sock, uint16_t local_port, const uint8_t dns_server[4], uint16_t dns_id, const char *server_name,
                   uint32_t nonce_sec, uint32_t nonce_frac, uint32_t timeout_us, ntp_time_t *out);
/* Host 헤더에는 host 를, port 가 80 이 아니면 "host:port" 를 쓴다. */
int http_get_name(w5500_t *w, unsigned sock, uint16_t local_port, const uint8_t dns_server[4], uint16_t dns_id, uint16_t port, const char *host, const char *path,
                  uint32_t timeout_us, http_sink_fn sink, void *ctx, int *status);
#endif
