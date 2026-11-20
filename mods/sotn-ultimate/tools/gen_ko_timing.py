#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
# gen_ko_timing.py -- retime the KO cutscene windows to the Dub voices.
#
# The engine shows a window for  glyphs * SET_SPEED + sum(SET_WAIT)  frames and
# never looks at the voice (src/st/*/cutscene.c, EntityCutscene; hangul has far
# fewer glyphs than the English the SET_WAIT/SET_SPEED values were authored for,
# and the Japanese voices are longer than the English ones).
# For every voiced window (PLAY_SOUND <voice id 0x300..0x5FF> .. NEXT_DIALOG /
# CLOSE_DIALOG / END_CUTSCENE) this makes
#     frames >= (voices of the window, longest lane each, summed) + MARGIN :
#   1. SET_SPEED runs are scaled up (never down, at most SPEED_CAP frames/glyph),
#   2. the rest goes into SET_WAIT ops (<= 255 each) just before the window's
#      closing op (before the SCRIPT_UNKNOWN_11 that precedes it, if any).
# A window that is already long enough is left alone, so a second run is a
# no-op. The subtitle may stay up longer than the voice; it never leaves early.
# Lanes: build/us/psp_dub/dub.json (PSP slot) and build/us/jp_dub/dub.json (JP
# slot), field unk228 = voice frames (60/s), replace[voice id - 0x300].
# A voice id missing from both lanes is reported (status "nolane") and left alone.
#
# Two sources:
#   yaml  the stage cutscene yamls under assets/ (default)
#   mar   the Saturn Maria fight scripts, the MOD_KO branch of g_MbScriptPre /
#         g_MbScriptPost in mods/sotn-ultimate/src/maria_saturn/mod_maria_boss_data.h
#         (written by gen_ko_mar.py, so run this after it; --hdr overrides the path)
#
# Size-locked yamls (gen_ko_splice keeps every script at its retail size, absolute
# intra-script targets must not move): each inserted SET_WAIT (2 bytes) is paid for
# by removing 2 of the 0x0D no-op pad rows (BYTE 0x0D) of the first pad run after the
# insertion point, and only when no intra-script target lies in between. A window
# that cannot be paid for is left as it is and reported (SHORT ... pad).
#
# usage: gen_ko_timing.py [--mar] [--out DIR] [--check] [--root REPO] [--hdr FILE]
#   --out DIR  write the retimed files under DIR (yaml: same relative paths; mar: DIR/mod_maria_boss_data.h), touch nothing
#   --check    only report; exit 1 if any voiced window is shorter than its voice
import argparse
import json
import os
import re
import sys

MARGIN = 15      # frames kept after the voices (0.25 s)
SPEED_CAP = 8    # frames per glyph at most
VOICE_LO, VOICE_HI = 0x300, 0x600
RE_OP = re.compile(r"^(\s*-\s*\[)(\w+)(?:,\s*(.*?))?(\]\s*)$")
RE_MACRO = re.compile(r"^(\w+)\((.*)\)$")
MAR_HDR = "mods/sotn-ultimate/src/maria_saturn/mod_maria_boss_data.h"
MAR_ARRAYS = ("g_MbScriptPre", "g_MbScriptPost")

FILES = [
    "assets/st/st0/cutscene_script_psx.yaml",
    "assets/st/dre/cutscene_script_psx.yaml",
    "assets/st/dai/cutscene_script_psx.yaml",
    "assets/st/rcen/cutscene_script_psx.yaml",
    "assets/st/cen/cutscene_data.yaml",
    "assets/st/lib/cutscene_data.yaml",
    "assets/st/no3/cutscene_data.yaml",
    "assets/st/nz0/cutscene_data.yaml",
    "assets/st/top/cutscene_data.yaml",
    "assets/st/no4/cutscene_data_ferryman_left_psx.yaml",
    "assets/st/no4/cutscene_data_ferryman_right_psx.yaml",
    "assets/st/sel/cutscene_endings.yaml",
]


def num(s):
    s = s.strip()
    return int(s, 16) if s.lower().startswith("0x") else int(s)


def lane_frames(root):
    lanes = []
    for rel in ("build/us/psp_dub/dub.json", "build/us/jp_dub/dub.json"):
        p = os.path.join(root, rel)
        if not os.path.exists(p):
            sys.exit("gen_ko_timing: missing %s (run psp_dub.py first)" % rel)
        lanes.append(json.load(open(p))["replace"])
    return lanes


