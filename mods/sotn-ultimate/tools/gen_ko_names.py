#!/usr/bin/env python3
# gen_ko_names.py -- KO enemy names (g_EnemyDefs) + relic names/descs (g_RelicDefs)
# from the translation table. Trial (MOD_KO) builds only; OFF builds never run it.
#
# Inputs : ko_data/검수/번역_v14.tsv  (enemydefs.h / enemydefs_mod.h / config_us.h:g_RelicDefs rows,
#                                     column 4 = 번역)
#          src/st/lib/ko_lib_names.h  (old LIB-only name list: fallback when the table cell is empty)
#          src/dra/gen/us/enemydefs.h, enemydefs_mod.h (extracted / gen_saturn_enemies.py, EN names)
#          src/config_us.h            (EN relic rows: leading-space column alignment)
#          glyph map = ko_font_profile (v14c), via ko_text_enc
# Outputs: src/dra/gen/us/enemydefs_ko.h, enemydefs_mod_ko.h  (config_us.h MOD_KO includes them)
#          src/dra/gen/us/ko_relic_str.h                       (KO_RELIC_NAME_n / KO_RELIC_DESC_n)
#          src/dra/gen/us/ko_names_pad.h                       (KO_NAMES_DRA_GROW for 42398.c MOD_DRA_PAD)
#          build/us/ko_enemy_names.tsv                        (id, EN, KO, source) -- review list
#          build/us/ko_enemy_missing.tsv                      (EN name on disc, no Korean anywhere)
#
# Enemy name bytes = _S-style (space 0x00, ASCII-0x20, Hangul F0xx, FF 00): the format BottomCornerText
# (KO_PopupText, eq_popt.c) and the LIB list (e_shop.c func_us_801B1064) both parse; both stop at FF 00
# (a C NUL alone does not end them -- the old ko_lib_names.h strings had no FF 00).
# KO source order per id: table cell -> ko_lib_names.h (decoded with the same map) -> another id with the
# same EN name that has Korean (name sharing, e.g. 0x059 / 0x12F Medusa Head).
#
# DRA layout: the name/desc literals sit in DRA rodata. Rule like gen_ko_equip: a KO name that fits
# the EN literal is padded to the EN size (no shift); a longer one grows rodata and the growth (4-byte
# aligned literals, MIPS gcc CONSTANT_ALIGNMENT) is summed with the relic delta into KO_NAMES_DRA_GROW,
# which 42398.c subtracts from MOD_DRA_PAD. check_dra_layout.sh stays the final judge.
#
# Relic familiar cards: the 7 card names hold "{카드}" in the table; it is replaced by the ONE table value
# config_us.h:g_RelicDefs:card:suffix (decision 2026-11-19: 「~의 근원」).
import io
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ko_paths import KO_WK, KO_REPO  # noqa: E402
import ko_text_enc as E  # noqa: E402

REPO = os.environ.get("SOTN_KR") or KO_REPO
GEN = os.path.join(REPO, "src", "dra", "gen", "us")
TSV = os.environ.get("KO_V14_TSV", KO_WK + "/검수/번역_v14.tsv")
KO_LIB = os.path.join(REPO, "src", "st", "lib", "ko_lib_names.h")
CONFIG = os.path.join(REPO, "src", "config_us.h")
BUILD = os.path.join(REPO, "build", "us")

# Hand-written KO relic literals this replaces (31 rows incl. Godspeed, measured on ko c77d4c3):
# 950 B raw, 1048 B with each literal 4-byte aligned. MOD_DRA_PAD 696 was balanced against these.
RELIC_BASELINE_ALIGNED = 1048
RELIC_NAME_PX_MAX = 120   # relic list: ShowText row buffer 0x3C bytes = 120 px (func_800F99B8 arg2)
RELIC_DESC_PX_MAX = 288   # info bar: 0x90 bytes = 288 px
CARD_TOKEN = "{카드}"
STRICT = "--strict" in sys.argv  # fail on any displayed enemy without Korean


def al4(n):
    return (n + 3) & ~3


def load_table():
    t = {}
    for ln in io.open(TSV, encoding="utf-8"):
        p = ln.rstrip("\n").split("\t")
        if len(p) > 3 and p[0] and not p[0].startswith("키"):
            t[p[0]] = p[3].strip()
    return t


# ---------------------------------------------------------------- enemies
ROW_RE = re.compile(r'^(/\* 0x([0-9A-Fa-f]+) \*/ )\{_S\("((?:[^"\\]|\\.)*)"\)(,.*)$')


