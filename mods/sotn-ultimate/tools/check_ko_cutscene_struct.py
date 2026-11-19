#!/usr/bin/env python3
# check_ko_cutscene_struct.py -- structural invariants of the KO cutscene scripts vs the retail (EN) ones (fail closed).
# A KO dialogue window must not change who speaks or when: for every cutscene script
#   1. same number of windows (NEXT_DIALOG)               -> no window was added/removed/merged
#   2. same speaker sequence (SET_PORTRAIT slot,side)     -> no speaker changes between windows
#   3. same voice clip per window (PLAY_SOUND ids)        -> speech/voice stays in the window it belongs to
#   4. every window has <= WINDOW_LINES lines (LINE_BREAK count + 1)  [profile window_lines]
# Sources: retail = assets/st/<stage>/cutscene_script_psx.yaml.ko_retail (EN original);
#          KO     = the KO-spliced yaml (while the build runs) and the baked header src/st/st0/cutscene_script_ko_st0.h.
# This cannot catch a translation row whose MEANING crosses the speaker boundary (the sihum43 st0 :2/:3 case);
# that needs the JP window tables (see 58/57 files).
import os
import re
import sys

REPO = os.environ.get("SOTN_KR", os.getcwd())
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ko_font_profile as FP  # noqa: E402

WL = FP.P["window_lines"]
ROW = re.compile(r"^\s*-\s*\[(\w+)(?:,\s*(.*))?\]\s*$")


def yaml_ops(path):
    ops = []
    for ln in open(path, encoding="utf-8"):
        m = ROW.match(ln)
        if m:
            ops.append((m.group(1), (m.group(2) or "").strip()))
    return ops


def header_ops(path):
    ops = []
    for ln in open(path, encoding="utf-8"):
        s = ln.strip().rstrip(",")
        if not s or s.startswith("//") or s.startswith("#"):
            continue
        m = re.match(r"^(\w+)\((.*)\)$", s)
        if m:
            ops.append((m.group(1), m.group(2).strip()))
        elif s.startswith("0x"):
            ops.append(("TEXT_BYTE", s))
    return ops


def windows(ops):
    ws = []
    cur = {"por": [], "snd": [], "lb": 0, "text": 0}
    for op, arg in ops:
        if op == "SET_PORTRAIT":
            cur["por"].append(re.sub(r"\s+", "", arg))
        elif op == "PLAY_SOUND":
            cur["snd"].append(arg.strip().lower())
        elif op == "LINE_BREAK":
            cur["lb"] += 1
        elif op in ("TEXT", "TEXT_BYTE"):
            cur["text"] += 1
        elif op == "NEXT_DIALOG":
            ws.append(cur)
            cur = {"por": [], "snd": [], "lb": 0, "text": 0}
    ws.append(cur)
    return ws


def compare(name, ret, ko):
    wr, wk = windows(ret), windows(ko)
    bad = []
    if len(wr) != len(wk):
        bad.append("%s: %d windows in the KO script, %d in retail" % (name, len(wk), len(wr)))
    for i, (a, b) in enumerate(zip(wr, wk)):
        if a["por"] != b["por"]:
            bad.append("%s window %d: speaker %s != retail %s" % (name, i, b["por"], a["por"]))
        if a["snd"] != b["snd"]:
            bad.append("%s window %d: voice %s != retail %s" % (name, i, b["snd"], a["snd"]))
        if b["text"] and b["lb"] + 1 > WL:
            bad.append("%s window %d: %d lines > window_lines %d" % (name, i, b["lb"] + 1, WL))
    return bad, len(wk)


def main():
    bad = []
    n = 0
    for st in ("st0", "dai", "dre", "rcen"):
        d = os.path.join(REPO, "assets/st", st)
        ret = os.path.join(d, "cutscene_script_psx.yaml.ko_retail")
        ko = os.path.join(d, "cutscene_script_psx.yaml")
        if not os.path.exists(ret):
            continue
        if os.path.exists(ko) and open(ko, "rb").read() != open(ret, "rb").read():
            b, k = compare("yaml %s" % st, yaml_ops(ret), yaml_ops(ko))
            bad += b
            n += 1
    hdr = os.path.join(REPO, "src/st/st0/cutscene_script_ko_st0.h")
    ret0 = os.path.join(REPO, "assets/st/st0/cutscene_script_psx.yaml.ko_retail")
    if os.path.exists(hdr) and os.path.exists(ret0):
        b, k = compare("header st0", yaml_ops(ret0), header_ops(hdr))
        bad += b
        n += 1
        print("st0 header: %d windows" % k)
    if bad:
        print("FAIL check_ko_cutscene_struct: %d violations" % len(bad))
        for x in bad:
            print("  " + x)
        sys.exit(1)
    print("OK   check_ko_cutscene_struct: %d KO scripts keep retail window count / speaker order / voice per window, <= %d lines" % (n, WL))


if __name__ == "__main__":
    main()
