#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
# gen_ko_splice.py -- KO trial: splice v13 cutscene KO bytes into extracted
# cutscene yaml assets (trial builds only).
#
# Background: stage cutscene scripts are extractor products -- retail BIN ->
# assets/st/<stage>/*.yaml -> src/st/<stage>/gen/*.h (u8 arrays in .data).
# Dialogue is re-extracted every build, so hand edits are forbidden; this
# tool re-derives the KO yaml from translation + retail yaml every build.
#
# Method per stage (SEL 0B案, 조사_dlg127_1111 §4 안1):
#   walk yaml TEXT rows, group by LINE_BREAK slots, match each TSV key's EN
#   segments (strict assert, fail closed), replace first TEXT row of each
#   non-empty group with single-byte [BYTE, 0x..] rows of KO text, emptied
#   groups -> [TEXT, ""], control ops untouched. KO shorter than EN, so pad
#   the script end with 0x00 BYTE rows to the identical total size (.data
#   layout unchanged). Register (overlay, symbol, size) into
#   build/us/mod_data_edits.json (merged, like gen_card_layouts.py) for
#   check_stage_data.py.
# Idempotent: first run backs up retail yaml to *.ko_retail; every run
# restores from backup, then splices. Refuses if yaml looks spliced but no
# backup exists.
# Excluded: prologue/opening (separate mechanisms). See 조사_t15_1111.md.
#
# Cutscene fixes (2026-11-19):
#  * C-side entry offsets (ko_entry_offsets.py, e.g. RCEN cutscene_script +
#    0x129 = Shaft's pre-fight lines): the 0x0D pads are also placed at every
#    entry, so the part before it ends exactly at the retail offset (no KO row
#    may straddle an entry; KO longer than EN before an entry = error). The
#    result is re-verified (ko_entry_offsets.verify) before it is written.
#  * boss overlays: MAR (yaml, C renderer with the KO glyph path) and the
#    asm-rendered bo2/bo5/bo6/rbo2/rbo6 (scripts addressed like psp_script
#    OVERLAYS "addr": spliced from the retail BIN into
#    build/us/ko_boss_splice.json, written into the linked BINs by
#    --apply-boss, run before psp_script --apply and build_bo6ric2.py).
#    Their TSV keys (mar_*, bo2_* ...) are OPTIONAL: no rows / empty
#    translation = the retail bytes stay (placeholder). --placeholders OUT
#    writes the rows translators fill (+ OUT.ref.tsv: BIN offset, voice id,
#    retail EN per row, local reference only).
#  * the asm renderers have no KO glyph path: a non-empty KO row for them
#    stops the build unless KO_BOSS_ASM_RENDER=1 (set it once the asm
#    entities are decompiled / hooked). bo5/bo6/rbo6 have a C
#    entity with the KO path under MOD_KO (src/boss/ko_boss_dialogue.h,
#    ko_entry_offsets.BOSS_KO_RENDER) and take KO rows; bo2/rbo2 still gated.
import io
from ko_paths import KO_WK, KO_REPO  # noqa: E402
import json
import os
import re
import sys

REPO = os.environ.get("SOTN_KR", KO_REPO)
WK = KO_WK
TSV = os.environ.get("KO_TSV", (KO_WK + "/검수/번역_v14_%s_%d줄.tsv") % (__import__("ko_font_profile").ACTIVE, __import__("ko_font_profile").P["window_lines"]))  # gen_ko_wrap.py output ("/" line, "|" window)
MAP = os.path.join(WK, "P4시험", "" + __import__("ko_font_profile").P["map_name"] + "")
CONFIG = os.path.join(REPO, "config", "assets.us.yaml")
EDITS_PATH = os.path.join(REPO, "build", "us", "mod_data_edits.json")

# (yaml asset, overlay, C symbol, TSV prefixes)
STAGES = [
    ("st/no3/cutscene_data.yaml", "stno3", "cutscene_data", ["no3_death"]),
    ("st/cen/cutscene_data.yaml", "stcen", "cutscene_data", ["cen_maria"]),
    ("st/lib/cutscene_data.yaml", "stlib", "cutscene_data", ["lib_librarian"]),
    ("st/nz0/cutscene_data.yaml", "stnz0", "cutscene_data", ["nz0_slogra_gaibon"]),
    ("st/sel/cutscene_endings.yaml", "stsel", "cutscene_endings", ["sel_ending"]),
    ("st/top/cutscene_data.yaml", "sttop", "cutscene_data", ["top_maria_richter"]),
    ("st/st0/cutscene_script_psx.yaml", "stst0", "cutscene_script",
     ["st0_richter_dracula"]),
    # no4 (ferryman): left EN 60B -> KO 38B (2 segs, pad 22),
    # right EN 36B -> KO 34B (2 segs, pad 2; TSV trims 4B).
    # US links L+R as ONE array (cutscene_data_ferryman_left, 0xAC) with the
    # right half at absolute 0x801853B7, so each half keeps its own total.
    ("st/no4/cutscene_data_ferryman_left_psx.yaml", "stno4",
     "cutscene_data_ferryman_left", ["no4_ferryman_left"]),
    ("st/no4/cutscene_data_ferryman_right_psx.yaml", "stno4",
     "cutscene_data_ferryman_left", ["no4_ferryman_right"]),
    ("st/rcen/cutscene_script_psx.yaml", "strcen", "cutscene_script",
     ["rcen_shaft"]),
    ("st/dai/cutscene_script_psx.yaml", "stdai", "cutscene_script",
     ["dai_huntinggirl"]),
    ("st/dre/cutscene_script_psx.yaml", "stdre", "cutscene_script",
     ["dre_succubus"]),
    # Maria (boss MAR, Clock Tower). Optional: keys mar_maria:N.
    ("boss/mar/cutscene_data.yaml", "bomar", "cutscene_data", ["mar_maria"]),
]
# stages whose TSV keys may be absent / untranslated (placeholders)
OPTIONAL = {"bomar"}

