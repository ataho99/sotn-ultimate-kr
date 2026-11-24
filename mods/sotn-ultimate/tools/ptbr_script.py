#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
import argparse
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import psp_script as P  # noqa: E402
import ptbr_glyphs as G  # noqa: E402

ROOT = P.ROOT
TEXT = os.path.join(ROOT, "mods/sotn-ultimate/tools/ptbr/dialogue.json")
WORK = os.path.join(ROOT, "build/us/ptbr_script")
PATCHES = os.path.join(WORK, "patches.json")
REPORT = os.path.join(WORK, "report.txt")
NOTES = os.path.expanduser("~/sotn-work/notes/temp-sotn/feat/ptbr")
SKIP = set()
END_OVL = {"sel"}  # copies in one block, loaded at the ending
END_BLOCK = 0x801E8000  # the blob's blocks' buffer (the demo key's)
END_AT = END_BLOCK + 4  # (after the magic)
END_MAGIC = 0x30444E45  # "END0"
END_MAX = 0x2000  # one demo-key load
ENJ_MAGIC = 0x304A4E45  # "ENJ0"
NAMES_MAGIC = 0x314D414E  # "NAM1"
NAMES_END = END_BLOCK + END_MAX
NAMES_TAIL = os.path.join(WORK, "names_tail.bin")
BLOB_OVL = {"no3", "no4"}  # copies in the resident blob, at its end
BLOB_END = 0x801F7800
BLOB_SCRIPTS = os.path.join(WORK, "blob_scripts.bin")
MODES = ("ps1", "psp", "jp")
SUBST = {"…": "...", "“": '"', "”": '"', "‘": "'", "’": "'", "—": "-", "–": "-",
         "ä": "a", "　": " "}


def enc(s):
    out = ""
    for ch in s:
        ch = SUBST.get(ch, ch)
        for c in ch:
            if c in G.ORDER:
                out += chr(G.cell(G.ORDER.index(c)) + 0x20)
            elif " " <= c <= "~":
                out += c
            else:
                raise ValueError("no font cell for %r in %r" % (c, s))
    return out


def tables():
    T = json.load(open(TEXT, encoding="utf-8"))
    jp = {int(k, 16): v for k, v in T.get("jp", {}).items()}
    out, fell = {}, {}
    for m in MODES:
        t = {int(k, 16): v for k, v in T.get(m, {}).items()}
        fell[m] = sorted(set(jp) - set(t)) if m != "jp" else []
        out[m] = {**jp, **t}
    return out, fell


def ptdict(text):
    """the PSP tool's line table shape (by its PSP ids) from PS1 ids -> text"""
    d = {}
    for vid, s in text.items():
        sid = P.TEXT_OVERRIDE.get(vid, vid - 2)
        d[sid] = [(1, [("T", enc(s))])]
    # APPEND's new lines are keyed by their PSP id already
    for vid, (psp_id, _, _) in P.APPEND.items():
        if psp_id + 2 in text:
            d[psp_id] = [(1, [("T", enc(text[psp_id + 2]))])]
    return d


