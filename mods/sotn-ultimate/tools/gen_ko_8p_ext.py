#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
# gen_ko_8p_ext.py -- 8px plate (KO_8P block) v246 (371 cells) -> v247: append the cells the Extra settings popup
# needs (rows "xset:" of 번역_설정화면.tsv, kind 메뉴(8px)): Hangul that the plate lacks and ASCII (code = the ASCII
# value) that it lacks. Existing cells and codes stay byte-identical (the new ones are appended behind them).
#
# Cell rule (same as gen_ko_8p.py): Hangul = the v11 MGalmuri8 16x15 cell, columns 0..7, 8 rows from the first ink
# row; a char v11 lacks comes from Galmuri7.ttf at size 8 (mono, top ink row). ASCII = Galmuri7 at size 8.
# Positive controls (fail closed): the Hangul rule reproduces every existing Hangul cell of v246; the ASCII rule
# reproduces every existing ASCII cell of v246.
# Block layout (eq_core.c): u16 count, count x u16 code (LE), count x 8 B cell (1bpp, MSB = pixel 0, 8 rows).
import hashlib
import io
import os
import struct
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ko_paths import KO_WK, KO_FONTS  # noqa: E402
import ko_font_profile as FP  # noqa: E402

P4 = KO_WK + "/P4시험/"
OLD = P4 + "ko_8p_block_246.bin"
OLD_MAP = P4 + "ko_8p_map_246.tsv"
OUT = P4 + "ko_8p_block_247.bin"
OUTMAP = P4 + "ko_8p_map_247.tsv"
V11B = P4 + "ko_glyph_block_mgalmuri8_v11.bin"
V11M = P4 + "ko_glyph_map_mgalmuri8_v11.tsv"
GMAP = P4 + FP.P["map_name"]
TSV = KO_WK + "/번역/번역_설정화면.tsv"


def is_h(c):
    return "가" <= c <= "힣"


def rd(path):
    m = {}
    for ln in io.open(path, encoding="utf-8"):
        q = ln.rstrip(chr(10)).split(chr(9))
        if len(q) >= 2 and len(q[0]) == 1:
            try:
                m[q[0]] = int(q[1], 16)
            except ValueError:
                pass
    return m