# asm-rendered boss overlays (ko_entry_offsets.BOSS_SCRIPTS);
# key prefix per script region = "<ovl>_<BIN offset hex>" (e.g. bo2_1ccc:N)
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ko_entry_offsets as EO  # noqa: E402
BOSS_PLAN = os.path.join(REPO, "build", "us", "ko_boss_splice.json")
BOSS_ASM_OK = os.environ.get("KO_BOSS_ASM_RENDER") == "1"

MARK = "# KO-SPLICE trial15"


def load_map():
    mp = {}
    for line in io.open(MAP, encoding="utf-8"):
        p = line.rstrip("\n").split("\t")
        if len(p) >= 2 and len(p[0]) == 1 and p[1].startswith("0x"):
            mp[p[0]] = int(p[1], 16)
    assert len(mp) >= 700, len(mp)  # glyph-map size sanity check
    mp["…"] = 0xF1A0  # v12f new cell (12px full-width ellipsis)
    return mp


PROMO_MAP = os.path.join(WK, "P4시험", "ko_promo_map.tsv")


def load_promo():
    """1-byte promotion (gen_ko_promo.py): char -> byte 0x80..0xEF."""
    pr = {}
    for line in io.open(PROMO_MAP, encoding="utf-8"):
        p = line.rstrip(chr(10)).split(chr(9))
        if len(p) >= 3 and p[1].startswith("0x"):
            pr[p[0]] = int(p[1], 16)
    assert 0 < len(pr) <= 112, len(pr)
    assert all(0x80 <= v <= 0xEF for v in pr.values())
    return pr


PROMO = load_promo()


def ell(s):
    """9 dots (silence) -> two ellipsis cells, 3 dots -> one cell."""
    return s.replace(".........", "……").replace("...", "…")


def enc(s, mp):
    out = bytearray()
    for ch in ell(s.replace("…", "...")):
        o = ord(ch)
        if o < 128:
            out.append(o)
        elif ch in PROMO:
            out.append(PROMO[ch])
        else:
            assert ch in mp, repr(ch)
            out.append(mp[ch] >> 8)
            out.append(mp[ch] & 0xFF)
    return bytes(out)


TAIL_SEP = chr(0x1F)  # base row KO text + TAIL_SEP + tail text: the tail is the text of the window behind the last slot (portrait/voice op between the slot's text rows)


def load_tsv():
    rows = [l.rstrip("\n").split("\t") for l in io.open(TSV, encoding="utf-8")]
    h = rows[0]
    ki, ci, ei, ti = h.index("키"), h.index("분류"), h.index("EN"), [i for i, x in enumerate(h) if x.startswith("번역")][0]
    d = {}
    for r in rows[1:]:
        if len(r) <= ti or r[ci] != "대사":
            continue
        d[r[0]] = (r[ei], r[ti].replace("…", "...").replace("\\n", "/"))
    # v14 split rows (key suffix 'b', e.g. rcen_shaft:5b): in the game script the
    # original text is ONE slot, so the split row is folded back into its base
    # row as extra KO lines (extra LINE_BREAK path in splice_stage).
    for k in [k for k in d if re.match(r".*:[0-9]+b$", k)]:
        base = k[:-1]
        assert base in d, k
        if d[k][1].strip() != "":
            if k in ("rcen_shaft:5b",):  # own window: the text goes to the last window of the base row's last slot (TAIL_SEP, see splice_rows); keep in step with gen_ko_wrap.B_SEPARATE
                d[base] = (d[base][0], d[base][1] + TAIL_SEP + d[k][1])
            else:
                d[base] = (d[base][0], d[base][1] + "/" + d[k][1])
        del d[k]
    return d


ALLOW_PATH = os.path.join(REPO, "build", "us", "mod_ko_psp_allow.json")


def config_sizes():
    """(asset_path, name) -> (start, size); also bin filename per asset_path.
    from config/assets.us.yaml."""
    cur = None
    tgt = None
    pending = {}
    sizes = {}
    bins = {}
    for line in io.open(CONFIG, encoding="utf-8"):
        m = re.match(r"\s*(?:-\s*)?target:\s*(\S+)", line)
        if m:
            tgt = m.group(1)
            continue
        m = re.match(r"\s*asset_path:\s*(\S+)", line)
        if m:
            if cur and pending:
                items = pending["items"]
                for i, (off, typ, name) in enumerate(items):
                    if typ == "cutscene" and i + 1 < len(items):
                        sizes[(pending["path"], name)] = (off, items[i + 1][0] - off)
                        bins[(pending["path"], name)] = pending["tgt"]
            cur = m.group(1)
            pending = {"path": cur, "tgt": tgt, "items": []}
            continue
        m = re.match(r"\s*- \[(0x[0-9A-Fa-f]+),\s*([A-Za-z0-9_]+)(,\s*([A-Za-z0-9_]+))?", line)
        if m and pending is not None:
            pending["items"].append(
                (int(m.group(1), 16), m.group(2), m.group(4)))
    if cur and pending:
        items = pending["items"]
        for i, (off, typ, name) in enumerate(items):
            if typ == "cutscene" and i + 1 < len(items):
                sizes[(pending["path"], name)] = (off, items[i + 1][0] - off)
                bins[(pending["path"], name)] = pending["tgt"]
    return sizes, bins



