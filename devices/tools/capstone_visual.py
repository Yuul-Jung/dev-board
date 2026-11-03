#!/usr/bin/env python3
"""Deterministic visual + structured assets for factory-generated capstones.

Only pages carrying the known first-party factory heading are upgraded.
Hand-authored capstone HTML and source RTL are never rewritten.
"""
import html
import json
import re
import sys
from pathlib import Path


def svg_diagram(a, b, output):
    esc = html.escape
    blocks = [
        (20, 30, 185, 66, "입력 A", a),
        (20, 155, 185, 66, "입력 B", b),
        (250, 30, 190, 66, "입력 인터페이스", "input_a_interface"),
        (250, 155, 190, 66, "입력 인터페이스", "input_b_interface"),
        (490, 95, 190, 76, "제어기 (제안)", "capstone_controller"),
        (730, 95, 190, 76, "출력 구동기", "output_driver"),
        (730, 265, 190, 66, "출력", output),
        (490, 265, 190, 66, "SoC 확장 (제안)", "status_registers"),
    ]
    lines = [
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 940 355" '
        'role="img" aria-label="입력 인터페이스에서 제어기와 출력 구동기로 이어지는 시스템 신호 흐름">',
        '<defs><marker id="arrow" markerWidth="8" markerHeight="8" refX="7" refY="4" orient="auto">'
        '<path d="M0 0 L8 4 L0 8" fill="none" stroke="#406b91"/></marker></defs>',
        '<g fill="none" stroke="#406b91" stroke-width="2.3" marker-end="url(#arrow)">',
        '<path d="M205 63 H250"/>', '<path d="M205 188 H250"/>',
        '<path d="M440 63 H465 V133 H490"/>',
        '<path d="M440 188 H465 V133 H490"/>',
        '<path d="M680 133 H730"/>',
        '<path d="M825 171 V265"/>',
        '<path d="M585 171 V265"/>',
        '</g>',
    ]
    for x, y, w, h, title, label in blocks:
        lines.append(
            f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="10" '
            'fill="#f0f6fc" stroke="#628caf" stroke-width="1.6"/>'
        )
        lines.append(f'<text x="{x + w // 2}" y="{y + 24}" text-anchor="middle" '
                     f'font-size="13" fill="#315779">{esc(title)}</text>')
        lines.append(f'<text x="{x + w // 2}" y="{y + 45}" text-anchor="middle" '
                     f'font-size="13" font-weight="600" fill="#173b58" '
                     f'textLength="{w-18}" lengthAdjust="spacingAndGlyphs">{esc(label)}</text>')
    lines.append('</svg>')
    return "\n".join(lines) + "\n"


def metadata(stem, title, a, b, output):
    return {
        "schema_version": 1,
        "id": stem.split("-", 2)[1],
        "title": title,
        "status": "DESIGN_ONLY_NOT_IMPLEMENTED",
        "input_modules": [{"name": a, "interface": "input_a_interface"},
                          {"name": b, "interface": "input_b_interface"}],
        "controller": "capstone_controller",
        "output_module": {"name": output, "driver": "output_driver"},
        "soc_extension": "status_registers",
        "fsm_states_proposed": ["IDLE", "CHECK", "ACTIVE", "HOLD", "FAULT"],
        "assets": {"markdown": stem + ".md", "html": stem + ".html",
                   "diagram_svg": stem + ".svg"},
        "verification": {"rtl_simulation": "not_run", "synthesis": "not_run",
                         "timing": "not_run", "board_test": "not_run"},
    }


def write_assets(cap, stem, title, a, b, output):
    svg = cap / (stem + ".svg")
    js = cap / (stem + ".json")
    svg.write_text(svg_diagram(a, b, output), encoding="utf-8")
    js.write_text(json.dumps(metadata(stem, title, a, b, output),
                             ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def figure(stem):
    return ('<figure class="system-visual">'
            '<img class="system-diagram" src="' + html.escape(stem) + '.svg" '
            'alt="입력 A와 B에서 제어기, 출력 구동기, SoC 상태 레지스터까지의 신호 흐름" '
            'style="display:block;width:100%;height:auto;max-width:940px;margin:auto">'
            '<figcaption>설계 개념도: 제안된 인터페이스·모듈명이며 실제 RTL 포트 확정 전입니다. '
            '<a href="' + html.escape(stem) + '.svg">SVG 원본</a> · '
            '<a href="' + html.escape(stem) + '.json">구조화 데이터(JSON)</a>'
            '</figcaption></figure>')


def upgrade_existing(cap):
    updated = []
    heading = '<h2>3. 개념 구조 (실제 RTL 포트 아님)</h2>'
    for page in sorted(cap.glob("capstone-??-*.html")):
        source = page.read_text(encoding="utf-8")
        if heading not in source:  # Hand-authored pages are not touched.
            continue
        names = []
        for label in ("입력 A", "입력 B", "출력"):
            m = re.search(r'<b>' + re.escape(label) + r'</b><p><a[^>]*>(.*?)</a>', source)
            if not m:
                raise ValueError("Missing factory signal card in " + page.name + ": " + label)
            names.append(html.unescape(m.group(1)))
        title_match = re.search(r'<h1>(.*?)</h1>', source, re.S)
        if not title_match:
            raise ValueError("Missing title: " + page.name)
        title = html.unescape(title_match.group(1))
        write_assets(cap, page.stem, title, *names)
        if 'class="system-diagram"' not in source:
            source = source.replace(heading, heading + "\n" + figure(page.stem), 1)
            page.write_text(source, encoding="utf-8")
            updated.append(page.name)
    return updated


if __name__ == "__main__":
    if "--repair" not in sys.argv:
        raise SystemExit("Usage: python3 devices/tools/capstone_visual.py --repair")
    root = Path(__file__).resolve().parents[1] / "capstone"
    print("Upgraded generated capstone diagrams:", upgrade_existing(root))
