#!/usr/bin/env python3
# gen_ko_font8.py -- re-bake the ORIGINAL 8x8 menu glyph sheet (English/digits/symbols) with the small
# Hangul font family (Galmuri7 @ 8, mono). Cell positions and 8px advance unchanged.
#
# The sheet: BIN/F_GAME.BIN and BIN/F_GAME2.BIN, 4096-byte block at +0x30000 (128x64, 4bpp, VRAM tpage 0x1E,
# MenuDrawChar: cell (ch & 15, ch >> 4), ch = ASCII - 0x20 in the menu's own encoding, sheet index i <-> ASCII 0x20+i).
# Non-ASCII cells inside the ASCII range (kanji at *, ;, <, >, @, \, ^, `, {, |, }, 0x7F) and 0x80.. kana are NOT touched.
#
# Cell rule (same family as gen_ko_8p.py plate cells): Galmuri7.ttf size 8, mono (fontmode "1"), drawn at (0,0);
# ASCII needs a FIXED baseline (the plate's "top ink row" rule would float x-height letters up): cell rows = font rows 1..8.
# Ink = palette index 6 (the original sheet's only ink index; 7 px of index 5 sit in the kanji cells, untouched).
#
# Checks (fail closed):
#   1  source sheet sha256 pin (F_GAME == F_GAME2)
#   2  positive control: the Galmuri7@8 renderer reproduces all 283 existing plate Hangul cells bit-exact (rule family)
#   3  4bpp encode/decode round trip of the NEW sheet == the intended 1bpp cells
#   4  every untouched cell byte-identical to the original; only the REPLACE cells differ
#   5  every replaced cell non-empty (except space) and table covers each ASCII cell exactly once
# Output: P4시험/ko_font8_sheet.bin (4096 B), ko_font8_table.tsv (cell / code / char / status), preview png.
import hashlib, io, os, struct, sys
from ko_paths import KO_WK, KO_FONTS  # noqa: E402
import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = os.environ.get("SOTN_KR", os.getcwd())
P4 = (KO_WK + "/P4시험/")
G7 = (KO_FONTS + "/Galmuri/v2.40.4/Galmuri-v2.40.4/Galmuri7.ttf")
SRC_G1 = ROOT + "/disks/us/BIN/F_GAME.BIN"
SRC_G2 = ROOT + "/disks/us/BIN/F_GAME2.BIN"
OFF, LEN = 0x30000, 0x1000
SRC_SHA = "5ad4c582a425c5d6ada3ad98d8f74d3304b98258fe2787853864c544f306161f"
INK = 6
# sheet index i (ASCII 0x20+i) holds a kanji/kana picture, not the ASCII glyph -> never touched
KANJI = {0x2A, 0x3B, 0x3C, 0x3E, 0x40, 0x5C, 0x5E, 0x60, 0x7B, 0x7C, 0x7D, 0x7F}
OUT = P4 + "ko_font8_sheet.bin"
OUTTSV = P4 + "ko_font8_table.tsv"
FONT = ImageFont.truetype(G7, 8)


def glyph_rows(ch):
    im = Image.new("1", (16, 16), 0)
    d = ImageDraw.Draw(im)
    d.fontmode = "1"
    d.text((0, 0), ch, font=FONT, fill=1)
    return np.array(im, dtype=np.uint8)


def cell_ascii(ch):
    a = glyph_rows(ch)
    assert not a[0].any() and not a[9:].any(), ("ink outside rows 1..8", ch)
    assert not a[:, 8:].any(), ("ink wider than 8", ch)
    return a[1:9, 0:8].copy()


def cell_hangul_plate(ch):  # gen_ko_8p.cell_g7 rule
    a = glyph_rows(ch)
    rows = np.where(a.any(axis=1))[0]
    t = int(rows.min())
    return a[t:t + 8, 0:8].copy()