# handler-accurate row sizes (matches psp_script.assemble for these rows)
_OPS = {
    "END_CUTSCENE": (0, []), "LINE_BREAK": (1, []), "SET_SPEED": (2, [1]),
    "SET_WAIT": (3, [1]), "HIDE_DIALOG": (4, []), "SET_PORTRAIT": (5, [1, 1]),
    "NEXT_DIALOG": (6, []), "SET_POS": (7, [1, 1]), "CLOSE_DIALOG": (8, []),
    "PLAY_SOUND": (9, [2]), "WAIT_FOR_SOUND": (10, []),
    "SCRIPT_UNKNOWN_11": (11, []), "SET_EVENTS": (12, [4]),
    "SCRIPT_UNKNOWN_13": (13, []), "SCRIPT_SWITCH": (14, None),
    "SCRIPT_UNKNOWN_15": (15, [4]), "WAIT_FOR_FLAG": (16, [1]),
    "SET_FLAG": (17, [1]), "SCRIPT_UNKNOWN_18": (18, []),
    "LOAD_PORTRAIT": (19, [4, 1]), "SCRIPT_UNKNOWN_20": (20, [2]),
    "SCRIPT_UNKNOWN_21": (21, []), "SCRIPT_UNKNOWN_22": (22, [1]),
    # The extractor (tools/sotn-assets cutscene/handler.go) names op
    # 22 RESET_FLAG with a 1-byte arg (MAR's script has it); was missing
    "RESET_FLAG": (22, [1]),
    "SCRIPT_UNKNOWN_23": (23, []), "WAIT_FOR_FLAG_RESET": (24, [1]),
}


def _text_len(pay):
    """script bytes of a yaml TEXT payload (handler.go writes \\xNN escapes)"""
    return len(re.sub(r"\\x[0-9A-Fa-f]{2}", "x", pay))


def _row_size(kind, pay):
    if kind in ("RAW", "TEXT"):
        return _text_len(pay) if kind == "TEXT" else 0
    if kind == "BYTE":
        return 1
    if kind == "SCRIPT_SWITCH":
        args = [a for a in pay.split(",") if a.strip()] if pay.strip() else []
        return 1 + 4 * len(args)
    code, params = _OPS[kind]
    args = [a.strip() for a in pay.split(",")] if pay.strip() else []
    n = 1
    for size, a in zip(params, args):
        n += 1 if size == 1 else (2 if size == 2 else 4)
    return n


def _intra_targets(rows, start, size):
    """row indexes that absolute intra-script code addresses point at."""
    offs = []
    acc = 0
    for i, (k, p, r) in enumerate(rows):
        offs.append(acc)
        acc += _row_size(k, p)
    out = set()
    for i, (k, p, r) in enumerate(rows):
        if k == "SCRIPT_SWITCH":
            for a in p.split(","):
                a = a.strip()
                try:
                    v = int(a, 0)
                except ValueError:
                    continue
                if 0x80180000 + start <= v < 0x80180000 + start + size and i > 0:
                    bo = v - 0x80180000 - start
                    assert bo in offs, (hex(v), bo)
                    out.add(offs.index(bo))
        elif k == "SCRIPT_UNKNOWN_15":
            try:
                v = int(p.strip(), 0)
            except ValueError:
                continue
            if 0x80180000 + start <= v < 0x80180000 + start + size:
                bo = v - 0x80180000 - start
                assert bo in offs, (hex(v), bo)
                out.add(offs.index(bo))
    return sorted(out)

def split_rows(lines):
    """Parse yaml script rows -> list of (kind, payload, rawline)."""
    out = []
    for ln in lines:
        m = re.match(r"(\s*- \[)([A-Z_0-9]+)(,\s*(.*))?(\]\s*)$", ln.rstrip("\n"))
        if not m:
            out.append(("RAW", None, ln))
            continue
        op, rest = m.group(2), m.group(4) or ""
        if op == "TEXT":
            tm = re.match(r'^"(.*)"$', rest)
            assert tm is not None, ln
            out.append(("TEXT", tm.group(1), ln))
        else:
            out.append((op, rest, ln))
    return out


EXTRA_LB = []
WAITS = []
WINDOWS = []
WARN = []
DROPPED_LB = []


class Untranslated(Exception):
    """optional stage: no TSV rows / no translated row -> keep retail"""


def _read_yaml_retail(asset):
    """(path, retail text): the retail yaml, from the .ko_retail backup when
    there is one (every run restores from it), else the yaml on disk (backed
    up first)."""
    path = os.path.join(REPO, "assets", asset)
    bak = path + ".ko_retail"
    cur = io.open(path, encoding="utf-8").read()
    if os.path.exists(bak):
        cur = io.open(bak, encoding="utf-8").read()
    else:
        assert MARK not in cur, "spliced yaml without backup: " + asset
        io.open(bak, "w", encoding="utf-8", newline="").write(cur)
    return path, cur


def splice_stage(asset, prefixes, tsv, mp, optional=False):
    path, cur = _read_yaml_retail(asset)
    lines = cur.split("\n")
    rows = split_rows([l for l in lines if l.strip() != ""])
    nm0 = asset.split("/")[-1].rsplit(".", 1)[0]
    key0 = ("assets/" + asset.rsplit("/", 1)[0], nm0)
    start0, size0 = config_sizes()[0][key0]
    entries = EO.offsets_for("assets/" + asset)
    try:
        out, stats = splice_rows(asset, rows, start0, size0, entries, prefixes, tsv, mp, optional)
    except Untranslated:
        # placeholder: the retail yaml stays (undo an older splice)
        io.open(path, "w", encoding="utf-8", newline="").write(cur)
        return None
    errs = EO.verify(rows, split_rows(out), _row_size, entries, asset)
    assert not errs, errs
    io.open(path, "w", encoding="utf-8", newline="").write("\n".join(out) + "\n")
    return stats


