#!/usr/bin/env python3
# gen_ko_equip.py — equipment/accessories KO bake (trial builds only).
# 입력: 한글작업/번역/번역_v4_장비.tsv + P4시험/ko_glyph_map_v4.tsv (desc) +
#       P4시험/ko_glyph_map_*.tsv (프로필 map_name) (name).
# 출력 A (SEL용, retail 레이아웃 유지): src/dra/gen/us/equipment_ko.h,
#         accessories_ko.h (desc만 KO F0xx, 이름은 bake_names()가 KO raw로).
#         config_us.h MOD_KO_EQUIP 게이트가 이 파일들을 포함. OFF 빌드는 호출 안 함.
# 출력 B (DRA용, Ultimate Saturn 레이아웃): src/dra/gen/us/equipment_mod_ko.h,
#         accessories_mod_ko.h + src/dra/gen/us/ko_jp_bundle.bin (121행 규칙 +
#         Saturn 신규행). config_us.h MOD_KO 게이트가 이 파일들을
#         포함 (MOD_SATURN && MOD_DRA_CONFIG 안). DRA retail 고정(#3518 계열).
# 이름 규칙 (_mod 대상): 공유행은 같은 idx로 equipment.h 키를 찾아 쓰고,
# Saturn 전용행(장비 0xA9-0xAF·0xE0-0xE3, 악세 0x5A-0x5C)은 equipment_mod.h 키.
# 특수행(retail 0xA9-0xD8 = mod 0xB0-0xDF)은 이름/desc가 비어 있어 스킵.
# 제자리 148행 규칙: KO 바이트+FF00+NUL이 EN 슬롯 안에 들고 TSV EN과 일치하면
# 제자리, 아니면 번들 포인터((const char*)0x801Fxxxx, 파일 순서대로).
# desc 규칙 (1안): 제자리 KO 교체 (KO가 EN보다 짧음, rodata shrinks).
import io
from ko_paths import KO_WK  # noqa: E402
import os
import re
import sys

REPO = (
    os.environ.get("SOTN_REPO")
    or os.environ.get("SOTN_KR")
    or os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
)
TSV = os.environ.get("KO_EQUIP_TSV", (KO_WK + "/번역/번역_v4_장비.tsv"))


def en_match(want, en):
    """TSV EN column = original text (legacy) or '#h' + sha1[:10] of it (public TSV)."""
    import hashlib
    return want == en or want == "#h" + hashlib.sha1(en.encode("utf-8")).hexdigest()[:10]
MAP = (KO_WK + "/P4시험/ko_glyph_map_v4.tsv")
GEN = os.path.join(REPO, "src", "dra", "gen", "us")


def load_map():
    mp = {}
    for line in io.open(MAP, encoding="utf-8"):
        p = line.rstrip("\n").split("\t")
        if len(p) >= 2 and len(p[0]) == 1 and "가" <= p[0] <= "힣":
            mp[p[0]] = int(p[1], 16)
    assert len(mp) == 529, len(mp)
    return mp


def load_map_v11():
    mp = {}
    for line in io.open(MAP_V11, encoding="utf-8"):
        p = line.rstrip("\n").split("\t")
        if len(p) >= 2 and len(p[0]) == 1 and "가" <= p[0] <= "힣":
            mp[p[0]] = int(p[1], 16)
    assert len(mp) >= 700, len(mp)  # glyph-map size sanity check
    return mp


MAP_V11 = (KO_WK + "/P4시험/") + __import__("ko_font_profile").P["map_name"] + ""

# ko_equip12: names in _S-style bytes (space=0x00, ascii=ord-0x20,
# hangul=F0 pair, end=FF 00), written as a RAW literal (no _S() wrapper:
# sotn_str would drop hangul). Chars (incl. spaces) must fit 12px x 10.
KO_EQ_NAME_MAX = 10


# B1: description ASCII -> 2-byte MGalmuri codes. ShowText
# (func_800F99B8) pairs any non-letter, non-space ASCII byte with the next byte
# (the "star" garbage) and draws a-z/A-Z via the JP KROM full-width glyphs, so a
# desc must carry no ASCII except the 0x20 space. Rules:
#   [ ] -> 「 」 | run of 2+ '.' -> … | '"' odd/even -> “ ” | other ASCII -> engtab251 code.
P4 = (KO_WK + "/P4시험/")
B1_ENGTAB = P4 + "ko_engtab_253.bin"
B1_NEWMAP = P4 + "ko_glyph_map_v12f_new.tsv"


def load_b1():
    import struct
    eng = open(B1_ENGTAB, "rb").read()
    assert len(eng) == 188, len(eng)
    e = {}
    for i in range(94):
        v = struct.unpack("<H", eng[2 * i:2 * i + 2])[0]
        if v:
            e[chr(0x21 + i)] = v
    new = {}
    for line in io.open(B1_NEWMAP, encoding="utf-8"):
        p = line.rstrip("\n").split("\t")
        new[p[0]] = int(p[1], 16)
    for ch in "「」…“”":
        assert ch in new, ch
    return e, new


