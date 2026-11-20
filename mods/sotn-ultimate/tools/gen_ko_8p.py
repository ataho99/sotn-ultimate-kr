#!/usr/bin/env python3
# gen_ko_8p.py -- equipment-name 8px plate (KO_8P block) v246: ko_8p_block_243.bin
# (283 cells) + characters that equipment names use but the plate lacks, and codes
# re-pointed to the current glyph map.
#
# Block layout (eq_core.c): u16 count, count x u16 code (LE), count x 8 B cell
# (1bpp, MSB = pixel 0, 8 rows). Name bytes are glyph-map codes (F0xx pairs), so
# the plate code of a character must be its glyph-map code.
#
# Cell source: v11 MGalmuri8 cells; characters v11 lacks come from Galmuri7.ttf at size 8 (mono, top ink
# row, 8x8) -- the same glyphs, verified bit-exact against all existing plate cells (positive control below).
# Cell rule (verified against all existing Hangul plate cells, see check below):
# plate cell = the 16x15 v11 MGalmuri8 cell (ko_glyph_block_mgalmuri8_v11.bin),
# columns 0..7, 8 rows starting at the first ink row.
#
# Positive control: the rule must reproduce every existing Hangul cell of the old
# block bit for bit before any new cell is added. Fail closed.
import io
from ko_paths import KO_WK, KO_FONTS  # noqa: E402
import struct
import sys

import numpy as np

P4 = (KO_WK + "/P4시험/")
OLD = P4 + "ko_8p_block_243.bin"
OLD_MAP = P4 + "ko_8p_map_243.tsv"
V11B = P4 + "ko_glyph_block_mgalmuri8_v11.bin"
V11M = P4 + "ko_glyph_map_mgalmuri8_v11.tsv"
V13P = P4 + "" + __import__("ko_font_profile").P["map_name"] + ""
TSV = (KO_WK + "/번역/번역_v4_장비.tsv")
OUT = P4 + "ko_8p_block_246.bin"
OUTMAP = P4 + "ko_8p_map_246.tsv"


def is_h(c):
    return "가" <= c <= "힣"


