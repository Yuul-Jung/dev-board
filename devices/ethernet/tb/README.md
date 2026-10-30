# tb

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [ethernet](../README.md)

호스트 테스트와 레지스터/장치 모델(mock)

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [fuzz_dhcp.c](fuzz_dhcp.c) | DHCP 응답 해석기 무작위 시험(고정 시드): 유효한 OFFER 를 무작위로 망가뜨리거나 자르거나 이어 붙여도 |
| [fuzz_dns.c](fuzz_dns.c) | DNS 응답 해석기 무작위 시험(고정 시드): 유효한 응답을 무작위로 망가뜨리거나 자르거나 이어 붙여도 |
| [fuzz_http.c](fuzz_http.c) | HTTP 응답 해석기 무작위 시험: 유효한 응답을 무작위로 망가뜨려 (1) 어떤 입력에도 ASan/UBSan 오류가 없고 |
| [fuzz_json.c](fuzz_json.c) | JSON 해석기 무작위 시험(고정 시드): 유효한 문서를 무작위로 망가뜨리거나 자르거나 토큰을 끼워 넣어도 |
| [json_diff.py](json_diff.py) | fuzz_json dump 의 출력(줄마다 "코드 16진수문서")을 파이썬 json 모듈의 판정과 비교한다. 사용: ./fuzz_json dump / python |
| [test_dhcp.c](test_dhcp.c) | DHCP 시험: 요청 바이트(손으로 계산한 오프셋), 응답 해석(옵션 경계·중복·오류), 시간 기본값, W5500 모델 위 dhcp_acquire·dhcp_app |
| [test_dns.c](test_dns.c) | DNS 클라이언트 시험: 질의 바이트(손으로 계산), 응답 해석(압축 포인터·CNAME·오류 코드·악성 포인터), IPv4 문자열, W5500 모델 위 dns_r |
| [test_http.c](test_http.c) | HTTP 클라이언트 시험: 요청 만들기, 응답 해석(통째로·1바이트씩·3바이트씩 넣어 같은 결과인지), W5500 모델 위의 http_get(상대 서버는 이 파일 |
| [test_json.c](test_json.c) | JSON 해석기 시험: 문법 경계(RFC 8259), 깊이, 숫자 변환(손계산), 문자열 풀기(\u·서러게이트), 경로 조회, 잘림 훑기(정확한 힙 버퍼), W5 |
| [test_ntp.c](test_ntp.c) | SNTP 시험: 순수 패킷 함수(손으로 계산한 값) + W5500 모델 위의 질의(상대 서버 응답은 이 파일의 훅이 만든다). |

<!-- index:end -->