def splice_rows(label, rows, start0, size0, entries, prefixes, tsv, mp, optional=False):
    """KO rows for one script (rows = split_rows of the retail script):
    -> (output yaml lines, (EN bytes, KO bytes, pad, keys)). entries = the
    C code's entry offsets: pads go there too, so each lands on its retail
    command (ko_entry_offsets)."""
    intra_targets = _intra_targets(rows, start0, size0)
    entry_row = EO.entry_rows(rows, _row_size, entries, label)  # retail boundary, else error
    # group TEXT rows into slots: a slot ends at LINE_BREAK or at a
    # box-terminating op (NEXT_DIALOG / CLOSE_DIALOG / END_CUTSCENE --
    # the next voice box starts without a LINE_BREAK)
    BOX_END = ("LINE_BREAK", "NEXT_DIALOG", "CLOSE_DIALOG", "END_CUTSCENE")
    slots = []
    slot_end = []
    cur_slot = []
    for idx, (kind, pay, raw) in enumerate(rows):
        if kind == "TEXT":
            cur_slot.append(idx)
        elif kind in BOX_END:
            if cur_slot:
                slots.append(cur_slot)
                slot_end.append(idx)
                cur_slot = []
    if cur_slot:
        slots.append(cur_slot)
        slot_end.append(len(rows))
    text_of = {}
    for idx, (kind, pay, raw) in enumerate(rows):
        if kind == "TEXT":
            text_of[idx] = pay
    # TSV rows in file order
    keys = sorted([k for k in tsv if k.rsplit(":", 1)[0] in prefixes],
                  key=lambda k: int(k.rsplit(":", 1)[1]))
    if optional and not any(tsv[k][1].strip() for k in keys):
        raise Untranslated(label)
    assert keys, label
    # EN source: legacy text column, or "#n1/n2/.." = slot count per EN segment
    # (public TSV: no original text) -> original EN rebuilt from this yaml's slots
    cnt_mode = [tsv[k][0].startswith("#") for k in keys]
    assert all(cnt_mode) or not any(cnt_mode), (label, "mixed EN counts/text")
    en_res = {}
    if all(cnt_mode):
        csi = 0
        for k in keys:
            segs = []
            for c in tsv[k][0][1:].split("/"):
                n = int(c)
                segs.append("".join(text_of[tix] for slot in slots[csi:csi + n] for tix in slot))
                csi += n
            en_res[k] = "/".join(segs)
    else:
        for k in keys:
            en_res[k] = tsv[k][0].replace("\\n", "/")
    # align: EN segs (split \\n of original EN i.e. '/' of stored) to slots
    en_segs = []
    for k in keys:
        en = en_res[k]
        for j, s in enumerate(en.split("/")):
            en_segs.append((k, j, s))
    si = 0
    seg_slot = {}
    seg_end = {}
    for (k, j, s) in en_segs:
        acc = ""
        n = 0
        while si + n < len(slots) and len(acc) < len(s):
            for tix in slots[si + n]:
                acc += text_of[tix]
            n += 1
            if acc == s:
                break
        assert acc == s, (label, k, j, repr(s), repr(acc))
        seg_slot[(k, j)] = slots[si:si + n]
        seg_end[(k, j)] = slot_end[si + n - 1] if n else None
        si += n
    _dump = os.environ.get("KO_DUMP_EN")
    if _dump and not all(cnt_mode):
        with io.open(_dump, "a", encoding="utf-8", newline="") as _f:
            for k in keys:
                _f.write(k + "\t#" + "/".join(str(len(seg_slot[(k, j)])) for j in range(len(en_res[k].split("/")))) + "\n")
    # KO distribution: KO segs fill non-empty groups in order (translators
    # reflowed lines; order preserved). ABSOLUTE intra-script addresses
    # (SCRIPT_SWITCH cases, UNKNOWN_15 gotos) and the C code's entry offsets
    # must keep exact offsets, so 0x0D (CSOP_SCRIPT_UNKNOWN_13 = 13 decimal)
    # no-op pads are inserted before every such row (and at the end), sized
    # to the cumulative EN-KO deficit at that point. 0x0D is a no-op in every
    # stage interpreter (CSOP_SCRIPT_UNKNOWN_13: continue).
    new_row = {}
    op_repl = {}
    en_total = ko_total = 0
    entry_set = sorted(entry_row.values())
    for k in keys:
        en = en_res[k]
        ko = tsv[k][1]
        tail_ko = None
        if TAIL_SEP in ko:
            ko, tail_ko = ko.split(TAIL_SEP, 1)
        es = en.split("/")
        groups = []
        gend = []
        for j, s in enumerate(es):
            if s == "":
                continue
            grp = seg_slot[(k, j)]
            assert len(grp) > 0, (label, k, j)
            groups.append(grp)
            gend.append(seg_end[(k, j)])
        en_total += sum(_text_len(x) for x in es if x != "")
        if optional and ko.strip() == "":
            # placeholder row: its retail TEXT rows stay as they are
            ko_total += sum(_text_len(x) for x in es if x != "")
            continue
        if groups:
            # a KO row must not straddle a C entry (its bytes move as one)
            lo = min(tix for g in groups for slot in g for tix in slot)
            hi = gend[-1] if gend[-1] is not None else len(rows)
            bad = [e for e in entry_set if lo < e < hi]
            assert not bad, (label, k, "row straddles a C entry offset", [hex(o) for o, r in entry_row.items() if r in bad])
        # KO text: "/" = line, "|" = window break (NEXT_DIALOG + speaker re-set)
        knon = []
        winstart = set()
        for wi, wtxt in enumerate(ko.split("|")):
            wl = [x for x in wtxt.split("/") if x != ""]
            if wi > 0 and wl:
                winstart.add(len(knon))
            knon += wl
        # speaker for the re-set after a window break: the SET_PORTRAIT in force
        # at this row's first text row (+ a SET_SPEED given after it, restored
        # because SET_PORTRAIT resets the typing speed)
        first_t = min(tix for slot in groups[0] for tix in slot)
        sp_args = None
        sp_speed = None
        for ri in range(first_t - 1, -1, -1):
            kd, pp, rw = rows[ri]
            if kd == "SET_PORTRAIT" and sp_args is None:
                sp_args = [int(x.strip(), 0) for x in pp.split(",")]
                break
        if sp_args is not None:
            for rj in range(first_t - 1, -1, -1):
                kd, pp, rw = rows[rj]
                if kd == "SET_SPEED":
                    sp_speed = int(pp.strip(), 0)
                    break
                if kd in ("SET_PORTRAIT", "NEXT_DIALOG"):
                    break
        wb_rows = ["  - [NEXT_DIALOG]"]
        wb_bytes = bytes([6])
        if sp_args is not None:
            wb_rows.append("  - [SET_PORTRAIT, %d, %d]" % (sp_args[0], sp_args[1]))
            wb_bytes += bytes([5, sp_args[0], sp_args[1]])
            if sp_speed is not None:
                wb_rows.append("  - [SET_SPEED, %d]" % sp_speed)
                wb_bytes += bytes([2, sp_speed])
        else:
            if winstart:
                WARN.append((k, "no SET_PORTRAIT before row: window break without speaker re-set"))
        WINDOWS.append((k, len(winstart), len(wb_bytes) - 1))
        # v14 follows the JP line breaks: a KO row may have MORE lines than EN.
        # Extra lines are appended to the last group, each behind one raw
        # LINE_BREAK (0x01) byte; the 0x0D pads absorb the extra bytes.
        extra = knon[len(groups):]
        if extra:
            EXTRA_LB.append((k, len(extra)))
        # window boundary between group gi-1 and gi: the slot-ending LINE_BREAK row
        # becomes NEXT_DIALOG (+ SET_PORTRAIT [+ SET_SPEED]); +len(wb_bytes)-1 bytes
        for gi in range(1, min(len(groups), len(knon))):
            if gi in winstart:
                opi = gend[gi - 1]
                assert opi is not None and rows[opi][0] == "LINE_BREAK", (label, k, gi, rows[opi][0] if opi is not None and opi < len(rows) else None)
                op_repl[opi] = (wb_rows, len(wb_bytes) - 1)
        # The EN structure keeps one LINE_BREAK after every group. When the KO row has
        # fewer lines than EN groups those trailing LINE_BREAKs (after the last KO line)
        # would run on as EMPTY lines: nextCharY passes 2, the box goes into scroll mode
        # and pushes the finished text up ~13 px (the "first line touches the speaker
        # name" defect, st0:1 window 2). Drop them; the pad absorbs the freed bytes.
        for gi in range(0, len(groups) - 1):
            opi = gend[gi]
            if gi >= len(knon) - 1 and opi is not None and opi < len(rows) and rows[opi][0] == "LINE_BREAK" and opi not in op_repl:
                op_repl[opi] = ([], -1)
                DROPPED_LB.append((k, gi))
        ko_total += sum(len(enc(x, mp)) for x in knon)
        tail_row = None
        if tail_ko:
            assert len(knon) < len(groups), (label, k, "tail window needs a free last slot")
            g_rows = sorted(tix for slot in groups[-1] for tix in slot)
            plays = [i for i in range(g_rows[0], g_rows[-1]) if rows[i][0] == "PLAY_SOUND"]
            assert plays, (label, k, "tail window: no PLAY_SOUND inside the last slot")
            cand = [t for t in g_rows if t > plays[-1]]
            assert cand, (label, k, "tail window: no TEXT row behind the voice op")
            tail_row = cand[0]
            ko_total += len(enc(tail_ko, mp))
        for gi, grp in enumerate(groups):
            payload = enc(knon[gi], mp) if gi < len(knon) else b""
            if extra and gi == len(groups) - 1:
                for xi, x in enumerate(extra):
                    li = len(groups) + xi  # line index
                    payload += (wb_bytes if li in winstart else bytes([1])) + enc(x, mp)
            grp_rows = [tix for slot in grp for tix in slot]
            first = True
            for tix in grp_rows:
                if tail_row is not None and gi == len(groups) - 1:
                    new_row[tix] = ("BYTE", enc(tail_ko, mp)) if tix == tail_row else ("TEXT", b"")
                elif first:
                    new_row[tix] = ("BYTE", payload)
                    first = False
                else:
                    new_row[tix] = ("TEXT", b"")
    # pad points: every intra-symbol absolute target row + every C entry row + end
    pad_rows = sorted(set([r for r in intra_targets] + entry_set + [len(rows)]))
    # retail cumulative TEXT bytes before each row
    en_cum = [0] * (len(rows) + 1)
    for i, (kind, pay, raw) in enumerate(rows):
        en_cum[i + 1] = en_cum[i] + (_text_len(pay) if kind == "TEXT" else 0)
    # voice sync: a window that plays a voice clip (PLAY_SOUND) must not be closed (NEXT_DIALOG ->
    # next speaker's portrait / next window) before the clip has ended -- the Korean text is shorter than
    # the English one the clip was timed for, so without this the portrait switches to the next speaker
    # while the previous voice is still playing. SCRIPT_UNKNOWN_11 = wait while the voice plays (the
    # retail script already uses it at the start of every voiced window); +1 byte, absorbed by the pads.
    # (optional stages: only in windows whose text was replaced -- a placeholder window keeps retail bytes)
    wait_before = set()
    voiced = False
    ko_win = False
    for idx, (kind, pay, raw) in enumerate(rows):
        if idx in new_row:
            ko_win = True
        if kind == "PLAY_SOUND" and idx + 1 < len(rows) and rows[idx + 1][0] == "WAIT_FOR_SOUND":
            voiced = True  # voice clip: PLAY_SOUND directly followed by WAIT_FOR_SOUND (SFX/music cues are not)
        elif kind == "NEXT_DIALOG":
            # (never at a C entry row: the entry must stay on the retail op)
            if voiced and idx not in op_repl and idx not in entry_set and (ko_win or not optional):
                wait_before.add(idx)
            voiced = False
            ko_win = False
        elif kind in ("CLOSE_DIALOG", "END_CUTSCENE"):
            voiced = False
            ko_win = False
    WAITS.append((label, len(wait_before)))
    # emit
    out = []
    ko_cum = 0
    pi = 0
    targets = sorted(pad_rows)
    for idx, (kind, pay, raw) in enumerate(rows):
        while pi < len(targets) and targets[pi] == idx:
            need = en_cum[idx] - ko_cum
            assert need >= 0, (label, "KO bytes overrun a jump target / C entry", idx,
                               "+0x%X" % next((o for o, r in entry_row.items() if r == idx), -1),
                               "over by %d B" % -need)
            for _ in range(need):
                out.append("  - [BYTE, 0x0D]")
            ko_cum += need
            pi += 1
        if idx in op_repl:
            for ol in op_repl[idx][0]:
                out.append(ol)
            ko_cum += op_repl[idx][1]
        elif idx in new_row:
            k2, p2 = new_row[idx]
            if k2 == "TEXT":
                out.append('  - [TEXT, ""]')
            else:
                if len(p2) == 0:
                    out.append('  - [TEXT, ""]')
                else:
                    for b in p2:
                        out.append("  - [BYTE, 0x%02X]" % b)
                    ko_cum += len(p2)
        else:
            if idx in wait_before:
                out.append("  - [SCRIPT_UNKNOWN_11]")
                ko_cum += 1
            if kind == "TEXT":
                ko_cum += _text_len(pay)  # placeholder row: retail text kept
            out.append(raw.rstrip("\n"))
    # end pad (after last row)
    need = en_cum[len(rows)] - ko_cum
    assert need >= 0, (label, "end", en_total, ko_cum)
    for _ in range(need):
        out.append("  - [BYTE, 0x0D]")
    ko_cum += need
    pad = en_total - ko_total
    assert ko_cum == en_total, (label, ko_cum, en_total)
    out.append(MARK + " EN=%d KO=%d pad13=%d" % (en_total, ko_total, pad))
    return out, (en_total, ko_total, pad, len(keys))