def voice_frames(lanes, vid):
    """longest lane frames for one voice id; None when no lane has it"""
    c = str(vid - 0x300)
    best = None
    for r in lanes:
        if c in r:
            best = max(best or 0, int(r[c]["unk228"]))
    return best


# ---------------------------------------------------------------- core
# ops: list of (op, arg) or None (a line/token that is not an op: skipped).
# op names: PLAY_SOUND, SET_SPEED, SET_WAIT, TEXT, BYTE, SCRIPT_UNKNOWN_11,
# NEXT_DIALOG, CLOSE_DIALOG, END_CUTSCENE ...; arg is a string.

def play_id(a):
    parts = [x.strip() for x in a.split(",")]
    return (num(parts[0]) << 4 | num(parts[1])) if len(parts) > 1 else num(parts[0])


def windows(ops):
    wins, cur, speed = [], None, 4
    for i, o in enumerate(ops):
        if o is None:
            continue
        op, a = o
        if op == "PLAY_SOUND":
            vid = play_id(a)
            if VOICE_LO <= vid < VOICE_HI:
                if cur is None:
                    cur = {"start": i, "ids": [], "speed0": speed}
                cur["ids"].append(vid)
        elif op == "SET_SPEED":
            speed = num(a)
        elif op in ("NEXT_DIALOG", "CLOSE_DIALOG", "END_CUTSCENE"):
            if cur is not None:
                cur["end"] = i
                wins.append(cur)
                cur = None
    if cur is not None:
        cur["end"] = len(ops)
        wins.append(cur)
    return wins


def measure(ops, w):
    """(glyph frames, wait frames, {SET_SPEED op idx or -1: glyphs})"""
    speed, pend = w["speed0"], False
    runs = {}
    run_key = -1
    g = wsum = 0
    for i in range(w["start"], w["end"]):
        o = ops[i]
        if o is None:
            continue
        op, a = o
        if op == "SET_SPEED":
            speed = num(a)
            run_key = i
        elif op == "SET_WAIT":
            wsum += num(a)
        elif op == "TEXT":
            try:
                s = json.loads(a)
            except Exception:
                s = a.strip('"')
            runs[run_key] = runs.get(run_key, 0) + len(s)
            g += len(s) * speed
        elif op == "BYTE":
            b = num(a) if a.startswith("0x") or a.isdigit() else ord(a.strip("'"))
            if pend:
                pend = False
                continue
            if b == 0x0D:  # CSOP_SCRIPT_UNKNOWN_13 no-op pad (gen_ko_splice), not a glyph
                continue
            if (b & 0xFC) == 0xF0:
                pend = True
            runs[run_key] = runs.get(run_key, 0) + 1
            g += speed
    return g, wsum, runs


def plan(ops, lanes, check):
    """-> (report rows, newspeed {idx: n}, inserts {idx: [n, ...]})"""
    newspeed, inserts, rep = {}, {}, []
    for k, w in enumerate(windows(ops)):
        fr = [voice_frames(lanes, v) for v in w["ids"]]
        g, wsum, runs = measure(ops, w)
        cur = g + wsum
        ids = ["0x%x" % v for v in w["ids"]]
        if any(f is None for f in fr):
            rep.append((k, ids, cur, 0, "nolane"))
            continue
        T = sum(fr) + MARGIN
        status = "ok"
        if cur < T:
            status = "short"
            if not check:
                cur2 = cur
                if g > 0:
                    f = (T - wsum) / float(g)
                    gnew = 0
                    for key, cnt in runs.items():
                        if key == -1:
                            gnew += cnt * w["speed0"]  # inherited speed: left as is
                            continue
                        base = num(ops[key][1])
                        ns = min(max(base, int(round(base * f))), SPEED_CAP)
                        newspeed[key] = ns
                        gnew += cnt * ns
                    cur2 = gnew + wsum
                rest = max(0, T - cur2)
                at = w["end"]
                if at >= len(ops):  # window runs to the end: append after the last op
                    at = max(j for j, o in enumerate(ops) if o is not None) + 1
                if at > 0 and ops[at - 1] is not None and ops[at - 1][0] == "SCRIPT_UNKNOWN_11":
                    at -= 1
                while rest > 0:
                    n = min(255, rest)
                    inserts.setdefault(at, []).append(n)
                    rest -= n
        rep.append((k, ids, cur, T, status))
    return rep, newspeed, inserts


# ---------------------------------------------------------------- yaml
PAD_LINE = re.compile(r"^\s*-\s*\[BYTE,\s*0x0[dD]\]\s*$")
_S = []
ROOT = None


