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
# Excluded: prologue/opening/MAR
# (separate mechanisms, next turn). See 조사_t15_1111.md.
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
    # no4 (ferryman, trial17a #225): left EN 60B -> KO 38B (2 segs, pad 22),
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
]

MARK = "# KO-SPLICE trial15"


def load_map():
    mp = {}
    for line in io.open(MAP, encoding="utf-8"):
        p = line.rstrip("\n").split("\t")
        if len(p) >= 2 and len(p[0]) == 1 and p[1].startswith("0x"):
            mp[p[0]] = int(p[1], 16)
    assert len(mp) >= 700, len(mp)  # v13p
    mp["…"] = 0xF1A0  # trial25: v12f new cell (12px full-width ellipsis)
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
    """trial25: 9 dots (silence) -> two ellipsis cells, 3 dots -> one cell."""
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
    "SCRIPT_UNKNOWN_21": (21, []), "SCRIPT_UNKNOWN_22": (22, []),
    "SCRIPT_UNKNOWN_23": (23, []), "WAIT_FOR_FLAG_RESET": (24, [1]),
}


def _row_size(kind, pay):
    if kind in ("RAW", "TEXT"):
        return len(pay) if kind == "TEXT" else 0
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


def splice_stage(asset, prefixes, tsv, mp):
    path = os.path.join(REPO, "assets", asset)
    bak = path + ".ko_retail"
    cur = io.open(path, encoding="utf-8").read()
    if MARK in cur:
        assert os.path.exists(bak), "spliced yaml without backup: " + asset
        cur = io.open(bak, encoding="utf-8").read()
    else:
        if not os.path.exists(bak):
            io.open(bak, "w", encoding="utf-8", newline="").write(cur)
    lines = cur.split("\n")
    rows = split_rows([l for l in lines if l.strip() != ""])
    nm0 = asset.split("/")[-1].rsplit(".", 1)[0]
    key0 = ("assets/" + asset.rsplit("/", 1)[0], nm0)
    start0, size0 = config_sizes()[0][key0]
    intra_targets = _intra_targets(rows, start0, size0)
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
    texts = {}
    text_of = {}
    for idx, (kind, pay, raw) in enumerate(rows):
        if kind == "TEXT":
            text_of[idx] = pay
    # TSV rows in file order
    keys = sorted([k for k in tsv if k.rsplit(":", 1)[0] in prefixes],
                  key=lambda k: int(k.rsplit(":", 1)[1]))
    assert keys, asset
    # EN source: legacy text column, or "#n1/n2/.." = slot count per EN segment
    # (public TSV: no original text) -> original EN rebuilt from this yaml's slots
    cnt_mode = [tsv[k][0].startswith("#") for k in keys]
    assert all(cnt_mode) or not any(cnt_mode), (asset, "mixed EN counts/text")
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
        assert acc == s, (asset, k, j, repr(s), repr(acc))
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
    # (SCRIPT_SWITCH cases, UNKNOWN_15 gotos) must keep exact offsets, so
    # 0x0D (CSOP_SCRIPT_UNKNOWN_13 = 13 decimal) no-op pads are inserted before every such target row (and at the
    # end), sized to the cumulative EN-KO deficit at that point. 0x0D is a
    # no-op in every stage interpreter (CSOP_SCRIPT_UNKNOWN_13: continue).
    new_row = {}
    op_repl = {}
    en_total = ko_total = 0
    for k in keys:
        en = en_res[k]
        ko = tsv[k][1]
        es = en.split("/")
        # KO text: "/" = line, "|" = window break (NEXT_DIALOG + speaker re-set)
        knon = []
        winstart = set()
        for wi, wtxt in enumerate(ko.split("|")):
            wl = [x for x in wtxt.split("/") if x != ""]
            if wi > 0 and wl:
                winstart.add(len(knon))
            knon += wl
        groups = []
        gend = []
        for j, s in enumerate(es):
            if s == "":
                continue
            grp = seg_slot[(k, j)]
            assert len(grp) > 0, (asset, k, j)
            groups.append(grp)
            gend.append(seg_end[(k, j)])
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
                assert opi is not None and rows[opi][0] == "LINE_BREAK", (asset, k, gi, rows[opi][0] if opi is not None and opi < len(rows) else None)
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
        en_total += sum(len(x) for x in es if x != "")
        ko_total += sum(len(enc(x, mp)) for x in knon)
        for gi, grp in enumerate(groups):
            payload = enc(knon[gi], mp) if gi < len(knon) else b""
            if extra and gi == len(groups) - 1:
                for xi, x in enumerate(extra):
                    li = len(groups) + xi  # line index
                    payload += (wb_bytes if li in winstart else bytes([1])) + enc(x, mp)
            grp_rows = [tix for slot in grp for tix in slot]
            first = True
            for tix in grp_rows:
                if first:
                    new_row[tix] = ("BYTE", payload)
                    first = False
                else:
                    new_row[tix] = ("TEXT", b"")
    # pad points: every intra-symbol absolute target row + end
    pad_rows = sorted(set([r for r in intra_targets] + [len(rows)]))
    # retail cumulative TEXT bytes before each row
    en_cum = [0] * (len(rows) + 1)
    for i, (kind, pay, raw) in enumerate(rows):
        en_cum[i + 1] = en_cum[i] + (len(pay) if kind == "TEXT" else 0)
    # voice sync (sihum39): a window that plays a voice clip (PLAY_SOUND) must not be closed (NEXT_DIALOG ->
    # next speaker's portrait / next window) before the clip has ended -- the Korean text is shorter than
    # the English one the clip was timed for, so without this the portrait switches to the next speaker
    # while the previous voice is still playing. SCRIPT_UNKNOWN_11 = wait while the voice plays (the
    # retail script already uses it at the start of every voiced window); +1 byte, absorbed by the pads.
    wait_before = set()
    voiced = False
    for idx, (kind, pay, raw) in enumerate(rows):
        if kind == "PLAY_SOUND" and idx + 1 < len(rows) and rows[idx + 1][0] == "WAIT_FOR_SOUND":
            voiced = True  # voice clip: PLAY_SOUND directly followed by WAIT_FOR_SOUND (SFX/music cues are not)
        elif kind == "NEXT_DIALOG":
            if voiced and idx not in op_repl:
                wait_before.add(idx)
            voiced = False
        elif kind in ("CLOSE_DIALOG", "END_CUTSCENE"):
            voiced = False
    WAITS.append((asset, len(wait_before)))
    # emit
    out = []
    ko_cum = 0
    pi = 0
    targets = sorted(pad_rows)
    for idx, (kind, pay, raw) in enumerate(rows):
        while pi < len(targets) and targets[pi] == idx:
            need = en_cum[idx] - ko_cum
            assert need >= 0, (asset, idx, en_cum[idx], ko_cum)
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
            out.append(raw.rstrip("\n"))
    # end pad (after last row)
    need = en_cum[len(rows)] - ko_cum
    assert need >= 0, (asset, "end", en_total, ko_cum)
    for _ in range(need):
        out.append("  - [BYTE, 0x0D]")
    ko_cum += need
    pad = en_total - ko_total
    assert ko_cum == en_total, (asset, ko_cum, en_total)
    out.append(MARK + " EN=%d KO=%d pad13=%d" % (en_total, ko_total, pad))
    io.open(path, "w", encoding="utf-8", newline="").write("\n".join(out) + "\n")
    return en_total, ko_total, pad, len(keys)


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
    tsv = load_tsv()
    mp = load_map()
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
        allow.setdefault(binname, []).append([start, size])
        en, ko, pad, nkeys = splice_stage(asset, prefs, tsv, mp)
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
    json.dump(allow, open(ALLOW_PATH, "w"), indent=2)
    print("TOTAL EN=%d KO=%d" % (total_en, total_ko))
    print("rows with extra LINE_BREAK:", EXTRA_LB)
    print("window breaks (row, count, extra bytes each):", [w for w in WINDOWS if w[1]])
    print("warnings:", WARN)
    print("voice-wait (SCRIPT_UNKNOWN_11) inserted per asset:", WAITS)
    print("dropped trailing LINE_BREAK rows:", len(DROPPED_LB))


if __name__ == "__main__":
    sys.exit(main())