def rd(path, base_ok=True):
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
    assert n == 283 and len(old) == 2 + n * 10, (n, len(old))
    codes = list(struct.unpack("<%dH" % n, old[2:2 + n * 2]))
    cells = [old[2 + n * 2 + i * 8:2 + n * 2 + i * 8 + 8] for i in range(n)]
    pmap = rd(OLD_MAP)  # char -> old plate code (hex without 0x: int(x,16) works)
    v13 = rd(V13P)
    v11b = open(V11B, "rb").read()
    v11 = {}
    for ln in io.open(V11M, encoding="utf-8"):
        q = ln.rstrip(chr(10)).split(chr(9))
        if len(q) >= 3 and len(q[0]) == 1 and q[1].startswith("0x"):
            v11[q[0]] = int(q[2])

    from PIL import Image, ImageDraw, ImageFont
    G7 = ImageFont.truetype((KO_FONTS + "/Galmuri/v2.40.4/Galmuri-v2.40.4/Galmuri7.ttf"), 8)

    def cell_g7(ch):
        im = Image.new("1", (16, 16), 0)
        d = ImageDraw.Draw(im)
        d.fontmode = "1"
        d.text((0, 0), ch, font=G7, fill=1)
        a = np.array(im, dtype=np.uint8)
        rows = np.where(a.any(axis=1))[0]
        assert len(rows) and rows.max() - rows.min() < 8, ("Galmuri7 glyph empty/too tall", ch)
        t = int(rows.min())
        return np.packbits(a[t:t + 8, 0:8], axis=1).reshape(8).tobytes(), t

    def cell_of(ch):
        if ch not in v11:
            return cell_g7(ch)
        k = v11[ch]
        a = np.unpackbits(np.frombuffer(v11b[k * 30:k * 30 + 30], dtype=np.uint8)).reshape(15, 16)
        rows = np.where(a.any(axis=1))[0]
        dy = int(rows.min())
        assert rows.max() - dy < 8, ("ink taller than 8 rows", ch)
        return np.packbits(a[dy:dy + 8, 0:8], axis=1).reshape(8).tobytes(), dy

    # positive control: rule == existing cells
    chk = bad = 0
    for ch, code in pmap.items():
        if not is_h(ch) or ch not in v11:
            continue
        i = codes.index(code)
        chk += 1
        if cell_of(ch)[0] != cells[i]:
            bad += 1
    g7bad = sum(1 for ch, code in pmap.items() if is_h(ch) and cell_g7(ch)[0] != cells[codes.index(code)])
    print("positive control: %d existing Hangul cells reproduced by the rule, %d differ; Galmuri7@8 vs all plate cells: %d differ" % (chk, bad, g7bad))
    assert g7bad == 0, "Galmuri7@8 does not reproduce the plate"
    assert chk > 200 and bad == 0, "cell rule does not reproduce the old plate"

    # plate cells of Hangul that left the glyph map (their code may now belong to another character)
    inv_old = {code: ch for ch, code in pmap.items()}
    keep = [i for i in range(len(codes)) if not (is_h(inv_old.get(codes[i], "a")) and inv_old[codes[i]] not in v13)]
    dropped = "".join(inv_old[codes[i]] for i in range(len(codes)) if i not in keep)
    codes = [codes[i] for i in keep]
    cells = [cells[i] for i in keep]
    for ch in dropped:
        del pmap[ch]
    if dropped:
        print("dropped plate cells of chars no longer in the glyph map:", dropped)

    # equipment names: Hangul that need a plate cell
    need = []
    for ln in io.open(TSV, encoding="utf-8"):
        q = ln.rstrip(chr(10)).split(chr(9))
        if len(q) > 3 and q[0].endswith(":name"):
            for ch in q[3]:
                if is_h(ch) and ch not in pmap and ch not in need:
                    need.append(ch)
    # System (config) screen 8px labels (gen_ko_cfg.py): rows of kind 메뉴(8px)
    for ln in io.open((KO_WK + "/번역/번역_설정화면.tsv"), encoding="utf-8").read().split(chr(10))[1:]:
        q = ln.split(chr(9))
        if len(q) > 4 and q[1].startswith("메뉴"):
            for ch in q[4]:
                if is_h(ch) and ch not in pmap and ch not in need:
                    need.append(ch)
    # re-point plate codes that differ from the glyph map
    fixed = []
    for ch, code in pmap.items():
        if is_h(ch) and v13.get(ch) is not None and v13[ch] != code:
            i = codes.index(code)
            codes[i] = v13[ch]
            pmap[ch] = v13[ch]
            fixed.append((ch, code, v13[ch]))
    for ch in need:
        assert ch in v13, ("no glyph code (run gen_ko_glyph_add.py first) for", ch)
        c, dy = cell_of(ch)
        codes.append(v13[ch])
        cells.append(c)
        pmap[ch] = v13[ch]
        print("added %s code %04X (top ink row %d)" % (ch, v13[ch], dy))
        print("\n".join("".join("#" if (c[r] >> (7 - x)) & 1 else "." for x in range(8)) for r in range(8)))
    assert len(set(codes)) == len(codes), "duplicate plate codes"
    blk = struct.pack("<H", len(codes)) + struct.pack("<%dH" % len(codes), *codes) + b"".join(cells)
    assert len(blk) <= 4096
    open(OUT, "wb").write(blk)
    with io.open(OUTMAP, "w", encoding="utf-8") as f:
        f.write("char" + chr(9) + "code" + chr(10))
        for ch, code in pmap.items():
            f.write("%s%s%04X%s" % (ch, chr(9), code, chr(10)))
    import hashlib
    print("re-pointed:", " ".join("%s:%04X->%04X" % x for x in fixed))
    print("block %d cells, %d B, sha256 %s" % (len(codes), len(blk), hashlib.sha256(blk).hexdigest()))


if __name__ == "__main__":
    sys.exit(main())