def main():
    old = open(OLD, "rb").read()
    n = struct.unpack("<H", old[:2])[0]
    assert n == 371 and len(old) == 2 + n * 10, (n, len(old))
    codes = list(struct.unpack("<%dH" % n, old[2:2 + n * 2]))
    cells = [old[2 + n * 2 + i * 8:2 + n * 2 + i * 8 + 8] for i in range(n)]
    pmap = rd(OLD_MAP)
    gmap = rd(GMAP)
    v11b = open(V11B, "rb").read()
    v11 = {}
    for ln in io.open(V11M, encoding="utf-8"):
        q = ln.rstrip(chr(10)).split(chr(9))
        if len(q) >= 3 and len(q[0]) == 1 and q[1].startswith("0x"):
            v11[q[0]] = int(q[2])
    G7 = ImageFont.truetype(KO_FONTS + "/Galmuri/v2.40.4/Galmuri-v2.40.4/Galmuri7.ttf", 8)

    def cell_g7(ch):
        im = Image.new("1", (16, 16), 0)
        d = ImageDraw.Draw(im)
        d.fontmode = "1"
        d.text((0, 0), ch, font=G7, fill=1)
        a = np.array(im, dtype=np.uint8)
        rows = np.where(a.any(axis=1))[0]
        assert len(rows) and rows.max() - rows.min() < 8, ("Galmuri7 glyph empty/too tall", ch)
        t = int(rows.min())
        return np.packbits(a[t:t + 8, 0:8], axis=1).reshape(8).tobytes()

    # ASCII: the baseline rule -- every glyph is cut with the SAME first row (that of the capital letters:
    # Galmuri7@8 puts the caps' top ink row at tcap), so floating glyphs (* - + , .) keep their height
    def ink_top(ch):
        im = Image.new("1", (16, 16), 0)
        d = ImageDraw.Draw(im)
        d.fontmode = "1"
        d.text((0, 0), ch, font=G7, fill=1)
        a = np.array(im, dtype=np.uint8)
        return a, int(np.where(a.any(axis=1))[0].min())

    tcap = ink_top("M")[1]

    def cell_a(ch):
        a, _ = ink_top(ch)
        assert a[tcap:tcap + 8, 0:8].any(), ("empty ASCII cell", ch)
        assert not a[tcap + 8:, :].any() and not a[:tcap, :].any(), ("ASCII glyph outside rows", ch)
        return np.packbits(a[tcap:tcap + 8, 0:8], axis=1).reshape(8).tobytes()

    def cell_h(ch):
        if ch not in v11:
            return cell_g7(ch)
        k = v11[ch]
        a = np.unpackbits(np.frombuffer(v11b[k * 30:k * 30 + 30], dtype=np.uint8)).reshape(15, 16)
        rows = np.where(a.any(axis=1))[0]
        dy = int(rows.min())
        assert rows.max() - dy < 8, ("ink taller than 8 rows", ch)
        return np.packbits(a[dy:dy + 8, 0:8], axis=1).reshape(8).tobytes()

    # positive control 1: the Hangul rule reproduces the existing Hangul cells
    chk = bad = 0
    for ch, code in pmap.items():
        if is_h(ch) and code in codes:
            chk += 1
            if cell_h(ch) != cells[codes.index(code)]:
                bad += 1
    print("positive control (Hangul): %d existing cells, %d differ" % (chk, bad))
    assert chk > 250 and bad == 0
    # positive control 2: the ASCII rule reproduces the existing ASCII cells
    achk = abad = 0
    ascii_have = [c for c in codes if c < 0x80]
    for code in ascii_have:
        ch = chr(code)
        if ch == " ":
            continue
        achk += 1
        if cell_a(ch) != cells[codes.index(code)]:
            abad += 1
            print("  ASCII %r differs from Galmuri7@8 (baseline rule)" % ch)
    print("positive control (ASCII): %d existing cells, %d differ" % (achk, abad))

    # the chars the popup needs
    need_h, need_a = [], []
    for ln in io.open(TSV, encoding="utf-8").read().split(chr(10))[1:]:
        q = ln.split(chr(9))
        if len(q) > 4 and q[0].startswith("xset:") and q[1].startswith("메뉴"):
            for ch in q[4]:
                if is_h(ch) and gmap.get(ch) is not None and gmap[ch] not in codes and ch not in need_h:
                    need_h.append(ch)
                elif is_h(ch) and gmap.get(ch) is None:
                    raise SystemExit("no glyph code for %s (run gen_ko_glyph_add.py first)" % ch)
                elif ch != " " and ord(ch) < 128 and ord(ch) not in codes and ch not in need_a:
                    need_a.append(ch)
    print("new Hangul cells:", "".join(need_h), "| new ASCII cells:", "".join(need_a))
    if abad:
        raise SystemExit("ASCII cell rule does not reproduce the existing ASCII cells: refusing to add ASCII")
    new_codes, new_cells = [], []
    for ch in need_h:
        new_codes.append(gmap[ch])
        new_cells.append(cell_h(ch))
    for ch in need_a:
        new_codes.append(ord(ch))
        new_cells.append(cell_a(ch))
    allcodes = codes + new_codes
    assert len(set(allcodes)) == len(allcodes), "duplicate plate codes"
    blk = struct.pack("<H", len(allcodes)) + struct.pack("<%dH" % len(allcodes), *allcodes) + b"".join(cells + new_cells)
    assert len(blk) <= 4096, len(blk)
    # existing cells and codes are a byte-identical prefix of each list
    assert allcodes[:n] == codes and (cells + new_cells)[:n] == cells
    open(OUT, "wb").write(blk)
    with io.open(OUTMAP, "w", encoding="utf-8", newline="") as f:
        f.write("char" + chr(9) + "code" + chr(10))
        for ch, code in pmap.items():
            f.write("%s%s%04X%s" % (ch, chr(9), code, chr(10)))
        for ch in need_h:
            f.write("%s%s%04X%s" % (ch, chr(9), gmap[ch], chr(10)))
        for ch in need_a:
            f.write("%s%s%04X%s" % (ch, chr(9), ord(ch), chr(10)))
    for ch in need_h + need_a:
        c = (new_cells[(need_h + need_a).index(ch)])
        print("added %s" % ch)
        print("\n".join("".join("#" if (c[r] >> (7 - x)) & 1 else "." for x in range(8)) for r in range(8)))
    print("block %d cells, %d B (%d sectors), sha256 %s" % (len(allcodes), len(blk), (4096 + 2047) // 2048, hashlib.sha256(blk).hexdigest()))
    print("NEXT: eq_core.c KO_8pValid n == %d; build_ko_trial.sh ko_8p_block_247.bin" % len(allcodes))


if __name__ == "__main__":
    sys.exit(main())