# ------------------------------------------------------------ boss (asm) scripts
def _psp():
    import psp_script as P  # constants only at import (OPS, disassemble, assemble)
    return P


def boss_regions(ovl):
    """[(BIN offset, retail tokens, [entry offsets])] of one asm boss overlay:
    each SetCutsceneScript address up to its END_CUTSCENE (+ the parts its
    jumps reach, psp_script.retail_script); an address inside an earlier
    region is that region's entry offset."""
    P = _psp()
    binp, _, addrs, _ = EO.BOSS_SCRIPTS[ovl]
    retail = open(os.path.join(REPO, "disks", "us", binp), "rb").read()
    regs = []
    for a in sorted(addrs):
        off = a - EO.BASE
        inside = [r for r in regs if r[0] < off < r[0] + len(P.assemble(r[1]))]
        if inside:
            r = inside[0]
            r[2].append(off - r[0])  # (psp_script treats it as its own script; same bytes)
            continue
        toks, o2 = P.retail_script(retail, {"addr": a}, ovl)
        assert o2 == off
        regs.append((off, toks, []))
    for i in range(1, len(regs)):
        assert regs[i - 1][0] + len(P.assemble(regs[i - 1][1])) <= regs[i][0], (ovl, "regions overlap")
    return retail, regs


def toks_to_rows(toks):
    """psp_script tokens -> split_rows rows (yaml-like lines, latin-1 text)"""
    lines = []
    for t in toks:
        if t[0] == "TEXT":
            lines.append('  - [TEXT, "%s"]' % t[1])
        elif t[0] == "BYTE":
            lines.append("  - [BYTE, 0x%02X]" % t[1])
        elif len(t) == 1:
            lines.append("  - [%s]" % t[0])
        else:
            lines.append("  - [%s, %s]" % (t[0], ", ".join("0x%X" % a for a in t[1:])))
    return split_rows(lines)


