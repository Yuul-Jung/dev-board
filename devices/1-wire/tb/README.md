# tb

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [1-wire](../README.md)

호스트 테스트와 레지스터/장치 모델(mock)

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [mock_ow.h](mock_ow.h) | endif |
| [tb_onewire_engine.v](tb_onewire_engine.v) | 엔진 단독 테스트. 소자 모델이 리셋에 응답하고, 읽기 슬롯에서 패턴 1,0,1,1 을 돌려준다. |
| [test_ds18b20.c](test_ds18b20.c) | DS18B20 드라이버 호스트 시험. 모의 소자는 1-Wire 마스터 IP 의 레지스터(rtl/README.md)와 DS18B20 슬레이브를 비트 단위로 흉내 낸 |

<!-- index:end -->