def splice_mod(root=None):
    if not _S:
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        root = root or ROOT
        if root:
            os.environ.setdefault("SOTN_KR", root)
        import gen_ko_splice as S
        _S.append(S)
    return _S[0]


def yaml_bytes(lines, root=None):
    """script size in bytes (gen_ko_splice row sizes), None when it cannot be told"""
    try:
        S = splice_mod()
        rows = S.split_rows([l for l in lines if l.strip() != ""])
        return sum(S._row_size(k, p) for k, p, r in rows if k != "RAW")
    except Exception:
        return None


def target_lines(lines, rel):
    """line indexes of the rows an absolute intra-script address points at"""
    S = splice_mod()
    nb = [i for i, l in enumerate(lines) if l.strip() != ""]
    rows = S.split_rows([lines[i] for i in nb])
    key0 = (rel.rsplit("/", 1)[0], rel.rsplit("/", 1)[1].rsplit(".", 1)[0])
    start0, size0 = S.config_sizes()[0][key0]
    return {nb[j] for j in S._intra_targets(rows, start0, size0)}


def pay_with_pads(lines, rel, inserts):
    """-> (inserts that can be paid, set of pad line idx to delete, dropped [(at, n bytes)])"""
    padruns, cur = [], None
    for i, ln in enumerate(lines):
        if PAD_LINE.match(ln):
            if cur is None:
                cur = [i, i]
                padruns.append(cur)
            else:
                cur[1] = i
        elif ln.strip() != "":
            cur = None
    if not padruns:
        return inserts, set(), []       # not a size-locked script
    try:
        targets = target_lines(lines, rel)
    except Exception as e:
        print("gen_ko_timing: %s: no target info (%s): inserts dropped" % (rel, e))
        return {}, set(), [(at, 2 * len(v)) for at, v in inserts.items()]
    avail = [r[1] - r[0] + 1 for r in padruns]
    used = [0] * len(padruns)
    ok, dele, dropped = {}, set(), []
    for at in sorted(inserts):
        for n in inserts[at]:
            run = None
            for k, r in enumerate(padruns):
                if r[0] < at:
                    continue
                if any(at <= t <= r[0] for t in targets):
                    break               # a target lies between: shifting it is not allowed
                if avail[k] - used[k] >= 2:
                    run = k
                break
            if run is None:
                dropped.append((at, 2))
                continue
            r = padruns[run]
            for _ in range(2):
                dele.add(r[1] - used[run])
                used[run] += 1
            ok.setdefault(at, []).append(n)
    return ok, dele, dropped


def retime_yaml(path, rel, lanes, out_root, check):
    lines = open(path, encoding="utf-8").read().split("\n")
    ops = []
    for ln in lines:
        m = RE_OP.match(ln.rstrip("\n"))
        ops.append((m.group(2), (m.group(3) or "").strip()) if m else None)
    rep, newspeed, inserts = plan(ops, lanes, check)
    dele, dropped = set(), []
    if inserts and not check:
        inserts, dele, dropped = pay_with_pads(lines, rel, inserts)
    rows = [(rel,) + r for r in rep]
    if dropped:
        print("PADSHORT %s: %d wait(s) (%d B) not paid by pads, windows stay short" % (rel, len(dropped), sum(d[1] for d in dropped)))
    changed = bool(inserts or newspeed)
    if check or (out_root is None and not changed):
        return rows, False
    out = []
    for i, ln in enumerate(lines):
        if i in dele:
            continue
        for n in inserts.get(i, []):
            out.append("  - [SET_WAIT, %d]" % n)
        if i in newspeed:
            ln = re.sub(r"(SET_SPEED,\s*)\d+", lambda m: m.group(1) + str(newspeed[i]), ln)
        out.append(ln)
    b0, b1 = yaml_bytes(lines), yaml_bytes(out)
    print("SIZE %s %s -> %s%s" % (rel, b0, b1, "" if b0 == b1 else "  (DIFFERENT)"))
    dst = path if out_root is None else os.path.join(out_root, rel)
    if out_root is not None:
        os.makedirs(os.path.dirname(dst), exist_ok=True)
    elif changed and not os.path.exists(path + ".ko_retail"):
        # unspliced yaml (boss/mar): keep the untouched copy; the build restores it before psp_script reads it
        with open(path + ".ko_retail", "w", encoding="utf-8", newline="") as f:
            f.write("\n".join(lines))
    with open(dst, "w", encoding="utf-8", newline="") as f:
        f.write("\n".join(out))
    return rows, changed