def rows_to_bytes(rows):
    P = _psp()
    out = bytearray()
    for kind, pay, raw in rows:
        if kind == "RAW":
            continue
        if kind == "TEXT":
            out += pay.encode("latin-1")
        elif kind == "BYTE":
            out.append(int(pay, 0))
        else:
            args = [int(a.strip(), 0) for a in pay.split(",")] if pay.strip() else []
            out += P.assemble([tuple([kind] + args)])
    return bytes(out)


def boss_prefix(ovl, off):
    return "%s_%x" % (ovl, off)


def splice_boss(tsv, mp, plan, allow):
    """asm boss overlays: KO bytes per script region -> plan (for --apply-boss)"""
    for ovl in sorted(EO.BOSS_SCRIPTS):
        binp = EO.BOSS_SCRIPTS[ovl][0]
        if not os.path.exists(os.path.join(REPO, "disks", "us", binp)):
            print("%s: no retail %s, skipped" % (ovl, binp))
            continue
        retail, regs = boss_regions(ovl)
        for off, toks, entries in regs:
            rows = toks_to_rows(toks)
            old = rows_to_bytes(rows)
            assert old == retail[off:off + len(old)], (ovl, hex(off), "token round trip")
            pref = boss_prefix(ovl, off)
            label = "%s@0x%X" % (binp, off)
            try:
                out, st = splice_rows(label, rows, off, len(old), entries, [pref], tsv, mp, True)
            except Untranslated:
                print("%s: keys=0 (placeholder %s:N) -- retail kept" % (label, pref))
                continue
            # bo5/bo6/rbo6 have the C KO renderer (EO.BOSS_KO_RENDER)
            assert ovl in EO.BOSS_KO_RENDER or BOSS_ASM_OK, (
                "%s: KO rows for an asm-rendered boss (no KO glyph path; "
                "set KO_BOSS_ASM_RENDER=1 only with a KO renderer)" % label)
            new = rows_to_bytes(split_rows(out))
            if ovl == "bo6" and off == 0x1A2FB:
                # A1: the Shaft-route case of this SWITCH must follow the Richter route's END (KO boxes moved)
                new = EO.fix_switch_after_end(new, off, 2, label)
            errs = EO.verify_bytes(old, new, [0] + entries, label)
            assert not errs, errs
            name = binp.rsplit("/", 1)[-1]
            plan.append({"ovl": ovl, "bin": name, "off": off, "old": old.hex(),
                         "new": new.hex(), "entries": entries})
            for b in [name] + (["BO6RIC2.BIN"] if ovl == "bo6" else []):
                allow.setdefault(b, []).append([off, len(old)])
            print("%s: keys=%d EN=%d KO=%d pad=%d entries=%s" % (
                label, st[3], st[0], st[1], st[2], [hex(e) for e in entries]))