_B1 = {}


def enc(s, mp):
    if not _B1:
        _B1["e"], _B1["n"] = load_b1()
    e, new = _B1["e"], _B1["n"]
    out = []
    q = 0
    i = 0
    while i < len(s):
        ch = s[i]
        o = ord(ch)
        if ch == ".":
            j = i
            while j < len(s) and s[j] == ".":
                j += 1
            code = new["…"] if j - i >= 2 else e["."]
            out.append("\\x%02X\\x%02X" % (code >> 8, code & 0xFF))
            i = j if j - i >= 2 else i + 1
            continue
        if ch == " ":
            out.append("\\x20")
        elif ch in "[]":
            code = new["「" if ch == "[" else "」"]
            out.append("\\x%02X\\x%02X" % (code >> 8, code & 0xFF))
        elif ch == '"':
            code = new["“" if q % 2 == 0 else "”"]
            q += 1
            out.append("\\x%02X\\x%02X" % (code >> 8, code & 0xFF))
        elif "가" <= ch <= "힣":
            code = mp[ch]
            out.append("\\x%02X\\x%02X" % (code >> 8, code & 0xFF))
        else:
            assert o < 128 and ch in e, repr(ch)
            code = e[ch]
            out.append("\\x%02X\\x%02X" % (code >> 8, code & 0xFF))
        i += 1
    return "".join(out)


def enc_name(s, mp):
    assert len(s) <= KO_EQ_NAME_MAX, (len(s), repr(s))
    out = []
    for ch in s:
        o = ord(ch)
        if ch == " ":
            out.append("\\x00")
        elif o < 128:
            assert 0x20 <= o < 0x7F, repr(ch)
            out.append("\\x%02X" % (o - 0x20))
        else:
            assert "가" <= ch <= "힣", repr(ch)
            code = mp[ch]
            assert 0xF040 <= code <= 0xF375, (ch, hex(code))
            out.append("\\x%02X\\x%02X" % (code >> 8, code & 0xFF))
    out.append("\\xFF\\x00")
    return "".join(out)


ROW_RE = re.compile(r'/\* (0x[0-9A-F]+) \*/ \{_S\("(.*)"\), "(.*)", (.*)\},?\s*$')


def load_tsv():
    ko = {}
    for r in [l.rstrip("\n").split("\t")
              for l in io.open(TSV, encoding="utf-8")]:
        if len(r) > 3 and r[2] and r[0].split(":")[-1] in ("name", "desc") and r[3]:
            ko[r[0]] = (r[2], r[3])
    return ko


def main():
    mp = load_map()
    ko = load_tsv()
    for src, dst, tag in [("equipment.h", "equipment_ko.h", "g_EquipDefs"),
                          ("accessories.h", "accessories_ko.h",
                           "g_AccessoryDefs")]:
        lines = io.open(os.path.join(GEN, src), encoding="utf-8").read().split("\n")
        n = 0
        for i, line in enumerate(lines):
            m = ROW_RE.match(line)
            if not m:
                continue
            idx, name = m.group(1), m.group(2)
            key = "%s:%s:%s:desc" % (src, tag, idx)
            if key in ko and ko[key]:
                lines[i] = '/* %s */ {_S("%s"), "%s", %s},' % (
                    idx, name, enc(ko[key][1], mp), m.group(4))
                n += 1
        io.open(os.path.join(GEN, dst), "w", encoding="utf-8").write("\n".join(lines))
        print("%s: %d descs" % (dst, n))


def bake_names():
    mp11 = load_map_v11()
    ko = load_tsv()
    for src, dst, tag in [("equipment.h", "equipment_ko.h", "g_EquipDefs"),
                          ("accessories.h", "accessories_ko.h",
                           "g_AccessoryDefs")]:
        lines = io.open(os.path.join(GEN, dst), encoding="utf-8").read().split("\n")
        n = 0
        for i, line in enumerate(lines):
            m = ROW_RE.match(line)
            if not m:
                continue
            idx = m.group(1)
            key = "%s:%s:%s:name" % (src, tag, idx)
            if key in ko and ko[key]:
                lines[i] = '/* %s */ {"%s", "%s", %s},' % (
                    idx, enc_name(ko[key][1], mp11), m.group(3), m.group(4))
                n += 1
        io.open(os.path.join(GEN, dst), "w", encoding="utf-8").write("\n".join(lines))
        print("%s: %d names" % (dst, n))


# --- DRA용 _mod_ko.h (_mod 표를 원본으로) ---
MOD_BUNDLE = "ko_jp_bundle.bin"
MOD_BUNDLE_BASE = 0x801F0000  # KR hanpae name-block address (bundle load target)


