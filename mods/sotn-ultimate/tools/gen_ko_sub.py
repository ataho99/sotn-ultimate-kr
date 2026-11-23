#!/usr/bin/env python3
# gen_ko_sub.py -- Familiar voice subtitles (12 XA lines) -> src/servant/ko_sub_str.h
# Input : ko_data/번역/번역_패밀리어자막.tsv (VO id hex, speaker faerie|demon, Korean text)
# Output: per line two rows (<= 15 glyph cells each, split at the space nearest the middle); a row is bytes: Hangul = glyph code
#         F0xx big endian (2 bytes), ASCII raw, space 0x20, end 0. The servant overlays (tt_002 faerie, tt_003 demon) draw them
#         as 12x16 glyph sprites (src/servant/ko_sub.h).
import io
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ko_paths import KO_WK  # noqa: E402
import ko_font_profile as FP  # noqa: E402

P4 = KO_WK + "/P4시험/"
TSV = KO_WK + "/번역/번역_패밀리어자막.tsv"
REPO = os.environ.get("SOTN_KR", os.getcwd())
OUT = os.path.join(REPO, "src/servant/ko_sub_str.h")
CELLS = 15  # u0 is 8 bits: 12px * 21 cells max, slots 6..20


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


MP = rd(P4 + FP.P["map_name"])


def is_h(c):
    return "가" <= c <= "힣" or c == "…"


def enc(s):
    b = bytearray()
    for ch in s:
        if is_h(ch):
            assert ch in MP, ("no glyph code", ch)
            b += bytes([MP[ch] >> 8, MP[ch] & 0xFF])
        else:
            assert 0x20 <= ord(ch) < 0x7F, ("not ASCII / Hangul", ch)
            b.append(ord(ch))
    return bytes(b)


def split(text):
    """two rows, each <= CELLS glyph cells (spaces count 1 cell), split at the space that balances them"""
    if len(text) <= CELLS:
        return [text, ""]
    best = None
    for i, c in enumerate(text):
        if c == " ":
            a, b = text[:i], text[i + 1:]
            if len(a) <= CELLS and len(b) <= CELLS:
                d = abs(len(a) - len(b))
                if best is None or d < best[0]:
                    best = (d, a, b)
    assert best, ("cannot split into two rows of <= %d cells" % CELLS, text)
    return [best[1], best[2]]


def esc(b):
    return "".join("\\x%02X" % x for x in b)


def main():
    rows = [l.rstrip(chr(10)).split(chr(9)) for l in io.open(TSV, encoding="utf-8")][1:]
    out = ["// gen_ko_sub.py: familiar subtitle rows (do not edit)"]
    ent = {"faerie": [], "demon": []}
    for r in rows:
        vid, who, text = r[0], r[1], r[2]
        a, b = split(text)
        k = "ko_sub_%s" % vid
        out.append('static const char %s_0[] = "%s";' % (k, esc(enc(a))))
        out.append('static const char %s_1[] = "%s";' % (k, esc(enc(b))))
        ent[who].append("{0x%s, %s_0, %s_1}" % (vid, k, k))
    out.append("#if defined(KO_SUB_FAERIE)")
    out.append("static const KoSubLine g_KoSubLines[] = {" + ", ".join(ent["faerie"]) + "};")
    out.append("#elif defined(KO_SUB_DEMON)")
    out.append("static const KoSubLine g_KoSubLines[] = {" + ", ".join(ent["demon"]) + "};")
    out.append("#endif")
    io.open(OUT, "w", encoding="utf-8", newline="").write(chr(10).join(out) + chr(10))
    print("ko_sub_str.h: %d lines (faerie %d, demon %d)" % (len(rows), len(ent["faerie"]), len(ent["demon"])))


if __name__ == "__main__":
    sys.exit(main())
