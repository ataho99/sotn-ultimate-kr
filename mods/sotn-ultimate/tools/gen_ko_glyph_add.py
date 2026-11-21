#!/usr/bin/env python3
# gen_ko_glyph_add.py -- incremental KO glyph block update (profile mgalmuri12, 23 B cells):
# keep EVERY existing code of the previous block/map (hand-coded sources, promotion
# table, snapshots stay valid) and only give the newly needed Hangul a cell:
#   * cells of Hangul that no text/source uses any more are reused first,
#   * then extension cells at the end (code space F040..F3FF, block <= 23280 B).
# Input : previous block/map (PREV_*), the same "used" sources as gen_ko_glyph16.py
# Output: the profile's block/map (blk_name / map_name)
import hashlib
from ko_paths import KO_WK  # noqa: E402
import io
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_ko_glyph16 as G  # noqa: E402
import ko_font_profile as FP  # noqa: E402

P4 = G.P4
# GLYPH_PREV_TAG (e.g. v14f): the previous block/map to extend (default v14a, the original run);
# GLYPH_OUT_TAG (e.g. v14g): write ko_glyph_block_mgalmuri12_<tag>.bin / ko_glyph_map_<tag>.tsv instead of
# the profile's names; GLYPH_NO_REUSE=1: never reuse a freed cell (existing cells stay byte-identical,
# new chars are appended at the end)
PREV_TAG = os.environ.get("GLYPH_PREV_TAG", "v14a")
OUT_TAG = os.environ.get("GLYPH_OUT_TAG")
PREV_BLK = P4 + "ko_glyph_block_mgalmuri12_%s.bin" % PREV_TAG
PREV_MAP = P4 + "ko_glyph_map_%s.tsv" % PREV_TAG
CB = 23
assert FP.P["cell_bytes"] == CB


def main():
    prev = open(PREV_BLK, "rb").read()
    assert len(prev) % CB == 0
    ncell = len(prev) // CB
    pmap = G.rd(PREV_MAP)
    used = G.tsv_hangul(G.V14) | G.tsv_hangul(G.V4)
    jpb = io.open(G.JPB, encoding="utf-8").read() if os.path.exists(G.JPB) else ""
    used |= set(c for c in jpb if G.is_h(c))
    mid = G.rd(G.MID_MAP)
    hand = G.handcoded_chars(mid)
    for ln in io.open((KO_WK + "/번역/번역_설정화면.tsv"), encoding="utf-8").read().split(chr(10))[1:]:
        q = ln.split(chr(9))
        if len(q) > 4:
            used |= set(c for c in q[4] if G.is_h(c))  # System (config) screen strings (gen_ko_cfg.py)
    used |= hand
    prot = G.protected_chars(G.rd(G.OLD_MAP))
    eng = open(P4 + "ko_engtab_252.bin", "rb").read()
    busy = set(int.from_bytes(eng[i:i + 2], "little") for i in range(0, len(eng), 2)) - {0}
    busy |= set(v for c, v in pmap.items() if not G.is_h(c))
    new = [c for c in sorted(used) if G.is_h(c) and c not in pmap]
    free = [c for c in sorted(pmap, key=lambda c: pmap[c])
            if G.is_h(c) and c not in used and c not in prot and pmap[c] not in busy]
    if os.environ.get("GLYPH_NO_REUSE") == "1":
        free = []
    print("previous block %d cells; new chars %d %s; free cells %d" % (ncell, len(new), "".join(new), len(free)))
    fmap = dict(pmap)
    blk = bytearray(prev)
    assign = {}
    ext = 0
    for i, c in enumerate(new):
        if i < len(free):
            code = pmap[free[i]]
            del fmap[free[i]]
        else:
            code = G.BASE + ncell + ext
            ext += 1
            blk += bytes(CB)
        assign[c] = code
        fmap[c] = code
    ncell2 = len(blk) // CB
    assert G.BASE + ncell2 <= 0xF400 and ncell2 * CB <= 23280, ncell2
    FK = ImageFont.truetype(FP.P["ttf"], FP.P["size"])
    off = FP.P["vert_offset"]
    for c, code in assign.items():
        im = Image.new("1", (32, 32), 0)
        d = ImageDraw.Draw(im)
        d.fontmode = "1"
        d.text((0, 0), c, font=FK, fill=1)
        a = np.array(im, dtype=bool)
        g = np.zeros((15, 12), dtype=np.uint8)
        for y in range(15):
            sy = y - off
            if 0 <= sy < 32:
                g[y, :] = a[sy, :12]
        assert g.any(), ("empty glyph", c)
        n = code - G.BASE
        bits = g.reshape(-1)
        blk[n * CB:n * CB + CB] = np.packbits(np.concatenate([bits, np.zeros(4, dtype=np.uint8)])).tobytes()
    # no code shared
    assert len(set(fmap.values())) == len(fmap)
    # every old code unchanged (except the freed Hangul)
    for c, v in pmap.items():
        if c in fmap:
            assert fmap[c] == v, ("code moved", c)
    blk_name = ("ko_glyph_block_mgalmuri12_%s.bin" % OUT_TAG) if OUT_TAG else FP.P["blk_name"]
    map_name = ("ko_glyph_map_%s.tsv" % OUT_TAG) if OUT_TAG else FP.P["map_name"]
    # positive control: every previous cell is byte-identical in the new block (cells of codes that stay)
    for c, v in pmap.items():
        if c in fmap:
            n0 = v - G.BASE
            assert bytes(blk[n0 * CB:n0 * CB + CB]) == prev[n0 * CB:n0 * CB + CB], ("cell changed", c)
    open(G.P4 + blk_name, "wb").write(bytes(blk))
    with io.open(G.P4 + map_name, "w", encoding="utf-8") as f:
        f.write("글자\t코드\tn\t오프셋\n")
        for c, v in sorted(fmap.items(), key=lambda kv: kv[1]):
            f.write("%s\t0x%04X\t%d\t%d\n" % (c, v, v - G.BASE, (v - G.BASE) * CB))
    print("added: " + " ".join("%s:%04X" % (c, v) for c, v in assign.items()))
    print("freed (cell reused): " + "".join(free[:len(new)]))
    print("block %d cells, %d B, sha256 %s" % (ncell2, len(blk), hashlib.sha256(blk).hexdigest()))


if __name__ == "__main__":
    sys.exit(main())
