# tb

<!-- nav -->[저장소 홈](../../../README.md) › [devices](../../README.md) › [can](../README.md)

호스트 테스트와 레지스터/장치 모델(mock)

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [exhaustive_timing.c](exhaustive_timing.c) | 적대적 시험: 클럭·비트레이트 조합을 넓게 훑어 can_calc_timing 결과가 항상 (1) 정확한 비트레이트를 만들고 (2) 필드 범위를 지키는지 검증한다. |
| [mock_can.h](mock_can.h) | endif |
| [test_can_mock.c](test_can_mock.c) | 모의: 루프백처럼 TX FIFO 4워드를 RX FIFO 큐로 넘긴다. RXNEMP 는 큐가 비어 있지 않으면 ICR 후에도 다시 세트된다고 가정(미확인 가정) |

<!-- index:end -->