def build_copy(part_toks, splits, off, blob, ovl, label, text, dur, extra_ids,
               leads, append, jp_dur=None, report=None):
    saved = dict(P.APPEND)
    if not append:
        P.APPEND.clear()
    saved_no = set(P.NO_PSP)
    if not append or jp_dur is not None:
        P.NO_PSP.difference_update(
            {v for v in saved_no if P.TEXT_OVERRIDE.get(v, v - 2) in text})
    for vid, (psp_id, define, _) in list(P.APPEND.items()):
        if psp_id not in text:
            P.APPEND.pop(vid)
            if report is not None:
                report.append("  %s: no Portuguese for the new line %s, left out" % (label, define))
    try:
        cuts, o, parts, cur = splits, 0, [], []
        for t in part_toks:
            if o in cuts and cur:
                parts.append(cur)
                cur = []
            cur.append(t)
            o += P.tok_size(t)
        parts.append(cur)
        rendered = []
        for part in parts:
            plan, _, miss = P.transplant(part, text, dur, report if report is not None else [], ovl)
            if miss:
                return None, "no text for " + " ".join("%X" % v for v in miss)
            rendered.append((part, P.render(part, plan, 0, extra_ids)))
    finally:
        P.NO_PSP.clear()
        P.NO_PSP.update(saved_no)
        P.APPEND.clear()
        P.APPEND.update(saved)
    old_map, starts, o, part_off = {}, [], 0, 0
    for part, newt in rendered:
        old_at, q = [], 0
        for t in part:
            old_at.append(q)
            q += P.tok_size(t)
        starts.append((part_off, o))
        for t, idx in newt:
            if idx is not None and off is not None:
                old_map[P.BASE + off + part_off + old_at[idx]] = ("S", o)
            o += P.tok_size(t)
        part_off += q
    lo = P.BASE + off if off is not None else 0
    hi = lo + len(blob) if off is not None else 0
    fixed = []
    for part, newt in rendered:
        for t, _ in newt:
            if t[0] in ("SCRIPT_SWITCH", "SCRIPT_UNKNOWN_15"):
                head = 2 if t[0] == "SCRIPT_SWITCH" else 1
                tg = []
                for a_ in t[head:]:
                    assert lo <= a_ < hi and a_ in old_map, (ovl, label, hex(a_))
                    tg.append(old_map[a_])
                t = tuple(list(t[:head]) + tg)
            fixed.append(t)
    fixed, ents, _ = P.jp_adjust(fixed, [so for _, so in starts], dur, False, leads)
    if jp_dur is not None:
        fixed, ents, _ = P.jp_adjust(fixed, ents, jp_dur, True, leads)
    P.check_widths(fixed, ovl)
    return (fixed, ents), None


