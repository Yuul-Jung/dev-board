#!/usr/bin/env python3
"""Fail-closed publication gate: visualize, structure and organize capstones.

Read-only: does not change RTL, existing content, generated assets or evidence.
The safe deterministic repair tool is capstone_visual.py --repair.
"""
import json
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CAP = ROOT / "devices" / "capstone"


def audit(root=ROOT):
    cap = root / "devices" / "capstone"
    errors = []
    readme = (cap / "README.md").read_text(encoding="utf-8")
    index = (cap / "index.html").read_text(encoding="utf-8")
    home = (root / "index.html").read_text(encoding="utf-8")
    if "devices/capstone/index.html" not in home:
        errors.append("main HTML does not link the capstone catalog")
    checked = 0
    visualized = 0
    structured = 0
    organized = 0
    for md in sorted(cap.glob("capstone-??-*.md")):
        checked += 1
        stem = md.stem
        page = md.with_suffix(".html")
        if not page.exists():
            errors.append(stem + ": missing standalone HTML")
            continue
        m = md.read_text(encoding="utf-8")
        h = page.read_text(encoding="utf-8")
        if "<h1" not in h or "<table" not in h or len(re.findall(r"<h2\b", h)) < 4:
            errors.append(stem + ": missing structured HTML title, sections or table")
        else:
            structured += 1
        if md.name not in readme or page.name not in index:
            errors.append(stem + ": not linked in both Markdown and HTML catalog")
        elif 'href="index.html"' not in h and 'href="README.md"' not in h:
            errors.append(stem + ": no return navigation to capstone catalog")
        else:
            organized += 1

        factory = "flowchart LR" in m and "input_a_interface" in m
        if factory:
            # Factory outputs must have an actually rendered SVG and machine-readable IR,
            # not only ASCII art or a decorative card.
            svg = cap / (stem + ".svg")
            data = cap / (stem + ".json")
            if not svg.is_file() or not data.is_file():
                errors.append(stem + ": missing SVG and/or structured JSON")
                continue
            try:
                tree = ET.parse(svg)
                labels = " ".join((t.text or "") for t in tree.findall(".//{*}text"))
                assert len(tree.findall(".//{*}rect")) >= 6
                assert len(tree.findall(".//{*}path")) >= 6
                for code_name in ("input_a_interface", "input_b_interface",
                                  "capstone_controller", "output_driver", "status_registers"):
                    assert code_name in labels and code_name in m, code_name
                j = json.loads(data.read_text(encoding="utf-8"))
                assert j["assets"]["markdown"] == md.name
                assert j["assets"]["html"] == page.name
                assert j["assets"]["diagram_svg"] == svg.name
                assert j["status"] == "DESIGN_ONLY_NOT_IMPLEMENTED"
                assert len(j["input_modules"]) == 2
                assert len(j["fsm_states_proposed"]) >= 3
                assert j["verification"]["board_test"] == "not_run"
            except (ET.ParseError, ValueError, KeyError, TypeError, AssertionError) as exc:
                errors.append(stem + ": invalid diagram or structured metadata: " + str(exc))
                continue
            if 'class="system-diagram"' not in h or svg.name not in h or data.name not in h:
                errors.append(stem + ": SVG/JSON not visible and linked in HTML")
            elif not all("## " + str(n) + "." in m for n in range(1, 8)):
                errors.append(stem + ": incomplete Markdown section hierarchy")
            elif not all(k in m for k in ("| 상태 |", "| 단계 |", "MicroBlaze V", "미실시")):
                errors.append(stem + ": missing FSM, validation table or SoC learning path")
            else:
                visualized += 1
        else:
            # Legacy hand-authored pages are preserved; card-and-arrow flows count as
            # existing visual layouts. They are not silently rewritten.
            if ('class="card"' in h and ("→" in h or 'class="arrow"' in h)):
                visualized += 1
            else:
                errors.append(stem + ": legacy page has no readable visual signal flow")
    if checked == 0:
        errors.append("no capstone pages found")
    print(f"PUBLICATION GATE: total={checked}, visual={visualized}, "
          f"structured={structured}, organized={organized}, errors={len(errors)}")
    for err in errors:
        print("::error::" + err)
    return errors


if __name__ == "__main__":
    sys.exit(1 if audit() else 0)
