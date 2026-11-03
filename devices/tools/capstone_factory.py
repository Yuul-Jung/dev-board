#!/usr/bin/env python3
"""Generate one NEW, distinct Arduino/Basys3 capstone on each run.

Offline first-party design briefs: no third-party code copied, no fabricated
simulation, synthesis, timing, or board evidence. The next stage can implement
real RTL and testbenches. This tool never edits existing capstone documents.
"""
import html
import itertools
import re
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

# Original teaching scenarios; these are independent capstone projects, not
# claims that the corresponding hardware integration already exists.
IDEAS = [
    ("smart-parking-gate", "차량 접근형 스마트 주차장 차단기", "HC-SR04 초음파", "../gpio/hc-sr04.md", "수동 승인 버튼", "../gpio/basic-io.md", "SG90 서보", "../motor/sg90.md", "유효한 거리 측정과 승인 이벤트가 모두 발생해야 게이트를 열고, 차량 이탈 또는 타임아웃 시 닫는다."),
    ("night-hallway", "움직임·조도 융합 야간 복도 조명", "HC-SR501 PIR", "../gpio/hc-sr501.md", "BH1750 조도", "../i2c/sensors.md", "LED PWM", "../gpio/basic-io.md", "어두운 시간에 움직임이 검출되면 조명을 켜고, 재검출 시 유지시간을 연장한다."),
    ("warehouse-door", "출입 움직임·터치 확인 창고 경보", "HC-SR501 PIR", "../gpio/hc-sr501.md", "TTP223 터치", "../gpio/ttp223.md", "능동 부저", "../gpio/buzzer.md", "움직임 이벤트를 래치하고 터치 확인으로만 경보를 해제한다."),
    ("fan-comfort", "온습도·엔코더 기반 쾌적 환기", "DHT22 온습도", "../gpio/dht22.md", "로터리 엔코더", "../gpio/rotary-encoder.md", "PWM 팬", "../gpio/fan-pwm.md", "온도 히스테리시스로 팬을 제어하고 엔코더로 사용자 설정값을 조정한다."),
    ("machine-guard", "기계 회전수·온도 이상 경보", "로터리 엔코더", "../gpio/rotary-encoder.md", "DHT22 온도", "../gpio/dht22.md", "능동 부저", "../gpio/buzzer.md", "회전 펄스 누락과 과온 조건을 독립적으로 검사하고 오류를 래치한다."),
    ("touch-sorter", "터치 승인형 자동 분류 게이트", "TTP223 터치", "../gpio/ttp223.md", "HC-SR04 거리", "../gpio/hc-sr04.md", "SG90 서보", "../motor/sg90.md", "물체가 감지되고 사용자 승인이 있을 때만 서보를 움직이며 동작 중 재명령을 차단한다."),
    ("aquarium-temperature", "수온 감시·경보 릴레이 제어", "DS18B20 수온", "../1-wire/ds18b20.md", "수동 승인 버튼", "../gpio/basic-io.md", "릴레이 모듈", "../gpio/relay-srd05.md", "정상 센서값과 수동 승인 조건에서만 외부 장치 구동을 허용하고 오류 시 안전 정지한다."),
    ("motor-jam-guard", "엔코더 기반 모터 걸림 감지", "로터리 엔코더", "../gpio/rotary-encoder.md", "수동 승인 버튼", "../gpio/basic-io.md", "TB6612FNG 모터", "../motor/tb6612fng.md", "모터 구동 중 회전 펄스가 일정 시간 없으면 정지하고 명시적 복귀를 요구한다."),
    ("escape-route", "비상 통로 접근 감지 안내등", "HC-SR501 PIR", "../gpio/hc-sr501.md", "HC-SR04 거리", "../gpio/hc-sr04.md", "LED 상태등", "../gpio/basic-io.md", "움직임과 거리 유효값으로 통로 접근을 판단하고 오류 시 안전 표시를 우선한다."),
    ("touch-fan", "터치 모드·온도 연동 개인용 팬", "TTP223 터치", "../gpio/ttp223.md", "DHT22 온도", "../gpio/dht22.md", "PWM 팬", "../gpio/fan-pwm.md", "터치로 수동·자동 모드를 전환하고 과온 보호와 히스테리시스를 적용한다."),
    ("motion-servo-camera", "움직임 감지형 서보 추적 거치대", "HC-SR501 PIR", "../gpio/hc-sr501.md", "수동 승인 버튼", "../gpio/basic-io.md", "SG90 서보", "../motor/sg90.md", "감지 이벤트 후 승인되면 서보를 사전 정의된 두 위치 사이에서 움직인다. 실제 영상 추적은 포함하지 않는다."),
    ("ambient-alert", "온도·조도 복합 환경 알림", "DHT22 온도", "../gpio/dht22.md", "BH1750 조도", "../i2c/sensors.md", "LED 상태등", "../gpio/basic-io.md", "온도와 조도의 독립 임계조건을 평가하여 상태 LED의 우선순위를 결정한다."),
]
SENSORS = [
    ("HC-SR04 초음파", "../gpio/hc-sr04.md"),
    ("HC-SR501 PIR", "../gpio/hc-sr501.md"),
    ("DHT22 온습도", "../gpio/dht22.md"),
    ("BH1750 조도", "../i2c/sensors.md"),
    ("로터리 엔코더", "../gpio/rotary-encoder.md"),
    ("TTP223 터치", "../gpio/ttp223.md"),
    ("DS18B20 온도", "../1-wire/ds18b20.md"),
    ("수동 승인 버튼", "../gpio/basic-io.md"),
]
OUTPUTS = [
    ("LED PWM", "../gpio/basic-io.md"),
    ("능동 부저", "../gpio/buzzer.md"),
    ("SG90 서보", "../motor/sg90.md"),
    ("PWM 팬", "../gpio/fan-pwm.md"),
    ("TB6612FNG 모터", "../motor/tb6612fng.md"),
    ("릴레이 모듈", "../gpio/relay-srd05.md"),
]
POLICIES = [
    "두 입력이 모두 유효할 때만 구동하며 하나라도 오류면 안전 상태로 전환한다.",
    "입력 1 후 입력 2가 발생하는 순서 이벤트를 검출하고 타임아웃을 기록한다.",
    "입력 변화가 일정 시간 지속될 때만 출력하고 채터링을 제거한다.",
    "상한·하한 히스테리시스를 적용하고 센서 무효값에는 구동을 정지한다.",
    "사용자 승인 이벤트와 센서 조건을 모두 만족해야 동작한다.",
    "누적 이벤트 횟수에 따라 출력 단계를 조절하고 Reset에서 초기화한다.",
    "오류 발생 시 출력 차단 상태를 래치하고 명시적 해제만 허용한다.",
    "두 입력이 불일치하면 진단 상태로 전환하고 안전 출력 우선순위를 적용한다.",
]
STATUS_PATHS = ["LED 오류 코드", "UART 상태 보고", "AXI 상태 레지스터", "7세그먼트 상태"]


