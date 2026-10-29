# 소켓 위 응용: DNS, NTP 시계 맞추기, HTTP GET

<!-- nav -->[저장소 홈](../../README.md) › [devices](../README.md) › [ethernet](README.md)

교재 30장(웹 클라이언트·NTP)의 유선 대체입니다(이름 풀기 DNS 포함). Wi-Fi/ESP 는 없고, W5500([드라이버](../spi/comm-storage-io.md)) 소켓 위에서 동작합니다. 전부 **호스트 모델 시험까지만** 했고 보드·실제 서버에서 실행한 적이 없습니다.

구분: **확인** = 문서/소스를 열어 확인, **D1(요약 경유)** = 웹 가져오기 요약을 거친 1차 자료, **D3** = 제3자 구현, **설계안**, **미확인**.

| 기능 | 파일 | 시험 |
|---|---|---|
| UDP 수신 `w5500_udp_recvfrom` | [`../spi/sw/w5500.c`](../spi/sw/w5500.c) | [`../spi/tb/test_w5500_model.c`](../spi/tb/test_w5500_model.c) |
| SNTP 패킷·질의 | [`sw/ntp.h`](sw/ntp.h), [`sw/ntp.c`](sw/ntp.c)(순수 함수), [`sw/ntp_client.c`](sw/ntp_client.c) | [`tb/test_ntp.c`](tb/test_ntp.c), 사실표 [`ref/ntp_facts.txt`](ref/ntp_facts.txt) |
| DNS 이름 풀기(A 레코드) | [`sw/dns.h`](sw/dns.h), [`sw/dns.c`](sw/dns.c)(순수 함수), [`sw/dns_client.c`](sw/dns_client.c), 이름으로 NTP·HTTP 를 부르는 [`sw/net_name.h`](sw/net_name.h)/[`.c`](sw/net_name.c) | [`tb/test_dns.c`](tb/test_dns.c), 무작위 [`tb/fuzz_dns.c`](tb/fuzz_dns.c), 사실표 [`ref/dns_facts.txt`](ref/dns_facts.txt) |
| HTTP/1.1 GET | [`sw/http.h`](sw/http.h), [`sw/http.c`](sw/http.c)(요청 만들기·스트리밍 응답 해석), [`sw/http_client.c`](sw/http_client.c) | [`tb/test_http.c`](tb/test_http.c), 무작위 [`tb/fuzz_http.c`](tb/fuzz_http.c) |
| W5500 장치 모델(공용) | — | [`../spi/tb/w5500_model.h`](../spi/tb/w5500_model.h) |

## 1. W5500 UDP 수신 헤더 (막혀 있던 항목)
| 항목 | 내용 | 근거 |
|---|---|---|
| 형식 | RX 버퍼에서 패킷마다 8바이트 헤더 뒤에 데이터: 바이트 0~3 상대 IP, 4~5 상대 포트, 6~7 데이터 길이 | WIZnet 문서 UDP 쪽 — D1(요약 경유). W5100 계열 Arduino `socket.cpp`(`read_data(..., head, 0x08)`, `head[0..3]` IP) — D3 |
| 바이트 순서 | 포트·길이를 상위 바이트 먼저로 조립(`(head[4] << 8) + head[5]`) | W5100 `socket.cpp` — **D3, W5100 코드이며 W5500 문서에는 순서 표기가 없음**. W5500 도 같다는 것은 가정(설계안) |
| `Sn_RX_RD` 전진량 | 헤더 8 + 데이터 길이 | WIZnet 문서 본문은 "길이 + 헤더 크기"라고 쓰는데 **같은 페이지의 예제 코드는 데이터 길이만 더한다** → 서로 어긋남. 드라이버는 본문을 따름(`!val` 로 사실표에 기록). 시험 모델은 "RECV 시 `Sn_RX_RD` 가 패킷 경계가 아니면 위반"으로 강제하지만 이것은 WIZnet 절차를 옮긴 내 규칙이지 칩 동작을 확인한 것이 아님 |
| 받을 수 없는 경우 | 버퍼보다 큰 패킷·IP 단편화는 받을 수 없음 | WIZnet 문서 — D1(요약 경유) |

