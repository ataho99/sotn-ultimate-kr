#!/usr/bin/env python3
# gen_ko_st0_snapshot.py -- regenerate src/st/st0/cutscene_script_ko_st0.h
# (ST0 static KO script snapshot) from the KO-spliced yaml written by
# gen_ko_splice.py. Run AFTER gen_ko_splice.py.
#
# Positive controls (fail closed):
#   * emitted byte size == container size (626 B, unit-exact)
#   * the sequence of non-text rows (control ops) equals the previous snapshot's
import io
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_ko_splice as S  # noqa: E402

REPO = os.environ.get("SOTN_KR", S.REPO)
YAML = os.path.join(REPO, "assets/st/st0/cutscene_script_psx.yaml")
OUT = os.path.join(REPO, "src/st/st0/cutscene_script_ko_st0.h")
SIZE = 626

HEAD = """// cutscene_script_ko_st0.h -- ST0 battle dialogue KO script (PSX).
// Snapshot of the KO-spliced assets/st/st0/cutscene_script_psx.yaml
// (gen_ko_split.py/gen_ko_splice.py + gen_ko_st0_snapshot.py: prefs
// st0_richter_dracula, v14 translation + 1-byte promotion, symbol
// cutscene_script size 626, unit-exact; 0x0D = CSOP_SCRIPT_UNKNOWN_13 no-op
// pads). Source-baked so the script does not depend on yaml splice timing.
// Regenerate: python3 gen_ko_splice.py && python3 gen_ko_st0_snapshot.py
#pragma once
// clang-format off
"""


def main():
    # control: control ops of the spliced yaml == control ops of the retail yaml
    ret = S.split_rows([l for l in io.open(YAML + ".ko_retail", encoding="utf-8").read().split(chr(10))
                        if l.strip() != ""][1:])
    old_ops = ["%s(%s)," % (k, ",".join(a.strip() for a in p.split(",")) if p.strip() else "")
               for k, p, r in ret if k not in ("RAW", "TEXT", "BYTE")]
    rows = S.split_rows([l for l in io.open(YAML, encoding="utf-8").read().split("\n")
                         if l.strip() != ""][1:])  # drop "script:" header
    out = []
    size = 0
    for kind, pay, raw in rows:
        if kind == "RAW":
            assert raw.lstrip().startswith("#"), raw
            continue
        if kind == "TEXT":
            assert pay == "", ("retail text row left in ST0 KO script", pay)
            continue
        size += S._row_size(kind, pay)
        if kind == "BYTE":
            out.append("0x%02X," % int(pay, 0))
        else:
            args = ",".join(a.strip() for a in pay.split(",")) if pay.strip() else ""
            out.append("%s(%s)," % (kind, args))
    assert size == SIZE, (size, SIZE)
    new_ops = [l for l in out if not l.startswith("0x")]
    if old_ops:
        # window breaks (gen_ko_splice): a LINE_BREAK may become NEXT_DIALOG + SET_PORTRAIT [+ SET_SPEED];
        # compare everything else exactly, and these four kinds by their total count only
        drop = ("LINE_BREAK(", "NEXT_DIALOG(", "SET_PORTRAIT(", "SET_SPEED(", "SCRIPT_UNKNOWN_11(", "SET_WAIT(")  # trial49: gen_ko_timing.py inserts SET_WAIT (paid by 0x0D pads)
        keep = lambda L: [x for x in L if not x.startswith(drop)]
        assert keep(new_ops) == keep(old_ops), "control op sequence differs from retail yaml"
    io.open(OUT, "w", encoding="utf-8", newline="\n").write(HEAD + "\n".join(out) + "\n")
    print("wrote %s: %d rows, %d B, %d control ops identical to retail yaml"
          % (OUT, len(out), size, len(new_ops)))


if __name__ == "__main__":
    main()
