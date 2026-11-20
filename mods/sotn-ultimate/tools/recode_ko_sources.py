#!/usr/bin/env python3
# recode_ko_sources.py -- v12g -> v13n glyph code migration for hand-coded KO
# strings ("\xF2\x5F" style) in e_shop.c, ko_lib_names.h, 2C048.c.
# The 31 moved characters (29 ASCII-engtab conflicts + 념 짐) got new cells in
# v13n; every other code is unchanged. Single simultaneous pass, idempotent only
# on a v12g source: refuses if a file already contains a v13n-only code of a
# moved char (marker) so it is not applied twice.
import io
from ko_paths import KO_WK  # noqa: E402
import os
import re
import sys

WK = (KO_WK + "/P4시험/")
ROOT = os.environ.get("SOTN_KR", os.getcwd())
FILES = ["src/st/lib/e_shop.c", "src/st/lib/ko_lib_names.h", "src/st/sel/2C048.c"]


def rd(p):
    m = {}
    for ln in io.open(p, encoding="utf-8"):
        q = ln.rstrip(chr(10)).split(chr(9))
        if len(q) >= 2 and q[1].startswith("0x") and len(q[0]) == 1:
            m[q[0]] = int(q[1], 16)
    return m


def main():
    # usage: recode_ko_sources.py [SRC_MAP DST_MAP]   (default v12g -> v13n)
    # The final map-code substitution is applied ONCE (guard file); the substitution is a single
    # simultaneous pass, so overlapping old/new code values are fine.
    src = sys.argv[1] if len(sys.argv) > 1 else "ko_glyph_map_v12g.tsv"
    dst = sys.argv[2] if len(sys.argv) > 2 else "ko_glyph_map_v13n.tsv"
    guard = WK + "ko_recode_done_%s_to_%s.txt" % (src[:-4], dst[:-4])
    if os.path.exists(guard):
        print("already recoded (guard %s) -- refusing" % guard)
        return
    old = rd(WK + src)
    new = rd(WK + dst)
    gone = [c for c in old if c not in new]
    mv = {old[c]: new[c] for c in old if c in new and old[c] != new[c]}
    print("moved chars", len(mv), "removed from map", "".join(gone))
    pat = re.compile(r"\\x([Ff][0-3])\\x([0-9A-Fa-f]{2})")
    total = 0
    for f in FILES:
        p = os.path.join(ROOT, f)
        t = io.open(p, encoding="utf-8").read()
        n = [0]

        def sub(m):
            code = (int(m.group(1), 16) << 8) | int(m.group(2), 16)
            if code in mv:
                n[0] += 1
                c = mv[code]
                return "\\x%02X\\x%02X" % (c >> 8, c & 0xFF)
            return m.group(0)

        t2 = pat.sub(sub, t)
        io.open(p, "w", encoding="utf-8", newline="").write(t2)
        print(f, "recoded", n[0])
        total += n[0]
    print("total", total)
    io.open(guard, "w", encoding="utf-8").write("done %d" % total + chr(10))


if __name__ == "__main__":
    main()