def candidates():
    yield from IDEAS
    total = len(SENSORS) * (len(SENSORS)-1) * len(OUTPUTS) * len(POLICIES) * len(STATUS_PATHS)
    for ordinal in range(total):
        index = (ordinal * 1009) % total
        status_i = index % len(STATUS_PATHS)
        index //= len(STATUS_PATHS)
        policy_i = index % len(POLICIES)
        index //= len(POLICIES)
        output_i = index % len(OUTPUTS)
        index //= len(OUTPUTS)
        second = index % (len(SENSORS)-1)
        first = index // (len(SENSORS)-1)
        second += second >= first
        a, b, output = SENSORS[first], SENSORS[second], OUTPUTS[output_i]
        slug = f"fusion-{first:02d}-{second:02d}-{output_i:02d}-{policy_i:02d}-{status_i:02d}"
        title = f"{a[0]}·{b[0]} 융합 {output[0]} 제어 ({STATUS_PATHS[status_i]})"
        yield (slug, title, a[0], a[1], b[0], b[1], output[0], output[1],
               POLICIES[policy_i])


def project_numbers(cap):
    nums = []
    for p in cap.glob("capstone-??-*.md"):
        m = re.match(r"capstone-(\d\d)-", p.name)
        if m:
            nums.append(int(m.group(1)))
    return nums