def en_unescape(s):
    return s.replace('\\"', '"').replace("\\\\", "\\")


def en_len(s):
    """bytes sotn_str gives an _S() literal before FF 00 (ASCII 1, other 2)."""
    return sum(1 if ord(c) < 128 else 2 for c in s)


def load_ko_lib():
    """id -> Korean text decoded from ko_lib_names.h (codes of the same map; ASCII raw)."""
    rev = {v: k for k, v in E.glyph_map().items()}
    out = {}
    if not os.path.exists(KO_LIB):
        return out
    rx = re.compile(r'^\s*"((?:[^"\\]|\\.)*)",\s*// 0x([0-9A-Fa-f]+)')
    for ln in io.open(KO_LIB, encoding="utf-8"):
        m = rx.match(ln)
        if not m or not m.group(1):
            continue
        bs = E.c_literal_bytes(m.group(1))
        s = []
        i = 0
        while i < len(bs):
            b = bs[i]
            if 0xF0 <= b <= 0xF3 and i + 1 < len(bs):
                code = (b << 8) | bs[i + 1]
                assert code in rev, ("ko_lib_names.h code not in map", m.group(2), hex(code))
                s.append(rev[code])
                i += 2
            elif b == 0xFF and i + 1 < len(bs) and bs[i + 1] == 0x00:
                break  # LIB convention: end = FF 00
            elif b == 0x00:
                s.append(" ")  # LIB convention: space = 00
                i += 1
            else:
                assert 0x20 <= b < 0x7F, ("ko_lib_names.h byte", m.group(2), hex(b))
                s.append(chr(b))
                i += 1
        out[int(m.group(2), 16)] = "".join(s)
    return out


def bake_enemies(tbl):
    kolib = load_ko_lib()
    review = []
    missing = []
    grow = 0
    stats = {"table": 0, "ko_lib": 0, "shared": 0, "inplace": 0, "longer": 0}
    for src, dst in (("enemydefs.h", "enemydefs_ko.h"), ("enemydefs_mod.h", "enemydefs_mod_ko.h")):
        path = os.path.join(GEN, src)
        if not os.path.exists(path):
            assert src != "enemydefs.h", "src/dra/gen/us/enemydefs.h missing: extract assets first"
            continue
        lines = io.open(path, encoding="latin-1").read().split("\n")  # byte-preserving
        rows = []
        for li, ln in enumerate(lines):
            m = ROW_RE.match(ln)
            if m:
                rows.append((li, m))
        # pass 1: direct KO (table, then ko_lib) and the EN -> KO share map
        ko = {}
        share = {}
        for li, m in rows:
            idx = int(m.group(2), 16)
            en = en_unescape(m.group(3))
            k = tbl.get("%s:g_EnemyDefs:0x%03X" % (src, idx), "")
            how = "table"
            if not k and src == "enemydefs.h" and kolib.get(idx):
                k, how = kolib[idx], "ko_lib"
            if k:
                ko[idx] = (k, how)
                if en:
                    share.setdefault(en, k)
        # pass 2: name sharing + emit
        for li, m in rows:
            idx = int(m.group(2), 16)
            en = en_unescape(m.group(3))
            k, how = ko.get(idx, ("", ""))
            if not k and en and en in share:
                k, how = share[en], "shared"
            if not k:
                if en:
                    missing.append((src, idx, en))
                continue
            stats[how] += 1
            kb = E.enc_s(k)
            slot = en_len(en) + 2
            if len(kb) <= slot:
                kb = kb + bytes(slot - len(kb))
                stats["inplace"] += 1
            else:
                grow += al4(len(kb) + 1) - al4(slot + 1)
                stats["longer"] += 1
            lines[li] = '%s{"%s"%s' % (m.group(1), E.esc(kb), m.group(4))
            review.append((src, idx, en, k, how))
        hdr = "// gen_ko_names.py: KO names (do not edit; source %s + v14 translation table)" % src
        io.open(os.path.join(GEN, dst), "w", encoding="latin-1", newline="").write(
            hdr + "\n" + "\n".join(lines))
    return review, missing, grow, stats


# ---------------------------------------------------------------- relics
def relic_en_rows():
    """EN relic names in table order (the #else rows of g_RelicDefs)."""
    txt = io.open(CONFIG, encoding="utf-8").read()
    a = txt.index("RelicDesc g_RelicDefs[] = {")
    b = txt.index("EnemyDef g_EnemyDefs[]", a)
    out = []
    for m in re.finditer(r'^\s*\{"((?:[^"\\]|\\.)*)", "', txt[a:b], re.M):
        if m.group(1):  # skip the blank row 31 ({"", "", 0, 0, 0})
            out.append(m.group(1))
    return out


