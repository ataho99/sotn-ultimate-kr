#!/usr/bin/env python3
# gen_ko_cfg.py -- Korean strings of the System (config) screen -> src/dra/gen/us/ko_cfg_str.h
# Input : 한글작업/번역/번역_설정화면.tsv (키, 분류, 영문, 일판, 한글, 비고)
# Output: ko_cfgNN[]   : 8px list/table labels, "_S"-style bytes (space 0x00, ascii-0x20,
#                        Hangul = glyph code F0xx big endian, 0xFF 0x00 end) -- drawn by KO_EqOvDraw
#                        (8px plate, same path as the equipment names)
#         ko_cfgdescN[]: info-bar text (12px, ShowText): Hangul F0xx, ASCII raw, space 0x20
# Fail closed: every Hangul must have a glyph code (v14b/profile map) and, for the 8px labels,
# a cell in the 8px plate with the same code.
import io
from ko_paths import KO_WK  # noqa: E402
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ko_font_profile as FP  # noqa: E402

WK = (KO_WK + "/")
TSV = WK + "번역/번역_설정화면.tsv"
P4 = WK + "P4시험/"
REPO = os.environ.get("SOTN_KR", os.getcwd())
OUT = os.path.join(REPO, "src/dra/gen/us/ko_cfg_str.h")
# the Extra settings popup (rows "xset:" of the same table; mods/sotn-ultimate/src/menu/sysopt.c, BIN/ARC_F2.BIN):
# same encodings, written apart so ko_cfg_str.h (DRA) keeps its bytes
OUT_XSET = os.path.join(REPO, "mods/sotn-ultimate/src/menu/sysopt_ko_str.h")


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


def is_h(c):
    return "가" <= c <= "힣"


def esc(b):
    return "".join("\\x%02X" % x for x in b)


def info_bytes(ch, key, mp):
    """one char of an info-bar string (12px, ShowText = func_800F99B8): a-z / A-Z / space are single bytes, every other
    byte is read together with the NEXT byte as a Shift-JIS pair -- a raw ASCII digit / punctuation byte would swallow the
    next byte and shift the rest of the line. Those are written full-width (cp932 symbols, lead 0x81..0x84, which the
    retail font has: the Japanese text uses them); a raw ASCII non-letter fails the build."""
    o = ord(ch)
    if ch == " " or (o < 128 and ch.isalpha()):
        return bytes([o])
    if o < 128:
        raise AssertionError(("ASCII non-letter in an info-bar row: write the full-width form", ch, key))
    if is_h(ch):
        assert ch in mp, ("no glyph code", ch, key)
        return bytes([mp[ch] >> 8, mp[ch] & 0xFF])
    sj = ch.encode("cp932")
    assert len(sj) == 2 and 0x81 <= sj[0] <= 0x84, ("not a full-width symbol the retail font has", ch, key)
    return sj


def main():
    mp = rd(P4 + FP.P["map_name"])
    pb = open(P4 + "ko_8p_block_248.bin", "rb").read()
    pn = struct.unpack("<H", pb[:2])[0]
    plate = set(struct.unpack("<%dH" % pn, pb[2:2 + pn * 2]))
    rows = [l.rstrip(chr(10)).split(chr(9)) for l in io.open(TSV, encoding="utf-8")][1:]
    out = ["// gen_ko_cfg.py: System (config) screen strings (do not edit)"]
    n8 = nd = 0
    for r in rows:
        if len(r) < 5 or not r[0].startswith("cfg:"):
            continue
        key, kind, ko = r[0][4:], r[1], r[4]
        b = bytearray()
        if kind.startswith("메뉴"):
            for ch in ko:
                if ch == " ":
                    b.append(0x00)
                elif ord(ch) < 128:
                    b.append(ord(ch) - 0x20)
                else:
                    assert is_h(ch) and ch in mp, ("no glyph code", ch, key)
                    assert mp[ch] in plate, ("no 8px plate cell", ch, key)
                    b += bytes([mp[ch] >> 8, mp[ch] & 0xFF])
            assert len(ko) <= 10, ("label too long for the 10-char plate parse", key, ko)
            b += bytes([0xFF, 0x00])
            out.append('static const char ko_%s[] = "%s";' % (key, esc(b)))
            n8 += 1
        else:
            for ch in ko:
                b += info_bytes(ch, key, mp)
            out.append('static const char ko_%s[] = "%s";' % (key, esc(b)))
            nd += 1
    # the Extra settings popup strings (xset:), same encodings, a header of its own
    xout = ["// gen_ko_cfg.py: Extra settings popup strings (do not edit)"]
    x8 = xd = 0
    for r in rows:
        if len(r) < 5 or not r[0].startswith("xset:"):
            continue
        key, kind, ko = r[0][5:], r[1], r[4]
        b = bytearray()
        if kind.startswith("메뉴"):
            for ch in ko:
                if ch == " ":
                    b.append(0x00)
                elif ord(ch) < 128:
                    assert (ord(ch) in plate), ("no 8px plate cell for ASCII", ch, key)
                    b.append(ord(ch) - 0x20)
                else:
                    assert is_h(ch) and ch in mp, ("no glyph code", ch, key)
                    assert mp[ch] in plate, ("no 8px plate cell", ch, key)
                    b += bytes([mp[ch] >> 8, mp[ch] & 0xFF])
            assert len(ko) <= 10, ("label too long for the 10-char plate parse", key, ko)
            b += bytes([0xFF, 0x00])
            xout.append('static const char ko_xset_%s[] = "%s";' % (key, esc(b)))
            x8 += 1
        else:
            for ch in ko:
                b += info_bytes(ch, key, mp)
            xout.append('static const char ko_xset_%s[] = "%s";' % (key, esc(b)))
            xd += 1
            xout.append('#define KO_XSET_%s_ASM "%s"' % (key.upper(), "".join(chr(92) * 2 + "%03o" % x for x in b)))  # gas octal escapes, for the ARC_F2 header
    io.open(OUT_XSET, "w", encoding="utf-8", newline="").write(chr(10).join(xout) + chr(10))
    print("sysopt_ko_str.h: %d 8px labels/values, %d info-bar strings" % (x8, xd))
    io.open(OUT, "w", encoding="utf-8", newline="").write("\n".join(out) + "\n")
    print("ko_cfg_str.h: %d 8px labels, %d info-bar strings" % (n8, nd))


if __name__ == "__main__":
    sys.exit(main())
