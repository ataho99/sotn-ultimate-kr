#!/usr/bin/env python3
# gen_ko_equip_jp.py — (branch ko_equip_jp): JP-style expansion.
# KR hanpae precedent: table structs stay in DRA, name strings move to an
# external bundle, rows repoint. Here: 148 rows fit in-place (exact bytes,
# earlier rule), the other 119 KO rows become absolute pointers to
# KO_JP_BUNDLE_BASE (external bundle file, loaded at menu-open by the tail
# stub). DRA layout-neutral via PAD: .data same size (ptr<->ptr),
# .rodata delta compensated by MOD_DRA_PAD (check_dra_layout mechanism).
# Run from repo root: MOD_KO_EQUIP=1 python3 mods/sotn-ultimate/tools/gen_ko_equip_jp.py
import io
from ko_paths import KO_WK, KO_REPO  # noqa: E402
import os
import re
import sys

REPO = os.environ.get("SOTN_REPO", KO_REPO)
H = KO_WK
TSV = os.environ.get("KO_EQUIP_TSV", os.path.join(H, "번역/번역_v4_장비.tsv"))


def en_match(want, en):
    """TSV EN column = original text (legacy) or '#h' + sha1[:10] of it (public TSV)."""
    import hashlib
    return want == en or want == "#h" + hashlib.sha1(en.encode("utf-8")).hexdigest()[:10]
MAP = os.path.join(H, "P4시험/" + __import__("ko_font_profile").P["map_name"] + "")
GEN = os.path.join(REPO, "src", "dra", "gen", "us")
BUNDLE_BASE = 0x801F0000  # KR hanpae name-block address (0x801F9C18..0x801FA601 inside it)

row_re = re.compile(
    r'^(/\* 0x([0-9A-F]+) \*/) \{_S\("((?:[^"\\]|\\.)*)"\),(.*)$')


def load_map():
    mp = {}
    for line in io.open(MAP, encoding="utf-8"):
        p = line.rstrip("\n").split("\t")
        if len(p) >= 2 and len(p[0]) == 1 and "가" <= p[0] <= "힣":
            mp[p[0]] = int(p[1], 16)
    assert len(mp) >= 700, len(mp)  # glyph-map size sanity check
    return mp


def ko_bytes(s, mp):
    out = bytearray()
    for ch in s:
        o = ord(ch)
        if ch == " ":
            out.append(0x00)
        elif o < 128:
            assert 0x20 <= o < 0x7F, repr(ch)
            out.append(o - 0x20)
        else:
            assert "가" <= ch <= "힣", repr(ch)
            code = mp[ch]
            assert 0xF040 <= code <= 0xF375, (ch, hex(code))
            out += bytes([code >> 8, code & 0xFF])
    return bytes(out)


def esc(bs):
    return "".join("\\x%02X" % b for b in bs)


def main():
    mp = load_map()
    ko = {}
    for r in [l.rstrip("\n").split("\t") for l in io.open(TSV, encoding="utf-8")]:
        if len(r) > 3 and r[2] and r[0].split(":")[-1] == "name" and r[3]:
            ko[r[0]] = (r[2], r[3])
    stats = {"inplace": 0, "ext": 0, "ext_bytes": 0, "saturn_en": 0,
             "warn": 0, "freed": 0, "matched": 0}
    bundle = bytearray()
    for fname, tag, alt in [("equipment_mod.h", "g_EquipDefs", "equipment.h"),
                            ("accessories_mod.h", "g_AccessoryDefs", "accessories.h")]:
        path = os.path.join(GEN, fname)
        lines = io.open(path, encoding="utf-8").read().split("\n")
        for li, line in enumerate(lines):
            m = row_re.match(line)
            if not m:
                continue
            stats["matched"] += 1
            head, idx, en, rest = m.group(1), "0x" + m.group(2), m.group(3), m.group(4)
            if not all(0x20 <= ord(c) < 0x7F for c in en):
                continue
            hit = None
            for k in (f"{fname}:{tag}:{idx}:name", f"{alt}:{tag}:{idx}:name"):
                if k in ko:
                    hit = ko[k]
                    break
            if hit is None:
                stats["saturn_en"] += 1
                continue
            want_en, s = hit
            if not en_match(want_en, en):
                # trial assumption: TSV KO wins anyway (documented in mail)
                stats["warn"] += 1
            kb = ko_bytes(s, mp)
            slot = len(en) + 1  # mapped bytes + FF (compiler NUL both sides)
            freed_baseline = len(en) + 2  # + compiler NUL
            need = len(kb) + 2  # + FF 00 end marker
            if need <= slot and en_match(want_en, en):
                pad = slot - need
                lit = esc(kb) + "\\xFF\\x00" + "\\x00" * pad
                lines[li] = "%s {\"%s\",%s" % (head, lit, rest)
                stats["inplace"] += 1
            else:
                addr = BUNDLE_BASE + len(bundle)
                lines[li] = "%s {(const char*)0x%X,%s" % (head, addr, rest)
                bundle += kb + bytes([0xFF, 0x00, 0x00])
                stats["ext"] += 1
                stats["ext_bytes"] = len(bundle)
                stats["freed"] += freed_baseline
        io.open(path, "w", encoding="utf-8").write("\n".join(lines))
    if stats["matched"] == 0:
        print("gen_ko_equip_jp: no _S rows (already applied), nothing to do")
        return
    assert stats["inplace"] + stats["ext"] == 269, stats  # 267 + 2 (TSV 대소문자 고정 가정)
    bpath = os.path.join(GEN, "ko_jp_bundle.bin")
    io.open(bpath, "wb").write(bytes(bundle))
    print("gen_ko_equip_jp: inplace=%d ext=%d ext_bytes=%d rodata_freed=%d saturn_en=%d warn=%d"
          % (stats["inplace"], stats["ext"], stats["ext_bytes"],
             stats["freed"], stats["saturn_en"], stats["warn"]))
    print("NOTE: DRA rodata shrinks by rodata_freed; compensate with MOD_DRA_PAD += rodata_freed (42398.c KO branch, check_dra_layout mechanism)")


if __name__ == "__main__":
    main()