`w5500_udp_recvfrom(w, n, ip, &port, buf, maxlen)`:
- 완전한 패킷이 없으면 `W5500_ERR_NOPKT`(-66)이고 아무것도 소비하지 않습니다(`Sn_RX_RSR` < 8, 또는 < 8 + 헤더 길이). 데이터가 `maxlen` 보다 길면 `maxlen` 만 복사하고 나머지는 버립니다(UDP 의 관례, 설계안). 길이 0 패킷도 소비하고 0 을 돌려줍니다.
- 헤더의 길이가 RX 버퍼에 들어갈 수 없으면 `W5500_ERR_FRAME`(-67): 칩 상태가 깨진 것이므로 소켓을 닫고 다시 열어야 합니다(설계안, 소비하지 않음).
- `Sn_RX_RD` 를 8 + 길이만큼 올리고 `RECV` 명령을 보냅니다. `Sn_IR.RECV` 는 건드리지 않습니다.
- 헤더가 일부만 도착한 상태에서 칩이 `Sn_RX_RSR` 에 이미 반영하는지는 문서에 없습니다(미확인). 그래서 부분 도착은 NOPKT 로 처리하고 소비하지 않습니다.

## 2. NTP (SNTP 클라이언트)
`ntp_query(w, sock, local_port, server_ip, nonce_sec, nonce_frac, timeout_us, &out)`: UDP 소켓을 열어 48바이트 요청을 서버 포트 123 으로 보내고, 응답을 기다린 뒤 소켓을 닫습니다. `out` 에는 서버의 **전송 시각**이 Unix 초(`int64`)·µs·스트라툼으로 들어갑니다.

| 사실 | 근거 |
|---|---|
| 패킷 48바이트, 필드 순서 LVM·스트라툼·폴·정밀도(각 1)·루트 지연·루트 분산·참조 ID(각 4)·참조/원점/수신/전송 시각(각 8) → 전송 시각은 오프셋 40 | chrony `ntp.h` 구조체, OpenBSD rdate 상수 — D3 |
| 첫 바이트: LI = 7~6 비트, VN = 5~3, 모드 = 2~0. 클라이언트 모드 3, 서버 모드 4, VN 4 → 요청 첫 바이트 0x23 | OpenBSD(`(NTP_VERSION << 3) \| NTP_MODE_CLIENT`, 디코드 시프트), chrony(`NTP_LVM` 매크로) — D3 둘이 일치 |
| 포트 123 | chrony `NTP_PORT` — D3 |
| 1900→1970 오프셋 2208988800, 분수는 2^32 단위 | OpenBSD `JAN_1970`·`NTP_SCALE`, EPICS 헤더 — D3. 70년(윤년 17번)으로 직접 계산해도 같음 |
| 요청의 전송 시각 칸에 임의 값을 넣고 응답의 원점 칸과 비교 | OpenBSD(난수 쿠키) — D3. 이 드라이버는 난수 대신 호출자가 주는 `nonce` |
| 응답 검사: 모드 4, 원점 일치, 스트라툼 1~15, LI ≠ 3, 전송 시각 ≠ 0, 보낸 IP·포트가 서버와 같음 | 원점 일치는 D3. 나머지는 **설계안**(스트라툼 0·LI 3 의 의미는 일반 지식이고 RFC 5905 원문으로 확인하지 못함) |
| 시대(era) | 32비트 초는 2036-02-07 에 한 바퀴 돈다. 값 ≥ 2208988800 이면 1900~2036, 미만이면 다음 시대로 봄 — **설계안**. OpenBSD 는 최상위 비트로 판정하는 다른 방식 |

**한계**: RFC 5905 원문을 이 환경에서 열지 못했습니다(미확인). 왕복 지연 보정·오프셋 계산은 하지 않았습니다(서버 전송 시각을 그대로 돌려줌, 오차 = 편도 지연 + 처리 시간, 보통 수 ms~수십 ms). 응답 패킷은 시험이 직접 만든 것이고 실제 서버 캡처가 아닙니다. 서버 이름(DNS)은 없고 IP 로 직접 지정합니다.

