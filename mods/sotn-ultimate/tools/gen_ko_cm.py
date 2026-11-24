#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
# gen_ko_cm.py -- Korean strings of the Richter / Maria pause menu (mods/sotn-ultimate/src/menu/charmenu.c)
# Input : ko_data/번역/번역_리히터마리아메뉴.tsv (키 cm:*, 분류, 영문, 일판, 한글, 비고)
# Output: mods/sotn-ultimate/src/menu/charmenu_ko_str.h
#   메뉴(8px)    -> kc_<key>[] "_S"-style bytes of the 8px plate renderer (space 0x00, ASCII-0x20, Hangul F0xx big
#                   endian, FF 00 end): drawn by DRA's KO_EQUIP_SEL, the same call as the Extra settings popup
#   설명줄(12px) -> kc_<key>[] the info bar font (func_800F99B8): Hangul F0xx, letters raw, space 0x20,
#                   other ASCII refused (full-width forms only)
# Fail closed: every Hangul needs a glyph code, and for 8px labels a cell in the 8px plate (ko_8p_block_249).
import io
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ko_paths import KO_WK  # noqa: E402
import ko_font_profile as FP  # noqa: E402
import gen_ko_cfg as C  # noqa: E402  (rd, is_h, esc, info_bytes)

REPO = os.environ.get("SOTN_KR", os.getcwd())
TSV = KO_WK + "/번역/번역_리히터마리아메뉴.tsv"
P4 = KO_WK + "/P4시험/"
OUT = os.path.join(REPO, "mods/sotn-ultimate/src/menu/charmenu_ko_str.h")


def main():
    mp = C.rd(P4 + FP.P["map_name"])
    pb = open(P4 + "ko_8p_block_249.bin", "rb").read()
    pn = struct.unpack("<H", pb[:2])[0]
    plate = set(struct.unpack("<%dH" % pn, pb[2:2 + pn * 2]))
    out = ["// gen_ko_cm.py: Richter / Maria pause menu strings (do not edit)"]
    n8 = n12 = 0
    for ln in io.open(TSV, encoding="utf-8").read().split(chr(10))[1:]:
        r = ln.split(chr(9))
        if len(r) < 5 or not r[0].startswith("cm:"):
            continue
        key, kind, ko = r[0][3:], r[1], r[4]
        b = bytearray()
        if kind.startswith("메뉴"):
            for ch in ko:
                if ch == " ":
                    b.append(0x00)
                elif ord(ch) < 128:
                    b.append(ord(ch) - 0x20)
                else:
                    assert C.is_h(ch) and ch in mp, ("no glyph code", ch, key)
                    assert mp[ch] in plate, ("no 8px plate cell", ch, key)
                    b += bytes([mp[ch] >> 8, mp[ch] & 0xFF])
            assert len(ko) <= 10, ("label too long for the 10-char plate parse", key, ko)
            b += bytes([0xFF, 0x00])
            n8 += 1
        else:
            for ch in ko:
                b += C.info_bytes(ch, key, mp)
            n12 += 1
        out.append('static const char kc_%s[] = "%s";' % (key, C.esc(b)))
    io.open(OUT, "w", encoding="utf-8", newline="").write(chr(10).join(out) + chr(10))
    print("charmenu_ko_str.h: %d 8px labels, %d info-bar strings" % (n8, n12))


if __name__ == "__main__":
    sys.exit(main())