def apply_boss():
    """--apply-boss: write the planned KO bytes into the linked boss BINs"""
    if not os.path.exists(BOSS_PLAN):
        print("ko boss splice: no plan (nothing translated)")
        return 0
    plan = json.load(open(BOSS_PLAN))
    n = 0
    for p in plan:
        path = os.path.join(REPO, "build", "us", p["bin"])
        b = bytearray(open(path, "rb").read())
        old, new, o = bytes.fromhex(p["old"]), bytes.fromhex(p["new"]), p["off"]
        cur = bytes(b[o:o + len(old)])
        if cur == new:
            continue
        if cur != old:
            sys.exit("ko boss splice: %s at 0x%X is not the retail script" % (p["bin"], o))
        b[o:o + len(new)] = new
        open(path, "wb").write(b)
        n += 1
    print("ko boss splice: %d of %d scripts written" % (n, len(plan)))
    return 0


def write_placeholders(out_path, tsv):
    """rows for the optional stages' keys: one row per dialogue window
    (NEXT_DIALOG / CLOSE_DIALOG / END_CUTSCENE), EN = "#1/1.." (one segment
    per text slot), translation empty. OUT.ref.tsv: BIN offset of the first
    text, voice ids, retail EN (local reference, not for the public TSV)."""
    jobs = []
    for asset, overlay, symbol, prefs in STAGES:
        if overlay in OPTIONAL:
            path = os.path.join(REPO, "assets", asset)
            if not os.path.exists(path):
                print("skip (not extracted): " + asset)
                continue
            _, cur = _read_yaml_retail(asset)
            nm0 = asset.split("/")[-1].rsplit(".", 1)[0]
            start0 = config_sizes()[0][("assets/" + asset.rsplit("/", 1)[0], nm0)][0]
            jobs.append((prefs[0], start0, EO.offsets_for("assets/" + asset),
                         split_rows([l for l in cur.split("\n") if l.strip()])))
    for ovl in sorted(EO.BOSS_SCRIPTS):
        binp = EO.BOSS_SCRIPTS[ovl][0]
        if not os.path.exists(os.path.join(REPO, "disks", "us", binp)):
            print("skip (no retail BIN): " + binp)
            continue
        _, regs = boss_regions(ovl)
        for off, toks, entries in regs:
            jobs.append((boss_prefix(ovl, off), off, entries, toks_to_rows(toks)))
    hdr = io.open(TSV, encoding="utf-8").readline().rstrip("\n").split("\t") \
        if os.path.exists(TSV) else ["키", "분류", "EN", "번역(v14 확정본)"]
    o1 = io.open(out_path, "w", encoding="utf-8", newline="")
    o2 = io.open(out_path + ".ref.tsv", "w", encoding="utf-8", newline="")
    o1.write("\t".join(hdr) + "\n")
    o2.write("키\t위치(BIN)\t음성\tEN(retail)\t진입구간\n")
    total = {}
    for pref, start, ents, rows in jobs:
        offs = EO.row_offsets(rows, _row_size)
        n = 0
        win, slot, voices, first = [], "", [], None
        for idx, (kind, pay, raw) in enumerate(rows):
            if kind == "TEXT" and pay != "":
                slot += pay
                if first is None:
                    first = start + offs[idx]
            elif kind == "PLAY_SOUND":
                voices.append(pay.strip())
            if kind in ("LINE_BREAK", "NEXT_DIALOG", "CLOSE_DIALOG", "END_CUTSCENE") and slot:
                win.append(slot)
                slot = ""
            if kind in ("NEXT_DIALOG", "CLOSE_DIALOG", "END_CUTSCENE") or idx == len(rows) - 1:
                if slot:
                    win.append(slot)
                    slot = ""
                if win:
                    key = "%s:%d" % (pref, n)
                    row = [key, "대사", "#" + "/".join("1" for _ in win), ""]
                    row += [""] * (len(hdr) - len(row))
                    if key in tsv and tsv[key][1].strip():
                        row[3] = "(TSV에 이미 있음)"
                    o1.write("\t".join(row[:len(hdr)]) + "\n")
                    seg = [e for e in sorted(ents) if e <= first - start]
                    o2.write("%s\t0x%X\t%s\t%s\t%s\n" % (
                        key, first, " ".join(voices), "/".join(win),
                        "+0x%X" % seg[-1] if seg else "+0"))
                    n += 1
                win, voices, first = [], [], None
        total[pref] = n
    o1.close()
    o2.close()
    for p, n in total.items():
        print("placeholder %s: %d rows" % (p, n))
    print("wrote %s (+ .ref.tsv)" % out_path)


