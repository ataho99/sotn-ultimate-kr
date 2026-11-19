#!/usr/bin/env python3
# gen_ko_glyph12.py -- KO glyph block v13n ("안 나"): 12-column storage.
#
# Original game text path (func_80106A28) squeezes a 16x15 1bpp cell to 12 px
# with a 3/4 box filter. Our MGalmuri12 dots only use columns 0..10 of the old
# 16-column cell, so the squeeze made them ~8 px wide. New layout: each cell is
# 12 columns x 15 rows 1bpp, packed MSB-first row after row = 180 bits = 23 B
# (no padding between cells). The MOD_KO branch of func_80106A28 expands it
# straight to the 4-bit intensity words (ink = full), skipping the squeeze.
#
# Inputs : ko_glyph_block_mgalmuri12_v12g.bin (776 x 30 B, 16x15)  [old cells]
#          ko_glyph_map_v12g.tsv, ko_glyph_map_v12f_new.tsv, ko_engtab_251.bin
#          MGalmuri12.ttf (new glyphs), translation TSV (extra chars)
# Outputs: ko_glyph_block_mgalmuri12_v13n.bin (N x 23 B)
#          ko_glyph_map_v13n.tsv
#          ko_glyph12_report.txt
import hashlib
from ko_paths import KO_WK, KO_FONTS  # noqa: E402
import io
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont

BASE = (KO_WK + "/")
P4 = BASE + "P4시험/"
TTF = (KO_FONTS + "/MonadABXY/mgalmuri-font/_ttf/MGalmuri12.ttf")
OLD_BLK = P4 + "ko_glyph_block_mgalmuri12_v12g.bin"
OLD_MAP = P4 + "ko_glyph_map_v12g.tsv"
NEW_MAP = P4 + "ko_glyph_map_v12f_new.tsv"
ENGTAB = P4 + "ko_engtab_251.bin"
OUT_BLK = P4 + "ko_glyph_block_mgalmuri12_v13n.bin"
OUT_MAP = P4 + "ko_glyph_map_v13n.tsv"
OUT_REP = P4 + "ko_glyph12_report.txt"
TSV_EXTRA = sys.argv[1] if len(sys.argv) > 1 else BASE + "번역/번역_v13_통합.tsv"
BASE_CODE = 0xF040
CELL_BYTES = 23
MAX_CELLS = 960  # codes must stay F040..F3FF (first byte 0xF0..0xF3)

# 29 hangul whose cell was reused for an ASCII glyph (engtab) but are still used
CONFLICT = list("뼈객겁겨견굴궁근꾸둔뛰랙럿렘례룡룩맏며묻붙빔뿜셰솔숭싫왈윌")
assert len(CONFLICT) == 29 and len(set(CONFLICT)) == 29

F = ImageFont.truetype(TTF, 12)


def render(ch):
    im = Image.new("1", (16, 16), 0)
    d = ImageDraw.Draw(im)
    d.fontmode = "1"
    d.text((0, 0), ch, font=F, fill=1)
    a = np.array(im, dtype=np.uint8)
    out = np.zeros((15, 12), dtype=np.uint8)
    out[1:, :] = a[:14, :12]  # +1 row shift (v12 layout: ink rows 2..12)
    return out


def read_map(path):
    m = {}
    for ln in open(path, encoding="utf-8"):
        p = ln.rstrip("\n").split("\t")
        if len(p) >= 2 and p[1].startswith("0x") and len(p[0]) == 1:
            m[p[0]] = int(p[1], 16)
    return m


def pack(design):
    bits = design.reshape(-1)
    pad = (-len(bits)) % 8
    b = np.packbits(np.concatenate([bits, np.zeros(pad, dtype=np.uint8)]))
    assert len(b) == CELL_BYTES, len(b)
    return b.tobytes()