def mod_key(mod_src, retail_src, tag, idx, ko, kind):
    """Saturn 전용행은 _mod 키, 공유행은 같은 idx의 retail 키. 특수행
    (retail 0xA9-0xD8 = mod 0xB0-0xDF)은 KO가 비어 있어 호출 전에 걸러짐."""
    k = "%s:%s:%s:%s" % (mod_src, tag, idx, kind)
    if k in ko:
        return k
    v = int(idx, 16)
    if mod_src == "equipment_mod.h" and 0xB0 <= v <= 0xDF:
        idx = "0x%03X" % (v - 7)
    k = "%s:%s:%s:%s" % (retail_src, tag, idx, kind)
    if k in ko:
        return k
    return None


def ko_name_bytes(s, mp):
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


def bake_mod():
    mp = load_map()
    mp11 = load_map_v11()
    ko = load_tsv()
    stats = {"inplace": 0, "ext": 0, "ext_bytes": 0, "saturn_en": 0,
             "warn": 0, "freed": 0, "desc": 0}
    ext_rows = (set(), set())  # table 0 = equip, 1 = acc: bundle-pointer rows
    bundle = bytearray()
    mod_row_re = re.compile(
        r'^(/\* 0x([0-9A-F]+) \*/) \{_S\("((?:[^"\\]|\\.)*)"\),(.*)$')
    for mod_src, retail_src, dst, tag in [
            ("equipment_mod.h", "equipment.h", "equipment_mod_ko.h",
             "g_EquipDefs"),
            ("accessories_mod.h", "accessories.h", "accessories_mod_ko.h",
             "g_AccessoryDefs")]:
        lines = io.open(os.path.join(GEN, mod_src),
                        encoding="utf-8").read().split("\n")
        for li, line in enumerate(lines):
            m = mod_row_re.match(line)
            if not m:
                continue
            head, idx, en, rest = m.group(1), "0x" + m.group(2), m.group(3), m.group(4)
            if not all(0x20 <= ord(c) < 0x7F for c in en):
                continue
            nkey = mod_key(mod_src, retail_src, tag, idx, ko, "name")
            dkey = mod_key(mod_src, retail_src, tag, idx, ko, "desc")
            if nkey is None and dkey is None:
                stats["saturn_en"] += 1
                continue
            # desc: 1안 제자리 KO (공유행 + Saturn 신규행, 빈 칸은 스킵)
            dm = re.match(r'^ "(.*)",(.*)$', rest, re.DOTALL)
            assert dm, (mod_src, idx)
            if dkey is not None:
                rest = ' "%s",%s' % (enc(ko[dkey][1], mp), dm.group(2))
                stats["desc"] += 1
            # name: 이름 규칙 (제자리 or 번들)
            if nkey is not None:
                want_en, s = ko[nkey]
                if not en_match(want_en, en):
                    stats["warn"] += 1
                kb = ko_name_bytes(s, mp11)
                slot = len(en) + 1
                freed_baseline = len(en) + 2
                need = len(kb) + 2
                if need <= slot and en_match(want_en, en):
                    pad = slot - need
                    lit = esc(kb) + "\\xFF\\x00" + "\\x00" * pad
                    lines[li] = '%s {"%s",%s' % (head, lit, rest)
                    stats["inplace"] += 1
                else:
                    addr = MOD_BUNDLE_BASE + len(bundle)
                    lines[li] = "%s {(const char*)0x%X,%s" % (head, addr, rest)
                    ext_rows[0 if tag == "g_EquipDefs" else 1].add(int(idx, 16))
                    bundle += kb + bytes([0xFF, 0x00, 0x00])
                    stats["ext"] += 1
                    stats["ext_bytes"] = len(bundle)
                    stats["freed"] += freed_baseline
            else:
                lines[li] = "%s {_S(%s),%s" % (head, '"%s"' % en, rest)
        io.open(os.path.join(GEN, dst), "w", encoding="utf-8").write("\n".join(lines))
    bpath = os.path.join(GEN, MOD_BUNDLE)
    io.open(bpath, "wb").write(bytes(bundle))
    io.open(os.path.join(GEN, "ko_jp_bundle_rows.json"), "w",
            encoding="utf-8").write(
        '{"equip": %s, "acc": %s}' % (sorted(ext_rows[0]), sorted(ext_rows[1])))
    print("mod_ko: inplace=%d ext=%d ext_bytes=%d rodata_freed=%d "
          "saturn_en=%d warn=%d desc=%d"
          % (stats["inplace"], stats["ext"], stats["ext_bytes"],
             stats["freed"], stats["saturn_en"], stats["warn"],
             stats["desc"]))
    print("NOTE: DRA rodata shrinks by rodata_freed + desc shrink; "
          "compensate with MOD_DRA_PAD (42398.c KO branch, check_dra_layout mechanism)")


if __name__ == "__main__":
    main()
    bake_names()
    bake_mod()