## 3. HTTP GET
`http_get(w, sock, local_port, ip, port, host, path, timeout_us, sink, ctx, &status)`: TCP 로 접속해 `GET <path> HTTP/1.1` / `Host` / `Connection: close` 를 보내고, 응답 본문을 `sink` 콜백으로 조각조각 넘깁니다. 4xx/5xx 도 0 으로 돌려주고 `*status` 로 알립니다. `timeout_us` 는 "데이터가 안 오고 지나간 시간"만 세고 데이터가 오면 다시 0 부터입니다.

| 사실(RFC 9112, datatracker 본문 — **D1(요약 경유)**, 요약이 앞 100000자만 읽었다고 밝힘) | 코드 |
|---|---|
| 요청줄 `method SP request-target SP HTTP-version`, 상태줄 `HTTP-version SP status-code SP [reason-phrase]` | `http_build_get`, 상태줄 해석 |
| HTTP/1.1 요청에는 Host 필수 | 요청에 항상 포함 |
| 줄 끝 CRLF(수신자는 LF 만 와도 받아들일 수 있음), 빈 줄이 헤더의 끝 | LF 단독 허용 |
| 헤더 이름 대소문자 무시, 값 앞뒤 공백은 값이 아님 | 이름 비교, 값 다듬기 |
| 본문 길이: HEAD·1xx·204·304 는 본문 없음 / chunked 면 청크 복호 / Content-Length 십진수 / 둘 다 없으면 서버가 닫을 때까지 | 해석기 상태기계 |
| `close` = 보낸 쪽이 응답 후 연결을 닫음 | `Connection: close` 를 보냄 |
| 청크 크기는 16진수 1자리 이상, 마지막 청크는 0 | 청크 해석 |

**설계안(문서로 확인하지 못한 선택)**: `Transfer-Encoding` 과 `Content-Length` 가 같이 오면 chunked 우선 / `Content-Length` 가 서로 다르면 오류 / 줄 이어쓰기·CR 단독은 오류 / 1xx 는 건너뛰고 다음 응답 / 마지막 코딩이 chunked 가 아닌 `Transfer-Encoding` 은 "닫을 때까지" / 청크 확장·트레일러 무시 / 헤더 한 줄 95자까지만 보관(`Content-Length`·`Transfer-Encoding` 줄이 잘리면 오류) / 청크 크기 8자리까지.
호스트·경로에 공백·제어문자·비ASCII 가 있으면 요청을 만들지 않습니다(요청 분할·헤더 주입 방지). 경로의 퍼센트 인코딩은 호출자 몫입니다.

**하지 않는 것**: HTTPS(TLS), 리다이렉트 따라가기, `Content-Encoding`(gzip) 복호, 연결 유지, 요청 본문, 날씨 API 같은 JSON 해석(교재 30장 날씨 예제는 이 위에 따로 얹어야 함). 이름으로 접속하려면 아래 4절의 `http_get_name` 을 쓴다.

## 4. DNS (이름 → IPv4)
`dns_resolve(w, sock, local_port, dns_server_ip, name, id, timeout_us, tries, ip, &ttl)`: UDP 53 으로 A 레코드를 묻고 첫 A 의 주소와 TTL 을 돌려줍니다. `tries` 번까지 다시 보내고(시도마다 `timeout_us`), 다른 ID·다른 보낸 쪽 IP/포트·질문이 다른 응답·깨진 응답은 무시하고 계속 기다립니다. 서버가 정상 응답으로 NXDOMAIN·SERVFAIL·기타 RCODE·A 없음·잘림(TC)을 알리면 바로 그 오류를 돌려줍니다. `ntp_query_name`, `http_get_name`([`net_name.h`](sw/net_name.h))은 이름이 점 십진 IPv4 면 DNS 를 건너뛰고, 아니면 먼저 `dns_resolve` 를 합니다(`http_get_name` 은 포트가 80 이 아니면 Host 헤더에 `:포트` 를 붙임).

