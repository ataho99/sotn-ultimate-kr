#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
# ko_entry_offsets.py -- the ONE table of C-side cutscene script entry offsets
# (fixCUT, final-battle freeze).
#
# The PS1 C code enters some cutscene scripts in the MIDDLE, at a hardcoded
# byte offset from the script's start (RCEN: SetCutsceneScript(cutscene_script
# + 0x129) = Shaft's pre-fight lines, after the post-fight ones). Any KO tool
# that changes the byte length of the part before such an offset (the splice
# gen_ko_splice.py, the retiming gen_ko_timing.py, ...) moves the command the
# C code expects there: the interpreter then starts inside text or inside an
# op's arguments -> freeze. Every tool that rewrites a script must keep each
# offset below on the SAME command as retail (pad / pay with the 0x0D no-op
# pads only inside one segment, never across an entry).
#
# Shared by gen_ko_splice.py (pads to every entry, fail closed) and meant for
# gen_ko_timing.py (entry barrier: a SET_WAIT before an entry must be paid by
# a pad run that also lies before it). psp_script.py OVERLAYS "split" holds
# the same two values for the PSP copies; check_psp_split() keeps them equal.
#
#   ko_entry_offsets.py --table      print the table
#   ko_entry_offsets.py --check      verify every (spliced / retimed) yaml on
#                                    disk and build/us/ko_boss_splice.json:
#                                    each entry lands on a command boundary
#                                    and holds the retail command; exit 1 if not
import json
import os
import re
import sys

BASE = 0x80180000

# Script entered by the C code at a byte offset > 0, keyed by the yaml asset
# (repo-relative, as gen_ko_timing.FILES / psp_script.OVERLAYS spell it).
# Found by grepping src/ for SetCutsceneScript(<script> + <const>),
# "scriptCur = <script> + <const>" and pointers to the middle of a script
# array (2026-11-19, ko c77d4c3). PS1 (VERSION_US) paths only; the PSP
# builds' pointers (D_pspeu_*, *_script_ptr*, cutscene_data_offset_eight)
# are not compiled into the PS1 overlays.
ENTRY_OFFSETS = {
    "assets/st/rcen/cutscene_script_psx.yaml": {
        0x129: "src/st/rcen/cutscene.c:266 SetCutsceneScript(cutscene_script + 0x129)"
               " -- params 0 = pre-fight Shaft (post-fight lines are 0..0x128)",
    },
    "assets/st/dre/cutscene_script_psx.yaml": {
        0x49D: "src/st/dre/e_cutscene_dialogue.c:270 SetCutsceneScript(cutscene_script + 0x49D)",
    },
}

# Entries that are a SEPARATE yaml but one array in the US link (each yaml is
# size-locked on its own, so the entry holds as long as the left one keeps
# its retail size; listed for the table / check only).
LINKED_ENTRIES = {
    "assets/st/no4/cutscene_data_ferryman_right_psx.yaml": (
        "assets/st/no4/cutscene_data_ferryman_left_psx.yaml", 0x63,
        "src/st/no4/cutscene.c:246 SetCutsceneScript(cutscene_data_ferryman_right)"
        " = cutscene_data_ferryman_left + 0x63 (0x801853B7)"),
}

# Boss overlays whose retail dialogue entity is asm (INCLUDE_ASM): the scripts
# are raw .data, entered at these absolute addresses (SetCutsceneScript args
# in asm/us/boss/*/nonmatchings, as psp_script.OVERLAYS). When two lie in one
# contiguous script region the later one is an entry offset of that region
# (gen_ko_splice.boss_regions works this out from the retail BIN).
BOSS_SCRIPTS = {
    "bo2": ("BOSS/BO2/BO2.BIN", "bobo2", [0x80181CCC, 0x80181DC5],
            "src/boss/bo2/e_cutscene_dialogue.c (asm EntityCutsceneDialogue)"),
    "bo5": ("BOSS/BO5/BO5.BIN", "bobo5", [0x80181D5C],
            "src/boss/bo5/e_cutscene_dialogue.c (MOD_KO: C EntityCutscene, ko_boss_dialogue.h)"),
    "bo6": ("BOSS/BO6/BO6.BIN", "bobo6", [0x8019A0D0, 0x8019A2E1, 0x8019A2FB],
            "src/boss/bo6/cutscene.c (MOD_KO: C func_us_801A7DC0, ko_boss_dialogue.h); BO6RIC2.BIN = copy"),
    "rbo2": ("BOSS/RBO2/RBO2.BIN", "borbo2", [0x80181878],
             "src/boss/rbo2/e_cutscene_dialogue.c (asm EntityCutsceneDialogue)"),
    "rbo6": ("BOSS/RBO6/RBO6.BIN", "borbo6", [0x80181720, 0x801819E7],
             "src/boss/rbo6/e_cutscene_dialogue.c (MOD_KO: C EntityCutscene, ko_boss_dialogue.h)"),
}
# fixBOSS: boss overlays whose MOD_KO build draws the dialogue with the KO
# glyph path (src/boss/ko_boss_dialogue.h: engtab / promotion / F0..F3 cells,
# 16 px line buffers). Only these may take KO rows; the others (still the
# retail asm renderer, 8x8 font only) need KO_BOSS_ASM_RENDER=1.
BOSS_KO_RENDER = {"bo2", "bo5", "bo6", "rbo2", "rbo6"}