def durations():
    d = json.load(open(P.DUB_JSON))
    psp = {int(k): v for k, v in d.get("durations", {}).items()}
    # replace: config index -> the PSP clip and the retail one's length
    ps1 = {0x300 + int(k): e["old_unk228"] for k, e in d.get("replace", {}).items()
           if "old_unk228" in e}
    return ps1, psp


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--off", action="store_true")
    ap.add_argument("--import", dest="imp", action="store_true")
    a = ap.parse_args()
    if a.imp:
        return import_jp()
    if a.off:
        return write_headers({})
    if a.apply:
        return apply()
    os.makedirs(WORK, exist_ok=True)
    texts, fell = tables()
    tdict = {m: ptdict(texts[m]) for m in MODES}
    ps1_dur, psp_dur = durations()
    extra_ids = json.load(open(os.path.join(P.WORK, "extra_ids.json")))
    jdur = P.jp_durations(extra_ids)
    report, spills = [], {}
    # psp_script.py's JP copies kept out of SEL / NO4 (mode "enjp": English
    # text, JP dub; their tokens' jumps are relative to the copy already)
    enjp = {(j["ovl"], j["off"]): j for j in json.load(open(P.JP_OUT))} \
        if os.path.exists(P.JP_OUT) else {}
    for ovl, (retail_rel, mapname, scripts) in P.OVERLAYS.items():
        if ovl in SKIP:
            report.append("%s: skipped (no room; English copies)" % ovl)
            continue
        retail = open(os.path.join(ROOT, "disks/us", retail_rel), "rb").read()
        for sc in scripts:
            if "c" in sc:
                full = P.read_c_array(sc["c"], sc["sym"])
                off, blob, label = None, P.assemble(full), sc["sym"]
                cut = P.drop_lines(full, P.NO_PSP, label)
            else:
                full, off = P.retail_script(retail, sc, ovl)
                blob = P.assemble(full)
                label = os.path.basename(sc["yaml"]) if "yaml" in sc else "0x%X" % sc["addr"]
                cut = full
            leads = P.flag_leads(full)
            splits = sc.get("split", [])
            rep = []
            # ps1: the PS1 skeleton (the cut line too), PS1 voice lengths
            c_ps1, why = build_copy(full, splits, off, blob, ovl, label, tdict["ps1"],
                                    ps1_dur, extra_ids, leads, False, report=rep)
            if c_ps1 is None:
                report.append("%s %s: no Portuguese copy (%s)" % (ovl, label, why))
                continue
            c_psp, why = build_copy(cut, splits, off, blob, ovl, label, tdict["psp"],
                                    psp_dur, extra_ids, leads, True)
            if c_psp is None:  # a script the PSP dub plays in PS1 (keep_ps1)
                c_psp = c_ps1
            base_jp = full if "c" in sc else cut
            # (no PSP added lines on the JP dub: the Japanese cast never
            # recorded them; psp_script.py's JP copy leaves them out too)
            c_jp, why = build_copy(base_jp, splits, off, blob, ovl, label, tdict["jp"],
                                   psp_dur, extra_ids, leads, False, jp_dur=jdur)
            if c_jp is None:
                c_jp = c_ps1
            sp = spills.setdefault(ovl, {"size": 0, "scripts": []})
            rec = {"off": off, "sym": sc.get("sym"), "old": blob.hex()}
            sizes = []
            for m, (toks, ents) in zip(MODES, (c_ps1, c_psp, c_jp)):
                n = len(P.assemble(P.resolve(toks, P.BASE)))
                rec[m] = {"at": sp["size"], "tokens": toks,
                          "entries": [[po, sp["size"] + so] for (po, _), so in
                                      zip([(p, 0) for p in [0] + splits], ents)]}
                sp["size"] += n
                sizes.append(n)
            ej = enjp.get((ovl, off))
            if ej and ovl in BLOB_OVL:
                rec["enjp_names"] = ej
            elif ej and ovl in END_OVL:
                rec["enjp_block"] = ej
            sp["scripts"].append(rec)
            report.append("%s %s: PT copies %s bytes (English %d)" % (
                ovl, label, "/".join(map(str, sizes)), len(blob)))
            report += rep
    for m in ("ps1", "psp"):
        if fell[m]:
            report.insert(0, "%s text falls back to jp for %d lines" % (m, len(fell[m])))
    # the blob overlays' copies: one run at the blob's end (4-aligned)
    total = sum(spills[o]["size"] for o in BLOB_OVL if o in spills)
    base = (BLOB_END - total) & ~3
    blob = bytearray()
    for o in sorted(BLOB_OVL):
        if o not in spills:
            continue
        sp = spills[o]
        sp["addr"] = base + len(blob)
        part = bytearray(sp["size"])
        for sc in sp["scripts"]:
            for m in MODES:
                c = sc[m]
                toks = [tuple(tuple(x) if isinstance(x, list) else x for x in t) for t in c["tokens"]]
                code = P.assemble(P.resolve(toks, sp["addr"] + c["at"]))
                part[c["at"]:c["at"] + len(code)] = code
        blob += part
    open(BLOB_SCRIPTS, "wb").write(bytes([base & 0xFF, (base >> 8) & 0xFF, (base >> 16) & 0xFF,
                                          base >> 24]) + bytes(blob))
    report.insert(0, "blob scripts (%s): %d bytes at 0x%08X" % (
        " ".join(sorted(BLOB_OVL)), len(blob), base))
    # the blob overlays' English JP copies: the names block's tail
    tail = [(o, sc) for o in sorted(BLOB_OVL) if o in spills
            for sc in spills[o]["scripts"] if "enjp_names" in sc]
    code = []
    for o, sc in tail:
        ej = sc["enjp_names"]
        toks = [tuple(tuple(x) if isinstance(x, list) else x for x in t) for t in ej["tokens"]]
        code.append((sc, toks, len(P.assemble(P.resolve(toks, P.BASE)))))
    tbase = (NAMES_END - sum((n + 3) & ~3 for _, _, n in code)) & ~3
    tb, at = bytearray(), tbase
    for sc, toks, n in code:
        sc["enjp_rows"] = [[po, at + so] for po, so in sc["enjp_names"]["entries"]]
        tb += P.assemble(P.resolve(toks, at))
        tb += b"\0" * (-len(tb) % 4)
        at = tbase + len(tb)
    open(NAMES_TAIL, "wb").write(tbase.to_bytes(4, "little") + bytes(tb) if tb else b"")
    report.insert(0, "names block tail (English JP copies): %d bytes at 0x%08X" % (len(tb), tbase))
    # the ending block: the three dubs' copies one after another from
    # END_AT (each 4-aligned)
    for o in END_OVL:
        sp = spills.get(o)
        blk = END_MAGIC.to_bytes(4, "little")
        if sp:
            assert len(sp["scripts"]) == 1, o
            for m in MODES:
                c = sp["scripts"][0][m]
                toks = [tuple(tuple(x) if isinstance(x, list) else x for x in t) for t in c["tokens"]]
                at = END_BLOCK + len(blk)
                c["entries"] = [[po, so - c["at"]] for po, so in c["entries"]]
                c["at"], c["addr"] = 0, at
                blk += P.assemble(P.resolve(toks, at))
                blk += b"\0" * (-len(blk) % 4)
        if len(blk) > END_MAX:
            sys.exit("FAIL ptbr script: the ending block is %d bytes, over %d" % (
                len(blk), END_MAX))
        open(os.path.join(WORK, "end.bin"), "wb").write(blk if sp else b"")
        report.insert(0, "%s ending block (ps1 + psp + jp): %d of %d bytes" % (o, len(blk), END_MAX))
        # block 4: the English JP copy
        ej = sp["scripts"][0].get("enjp_block") if sp else None
        eblk = b""
        if ej:
            toks = [tuple(tuple(x) if isinstance(x, list) else x for x in t) for t in ej["tokens"]]
            assert ej["entries"] == [[0, 0]], ej["entries"]
            eblk = ENJ_MAGIC.to_bytes(4, "little") + P.assemble(P.resolve(toks, END_AT))
            if len(eblk) > END_MAX:
                sys.exit("FAIL ptbr script: the English JP ending block is %d bytes, over %d" % (
                    len(eblk), END_MAX))
            report.insert(0, "%s English JP ending block: %d of %d bytes" % (o, len(eblk), END_MAX))
        open(os.path.join(WORK, "end_en.bin"), "wb").write(eblk)
    write_headers(spills)
    json.dump([{"ovl": o, "bin": "build/us/%s.BIN" % o.upper(),
                "map": "build/us/%s.map" % P.OVERLAYS[o][1], "spill": sp}
               for o, sp in spills.items()
               if o not in P.C_ARRAYS and o not in BLOB_OVL and o not in END_OVL],
              open(PATCHES, "w"), indent=0)
    print("ptbr script: %d bytes of NO3 / NO4 copies at the blob's end (0x%08X)" % (len(blob), base))
    open(REPORT, "w", encoding="utf-8").write("\n".join(report) + "\n")
    print("ptbr script: Portuguese copies of %d scripts in %d overlays (%s)" % (
        sum(len(s["scripts"]) for s in spills.values()), len(spills),
        os.path.relpath(REPORT, ROOT)))
    for line in report:
        if "no Portuguese copy" in line or "falls back" in line:
            print("  " + line)