| 사실 | 근거 |
|---|---|
| 헤더 12바이트(id, flags, qdcount, ancount, nscount, arcount 각 16비트 빅엔디안), QR 0x8000 · TC 0x0200 · RD 0x0100, RCODE 0~5(2 = SERVFAIL, 3 = NXDOMAIN), TYPE A = 1 · CNAME = 5, CLASS IN = 1, 포트 53, UDP 512바이트, 이름 255바이트 이하 | Chromium `dns_protocol.h` — D3(요약 경유). 포트·헤더 구조·타입·압축 매크로는 iPXE `dns.h` — D3 와 일치 |
| 이름 = (길이 바이트 + 글자들)의 반복, 0 으로 끝. 레이블 최대 63(상위 2비트 00). 상위 2비트 11 = 압축 포인터(나머지 14비트 = 메시지 시작부터의 오프셋), 포인터는 이름을 끝냄. flags 안의 QR·OPCODE(4비트)·AA·TC·RD·RA 위치 | zytrax DNS 해설 + Chromium(`kLabelPointer`, `kOffsetMask`) + iPXE(`DNS_COMPRESSED_OFFSET`) — D3 |

**설계안(문서로 확인하지 못한 선택)**: 이름 총길이 255 에 끝의 0 바이트까지 포함 / 이름 글자는 영문자·숫자·`-`·`_` 만 / 응답의 질문이 보낸 질문(이름 대소문자 무시, A, IN)과 다르면 거부 / 답변을 순서대로 한 번 훑으며 CNAME 은 최대 8번 따라감(A 가 CNAME 보다 앞서 나오는 응답은 못 찾음) / A 가 여럿이면 첫 번째 / A 의 RDLENGTH ≠ 4 는 형식 오류 / TC 가 켜져 있어도 쓸 수 있는 A 가 있으면 사용 / 포인터는 최대 16번 따라감(무한 루프 방지, 메시지 밖을 가리키면 오류).
**미확인**: RFC 1035 원문 대조, RA 비트와 RCODE 폭(일반 지식으로 하위 4비트), 실제 DNS 서버(공유기·공용 DNS)와의 동작, DNS 서버 IP 를 어디서 얻는지(`dns_resolve` 는 호출자가 준 주소를 쓰고, 자동으로 받으려면 5절의 DHCP).
**보안 한계**: 16비트 ID 와 보낸 쪽 IP·포트 검사만으로는 위조 응답을 막기 어렵습니다(로컬 포트를 호출자가 정함, 무작위화는 하지 않음). DNSSEC 없음.
**하지 않는 것**: AAAA(IPv6), TCP 폴백, EDNS, 캐시, mDNS.

## 5. DHCP (IP·마스크·게이트웨이·DNS 서버 자동 받기)
`dhcp_acquire(w, sock, mac, xid, timeout_us, tries, &lease)`: 소켓을 UDP 68 로 열고 DISCOVER → OFFER → REQUEST → ACK 를 진행해 IP·서브넷 마스크·라우터·DNS 서버·임대 시간(T1/T2 포함)을 `dhcp_lease_t` 로 돌려주고 소켓을 닫습니다. 각 단계에서 `tries` 번까지 보내고(시도마다 `timeout_us`), xid·MAC·서버 포트(67)가 맞지 않거나 형식이 깨진 메시지, 기다리는 종류가 아닌 메시지(OFFER 를 기다리는 중의 ACK 등), REQUEST 를 보낸 서버가 아닌 곳의 응답은 무시합니다. NAK 은 바로 `DHCP_ERR_NAK`, ACK 의 주소가 OFFER 와 다르면 `DHCP_ERR_MISMATCH`, 마스크가 없으면 `DHCP_ERR_NOMASK`. 호출자는 W5500 을 IP 0.0.0.0 으로 `w5500_init` 한 뒤 부르고, 성공하면 `dhcp_apply(w, &lease)` 가 GAR·SUBR·SIPR 을 쓰고 읽어 확인합니다(라우터가 없으면 GAR 0.0.0.0). 그 뒤 `lease.dns` 를 `dns_resolve` 의 서버로 쓰면 됩니다. 순수 함수는 `dhcp_build_discover/request`, `dhcp_parse`, `dhcp_times`(`dhcp.c`), W5500 위의 거래는 `dhcp_client.c`.