def main():
    raw = open(OLD_BLK, "rb").read()
    assert len(raw) == 776 * 30
    old = np.unpackbits(np.frombuffer(raw, dtype=np.uint8)).reshape(776, 15, 16)
    assert not old[:, :, 12:].any(), "ink beyond column 11"
    design = [old[i, :, :12].copy() for i in range(776)]
    omap = read_map(OLD_MAP)
    nmap = read_map(NEW_MAP)
    eng = np.frombuffer(open(ENGTAB, "rb").read(), dtype="<u2")
    ecodes = set(int(c) for c in eng if c)
    rep = []

    # hangul whose old code was reassigned to a symbol cell (v12f: 「 」 … " " 1 2 5 + % B):
    # still-used ones must get a new cell; unused ones are dropped
    ncodes = set(nmap.values())
    usedall = set()
    for ln in io.open((KO_WK + "/검수/번역_v14.tsv"), encoding="utf-8"):
        p = ln.rstrip("\n").split("\t")
        if len(p) > 3:
            usedall.update(c for c in p[3] if "가" <= c <= "힣")
    shared = [c for c, v in omap.items() if v in ncodes and c not in nmap]
    for c in shared:
        del omap[c]
    rep.append("hangul sharing a symbol cell: %s (used in translation: %s)" % (
        "".join(shared), "".join(c for c in shared if c in usedall)))

    # extra characters used by the v14 dialogue/translation that have no cell yet
    used = set()
    for ln in open(TSV_EXTRA, encoding="utf-8"):
        p = ln.rstrip("\n").split("\t")
        if len(p) < 4 or p[1] != "대사":
            continue
        for ch in p[3]:
            if "가" <= ch <= "힣":
                used.add(ch)
    v14 = KO_WK + "/검수/번역_v14.tsv"
    extra = []
    for ln in open((KO_WK + "/검수/번역_v14.tsv"), encoding="utf-8"):
        p = ln.rstrip("\n").split("\t")
        if len(p) < 4 or p[1] != "대사":
            continue
        for ch in p[3]:
            if "가" <= ch <= "힣" and ch not in omap and ch not in extra:
                extra.append(ch)
    for ch in shared:
        if ch in usedall and ch not in extra:
            extra.append(ch)
    # 뛴 (e_shop 「뛴다」) is not in the table either
    for ch in ("뛴",):
        if ch not in omap and ch not in extra:
            extra.append(ch)

    newchars = CONFLICT + extra
    assert len(newchars) + 776 <= MAX_CELLS
    new_map = dict(omap)
    cells = list(design)
    rep.append("old cells 776, conflict chars %d, extra new chars %d: %s" % (len(CONFLICT), len(extra), "".join(extra)))
    mism = 0
    for k, ch in enumerate(newchars):
        n = 776 + k
        code = BASE_CODE + n
        d = render(ch)
        if ch in CONFLICT:
            # positive control: rendering recipe must equal the pre-engtab v12c cell shape
            c = omap[ch] - BASE_CODE
            v12c = np.unpackbits(np.frombuffer(open(P4 + "ko_glyph_block_mgalmuri12_v12c.bin", "rb").read(), dtype=np.uint8)).reshape(776, 15, 16)
            ref = v12c[c, :, :12]
            diff = int((ref != d).sum())
            if diff:
                mism += 1
                d = ref.copy()  # keep the shipped (v12c) shape when the recipe differs
                rep.append("  %s: render != v12c by %d px -> using v12c shape" % (ch, diff))
        cells.append(d)
        new_map[ch] = code
    rep.append("conflict chars where TTF recipe != v12c: %d (v12c used)" % mism)
    # chars in the new symbol map (「 」 … " " 1 2 5 + % B) keep their existing cells
    for ch, code in nmap.items():
        new_map[ch] = code
    n_cells = len(cells)
    blk = b"".join(pack(c) for c in cells)
    open(OUT_BLK, "wb").write(blk)
    codes = {}
    for ch, code in new_map.items():
        codes.setdefault(code, []).append(ch)
    dup = {c: v for c, v in codes.items() if len(v) > 1}
    # old swap artefacts (unused chars sharing a code with the moved chars)? report
    with open(OUT_MAP, "w", encoding="utf-8") as f:
        f.write("글자\t코드\tn\t오프셋\n")
        for ch, code in sorted(new_map.items(), key=lambda x: x[1]):
            n = code - BASE_CODE
            f.write("%s\t0x%04X\t%d\t%d\n" % (ch, code, n, n * CELL_BYTES))
    rep.append("cells %d, block %d B (%d B per cell), sha256 %s" % (n_cells, len(blk), CELL_BYTES, hashlib.sha256(blk).hexdigest()))
    rep.append("capacity: A-gap glyph area 23280 B holds %d cells at 23 B; code space limit F040..F3FF = %d cells" % (23280 // 23, MAX_CELLS))
    rep.append("duplicate codes in map: %d" % len(dup))
    for c, v in list(dup.items())[:20]:
        rep.append("  dup 0x%04X: %s" % (c, "".join(v)))
    open(OUT_REP, "w", encoding="utf-8").write("\n".join(rep) + "\n")
    print("\n".join(rep))


main()