HDR_FN = """static u8* ModPtbrScript(u8* script) {
    u8* const (*m)[2];
    u32 i, n;

    if (!MOD_PTBR_TEXT_PT()) {
        return NULL;
    }
    if (MOD_JP_DUB_ON()) {
        m = g_ModPtJpMap;
        n = sizeof(g_ModPtJpMap) / sizeof(g_ModPtJpMap[0]);
    } else if (MOD_PSP_DUB_ON()) {
        m = g_ModPtPspMap;
        n = sizeof(g_ModPtPspMap) / sizeof(g_ModPtPspMap[0]);
    } else {
        m = g_ModPtPs1Map;
        n = sizeof(g_ModPtPs1Map) / sizeof(g_ModPtPs1Map[0]);
    }
    for (i = 0; i < n; i++) {
        if (script == m[i][0]) {
            return m[i][1];
        }
    }
    return NULL;
}
"""
END_FN = """// the ending's first step has the blob load it (ModPtbrLoad 3; 34B30.c)
// while the Portuguese text is on, block 4 (the English JP copy) in English
// with the JP dub; a load that failed leaves none there
static u8* ModPtbrScript(u8* script) {
    u32 magic = *(u32*)0x%08X;

    if (!MOD_PTBR_TEXT_PT()) {
        return magic == 0x%08X && MOD_JP_DUB_ON() ? (u8*)0x%08X : NULL;
    }
    if (magic != 0x%08X) {
        return NULL;
    }
    return MOD_JP_DUB_ON()    ? (u8*)0x%08X
           : MOD_PSP_DUB_ON() ? (u8*)0x%08X
                              : (u8*)0x%08X;
}
"""
HDR_FN_ENJP = HDR_FN.replace("""    if (!MOD_PTBR_TEXT_PT()) {
        return NULL;
    }
    if (MOD_JP_DUB_ON()) {""", """    if (!MOD_PTBR_TEXT_PT()) {
        // English with the JP dub: the retranslation's copy, in the area
        // names' block for want of room in the overlay (psp_script.py
        // PTBR_JP_OUT; ptbr_script.py NAMES_TAIL)
        if (!MOD_JP_DUB_ON() || *(u32*)0x801E8000 != 0x314D414E) {
            return NULL;
        }
        m = g_ModPtEnJpMap;
        n = sizeof(g_ModPtEnJpMap) / sizeof(g_ModPtEnJpMap[0]);
    } else if (MOD_JP_DUB_ON()) {""")
