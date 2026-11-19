#!/usr/bin/env python3
# gen_ko_promo.py -- cutscene dialogue "1-byte promotion" table (Xanadu-style).
#
# The cutscene render loop sends every byte that is not a control opcode
# (0x00..0x18) to its `default:` path. KO dialogue uses only ASCII ! , . ? 4
# and space, and Hangul lead bytes 0xF0..0xF3, so 0x80..0xEF (112 values) are
# free. The 112 most frequent non-ASCII characters of the cutscene dialogue
# (all scenes, v14 translation) get a 1-byte code 0x80+i; the render hook in
# each cutscene overlay maps it to the normal 2-byte glyph code through a
# u16 table (KO_PROMO_TBL_ADDR, 224 B inside the KO glyph block region).
#
# Inputs : 번역_v14.tsv (dialogue rows, 번역 column), ko_glyph_map_v13p.tsv
# Outputs: P4시험/ko_promo_tbl.bin (112 x u16 LE), P4시험/ko_promo_map.tsv
import collections
from ko_paths import KO_WK  # noqa: E402
import io
import sys

WK = (KO_WK + "/")
TSV = (KO_WK + "/검수/번역_v14.tsv")
MAP = WK + "P4시험/" + __import__("ko_font_profile").P["map_name"] + ""
OUT_BIN = WK + "P4시험/ko_promo_tbl.bin"
OUT_MAP = WK + "P4시험/ko_promo_map.tsv"
# scene prefixes whose script goes through gen_ko_splice (prologue / psp / mod
# rows are separate mechanisms and excluded from the frequency base)
SCENES = ("st0_richter_dracula", "sel_ending", "no3_death", "cen_maria",
          "nz0_slogra_gaibon", "lib_librarian", "no4_ferryman_left",
          "no4_ferryman_right", "rcen_shaft", "dre_succubus",
          "dai_huntinggirl", "top_maria_richter")
SLOTS = 112
FIRST = 0x80


def norm(s):
    s = s.replace("…", "...")
    return s.replace(".........", "……").replace("...", "…")


def main():
    mp = {}
    for ln in io.open(MAP, encoding="utf-8"):
        p = ln.rstrip("\n").split("\t")
        if len(p) >= 2 and len(p[0]) == 1 and p[1].startswith("0x"):
            mp[p[0]] = int(p[1], 16)
    rows = [l.rstrip("\n").split("\t") for l in io.open(TSV, encoding="utf-8")]
    h = rows[0]
    ti = [i for i, x in enumerate(h) if x.startswith("번역")][0]
    freq = collections.Counter()
    for r in rows[1:]:
        if len(r) <= ti or r[1] != "대사" or r[0].split(":")[0] not in SCENES:
            continue
        for ch in norm(r[ti].replace("\\n", "/").replace("/", "")):
            if ord(ch) >= 128:
                freq[ch] += 1
    top = sorted(freq.items(), key=lambda kv: (-kv[1], kv[0]))[:SLOTS]
    tbl = bytearray()
    with io.open(OUT_MAP, "w", encoding="utf-8") as f:
        f.write("글자\t1바이트\t2바이트코드\t빈도\n")
        for i, (ch, n) in enumerate(top):
            assert ch in mp, ("no glyph cell", ch)
            code = mp[ch]
            assert 0xF040 <= code <= 0xF3FF, (ch, hex(code))
            tbl += code.to_bytes(2, "little")
            f.write("%s\t0x%02X\t0x%04X\t%d\n" % (ch, FIRST + i, code, n))
    tbl += bytes(SLOTS * 2 - len(tbl))
    assert len(tbl) == 224
    open(OUT_BIN, "wb").write(bytes(tbl))
    tot = sum(freq.values())
    cov = sum(n for _, n in top)
    print("distinct %d, occurrences %d, promoted %d (%.1f%%), table %d B" % (
        len(freq), tot, len(top), 100.0 * cov / tot, len(tbl)))


if __name__ == "__main__":
    sys.exit(main())