def offsets_for(rel):
    """sorted entry offsets (> 0) of the yaml asset rel"""
    rel = rel.replace("\\", "/")
    if rel.endswith(".ko_retail"):
        rel = rel[:-len(".ko_retail")]
    return sorted(ENTRY_OFFSETS.get(rel, {}))


def row_offsets(rows, row_size):
    """byte offset of each row (rows = gen_ko_splice.split_rows output)"""
    out, acc = [], 0
    for kind, pay, raw in rows:
        out.append(acc)
        acc += row_size(kind, pay)
    out.append(acc)
    return out


def entry_rows(rows, row_size, offsets, label=""):
    """{offset: index of the first non-empty row at that offset}; ValueError
    when an offset falls inside a command (retail must be a boundary)"""
    offs = row_offsets(rows, row_size)
    res = {}
    for e in offsets:
        cand = [i for i in range(len(rows)) if offs[i] == e and row_size(rows[i][0], rows[i][1]) > 0]
        if not cand:
            raise ValueError("%s: entry +0x%X is not on a command boundary" % (label, e))
        res[e] = cand[0]
    return res


def verify(retail_rows, new_rows, row_size, offsets, label=""):
    """errors (list of str): total size, and each entry on a boundary holding
    the retail command (same op and args)"""
    errs = []
    ro, no = row_offsets(retail_rows, row_size), row_offsets(new_rows, row_size)
    if ro[-1] != no[-1]:
        errs.append("%s: size %d != retail %d" % (label, no[-1], ro[-1]))
    try:
        er = entry_rows(retail_rows, row_size, offsets, label + " (retail)")
    except ValueError as ex:
        return errs + [str(ex)]
    for e, ri in er.items():
        try:
            ni = entry_rows(new_rows, row_size, [e], label)[e]
        except ValueError as ex:
            errs.append(str(ex))
            continue
        rk, rp, _ = retail_rows[ri]
        nk, np_, _ = new_rows[ni]
        if rk in ("TEXT", "BYTE"):
            continue  # (retail enters at text: a boundary is all one can ask)
        if (rk, (rp or "").replace(" ", "")) != (nk, (np_ or "").replace(" ", "")):
            errs.append("%s: +0x%X holds [%s %s], retail [%s %s]" % (label, e, nk, np_, rk, rp))
    return errs


# --------------------------------------------------------------- raw bytes
def _is_word(q):
    """q (4 bytes) is a script address word (psp_script.word/unword)"""
    v = 0
    for x in q:
        v = (v << 4) | x
    v &= 0xFFFFF
    return bytes([(v & 0xFF000) >> 12, (v & 0xFF00) >> 8, (v & 0xFF0) >> 4, v & 0xFF]) == bytes(q)


def _op_len(b, i):
    """length of the command at b[i] (KO text aware): F0..F3 = 2-byte glyph,
    0x20.. = 1-byte char / promoted glyph, < 0x20 = op (psp_script.OPS)"""
    c = b[i]
    if 0xF0 <= c <= 0xF3:
        return 2
    if c >= 0x20:
        return 1
    if c == 14:  # SCRIPT_SWITCH: the variable + cases (psp_script.disassemble's rule)
        k = i + 1
        while k + 4 <= len(b) and 0x80 <= b[k] < 0xC0 and _is_word(b[k:k + 4]):
            k += 4
        return k - i
    sizes = {0: 0, 1: 0, 2: 1, 3: 1, 4: 0, 5: 2, 6: 0, 7: 2, 8: 0, 9: 2, 10: 0, 11: 0, 12: 4,
             13: 0, 15: 4, 16: 1, 17: 1, 18: 0, 19: 5, 20: 2, 21: 0, 22: 1, 23: 0, 24: 1}
    return 1 + sizes[c]