def project_rows(cap):
    rows = [(1, "PIR 침입 감지·거리 연동 보안 제어", "index.html", "README.md")]
    for p in sorted(cap.glob("capstone-??-*.md")):
        m = re.match(r"capstone-(\d\d)-", p.name)
        if not m:
            continue
        n = int(m.group(1))
        title = p.read_text(encoding="utf-8").splitlines()[0].lstrip("# ").strip()
        rows.append((n, title, p.with_suffix(".html").name, p.name))
    return sorted(rows)


def update_catalog(cap):
    readme = cap / "README.md"
    original = readme.read_text(encoding="utf-8")
    rows = project_rows(cap)
    table = ["| 번호 | 주제 | 문서 | 독립 HTML | 구현 상태 |",
             "|---|---|---|---|---|"]
    for n, title, page, md in rows:
        stage = "기존 소스 연계·통합 미검증" if n == 1 else "설계안·통합 미구현"
        table.append(f"| {n:02d} | {title} | [설계서]({md}) | [HTML]({page}) | {stage} |")
    pat = r"(## 캡스톤 목록\s*\n)(.*?)(\n\[주변장치 홈\])"
    updated, count = re.subn(pat, lambda m: m.group(1) + "\n".join(table) + "\n" + m.group(3),
                             original, count=1, flags=re.S)
    if count != 1:
        raise ValueError("Cannot locate capstone README catalog region")
    readme.write_text(updated, encoding="utf-8")

    index = cap / "index.html"
    html_src = index.read_text(encoding="utf-8")
    cards = ['<section id="capstone-list"><div class="cards">']
    for n, title, page, md in rows:
        cards.append('<a class="card" href="' + html.escape(page, quote=True) + '">' +
                     '<strong>CAPSTONE-' + f"{n:02d}" + '</strong><p>' +
                     html.escape(title) + '</p></a>')
    cards.append("</div></section>")
    generated = "\n".join(cards)
    start, end = "<!-- capstone-factory:begin -->", "<!-- capstone-factory:end -->"
    if start in html_src and end in html_src:
        html_src = re.sub(re.escape(start) + r".*?" + re.escape(end),
                          lambda _: start + "\n" + generated + "\n" + end,
                          html_src, count=1, flags=re.S)
    else:
        pat = r"(<h2>캡스톤 목록</h2>)(.*?)(<h2>1\. 공개 원본 사례</h2>)"
        html_src, count = re.subn(pat, lambda m: m.group(1) + "\n" + start + "\n" +
                                   generated + "\n" + end + "\n" + m.group(3),
                                   html_src, count=1, flags=re.S)
        if count != 1:
            raise ValueError("Cannot locate manual capstone index catalog region")
    index.write_text(html_src, encoding="utf-8")