def bake_relics(tbl):
    en = relic_en_rows()
    assert len(en) == 31, ("expected 30 retail + Godspeed EN relic rows", len(en))
    suffix = tbl.get("config_us.h:g_RelicDefs:card:suffix", "")
    assert suffix, "table: config_us.h:g_RelicDefs:card:suffix is empty"
    out = ["// gen_ko_names.py: KO relic names/descs from 번역_v14.tsv (do not edit)",
           "// familiar-card suffix (table config_us.h:g_RelicDefs:card:suffix): %s" % suffix]
    total = 0
    for i in range(31):
        name = tbl.get("config_us.h:g_RelicDefs:%d:name" % i, "")
        desc = tbl.get("config_us.h:g_RelicDefs:%d:desc" % i, "")
        assert name and desc, ("relic %d: empty table cell" % i)
        name = name.replace(CARD_TOKEN, suffix)
        assert CARD_TOKEN not in desc and "{" not in name, (i, name, desc)
        # retail two-column alignment: every left-column (even) name starts with one space. The EN row of God Speed Shoes
        # (30, left column, added by the Saturn port) has none, so its KO name started 4 px left of the others
        # (VRAM band probe 2026-11-23: ink at col 0 vs 4); the even-index rule covers it and changes no other row.
        if en[i].startswith(" ") or i % 2 == 0:
            name = " " + name
        px = E.show_px(name)
        assert px <= RELIC_NAME_PX_MAX, ("relic %d name too wide" % i, name, px)
        assert E.show_px(desc) <= RELIC_DESC_PX_MAX, ("relic %d desc too wide" % i, desc)
        nb = E.enc_show(name)
        db = E.enc_show(desc)
        assert len(nb) < 38, ("relic %d name over func_800F9E18 strcpy buffer[38]" % i, name)
        total += al4(len(nb) + 1) + al4(len(db) + 1)
        out.append('#define KO_RELIC_NAME_%d "%s" /* %s */' % (i, E.esc(nb), name.strip()))
        out.append('#define KO_RELIC_DESC_%d "%s" /* %s */' % (i, E.esc(db), desc))
    out.append("#define KO_RELIC_STR_BYTES %d /* baseline (hand bytes) %d */" % (
        total, RELIC_BASELINE_ALIGNED))
    io.open(os.path.join(GEN, "ko_relic_str.h"), "w", encoding="utf-8", newline="").write(
        "\n".join(out) + "\n")
    return total - RELIC_BASELINE_ALIGNED


def main():
    tbl = load_table()
    review, missing, egrow, st = bake_enemies(tbl)
    rgrow = bake_relics(tbl)
    grow = egrow + rgrow
    if not os.path.isdir(BUILD):
        os.makedirs(BUILD)
    with io.open(os.path.join(BUILD, "ko_enemy_names.tsv"), "w", encoding="utf-8") as f:
        f.write("file\tid\tEN\tKO\tsource\n")
        for src, idx, en, k, how in review:
            f.write("%s\t0x%03X\t%s\t%s\t%s\n" % (src, idx, en, k, how))
    with io.open(os.path.join(BUILD, "ko_enemy_missing.tsv"), "w", encoding="utf-8") as f:
        f.write("file\tid\tEN\n")
        for src, idx, en in missing:
            f.write("%s\t0x%03X\t%s\n" % (src, idx, en))
    io.open(os.path.join(GEN, "ko_names_pad.h"), "w", encoding="utf-8", newline="").write(
        "// gen_ko_names.py: DRA rodata growth of the KO enemy/relic literals (do not edit)\n"
        "#define KO_NAMES_DRA_GROW (%d)\n" % grow)
    print("ko names: enemies table=%d ko_lib=%d shared=%d (inplace=%d longer=%d, +%d B); "
          "relics delta %+d B; KO_NAMES_DRA_GROW=%d"
          % (st["table"], st["ko_lib"], st["shared"], st["inplace"], st["longer"], egrow, rgrow, grow))
    print("ko names: %d enemy ids with an EN name and no Korean (build/us/ko_enemy_missing.tsv)" % len(missing))
    for src, idx, en in missing:
        print("  MISSING %s 0x%03X %s" % (src, idx, en))
    if grow > 696:
        print("FAIL: KO names grow DRA rodata by %d B > MOD_DRA_PAD 696 (shorten names or use a bundle)" % grow)
        return 1
    if STRICT and missing:
        print("FAIL (--strict): enemy names without Korean")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