| 사실 | 근거 |
|---|---|
| 메시지 배치: op · htype · hlen · hops(각 1), xid 4, secs 2, flags 2, ciaddr · yiaddr · siaddr · giaddr(각 4), chaddr 16, sname 64, file 128, 쿠키 4, 옵션 → chaddr 오프셋 28, 쿠키 236, 옵션 240 | dhcpcd `dhcp.h`(Chromium OS 사본) — D3(요약 경유). 오프셋은 필드 크기 합(계산) |
| 쿠키 0x63825363, BROADCAST 플래그 0x8000, 포트 서버 67 / 클라이언트 68, op 1 = BOOTREQUEST · 2 = BOOTREPLY, 메시지 종류 DISCOVER 1 · OFFER 2 · REQUEST 3 · ACK 5 · NAK 6 | 같음 — D3 |
| 옵션 번호: 0 패드 · 1 마스크 · 3 라우터 · 6 DNS · 50 요청 IP · 51 임대 · 53 종류 · 54 서버 ID · 55 요청 목록 · 58 T1 · 59 T2 · 255 끝 | 같음 — D3 |
| 목적 IP 255.255.255.255 로 보내면 ARP 없이 브로드캐스트되고 소켓이 브로드캐스트를 받음 | WIZnet 문서 W5500 UDP 쪽 — D1(요약 경유). 그 문서는 SIPR 이 0.0.0.0 일 때의 송신과 `Sn_MR` 의 브로드캐스트 차단 비트(BCASTB)는 다루지 않음 |

**설계안(문서로 확인하지 못한 선택)**: 요청을 300바이트로 채움(BOOTP 최소 크기 관례) / 한 거래 동안 xid 고정 / 요청에 BROADCAST 플래그를 켬(주소 받기 전에는 유니캐스트 응답을 못 받는다고 가정) / T1 = 임대/2, T2 = 임대의 7/8 을 기본값으로 쓰고 서버가 준 값이 T1 < T2 < 임대가 아니면 둘 다 기본값(임대 0xFFFFFFFF = 무기한) / 클라이언트 식별자(61)는 안 쓰고 chaddr 만 사용 / 옵션 52(sname·file 겹침)는 읽지 않음 / 같은 옵션이 반복되면 첫 값 / OFFER·ACK 에는 서버 ID 와 임대 시간이 있어야 하고 주소가 0·127·224 이상이면 거부, 마스크는 연속 1 이 아니면 거부(NAK 은 그런 필드가 없어도 받음, 서버 ID 없는 NAK 도 xid·chaddr 가 맞으면 받음) / 먼저 온 OFFER 를 선택 / ACK 에 서버 ID 가 있고 OFFER 의 서버와 다르면 무시.
**미확인**: RFC 2131·2132 원문 대조(타이머, 요청 크기, BROADCAST 플래그가 필요한지, 최소 필수 옵션), **W5500 이 SIPR=0.0.0.0 상태에서 255.255.255.255 로 송신·수신하는지**와 `Sn_MR` 브로드캐스트 플래그 설정(이 모델은 그 규칙을 내가 가정한 것이라 시험이 아무것도 증명하지 못함), 실제 DHCP 서버·공유기와의 동작, FRAME 오류 뒤 소켓을 닫고 다시 열 때 칩이 수신 버퍼를 비우는지.
**보안 한계**: xid 와 chaddr 검사만으로는 같은 망의 위조 서버를 막을 수 없습니다(DHCP 자체의 한계). xid 는 호출자가 정하며 무작위화하지 않음.
**하지 않는 것**: 임대 갱신(T1/T2 에 호출자가 `dhcp_acquire` 를 다시 부르면 됨), DECLINE·RELEASE, ARP 충돌 검사, INIT-REBOOT, 릴레이, 호스트 이름·클라이언트 식별자, 다중 NIC.

