# RTL Directed Test 검증

- 대상: `rtl/rtl01_...`~`rtl/rtl32_...`의 기존 테스트벤치.
- 도구: Icarus Verilog의 `iverilog` 컴파일 및 `vvp` 실행.
- 실행 스크립트: [run.sh](run.sh).
- 결과 파일: `results/rtl-summary.tsv` (CI에서 생성·업로드, 저장소에는 템플릿만 등록).
- 판정: 컴파일과 실행 모두 정상 종료한 TB만 PASS, 하나라도 실패하면 FAIL.
- 주의: TB의 자체 검사 범위 밖의 동작은 PASS로 인정되지 않는다.

GitHub Actions CI는 이 스크립트를 실행하며 결과를 Artifact로 보존한다.
