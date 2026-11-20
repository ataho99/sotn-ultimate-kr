#!/usr/bin/env python3
# gen_ko_glyph16.py -- KO glyph block: 16px, original
# 16x15 1bpp 30 B cells, 776 cells (the original 23280 B block), original path
# (16->12 squeeze + kind1 interpolation) unchanged.
#
# Every still-used Hangul keeps its v12g cell index (= code). The 31 characters
# that had no usable cell (29 engtab conflicts + nyeom/jim) and the extra v14
# characters take the cells of Hangul that no text uses any more (their old
# shapes are replaced). ASCII / symbol cells keep the v12g shapes.
#
# Rasteriser: 16x supersample, coverage threshold 0.30 (matches the Chrome
# 1-bit rendering of the _ots file).
#
# Outputs: P4시험/ko_glyph_block_*.bin (776 x 30 B; the profile's blk_name)
#          P4시험/ko_glyph_map_*.tsv (the profile's map_name), ko_glyph16_report.txt
import collections
from ko_paths import KO_WK  # noqa: E402
import hashlib
import io
import json
import os
import re
import sys

import numpy as np
from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFont

WK = (KO_WK + "/")
P4 = WK + "P4시험/"
import ko_font_profile as FP  # noqa: E402  (font, size, raster rule, vertical offset)

FONT = FP.P["ttf"]
SIZE = FP.P["size"]
THR = FP.P["coverage_threshold"]
K = FP.P["supersample"]
BASE = 0xF040
NCELL = 776
OLD_BLK = P4 + "ko_glyph_block_mgalmuri12_v12g.bin"
OLD_MAP = P4 + "ko_glyph_map_v12g.tsv"
MID_MAP = P4 + "ko_glyph_map_v13n.tsv"  # symbols + list of new characters
V14 = (KO_WK + "/검수/번역_v14.tsv")
V13 = WK + "번역/번역_v13_통합.tsv"
V4 = WK + "번역/번역_v4_장비.tsv"
JPB = P4 + "ko_jp_bundle_rows.json"
HANDCODED = P4 + "ko_handcoded_chars.txt"
CB = FP.P["cell_bytes"]  # 30 = 16x15 original path, 23 = 12x15 direct path
OUT_BLK = P4 + FP.P["blk_name"]
OUT_MAP = P4 + FP.P["map_name"]
OUT_REP = P4 + "ko_glyph16_report.txt"
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
ROOT = os.environ.get("SOTN_KR", os.getcwd())


def is_h(c):
    return "가" <= c <= "힣"


def rd(p):
    m = {}
    for ln in io.open(p, encoding="utf-8"):
        q = ln.rstrip(chr(10)).split(chr(9))
        if len(q) >= 2 and q[1].startswith("0x") and len(q[0]) == 1:
            m[q[0]] = int(q[1], 16)
    return m


def tsv_hangul(path, col_prefix="번역"):
    s = set()
    rows = [l.rstrip(chr(10)).split(chr(9)) for l in io.open(path, encoding="utf-8")]
    h = rows[0]
    ti = [i for i, x in enumerate(h) if x.startswith(col_prefix)]
    ti = ti[0] if ti else None
    for r in rows[1:]:
        cells = r if ti is None else r[ti:ti + 1]
        for c in cells:
            s.update(ch for ch in c if is_h(ch))
    return s


def handcoded_chars(mid):
    """Hangul used by hand-coded sources; cached because the sources get
    recoded to the final map afterwards."""
    if os.path.exists(HANDCODED):
        return set(io.open(HANDCODED, encoding="utf-8").read().strip())
    inv = {v: k for k, v in mid.items()}
    s = set()
    pat = re.compile(r"\\x([Ff][0-3])\\x([0-9A-Fa-f]{2})")
    for f in ("src/st/lib/e_shop.c", "src/st/lib/ko_lib_names.h", "src/st/sel/2C048.c"):
        t = io.open(os.path.join(ROOT, f), encoding="utf-8").read()
        for m in pat.finditer(t):
            c = (int(m.group(1), 16) << 8) | int(m.group(2), 16)
            if c in inv and is_h(inv[c]):
                s.add(inv[c])
    io.open(HANDCODED, "w", encoding="utf-8").write("".join(sorted(s)))
    return s