def create(root=None):
    root = Path(root) if root is not None else ROOT
    cap = root / "devices" / "capstone"
    used = {p.name.split("-", 2)[-1].removesuffix(".md")
            for p in cap.glob("capstone-??-*.md")}
    selected = next((x for x in candidates() if x[0] not in used), None)
    if selected is None:
        raise RuntimeError("All distinct creative capstone combinations exhausted")
    slug, title, a, a_doc, b, b_doc, out, out_doc, policy = selected
    number = max([1] + project_numbers(cap)) + 1
    name = f"capstone-{number:02d}-{slug}"
    md = cap / (name + ".md")
    page = cap / (name + ".html")
    if md.exists() or page.exists():
        raise FileExistsError(name)
    title_full = f"CAPSTONE-{number:02d} — {title}"
    safety = ("Basys3 GPIO는 3.3V 기준. HC-SR04 일반형 ECHO 5V 직접 연결 금지; "
              "XADC 단극성 입력은 0~1V 범위를 지켜야 한다. "
              "서보·모터·릴레이는 FPGA GPIO에서 전력을 공급하지 않고 "
              "외부 전원·구동 회로·공통 접지/절연 조건을 확인한다. "
              "I²C 모듈은 칩과 브레이크아웃의 전원·풀업 전압을 각각 확인한다.")
    md_text = f"""# {title_full}

[캡스톤 목록](README.md) · [독립 HTML]({page.name}) · [주변모듈 기술자료](../README.md)

**분류:** 독립 창작·응용 융합형 캡스톤. **현재 상태: 설계안 작성 완료 / 신규 통합 RTL·C·AXI·Vivado·실물 시험 미실시.**

## 1. 실제로 만드는 시스템
- 입력 A: [{a}]({a_doc})
- 입력 B: [{b}]({b_doc})
- 출력: [{out}]({out_doc})
- 핵심 동작: {policy}
- Basys3 단독 RTL → MicroBlaze V + AXI 레지스터 확장 순으로 구현한다.

## 2. 하드웨어와 신호 흐름

```mermaid
flowchart LR
  A["{a}"] --> IA["input_a_interface (제안)"]
  B["{b}"] --> IB["input_b_interface (제안)"]
  IA --> CTRL["capstone_controller (제안)"]
  IB --> CTRL
  CTRL --> DRV["output_driver (제안)"]
  DRV --> OUT["{out}"]
  CTRL --> REG["status_registers (제안, SoC 확장)"]
```

위 인터페이스·모듈명은 **신규 설계용 이름**이며, 실제 저장소에 동일한 RTL 포트나 파일이 있다고 주장하지 않는다.

## 3. 제어 정책 및 FSM
| 상태 | 진입 조건 | 출력·다음 상태 |
|---|---|---|
| IDLE | Reset 후 | 안전 출력 유지, 입력 유효 대기 |
| CHECK | 입력 A 또는 B 이벤트 | 두 입력의 유효값·순서·타임아웃 확인 |
| ACTIVE | 정책 조건 성립 | {out} 구동, 상태 기록 |
| HOLD | 동작 완료 또는 유지 조건 | 재진입·채터링 방지, 종료 대기 |
| FAULT | 입력 오류·타임아웃·전원 안전 위반 | 안전 출력 및 오류 래치, 명시적 복귀 |

**정책 상세:** {policy}

## 4. 구현 단계
1. 센서·출력 모듈의 실제 IC/기판, VCC, I/O HIGH, 풀업, 구동 전류 확인
2. 각 입력의 유효 펄스·샘플 주기·동기화·Reset 및 오류 상태 정의
3. 기능별 Controller와 출력 Driver를 Top과 분리하여 RTL 구현
4. 입력 A만, B만, 순서 반대, 동시 입력, 타임아웃, Reset, 경계값 TB 작성
5. 실제 사용되는 IP에 한해 AXI GPIO 또는 사용자 AXI4-Lite Register Map 추가
6. Vivado 합성·Timing·보드 시험을 별도로 기록하고 검증된 결과만 배포

## 5. 검증 시나리오
- Reset에서 출력이 안전 상태인지 확인
- 입력 A만 발생 / 입력 B만 발생 / A→B / B→A / 동시 발생
- 정상 동작 중 재입력·바운스·센서 무효값·타임아웃
- 정책 충족·불충족 시 출력 구동과 상태 레지스터 일치
- 보드 연결 전 각 핀의 실제 전압·전류·드라이버 적합성 확인

## 6. 전기 안전과 원본 구분
{safety}

이 과제의 기능 조합·FSM·실습 시나리오는 자체 작성했다. 연결한 기존 기술자료는 참고 근거이며 공개 튜토리얼의 코드·이미지·회로도를 복제하지 않았다.

## 7. 검증 상태
| 단계 | 상태 |
|---|---|
| 교육용 설계서·독립 HTML | 생성됨 |
| 기존 개별 모듈 기술자료 | 링크로 참조, 실제 변형별 확인 필요 |
| 신규 통합 RTL·테스트벤치 | 미구현 |
| C 드라이버·AXI 통합 | 미구현 |
| RTL 시뮬레이션·합성·Timing | 미실시 |
| Basys3 실물 검증 | 미실시 |
"""
    esc = html.escape
    html_text = f"""<!doctype html><html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>{esc(title_full)}</title><style>:root{{font:16px/1.65 system-ui,"Noto Sans KR",sans-serif;color-scheme:light dark}}body{{margin:0;background:Canvas;color:CanvasText}}main{{max-width:960px;margin:auto;padding:22px 18px 70px}}a{{color:#3483b8}}h1{{line-height:1.3}}h2{{margin-top:30px;border-bottom:1px solid #8887;padding-bottom:6px}}.note{{padding:14px;border-left:4px solid #c98e33;background:color-mix(in srgb,CanvasText 5%,Canvas)}}.grid{{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:10px}}.card{{border:1px solid #8887;border-radius:10px;padding:12px}}table{{border-collapse:collapse;width:100%}}td,th{{padding:8px;border-bottom:1px solid #8887;text-align:left}}pre{{white-space:pre-wrap;overflow-wrap:anywhere;background:color-mix(in srgb,CanvasText 5%,Canvas);padding:14px}}</style></head><body><main>
<nav><a href="../../index.html">메인</a> › <a href="index.html">캡스톤 목록</a> › {number:02d}</nav>
<h1>{esc(title_full)}</h1><p class="note"><strong>새로운 자체 캡스톤 설계서</strong> — 실제 통합 RTL·AXI·합성·보드 시험은 미실시.</p>
<h2>1. 시스템 구성</h2><div class="grid">
<div class="card"><b>입력 A</b><p><a href="{esc(a_doc,quote=True)}">{esc(a)}</a></p></div>
<div class="card"><b>입력 B</b><p><a href="{esc(b_doc,quote=True)}">{esc(b)}</a></p></div>
<div class="card"><b>제어</b><p>capstone_controller (신규 제안)</p></div>
<div class="card"><b>출력</b><p><a href="{esc(out_doc,quote=True)}">{esc(out)}</a></p></div></div>
<h2>2. 동작 정책</h2><p>{esc(policy)}</p>
<h2>3. 개념 구조 (실제 RTL 포트 아님)</h2><pre>{esc(a)} → input_a_interface ─┐
{esc(b)} → input_b_interface ─┤→ capstone_controller → output_driver → {esc(out)}
                         └→ status_registers (MicroBlaze V 확장)</pre>
<h2>4. 상태 흐름</h2><table><tr><th>상태</th><th>역할</th></tr>
<tr><td>IDLE</td><td>안전 출력, 입력 대기</td></tr><tr><td>CHECK</td><td>유효값·순서·타임아웃 검사</td></tr>
<tr><td>ACTIVE</td><td>정책 조건 성립 시 출력</td></tr><tr><td>HOLD</td><td>유지·재입력 억제</td></tr>
<tr><td>FAULT</td><td>안전 정지 및 오류 래치</td></tr></table>
<h2>5. 따라하기 구현 절차</h2><ol><li>데이터시트·전압·핀·구동전류 확인</li><li>입력 유효/Reset/타임아웃 정의</li>
<li>Controller/Driver 독립 RTL 설계</li><li>Reset·정상·동시·오류 자기검증 TB 작성</li>
<li>필요시 AXI/소프트웨어 연계</li><li>Vivado 합성·Timing·실물 보드 검증</li></ol>
<h2>6. 전기적 안전</h2><p>{esc(safety)}</p>
<h2>7. 검증 상태</h2><p>설계 문서·HTML 생성 완료. 신규 통합 RTL·C·AXI·시뮬레이션·합성·Timing·보드 시험은 미구현/미실시.</p>
<p><a href="{esc(md.name)}">전체 Markdown 설계서</a> · <a href="README.md">캡스톤 전체 목록</a></p></main></body></html>
"""
    md.write_text(md_text, encoding="utf-8")
    page.write_text(html_text, encoding="utf-8")
    update_catalog(cap)
    return number, name


def selftest():
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        cap = root / "devices" / "capstone"
        cap.mkdir(parents=True)
        (cap / "README.md").write_text("# CAPSTONE-01\n\n## 캡스톤 목록\n\n| old |\n\n[주변장치 홈](../README.md)\n", encoding="utf-8")
        (cap / "index.html").write_text("<h2>캡스톤 목록</h2><p>old</p><h2>1. 공개 원본 사례</h2>", encoding="utf-8")
        a = create(root)
        b = create(root)
        assert a[0] == 2 and b[0] == 3 and a[1] != b[1]
        assert "capstone-03" in (cap / "README.md").read_text(encoding="utf-8")
        assert "capstone-03" in (cap / "index.html").read_text(encoding="utf-8")
        print("PASS capstone factory: distinct Markdown, HTML and indexes")


if __name__ == "__main__":
    if "--selftest" in sys.argv:
        selftest()
    else:
        n, name = create()
        print(f"NEW CAPSTONE-{n:02d}: {name}")