# ---------------------------------------------------------------- mar header
def split_top(body):
    toks, depth, cur, inch, esc = [], 0, "", False, False
    for ch in body:
        if inch:
            cur += ch
            if esc:
                esc = False
            elif ch == "\\":
                esc = True
            elif ch == "'":
                inch = False
            continue
        if ch == "'":
            inch = True
            cur += ch
            continue
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            toks.append(cur)
            cur = ""
            continue
        cur += ch
    toks.append(cur)
    return [t.strip() for t in toks if t.strip()]


def tok_op(t):
    m = RE_MACRO.match(t)
    if m:
        return (m.group(1), m.group(2).strip())
    if t.startswith("0x") or (t.startswith("'") and t.endswith("'")):
        return ("BYTE", t)
    return None


def retime_mar(hdr, lanes, out_dir, check):
    src = open(hdr, encoding="utf-8").read()
    rows, changed = [], False
    for arr in MAR_ARRAYS:
        m = re.search(r"(u8 " + arr + r"\[\] MOD_RO = \{\n#ifdef MOD_KO\n)(.*?)(\n#else)", src, re.S)
        if not m:
            # this copy of the header has no KO branch (gen_ko_mar.py patches only its own REPO's copy)
            print("gen_ko_timing: skip %s: no MOD_KO branch in %s" % (arr, hdr))
            continue
        toks = split_top(m.group(2))
        ops = [tok_op(t) for t in toks]
        rep, newspeed, inserts = plan(ops, lanes, check)
        rows += [("mar:" + arr,) + r for r in rep]
        if check or not (newspeed or inserts):
            continue
        out = []
        for i, t in enumerate(toks):
            for n in inserts.get(i, []):
                out.append("SET_WAIT(%d)" % n)
            if i in newspeed:
                t = re.sub(r"SET_SPEED\(\s*\d+\s*\)", "SET_SPEED(%d)" % newspeed[i], t)
            out.append(t)
        for n in inserts.get(len(toks), []):
            out.append("SET_WAIT(%d)" % n)
        lines, line = [], []
        for t in out:
            line.append(t)
            if len(line) >= 12:
                lines.append("    " + ", ".join(line) + ",")
                line = []
        if line:
            lines.append("    " + ", ".join(line))
        block = "\n" + "\n".join(lines)
        src = src[:m.start(2)] + block + src[m.end(2):]
        changed = True
    if not check and (changed or out_dir):
        dst = hdr if out_dir is None else os.path.join(out_dir, os.path.basename(hdr))
        if out_dir is not None:
            os.makedirs(out_dir, exist_ok=True)
        with open(dst, "w", encoding="utf-8", newline="") as f:
            f.write(src)
    return rows, changed


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--mar", action="store_true")
    ap.add_argument("--root", default=os.getcwd())
    ap.add_argument("--hdr")
    a = ap.parse_args()
    global ROOT
    ROOT = os.path.abspath(a.root)
    lanes = lane_frames(a.root)
    rows, nchg = [], 0
    if a.mar:
        hdr = a.hdr or os.path.join(a.root, MAR_HDR)
        r, ch = retime_mar(hdr, lanes, a.out, a.check)
        rows += r
        nchg += 1 if ch else 0
    else:
        for rel in FILES:
            p = os.path.join(a.root, rel)
            if not os.path.exists(p):
                print("skip (missing):", rel)
                continue
            r, ch = retime_yaml(p, rel, lanes, a.out, a.check)
            rows += r
            nchg += 1 if ch else 0
    nshort = sum(1 for r in rows if r[5] == "short")
    nolane = sum(1 for r in rows if r[5] == "nolane")
    print("windows %d, short before %d, no-lane %d, files changed %d" % (len(rows), nshort, nolane, nchg))
    for r in rows:
        if r[5] in ("short", "nolane"):
            print(r[5].upper(), r[0], "window", r[1], ",".join(r[2]), "frames", r[3], "target", r[4])
    if a.out:
        os.makedirs(a.out, exist_ok=True)
        with open(os.path.join(a.out, "gen_ko_timing_report%s.tsv" % ("_mar" if a.mar else "")), "w", encoding="utf-8") as f:
            f.write("file\twindow\tids\tframes_before\ttarget\tstatus\n")
            for r in rows:
                f.write("%s\t%d\t%s\t%d\t%d\t%s\n" % (r[0], r[1], "+".join(r[2]), r[3], r[4], r[5]))
    return 1 if (a.check and (nshort or nolane)) else 0


if __name__ == "__main__":
    sys.exit(main())
