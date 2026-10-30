# sw

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [ethernet](../README.md)

드라이버 C 소스

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [dhcp.c](dhcp.c) |  |
| [dhcp.h](dhcp.h) | DHCP 클라이언트(IPv4, 처음 주소 받기). 메시지 만들기·해석은 입출력 없는 순수 함수(dhcp.c), W5500 위의 DISCOVER→OFFER→REQU |
| [dhcp_client.c](dhcp_client.c) | 허용한 종류(want_mask 의 비트 = 종류 번호)의 우리 응답이 올 때까지 기다린다. 0: 받음, TIMEOUT, 또는 하위 오류 |
| [dns.c](dns.c) | 이름을 검사하고 소문자 점 표기(끝 점 없음)로 out 에 담는다. 반환: 0 또는 DNS_ERR_NAME |
| [dns.h](dns.h) | DNS 클라이언트(UDP 53, A 레코드만). 질의 만들기·응답 해석은 입출력 없는 순수 함수(dns.c), W5500 위의 질의는 dns_client.c, 이 |
| [dns_client.c](dns_client.c) |  |
| [http.c](http.c) |  |
| [http.h](http.h) | HTTP/1.1 GET 클라이언트(응답은 스트리밍 해석). 요청 만들기·응답 해석은 입출력 없는 순수 함수(http.c), W5500 TCP 위의 http_get |
| [http_client.c](http_client.c) | status = 0; |
| [json.c](json.c) |  |
| [json.h](json.h) | JSON 해석기(RFC 8259), 할당 없음·재귀 깊이 JSON_MAX_DEPTH. 입력은 메모리에 통째로 있는 텍스트(예: http_get 의 본문을 모은 버 |
| [net_name.c](net_name.c) |  |
| [net_name.h](net_name.h) | 이름(호스트 이름)으로 NTP·HTTP 를 부르는 얇은 함수. 점 십진 IPv4 문자열이면 DNS 를 건너뛰고 그대로 씀. DNS 소켓과 응용 소켓은 같은 W55 |
| [ntp.c](ntp.c) |  |
| [ntp.h](ntp.h) | SNTP 클라이언트(UDP 123). 패킷 만들기·해석은 입출력이 없는 순수 함수(ntp.c), W5500 위의 질의는 ntp_client.c. |
| [ntp_client.c](ntp_client.c) |  |

<!-- index:end -->