def protected_chars(old):
    """Hangul whose v12g code token appears in any source/script text (build
    asserts, data headers ...): never reuse their cell."""
    cand = {code: ch for ch, code in old.items() if is_h(ch)}
    pat = re.compile(r"(?i)(?<![0-9a-f])f[0-3][0-9a-f]{2}(?![0-9a-f])")
    pat2 = re.compile(r"(?i)" + re.escape(chr(92)) + r"xf([0-3])" + re.escape(chr(92)) + r"x([0-9a-f]{2})")
    prot = set()
    for base in ("src", "mods", "include", "tools"):
        for dp, dn, fn in os.walk(os.path.join(ROOT, base)):
            if any(x in dp for x in ("saturn", "psxsdk", "/gen", "assets", ".git")):
                continue
            for f in fn:
                if not f.endswith((".c", ".h", ".sh", ".py", ".inc", ".S", ".tsv", ".txt")):
                    continue
                # outputs of this very pipeline (they carry the NEW codes): hand-coded
                # sources are covered by the cached character list, snapshots by promo
                if f in ("e_shop.c", "ko_lib_names.h", "2C048.c", "cutscene_script_ko_st0.h"):
                    continue
                try:
                    t = io.open(os.path.join(dp, f), encoding="utf-8", errors="replace").read()
                except Exception:
                    continue
                toks = set(int(m.group(0), 16) for m in pat.finditer(t))
                toks |= set(0xF000 | (int(m.group(1)) << 8) | int(m.group(2), 16) for m in pat2.finditer(t))
                for c in toks:
                    if c in cand:
                        prot.add(cand[c])
    return prot