MAPNAME = {"ps1": "g_ModPtPs1Map", "psp": "g_ModPtPspMap", "jp": "g_ModPtJpMap",
           "enjp": "g_ModPtEnJpMap"}


def write_headers(spills):
    for ovl, c in P.SPILL_C.items():
        path = os.path.join(ROOT, os.path.dirname(c), "mod_ptbr_script.h")
        s = ("// Generated by mods/sotn-ultimate/tools/ptbr_script.py, do not edit\n"
             "// PT-BR disc: the Portuguese copies of this overlay's scripts, one per\n"
             "// dub (mod_psp_script.h's ModPspScript asks ModPtbrScript first)\n")
        sp = spills.get(ovl)
        if sp:
            s += "#define MOD_PTBR_SCRIPT\n"
            rows = {m: [] for m in MODES + ("enjp",)}
            if ovl in P.C_ARRAYS:
                for k, sc in enumerate(sp["scripts"]):
                    s += "extern u8 %s[];\n" % sc["sym"]
                    for m in MODES:
                        code = P.assemble(sc[m]["tokens"])
                        name = "g_ModPtCopy%s%d" % (m.capitalize(), k)
                        s += "u8 %s[] __attribute__((section(\".rodata\"))) = {\n" % name
                        for i in range(0, len(code), 16):
                            s += "    " + ", ".join("0x%02X" % x for x in code[i:i + 16]) + ",\n"
                        s += "};\n"
                        rows[m].append("    {%s, %s}," % (sc["sym"], name))
            elif ovl in END_OVL:
                s += ("// (the copies are in the ending block, loaded by the blob at\n"
                      "// 0x%08X: ptbr_script.py END_OVL)\n" % END_BLOCK)
                sc = sp["scripts"][0]
                s += END_FN % (END_BLOCK, ENJ_MAGIC, END_AT, END_MAGIC, sc["jp"]["addr"],
                               sc["psp"]["addr"], sc["ps1"]["addr"])
                if not os.path.exists(path) or open(path).read() != s:
                    open(path, "w").write(s)
                continue
            elif ovl in BLOB_OVL:
                s += "// (the copies are in the resident PT-BR blob, ptbr_script.py BLOB_OVL)\n"
                for sc in sp["scripts"]:
                    for m in MODES:
                        for po, so in sc[m]["entries"]:
                            rows[m].append("    {(u8*)0x%08X, (u8*)0x%08X}," % (
                                P.BASE + sc["off"] + po, sp["addr"] + so))
                    for po, addr in sc.get("enjp_rows", []):
                        rows["enjp"].append("    {(u8*)0x%08X, (u8*)0x%08X}," % (
                            P.BASE + sc["off"] + po, addr))
            else:
                s += ("// (g_ModPtSpill is filled in the built overlay by --apply)\n"
                      "u8 g_ModPtSpill[%d] __attribute__((section(\".rodata\"))) = {0};\n"
                      % sp["size"])
                for sc in sp["scripts"]:
                    for m in MODES:
                        for po, so in sc[m]["entries"]:
                            rows[m].append("    {(u8*)0x%08X, g_ModPtSpill + %d}," % (
                                P.BASE + sc["off"] + po, so))
            for m in MODES + (("enjp",) if rows["enjp"] else ()):
                s += "static u8* const %s[][2] = {\n%s\n};\n" % (MAPNAME[m], "\n".join(rows[m]))
            s += HDR_FN_ENJP if rows["enjp"] else HDR_FN
        if not os.path.exists(path) or open(path).read() != s:
            open(path, "w").write(s)


