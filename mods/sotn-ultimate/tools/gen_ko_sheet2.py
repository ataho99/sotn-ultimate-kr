#!/usr/bin/env python3
# gen_ko_sheet2.py -- big (12x16) Hangul labels of the status/equipment screen:
# second sheet upload list + index strings.
#
# The menu draws labels from a glyph "sheet" in VRAM: func_800F98AC(list, y)
# uploads the glyphs of a code list (cell k at x = 3*k words), func_800F66BC
# draws an 0xFF-terminated string of cell indexes (index & 0x10 selects the
# second page, index & 0xF the column). Page 1 (indexes 0x00..0x0F) is full of
# the old labels; page 2 (0x10..0x1F) holds the glyphs uploaded by UPLOAD2.
# This tool writes the NEW UPLOAD2 list and the index strings into the zero gap
# of ARC_KO behind the engtab (RAM 0x8009F100..), see dra.h KO_SHEET2_*.
#   page-2 cell : glyph
#   0x10 오  0x11 른  0x12 손  0x13 왼   (old hand labels, same indexes)
#   0x14 력  (공격력 / 방어력)   0x15 머 0x16 리 0x17 몸 0x18 망 0x19 토 0x1A 타 0x1B 홀 0x1C 더
#   page-1 reuse: 공=0x0A 격=0x0B 방=0x0C 어=0x0D 기=0x02
# Output: P4시험/ko_sheet2.bin  (zero based; goes to ARC offset 0x6100; <= 0x100 B, KO_FONT8_ADV at 0x6200)
#   + menu titles FAMLIST/SPECIAL (8 B slots at +0x38/+0x40) and UPLOAD3 (+0x48, sheet row 1, indexes 0x30..)
import io
from ko_paths import KO_WK  # noqa: E402
import os
import sys

WK = (KO_WK + "/P4시험/")
MAP = WK + "" + __import__("ko_font_profile").P["map_name"] + ""
OUT = WK + "ko_sheet2.bin"
BASE_ADDR = 0x8009F100

UPLOAD2 = list("오른손왼력머리몸망토타홀더")  # index 0x10 + position
assert len(UPLOAD2) <= 16
IDX = {c: 0x10 + i for i, c in enumerate(UPLOAD2)}
IDX.update({"공": 0x0A, "격": 0x0B, "방": 0x0C, "어": 0x0D, "기": 0x02})
LABELS = [  # name, text
    ("ATK", "공격력"), ("DEF", "방어력"),
    ("HEAD", "머리"), ("BODY", "몸"), ("CLOAK", "망토"), ("OTHER", "기타"), ("HOLDER", "홀더"),
]
# Menu titles (g_MenuStr[97] Familiar List / [98] Special Moves List, menu.c MOD_KO), text from 번역_v14.tsv
# (the 8px English titles had no KO path). Fixed 8 B slots after HOLDER (dra.h KO_SHEET_FAMLIST/SPECIAL),
# so a longer translation fails here instead of moving the other labels. Their glyphs that are not on
# the sheet yet go to UPLOAD3 = sheet row 1 of tpage 7 (indexes 0x30.., uploaded with y 0x101 like the
# PSP's fourth list; US retail uploads only row 0). Page-1 reuse: 사=0x08 역=0x09 마=0x04 (KO_SHEET_ALL).
IDX.update({"사": 0x08, "역": 0x09, "마": 0x04})
V14 = __import__("os").environ.get("KO_V14_TSV") or (KO_WK + "/검수/번역_v14.tsv")
TITLES = [("FAMLIST", "config_us.h:g_MenuStr:97", "사역마목록"),
          ("SPECIAL", "config_us.h:g_MenuStr:98", "필살기목록")]
TITLE_SLOT = 8      # 7 cells + 0xFF
UPLOAD3_OFF = 0x48  # KO_SHEET_UPLOAD3 = 0x8009F148
UPLOAD3_MAX = 16    # one sheet row
BLOB_MAX = 0x100    # 0x8009F200 = KO_FONT8_ADV_ADDR


def load_titles():
    t = {}
    for ln in io.open(V14, encoding="utf-8"):
        q = ln.rstrip(chr(10)).split(chr(9))
        if len(q) > 3:
            t[q[0]] = q[3].strip()
    return [(n, (t.get(k) or d).replace(" ", "")) for n, k, d in TITLES]


def load_map():
    mp = {}
    for ln in io.open(MAP, encoding="utf-8"):
        q = ln.rstrip(chr(10)).split(chr(9))
        if len(q) >= 2 and q[1].startswith("0x") and len(q[0]) == 1:
            mp[q[0]] = int(q[1], 16)
    return mp


def main():
    mp = load_map()
    blob = bytearray()
    addr = {}
    addr["UPLOAD2"] = BASE_ADDR
    for c in UPLOAD2:
        assert c in mp, ("no glyph cell for", c)
        blob += mp[c].to_bytes(2, "big")
    blob += b"\x00"
    while len(blob) % 4:
        blob += b"\x00"
    for name, text in LABELS:
        addr[name] = BASE_ADDR + len(blob)
        for ch in text:
            blob.append(IDX[ch])
        blob.append(0xFF)
        while len(blob) % 4:
            blob.append(0)
    assert BASE_ADDR + len(blob) == 0x8009F138, hex(BASE_ADDR + len(blob))  # dra.h KO_SHEET_HOLDER + 4
    titles = load_titles()
    up3 = []
    for name, text in titles:
        for ch in text:
            if ch not in IDX and ch not in up3:
                up3.append(ch)
    assert len(up3) <= UPLOAD3_MAX, ("UPLOAD3 over one sheet row", "".join(up3))
    idx3 = dict(IDX)
    idx3.update({c: 0x30 + i for i, c in enumerate(up3)})
    for name, text in titles:
        addr[name] = BASE_ADDR + len(blob)
        assert 0 < len(text) < TITLE_SLOT, ("title too long for its slot", name, text)
        for ch in text:
            blob.append(idx3[ch])
        blob.append(0xFF)
        blob += bytes(TITLE_SLOT - 1 - len(text))
    assert len(blob) == UPLOAD3_OFF, hex(len(blob))
    addr["UPLOAD3"] = BASE_ADDR + len(blob)
    for c in up3:
        assert c in mp, ("no glyph cell for", c)
        blob += mp[c].to_bytes(2, "big")
    blob += b"\x00"
    while len(blob) % 4:
        blob += b"\x00"
    assert len(blob) <= BLOB_MAX, len(blob)
    open(OUT, "wb").write(bytes(blob))
    print("ko_sheet2.bin %d B" % len(blob))
    for k, v in addr.items():
        print("%-8s 0x%08X" % (k, v))


if __name__ == "__main__":
    sys.exit(main())