def check_all_glyphs(tsv, mp):
    """pre-splice : every character of every KO row must have a glyph cell; list ALL missing at once"""
    miss = {}
    for k, (en, ko) in tsv.items():
        if __import__("re").match(r"(bo|rbo)\d+_|mar_|mod_maria_boss_data", k):
            continue  # boss rows: apply_boss path (own encoder), not this check
        for ch in ell(ko.replace("\u2026", "...")):
            if ord(ch) >= 128 and ch not in PROMO and ch not in mp:
                miss.setdefault(ch, []).append(k)
    if miss:
        print("glyph map misses %d characters:" % len(miss))
        for ch, ks in sorted(miss.items(), key=lambda x: ord(x[0])):
            print("  %s U+%04X rows=%d: %s" % (ch, ord(ch), len(ks), ", ".join(sorted(set(ks))[:8])))
        sys.exit("pre-splice check: %d characters have no glyph cell (list above)" % len(miss))
    print("pre-splice check: all KO row characters are in the glyph map (%d mapped, %d promoted)" % (len(mp), len(PROMO)))


def main():
    if len(sys.argv) > 1 and sys.argv[1] == "--restore-only":
        n = 0
        for asset, _, _, _ in STAGES:
            path = os.path.join(REPO, "assets", asset)
            bak = path + ".ko_retail"
            if os.path.exists(bak):
                cur = io.open(path, encoding="utf-8").read()
                if MARK in cur:
                    io.open(path, "w", encoding="utf-8", newline="").write(
                        io.open(bak, encoding="utf-8").read())
                    n += 1
        print("restored %d yamls" % n)
        return
    if len(sys.argv) > 1 and sys.argv[1] == "--apply-boss":
        return apply_boss()
    if len(sys.argv) > 2 and sys.argv[1] == "--placeholders":
        return write_placeholders(sys.argv[2], load_tsv())
    tsv = load_tsv()
    mp = load_map()
    check_all_glyphs(tsv, mp)
    sizes, bins = config_sizes()
    edits = json.load(open(EDITS_PATH)) if os.path.exists(EDITS_PATH) else {}
    managed = [s for (_, o, _, _) in STAGES for s in [o]] + ["stno4"]
    for o in managed:
        if o in edits:
            edits[o] = [e for e in edits[o] if not e[0].startswith("cutscene")]
    total_en = total_ko = 0
    allow = {}
    for asset, overlay, symbol, prefs in STAGES:
        name = asset.split("/")[-1].rsplit(".", 1)[0]
        key = ("assets/" + asset.rsplit("/", 1)[0], name)
        assert key in sizes, ("no config size for", key)
        start, size = sizes[key]
        tgt = bins[key]
        binname = tgt.rsplit("/", 1)[-1]
        st = splice_stage(asset, prefs, tsv, mp, overlay in OPTIONAL)
        if st is None:
            print("%s: keys=0 (placeholder %s:N) -- retail kept" % (overlay, prefs[0]))
            continue
        en, ko, pad, nkeys = st
        allow.setdefault(binname, []).append([start, size])
        edits.setdefault(overlay, []).append([symbol, size])
        total_en += en
        total_ko += ko
        print("%s: keys=%d EN=%d KO=%d pad=%d symbol=%s size=%d bin=%s@%#x" %
              (overlay, nkeys, en, ko, pad, symbol, size, binname, start))
    os.makedirs(os.path.dirname(EDITS_PATH), exist_ok=True)
    # no4 L/R share one US .data array (cutscene_data_ferryman_left, 0xAC):
    # merge the two per-yaml edits into the combined span.
    if "stno4" in edits:
        rest = [e for e in edits["stno4"]
                if e[0] != "cutscene_data_ferryman_left"]
        edits["stno4"] = rest + [["cutscene_data_ferryman_left", 0xAC]]
    json.dump(edits, open(EDITS_PATH, "w"), indent=2)
    plan = []
    splice_boss(tsv, mp, plan, allow)
    json.dump(plan, open(BOSS_PLAN, "w"), indent=0)
    json.dump(allow, open(ALLOW_PATH, "w"), indent=2)
    print("TOTAL EN=%d KO=%d" % (total_en, total_ko))
    print("rows with extra LINE_BREAK:", EXTRA_LB)
    print("window breaks (row, count, extra bytes each):", [w for w in WINDOWS if w[1]])
    print("warnings:", WARN)
    print("voice-wait (SCRIPT_UNKNOWN_11) inserted per asset:", WAITS)
    print("dropped trailing LINE_BREAK rows:", len(DROPPED_LB))


if __name__ == "__main__":
    sys.exit(main())