def apply():
    if not os.path.exists(PATCHES):
        sys.exit("ptbr_script: run it without --apply first")
    n = 0
    for p in json.load(open(PATCHES)):
        path = os.path.join(ROOT, p["bin"])
        b = bytearray(open(path, "rb").read())
        mp = open(os.path.join(ROOT, p["map"])).read()
        m = re.search(r"0x([0-9a-f]+)\s+g_ModPtSpill\n", mp)
        assert m, "%s: g_ModPtSpill not linked (mod_ptbr_script.h?)" % p["map"]
        base = int(m[1], 16)
        for sc in p["spill"]["scripts"]:
            old = bytes.fromhex(sc["old"])
            if bytes(b[sc["off"]:sc["off"] + len(old)]) != old:
                sys.exit("ptbr_script: %s at 0x%X: the retail script changed" % (p["bin"], sc["off"]))
            for mode in MODES:
                c = sc[mode]
                addr = base + c["at"]
                toks = [tuple(tuple(x) if isinstance(x, list) else x for x in t) for t in c["tokens"]]
                code = P.assemble(P.resolve(toks, addr))
                o = addr - P.BASE
                b[o:o + len(code)] = code
                n += 1
        open(path, "wb").write(b)
    print("ptbr script: %d Portuguese copies filled" % n)


def spaced(s):
    s = re.sub(r"([?!,;:])(?=[^\W\d_])", r"\1 ", s)
    return re.sub(r"(?<=[^\W\d_])\.(?=[^\W\d_])", ". ", s)


def import_jp():
    d = json.load(open(os.path.join(NOTES, "rafael/out/json/ptbr_translation.json"), encoding="utf-8"))
    jp = {}
    for s in d["dialogue"]:
        for b in s["boxes"]:
            v = b["us_voice_id"] if b["us_voice_id"] is not None else (
                b["pt_voice_id"] + 2 if b["pt_voice_id"] is not None else None)
            if v is None or b.get("untranslated_jp_remnant") or not b["pt_text"].strip():
                continue
            jp.setdefault(v, spaced(" ".join(b["pt_text"].split())))

    def tsv(name):
        rows = open(os.path.join(NOTES, "review", name), encoding="utf-8").read().splitlines()
        h = rows[0].split("\t")
        return [dict(zip(h, r.split("\t"))) for r in rows[1:] if r.strip()]

    for r in tsv("dialogue_long_l1.tsv"):
        jp[int(r["voice"], 16)] = r["proposal"]
    for r in tsv("dialogue_n1.tsv"):
        k = [k for k in r if k.startswith("pt")][0]
        jp[int(r["voice"], 16)] = " ".join(r[k].replace(" / ", " ").split())
    for r in tsv("dialogue_n2.tsv"):
        jp[int(r["voice"], 16)] = r["proposal"]
    T = json.load(open(TEXT, encoding="utf-8")) if os.path.exists(TEXT) else {}
    T["jp"] = {"0x%X" % k: v for k, v in sorted(jp.items())}
    T.setdefault("ps1", {})
    T.setdefault("psp", {})
    os.makedirs(os.path.dirname(TEXT), exist_ok=True)
    json.dump(T, open(TEXT, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    bad = []
    for k, v in jp.items():
        try:
            enc(v)
        except ValueError as e:
            bad.append("%X: %s" % (k, e))
    print("dialogue.json jp: %d lines%s" % (len(jp), ("; NOT encodable:\n  " + "\n  ".join(bad)) if bad else ""))


if __name__ == "__main__":
    main()
