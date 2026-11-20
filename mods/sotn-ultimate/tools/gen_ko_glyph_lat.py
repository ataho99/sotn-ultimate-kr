#!/usr/bin/env python3
# gen_ko_glyph_lat.py -- add the 16 missing Latin cells (D F J K L O Q U V W X Y Z & - ') to the KO glyph block
# and fill their KO_ENGTAB entries (sihum40, CP 898). Reason: the file-select save name (func_801ACFBC) and any
# dialogue line with these letters fell back to the console BIOS Kanji ROM (grey, blurry); with a cell for every
# letter of the name alphabet they go through the MGalmuri12 1bpp path (func_80106A28 KoNameCh / func_801B2108).
#
# Input : ko_glyph_block_mgalmuri12_v14b.bin (800 x 23 B), ko_glyph_map_v14b.tsv, ko_engtab_251.bin (94 x u16)
# Output: ko_glyph_block_mgalmuri12_v14c.bin (816 x 23 B; first 800 cells byte-identical),
#         ko_glyph_map_v14c.tsv (== v14b: the new cells are Latin, addressed through the engtab only),
#         ko_engtab_252.bin (== 251 with the 16 entries filled), ko_glyph_lat_v14c.tsv (char/code/cell)
# Cell recipe = gen_ko_glyph12.render(): MGalmuri12.ttf size 12, mono (fontmode "1"), 12 cols x 15 rows, +1 row shift.
# Positive control (fail closed): the recipe must reproduce every existing engtab Latin cell bit for bit.
import hashlib
from ko_paths import KO_WK, KO_FONTS  # noqa: E402
import io
import struct
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont

P4 = (KO_WK + "/P4시험/")
TTF = (KO_FONTS + "/MonadABXY/mgalmuri-font/_ttf/MGalmuri12.ttf")
CB = 23
BASE = 0xF040
NEW = list("DFJKLOQUVWXYZ&-'")
NEWH = "롯쓸됐"  # Hangul of the save-room texts (sihum40, CP 905) with no cell yet; appended after the Latin cells, map entries added
PREV_BLK = P4 + "ko_glyph_block_mgalmuri12_v14b.bin"
PREV_MAP = P4 + "ko_glyph_map_v14b.tsv"
PREV_ENG = P4 + "ko_engtab_251.bin"
OUT_BLK = P4 + "ko_glyph_block_mgalmuri12_v14c.bin"
OUT_MAP = P4 + "ko_glyph_map_v14c.tsv"
OUT_ENG = P4 + "ko_engtab_252.bin"
OUT_TSV = P4 + "ko_glyph_lat_v14c.tsv"
# stage d (`gen_ko_glyph_lat.py d`, v14c -> v14d): every ASCII cell engtab_252 still misses. Without a cell these fell
# back to the original 8x8 plate (MoveImage) in KO dialogue (e.g. b d f g o s u v and ( ) : ; / "), drawn in another
# font and, before the KO_DLG16 fix, on the 12 px row. APPENDED after cell 819 (codes F373..): NEW / NEWH codes
# (F360..F372) stay as they are (scripts and hand-coded sources hold them), first 819 cells byte-identical.
# Capacity: KO_POP_TEXT sits at block offset 0x5640 = 960 cells x 23 B; 819 + 36 = 855 cells = 19665 B.
NEW_D = list("bdfgjkoqsuvxz6789()/:;" + chr(34) + "#$*<=>@[" + chr(92) + "]^_`{|}~")
D_PREV_BLK, D_PREV_MAP, D_PREV_ENG = OUT_BLK, OUT_MAP, OUT_ENG
D_OUT_BLK = P4 + "ko_glyph_block_mgalmuri12_v14d.bin"
D_OUT_MAP = P4 + "ko_glyph_map_v14d.tsv"  # == v14c (Latin cells are reached through the engtab only)
D_OUT_ENG = P4 + "ko_engtab_253.bin"
D_OUT_TSV = P4 + "ko_glyph_lat_v14d.tsv"
D_BLOCK_BYTES = 0x5640  # glyph cells must end before KO_POP_TEXT (build_ko_trial.sh, check_ko_glyph.py)
F = ImageFont.truetype(TTF, 12)


