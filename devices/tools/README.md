# tools

<!-- nav -->[저장소 홈](../../README.md) › [devices](../README.md)

검사·생성 스크립트

## 이 폴더의 목록

<!-- index:begin -->
<!-- 이 목록은 devices/tools/gen_index.py 가 만든다. 직접 고치지 말 것. -->

### 파일

| 파일 | 설명 |
|---|---|
| [check_links.py](check_links.py) | 저장소 안 모든 .md 의 상대 링크가 실제 파일/폴더를 가리키는지 검사한다. 사용: python3 devices/tools/check_links.py |
| [crosscheck_pdf.py](crosscheck_pdf.py) | 코드 헤더에 적은 레지스터 오프셋·비트 이름을 AMD PG 문서 텍스트(pdftotext -layout 결과)와 자동 대조한다. |
| [gen_index.py](gen_index.py) | 폴더별 README.md 에 자동 목록(하위 폴더·파일)을 넣는다. 없는 README 는 만든다. |
| [gen_pages.py](gen_pages.py) | 폴더마다 index.html (GitHub Pages 용 정적 페이지)을 만든다. README.md·데이터에서 읽기만 하고 .md 는 건드리지 않는다. |
| [gen_status.py](gen_status.py) | data/module-status.csv 와 data/chapter-status.csv 로 STATUS.md 의 표를 만든다. 사용: python3 tools/g |

<!-- index:end -->