## 시험
- W5500 UDP 수신: 모델에 패킷 경계 규칙·피어 훅·틱 훅을 추가하고 `t_udp_recv` 28항목(300바이트 패킷으로 길이 상·하위 바이트 구분, 연속 패킷, 잘림, 길이 0, 부분 도착, 블록 경계·16비트 올림, 8 + 2040 / 2041 경계, 잘못된 길이, 인자·상태). 변형 23/23 잡힘.
- NTP: 순수 함수(손으로 계산한 값 — 2024-01-01 = 3913056000, 시대 경계, 비트 경계 0xDB·0x1C) + 모델 위 질의(정상, 무응답, 원점·IP·포트·스트라툼 0·LI 3·모드·전송 시각 0 이 틀린 응답 무시, 남의 패킷 뒤의 정상 응답, 짧은 패킷, 68바이트 응답, 시대 1, 송신 실패, 사용 중 소켓). 변형 60/60 잡힘.
- HTTP: 요청 만들기, 응답 해석 75건(각각 통째·1바이트씩·3바이트씩 넣어 같은 결과인지 3번씩), W5500 위 `http_get`(Content-Length·chunked·닫을 때까지, 데이터와 FIN 동시, 상대 RST, 5000바이트 본문, 중간 끊김, 무응답, 형식 오류, sink 중단, 접속 거부, 송신 실패, 인자). 무작위 15만 건(파서가 쪼개는 방식에 따라 결과가 달라지는지, ASan/UBSan). 변형 85/85 잡힘(동치 변형은 목록에서 제외).
- DNS: 질의 바이트 손계산(29바이트), 이름 규칙 경계(레이블 63/64, 이름 255/256, 버퍼 29/28), IPv4 문자열 18개 거절, 응답 해석 50여 항목(압축 포인터, CNAME 1·2·8·9번과 순환, RCODE 1~5, TC, 다른 CLASS·TYPE·소유자 이름 건너뛰기, 자기 자신/서로 가리키는/범위 밖 포인터, 예약 레이블, 경계 초과), 모델 위 `dns_resolve`(재시도, 다른 ID·IP·포트·질문 무시, 쓰레기 뒤 정상 응답, NXDOMAIN 등 즉시 반환, 송신 실패, 사용 중 소켓), 이름으로 NTP·HTTP. 무작위 40만 건(ASan/UBSan, 성공이면 IP 가 메시지 안에 실제로 있는지). 변형 109/109 잡힘(동치 변형은 목록에서 제외).
- DHCP: 요청 바이트를 손으로 계산한 오프셋과 비교(300바이트, 쿠키 236, REQUEST 의 옵션 50·54), 응답 해석 80여 항목(op·htype·hlen·쿠키·xid·chaddr 각 비트 경계, 부적합 주소 경계 0/127/224, 마스크 연속성, 필수 옵션, 옵션 경계·PAD·END 뒤 쓰레기·길이 오류·반복 옵션, 잘림 훑기를 정확한 힙 버퍼로), T1/T2 기본값 규칙, 모델 위 `dhcp_acquire`(정상 거래와 송신 2번의 대상·길이·종류, 재시도 횟수와 대기 시간이 정확히 제한, xid·포트가 틀린 응답·쓰레기·종류 32 이상·OFFER 기다리는 중의 ACK·두 서버의 OFFER·다른 서버의 NAK 무시, NAK·MISMATCH·NOMASK·FRAME·무기한 임대·라우터/DNS 없음, 송신 실패, 사용 중 소켓)와 `dhcp_apply`(레지스터 위치, 쓰기가 반영 안 되면 VERIFY). 무작위 40만 건. 변형 101개: 일반 빌드에서 100개, ASan+UBSan 빌드에서 101개 잡힘(한 바이트 넘어 읽는 변형은 ASan 에서만).
- 한계: 상대 서버·패킷이 전부 내가 만든 것이라, 규격 해석이 틀렸으면 시험도 같이 틀립니다. 실제 서버와의 상호 동작은 확인하지 못했습니다.

## 미확인
DNS 항목은 4절, DHCP 항목은 5절. W5500 UDP 헤더의 바이트 순서(W5500 자체 확인), `Sn_RX_RD` 전진량의 칩 동작, 부분 도착 시 `Sn_RX_RSR` 반영 시점, RFC 5905·9112 원문 대조, 실제 NTP 서버·HTTP 서버와의 통신, 모듈의 PHY/RJ45 구성.

<!-- nav-foot -->[ethernet 목차](README.md)