def byte_boundaries(b, start=0, stop=None):
    """set of command start offsets walking b[start:stop]"""
    stop = len(b) if stop is None else stop
    out, i = set(), start
    while i < stop:
        out.add(i)
        i += _op_len(b, i)
    return out


def verify_bytes(old, new, entries, label=""):
    errs = []
    if len(old) != len(new):
        errs.append("%s: size %d != retail %d" % (label, len(new), len(old)))
    bo, bn = byte_boundaries(old), byte_boundaries(new)
    for e in entries:
        if e not in bo:
            errs.append("%s: retail +0x%X is not a boundary (table wrong?)" % (label, e))
        elif e not in bn:
            errs.append("%s: +0x%X is inside a command" % (label, e))
        elif old[e:e + _op_len(old, e)] != new[e:e + _op_len(new, e)] and old[e] < 0x20:
            errs.append("%s: +0x%X op %02X != retail %02X" % (label, e, new[e], old[e]))
    return errs


# --------------------------------------------------------------- CLI
def check_psp_split(root):
    """psp_script.OVERLAYS split values == this table"""
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import psp_script as P
    errs = []
    for ovl, (_, _, scripts) in P.OVERLAYS.items():
        for sc in scripts:
            if "yaml" in sc:
                if sorted(sc.get("split", [])) != offsets_for(sc["yaml"]):
                    errs.append("psp_script %s %s split %s != table %s" % (
                        ovl, sc["yaml"], sc.get("split", []), offsets_for(sc["yaml"])))
    return errs


def check(root):
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    os.environ.setdefault("SOTN_KR", root)
    import gen_ko_splice as S
    errs, n = [], 0
    for rel in sorted(set(ENTRY_OFFSETS) | set(LINKED_ENTRIES) |
                      set(v[0] for v in LINKED_ENTRIES.values())):
        path = os.path.join(root, rel)
        bak = path + ".ko_retail"
        if not os.path.exists(path):
            print("skip (not extracted): " + rel)
            continue
        cur = [l for l in open(path, encoding="utf-8").read().split("\n") if l.strip()]
        ret = cur
        if os.path.exists(bak):
            ret = [l for l in open(bak, encoding="utf-8").read().split("\n") if l.strip()]
        e = verify(S.split_rows(ret), S.split_rows(cur), S._row_size, offsets_for(rel), rel)
        errs += e
        n += 1
        print("%s %s entries %s" % ("FAIL" if e else "OK  ", rel,
                                    " ".join("+0x%X" % x for x in offsets_for(rel)) or "-"))
    plan = os.path.join(root, "build", "us", "ko_boss_splice.json")
    if os.path.exists(plan):
        for p in json.load(open(plan)):
            e = verify_bytes(bytes.fromhex(p["old"]), bytes.fromhex(p["new"]),
                             [0] + p.get("entries", []), "%s@0x%X" % (p["bin"], p["off"]))
            errs += e
            n += 1
            print("%s %s@0x%X entries %s" % ("FAIL" if e else "OK  ", p["bin"], p["off"],
                                             " ".join("+0x%X" % x for x in p.get("entries", [])) or "-"))
    try:
        errs += check_psp_split(root)
    except Exception as ex:  # psp_script needs nothing at import; report, do not mask
        errs.append("psp_script split check: %s" % ex)
    for x in errs:
        print("ERROR " + x)
    print("ko_entry_offsets: %d scripts checked, %d errors" % (n, len(errs)))
    return 1 if errs else 0


def table():
    print("script\toffset\twhere")
    for rel, d in sorted(ENTRY_OFFSETS.items()):
        for off, why in sorted(d.items()):
            print("%s\t+0x%X\t%s" % (rel, off, why))
    for rel, (base, off, why) in sorted(LINKED_ENTRIES.items()):
        print("%s\t%s +0x%X\t%s" % (rel, base.rsplit("/", 1)[-1], off, why))
    for ovl, (binp, _, addrs, why) in sorted(BOSS_SCRIPTS.items()):
        for a in addrs:
            print("%s\t0x%X (BIN +0x%X)\t%s" % (binp, a, a - BASE, why))


if __name__ == "__main__":
    root = os.environ.get("SOTN_KR") or os.path.abspath(
        os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
    if "--root" in sys.argv:
        root = sys.argv[sys.argv.index("--root") + 1]
    if "--table" in sys.argv:
        table()
        sys.exit(0)
    sys.exit(check(root))