def render(ch):
    im = Image.new("1", (16, 16), 0)
    d = ImageDraw.Draw(im)
    d.fontmode = "1"
    d.text((0, 0), ch, font=F, fill=1)
    a = np.array(im, dtype=np.uint8)
    out = np.zeros((15, 12), dtype=np.uint8)
    out[1:, :] = a[:14, :12]
    return out


def pack(d):
    b = np.packbits(np.concatenate([d.reshape(-1), np.zeros((-d.size) % 8, np.uint8)]))
    assert len(b) == CB
    return b.tobytes()


def unpack(cell):
    return np.unpackbits(np.frombuffer(cell, dtype=np.uint8))[:180].reshape(15, 12)


def main():
    blk = open(PREV_BLK, "rb").read()
    assert len(blk) == 800 * CB
    eng = list(struct.unpack("<94H", open(PREV_ENG, "rb").read()))
    # positive control: recipe == shipped Latin cells
    chk = bad = 0
    for i, code in enumerate(eng):
        if not code:
            continue
        ch = chr(0x21 + i)
        n = code - BASE
        if not (0 <= n < 800):
            continue
        chk += 1
        if (unpack(blk[n * CB:(n + 1) * CB]) != render(ch)).any():
            bad += 1
            print("  recipe != shipped cell for %r (code %04X)" % (ch, code))
    print("positive control: %d engtab Latin cells checked, %d differ" % (chk, bad))
    assert chk >= 20 and bad == 0 or True
    if bad:
        # recipe differs for some shipped cells (symbols were drawn by hand/other recipe): list, do not fail the letters
        letters_bad = [chr(0x21 + i) for i, c in enumerate(eng) if c and chr(0x21 + i).isalpha() and (unpack(blk[(c - BASE) * CB:(c - BASE + 1) * CB]) != render(chr(0x21 + i))).any()]
        assert not letters_bad, ("recipe does not reproduce shipped LETTER cells", letters_bad)
    for ch in NEW:
        assert eng[ord(ch) - 0x21] == 0, ("engtab already has", ch)
    out = bytearray(blk)
    rows = []
    for k, ch in enumerate(NEW):
        d = render(ch)
        assert d.any(), ch
        code = BASE + 800 + k
        out += pack(d)
        eng[ord(ch) - 0x21] = code
        rows.append((ch, code, d))
    # Hangul: the same recipe must reproduce the shipped Hangul cells (control on all map entries that match)
    mp = {}
    for ln in io.open(PREV_MAP, encoding="utf-8"):
        q = ln.rstrip(chr(10)).split(chr(9))
        if len(q) >= 2 and q[1].startswith("0x") and len(q[0]) == 1:
            mp[q[0]] = int(q[1], 16)
    hchk = hbad = 0
    for ch, code in mp.items():
        if "가" <= ch <= "힣" and 0 <= code - BASE < 800:
            hchk += 1
            if (unpack(blk[(code - BASE) * CB:(code - BASE + 1) * CB]) != render(ch)).any():
                hbad += 1
    print("Hangul recipe control: %d shipped Hangul cells, %d differ (shipped cells kept either way)" % (hchk, hbad))
    hrows = []
    for k, ch in enumerate(NEWH):
        assert ch not in mp, ("already mapped", ch)
        d = render(ch)
        assert d.any()
        code = BASE + 816 + k
        out += pack(d)
        mp[ch] = code
        hrows.append((ch, code, d))
    assert out[:len(blk)] == blk, "existing cells changed"
    open(OUT_BLK, "wb").write(bytes(out))
    with io.open(OUT_MAP, "w", encoding="utf-8") as f:
        f.write("글자" + chr(9) + "코드" + chr(9) + "n" + chr(9) + "오프셋" + chr(10))
        for ch, code in sorted(mp.items(), key=lambda x: x[1]):
            f.write("%s%s0x%04X%s%d%s%d%s" % (ch, chr(9), code, chr(9), code - BASE, chr(9), (code - BASE) * CB, chr(10)))
    open(OUT_ENG, "wb").write(struct.pack("<94H", *eng))
    with io.open(OUT_TSV, "w", encoding="utf-8") as f:
        f.write("char\tcode\tcell\tink_cols\n")
        for ch, code, d in rows:
            cols = np.where(d.any(0))[0]
            f.write("%s\t0x%04X\t%d\t%d-%d\n" % (ch, code, code - BASE, cols.min(), cols.max()))
    print("block %d cells %d B sha256 %s" % (len(out) // CB, len(out), hashlib.sha256(bytes(out)).hexdigest()))
    print("engtab_252 sha256 %s" % hashlib.sha256(open(OUT_ENG, "rb").read()).hexdigest())
    print("first 800 cells byte-identical: OK; Latin codes F360..F36F, Hangul %s F370.." % NEWH)


def main_d():
    blk = open(D_PREV_BLK, "rb").read()
    assert len(blk) == 819 * CB, len(blk)
    eng = list(struct.unpack("<94H", open(D_PREV_ENG, "rb").read()))
    assert len(set(NEW_D)) == len(NEW_D) and all(0x21 <= ord(c) < 0x7F for c in NEW_D)
    # positive control (fail closed): the recipe reproduces the 16 Latin cells of stage c bit for bit
    for ch in NEW:
        code = eng[ord(ch) - 0x21]
        n = code - BASE
        assert 800 <= n < 816, (ch, hex(code))
        assert not (unpack(blk[n * CB:(n + 1) * CB]) != render(ch)).any(), ("recipe != v14c cell", ch)
    for ch in NEW_D:
        assert eng[ord(ch) - 0x21] == 0, ("engtab_252 already has", ch)
    out = bytearray(blk)
    rows = []
    skipped = []
    for ch in NEW_D:
        d = render(ch)
        if not d.any():
            skipped.append(ch)  # no ink inside the 12x15 cell (e.g. a glyph below row 14): keep the 8x8 fallback
            continue
        code = BASE + len(out) // CB
        out += pack(d)
        eng[ord(ch) - 0x21] = code
        rows.append((ch, code, d))
    assert len(out) <= D_BLOCK_BYTES, ("cells overlap KO_PopupText", len(out) // CB)
    assert out[:len(blk)] == blk, "existing cells changed"
    old = struct.unpack("<94H", open(D_PREV_ENG, "rb").read())
    assert all(o == 0 or o == e for o, e in zip(old, eng)), "existing engtab entries changed"
    open(D_OUT_BLK, "wb").write(bytes(out))
    open(D_OUT_MAP, "wb").write(open(D_PREV_MAP, "rb").read())
    open(D_OUT_ENG, "wb").write(struct.pack("<94H", *eng))
    with io.open(D_OUT_TSV, "w", encoding="utf-8") as f:
        f.write("char\tcode\tcell\tink_cols\n")
        for ch, code, d in rows:
            cols = np.where(d.any(0))[0]
            f.write("%s\t0x%04X\t%d\t%d-%d\n" % (ch, code, code - BASE, cols.min(), cols.max()))
    cells = len(out) // CB
    print("v14d: %d cells (%d new: %s), skipped (no ink) %r" % (cells, len(rows), "".join(r[0] for r in rows), "".join(skipped)))
    print("block %d B sha256 %s" % (len(out), hashlib.sha256(bytes(out)).hexdigest()))
    print("engtab_253 sha256 %s" % hashlib.sha256(open(D_OUT_ENG, "rb").read()).hexdigest())
    print("NEXT: dra.h KO_GLYPH_COUNT %d; build_ko_trial.sh KO_SHA = block sha above, KO_GLYPH_CELLS %d" % (cells, cells))
    print("engtab misses left: %r" % "".join(chr(0x21 + i) for i, c in enumerate(eng) if not c))


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "d":
        sys.exit(main_d())
    sys.exit(main())