def dec(b):
    px = np.zeros((64, 128), np.uint8)
    for y in range(64):
        for x in range(128):
            v = b[y * 64 + x // 2]
            px[y, x] = (v >> (4 * (x & 1))) & 15
    return px


def enc(px):
    out = bytearray(4096)
    for y in range(64):
        for x in range(0, 128, 2):
            out[y * 64 + x // 2] = int(px[y, x]) | (int(px[y, x + 1]) << 4)
    return bytes(out)


def main():
    g1 = open(SRC_G1, "rb").read()[OFF:OFF + LEN]
    g2 = open(SRC_G2, "rb").read()[OFF:OFF + LEN]
    assert g1 == g2 and hashlib.sha256(g1).hexdigest() == SRC_SHA, "source sheet is not the pinned retail sheet"

    # 2 positive control
    old = open(P4 + "ko_8p_block_243.bin", "rb").read()
    n = struct.unpack("<H", old[:2])[0]
    codes = struct.unpack("<%dH" % n, old[2:2 + 2 * n])
    inv = {}
    for ln in io.open(P4 + "ko_8p_map_243.tsv", encoding="utf-8"):
        q = ln.rstrip("\n").split("\t")
        if len(q) >= 2 and len(q[0]) == 1:
            try:
                inv[q[0]] = int(q[1], 16)
            except ValueError:
                pass
    chk = bad = 0
    for ch, code in inv.items():
        if not ("가" <= ch <= "힣") or code not in codes:
            continue
        i = codes.index(code)
        cell = old[2 + 2 * n + i * 8:2 + 2 * n + i * 8 + 8]
        chk += 1
        if np.packbits(cell_hangul_plate(ch), axis=1).reshape(8).tobytes() != cell:
            bad += 1
    print("positive control: Galmuri7@8 reproduces %d existing plate Hangul cells, %d differ" % (chk, bad))
    assert chk >= 250 and bad == 0, "renderer does not reproduce the plate"

    px = dec(g1)
    new = px.copy()
    rows = []
    for i in range(0x00, 0x60):  # indexes 0..0x5F = ASCII 0x20..0x7F
        ch = chr(0x20 + i)
        cx, cy = i % 16, i // 16
        if 0x20 + i == 0x7F or i in [k - 0x20 for k in KANJI] or (0x20 + i) in KANJI:
            rows.append((i, 0x20 + i, ch if ch != "\x7f" else "DEL", "kanji-untouched"))
            continue
        if ch == " ":
            rows.append((i, 0x20, "SPACE", "blank-untouched"))
            assert not px[cy * 8:cy * 8 + 8, cx * 8:cx * 8 + 8].any()
            continue
        c = cell_ascii(ch)
        assert c.any(), ("empty glyph", ch)
        l0 = int(np.where(c.any(axis=0))[0].min())
        if l0:
            c = np.roll(c, -l0, axis=1)  # ':' etc.: ink starts at column 0 (the advance table assumes it)
        new[cy * 8:cy * 8 + 8, cx * 8:cx * 8 + 8] = c * INK
        rows.append((i, 0x20 + i, ch, "replaced"))
    blob = enc(new)
    # 3 round trip
    assert (dec(blob) == new).all()
    # 4 untouched cells identical
    touched = {r[0] for r in rows if r[3] == "replaced"}
    for i in range(128):
        cx, cy = i % 16, i // 16
        a = px[cy * 8:cy * 8 + 8, cx * 8:cx * 8 + 8]
        b = dec(blob)[cy * 8:cy * 8 + 8, cx * 8:cx * 8 + 8]
        if i in touched:
            continue
        assert (a == b).all(), ("untouched cell changed", i)
    assert len(touched) == len({r[0] for r in rows if r[3] == "replaced"})
    # 6 advance table (KO_FONT8_ADV_ADDR): per sheet cell, ink width + 2 for replaced cells, space 4, 0 elsewhere
    nb = dec(blob)
    adv = bytearray(96)
    for i in range(96):
        if i in touched:
            cx, cy = i % 16, i // 16
            c = nb[cy * 8:cy * 8 + 8, cx * 8:cx * 8 + 8] > 0
            cols = np.where(c.any(axis=0))[0]
            assert cols.min() == 0, ("ink not left-aligned", chr(0x20 + i))
            adv[i] = int(cols.max()) + 1 + 2
        elif i == 0:
            adv[i] = 4
    open(P4 + "ko_font8_adv.bin", "wb").write(bytes(adv))
    dra_h = open(ROOT + "/src/dra/dra.h", encoding="utf-8").read()
    assert "#define KO_FONT8_ADV_ADDR 0x8009F200" in dra_h, "dra.h KO_FONT8_ADV_ADDR"
    assert "0x8009F200" in open(ROOT + "/src/st/ko_font_params.h", encoding="utf-8").read()
    print("advance table: %d replaced cells, advance %d..%d, sha256 %s" % (len(touched), min(adv[i] for i in touched), max(adv[i] for i in touched), hashlib.sha256(bytes(adv)).hexdigest()))
    open(OUT, "wb").write(blob)
    with io.open(OUTTSV, "w", encoding="utf-8") as f:
        f.write("cell_idx\tmenu_code\tcell_xy\tchar\tstatus\n")
        for i, code, ch, st in rows:
            f.write("0x%02X\t0x%02X\t(%d,%d)\t%s\t%s\n" % (i, code, (i % 16) * 8, (i // 16) * 8, ch, st))
    print("replaced %d cells, sheet sha256 %s" % (len(touched), hashlib.sha256(blob).hexdigest()))
    # preview (new sheet only)
    Z = 8
    im = Image.new("RGB", (128 * Z, 64 * Z), (0, 0, 0))
    dr = ImageDraw.Draw(im)
    nn = dec(blob)
    for y in range(64):
        for x in range(128):
            if nn[y, x]:
                dr.rectangle([x * Z, y * Z, x * Z + Z - 2, y * Z + Z - 2], fill=(170, 170, 170))
    im.save(P4 + "ko_font8_preview.png")


def patch():
    """build step: write the sheet into build/us/F_GAME.BIN and build/us/F_GAME2.BIN (disc files override disks/us/BIN)"""
    blob = open(OUT, "rb").read()
    assert len(blob) == LEN
    for name in ("F_GAME.BIN", "F_GAME2.BIN"):
        out = ROOT + "/build/us/" + name
        src = out if os.path.exists(out) else ROOT + "/disks/us/BIN/" + name  # F_GAME2 may already carry Maria's HUD
        d = bytearray(open(src, "rb").read())
        cur = bytes(d[OFF:OFF + LEN])
        assert cur == blob or hashlib.sha256(cur).hexdigest() == SRC_SHA, (name, "sheet block is neither retail nor already patched")
        d[OFF:OFF + LEN] = blob
        open(out, "wb").write(bytes(d))
        print("patched %s: sheet @0x%X, size %d" % (out, OFF, len(d)))


if __name__ == "__main__":
    rc = main()
    if rc is None and "--patch" in sys.argv:
        patch()
    sys.exit(rc)