def main():
    old = rd(OLD_MAP)
    mid = rd(MID_MAP)
    raw = open(OLD_BLK, "rb").read()
    assert len(raw) == NCELL * 30
    oldh = {c: v for c, v in old.items() if is_h(c)}
    new_chars = [c for c, v in sorted(mid.items(), key=lambda kv: kv[1])
                 if is_h(c) and v >= BASE + NCELL]
    used = tsv_hangul(V14) | tsv_hangul(V4)  # v14 is the single source (generators read it)
    jpb = io.open(JPB, encoding="utf-8").read() if os.path.exists(JPB) else ""
    used |= set(c for c in jpb if is_h(c))
    hand = handcoded_chars(mid)
    prot = protected_chars(old)
    # cells that hold a non-Hangul shape (symbols, ASCII via the engtab) are never free
    eng = open(P4 + "ko_engtab_252.bin", "rb").read()
    busy = set(int.from_bytes(eng[i:i + 2], "little") for i in range(0, len(eng), 2)) - {0}
    busy |= set(v for c, v in mid.items() if not is_h(c))
    free = [c for c in sorted(oldh, key=lambda c: oldh[c])
            if c not in used and c not in hand and c not in prot and c not in new_chars
            and oldh[c] not in busy]
    # old hangul entries whose cell is busy do not keep their map entry either
    stale = [c for c in oldh if oldh[c] in busy and c not in new_chars]
    rep0 = "stale hangul map entries (cell holds a symbol/ASCII shape): %s" % "".join(stale)
    breaking = []
    used |= hand
    rep = [rep0, "used hangul (all tables) %d, protected by token scan %d, new chars %d, free cells %d"
           % (len(used), len(prot), len(new_chars), len(free))]
    # new characters take the cells of unused Hangul first, then extension cells
    # 776.. (the dead blob slot after the 776-cell block, KO_BLOB_ADDR; max 29)
    assign = {}
    ext = 0
    for i, c in enumerate(new_chars):
        if i < len(free):
            assign[c] = oldh[free[i]]
        else:
            assign[c] = BASE + NCELL + ext
            ext += 1
    assert CB == 23 or ext <= 29, ("extension cells > blob slot", ext)
    ncell = NCELL + ext
    rep.append("extension cells (dead blob slot, 896 B = 29 cells max): %d -> KO_GLYPH_COUNT %d" % (ext, ncell))
    dropped = free  # their old shapes are replaced / unused
    rep.append("hand-coded-only chars given up (their hand-coded uses now show another glyph): %s" % " ".join("%s x%d" % x for x in breaking))
    rep.append("free cells reused for new chars: %d, left unused: %d"
               % (min(len(new_chars), len(free)), max(0, len(free) - len(new_chars))))
    # final map
    fmap = {}
    for c, v in old.items():
        if c in new_chars or c in free or c in stale:
            continue
        fmap[c] = v
    for c, v in mid.items():
        if not is_h(c):
            fmap.setdefault(c, v)
    for c, v in assign.items():
        fmap[c] = v
    # which characters get a font cell
    render = [c for c in fmap if is_h(c)]
    # global vertical offset: maximise glyphs fully inside rows 0..14
    FK = ImageFont.truetype(FONT, SIZE * K)
    cm = TTFont(FONT).getBestCmap()
    shapes = {}
    missing = []
    for c in render:
        if ord(c) not in cm:
            missing.append(c)
            continue
        if CB == 23:
            im = Image.new("1", (32, 32), 0)
            d = ImageDraw.Draw(im)
            d.fontmode = "1"
            d.text((0, 0), c, font=FK, fill=1)
            shapes[c] = np.array(im, dtype=bool)
        else:
            im = Image.new("L", (32 * K, 32 * K), 0)
            ImageDraw.Draw(im).text((0, 0), c, font=FK, fill=255)
            a = (np.array(im.resize((32, 32), Image.BOX)) / 255.0 >= THR)
            shapes[c] = a
    tb = {c: (np.where(a.any(axis=1))[0].min(), np.where(a.any(axis=1))[0].max()) for c, a in shapes.items() if a.any()}
    best = None
    for off in range(-4, 6):
        fit = sum(1 for t, b in tb.values() if t + off >= 0 and b + off <= 14)
        key = (fit, -abs(off - 1))
        if best is None or key > best[0]:
            best = (key, off)
    off = FP.P["vert_offset"] if FP.P["vert_offset"] is not None else best[1]
    rep.append("vertical offset: profile %s, auto-best %d" % (FP.P["vert_offset"], best[1]))
    clipped = [c for c, (t, b) in tb.items() if t + off < 0 or b + off > 14]
    wide = [c for c, a in shapes.items() if a.any() and np.where(a.any(axis=0))[0].max() > (11 if CB == 23 else 15)]
    rep.append("font %s %dpx thr %.2f, vertical offset %d, rendered %d, missing in font %d %s, clipped(top/bottom) %d %s, ink wider than 16 cols %d"
               % (os.path.basename(FONT), SIZE, THR, off, len(shapes), len(missing), "".join(missing),
                  len(clipped), "".join(clipped[:40]), len(wide)))
    if CB == 23:
        # all cells re-packed 12 cols x 15 rows 1bpp MSB-first, 180 bits = 23 B
        oldc = np.unpackbits(np.frombuffer(raw, dtype=np.uint8)).reshape(NCELL, 15, 16)
        assert not oldc[:, :, 12:].any(), "old cell ink beyond column 11"
        blk = bytearray(ncell * 23)
        for n in range(NCELL):
            bits = oldc[n, :, :12].reshape(-1)
            blk[n * 23:n * 23 + 23] = np.packbits(np.concatenate([bits, np.zeros(4, dtype=np.uint8)])).tobytes()
        for c, a in shapes.items():
            n = fmap[c] - BASE
            assert 0 <= n < ncell, (c, n)
            g = np.zeros((15, 12), dtype=np.uint8)
            for y in range(15):
                sy = y - off
                if 0 <= sy < 32:
                    g[y, :] = a[sy, :12]
            bits = g.reshape(-1)
            blk[n * 23:n * 23 + 23] = np.packbits(np.concatenate([bits, np.zeros(4, dtype=np.uint8)])).tobytes()
    else:
        blk = bytearray(raw + bytes(ext * 30))
        for c, a in shapes.items():
            n = fmap[c] - BASE
            assert 0 <= n < ncell, (c, n)
            g = np.zeros((15, 16), dtype=int)
            for y in range(15):
                sy = y - off
                if 0 <= sy < 32:
                    g[y, :16] = a[sy, :16]
            cell = bytearray()
            for y in range(15):
                for h in (0, 8):
                    b = 0
                    for x in range(8):
                        if g[y, h + x]:
                            b |= 1 << (7 - x)
                    cell.append(b)
            blk[n * 30:n * 30 + 30] = bytes(cell)

    codes = collections.defaultdict(list)
    for c, v in fmap.items():
        codes[v].append(c)
    dup = {v: x for v, x in codes.items() if len(x) > 1}
    assert not dup, dup
    assert all(BASE <= v < BASE + ncell for v in fmap.values())
    open(OUT_BLK, "wb").write(bytes(blk))
    with io.open(OUT_MAP, "w", encoding="utf-8") as f:
        f.write("글자\t코드\tn\t오프셋\n")
        for c, v in sorted(fmap.items(), key=lambda kv: kv[1]):
            n = v - BASE
            f.write("%s\t0x%04X\t%d\t%d\n" % (c, v, n, n * CB))
    rep.append("block %d B (ncell x cell_bytes), sha256 %s" % (len(blk), hashlib.sha256(blk).hexdigest()))
    rep.append("moved chars: " + " ".join("%s:%04X" % (c, v) for c, v in assign.items()))
    rep.append("hangul left without cell mapping (unused, old shape remains): " + "".join(free[len(new_chars):]))
    io.open(OUT_REP, "w", encoding="utf-8").write("\n".join(rep) + "\n")
    print("\n".join(rep))


if __name__ == "__main__":
    main()
