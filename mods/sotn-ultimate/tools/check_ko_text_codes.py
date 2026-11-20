#!/usr/bin/env python3
# check_ko_text_codes.py -- fail the KO build when a KO string carries a glyph code that is not a
# cell of the CURRENT glyph table (ko_font_profile map + Latin cells + engtab cells, ko_text_enc).
#
# Scans the generated KO string headers (relic names/descs, enemy names, config strings, stage
# field messages) and the sheet-2 upload lists. Also fails when g_RelicDefs in src/config_us.h holds
# hand-written KO bytes again (they must come from gen_ko_names.py; the hand bytes were v4-map codes,
# e.g. 뼈 = F05F, which is the '!' engtab cell in v14c).
# A code that is a valid cell but decodes to the wrong character cannot be seen here: that is what
# generating from the table (one encoder, one map) prevents.
import io
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ko_paths import KO_REPO  # noqa: E402
import ko_text_enc as E  # noqa: E402

REPO = os.environ.get("SOTN_KR") or KO_REPO
FILES = [
    "src/dra/gen/us/ko_relic_str.h",
    "src/dra/gen/us/enemydefs_ko.h",
    "src/dra/gen/us/enemydefs_mod_ko.h",
    "src/dra/gen/us/ko_cfg_str.h",
    "src/st/ko_field_msg.h",
]
REQUIRED = {"src/dra/gen/us/ko_relic_str.h", "src/dra/gen/us/enemydefs_ko.h"}
SHEET2 = E.P4 + "ko_sheet2.bin"
LIT = re.compile(r'"((?:[^"\\]|\\.)*)"')


def strip_comments(t):
    t = re.sub(r"/\*.*?\*/", "", t, flags=re.S)
    return re.sub(r"//[^\n]*", "", t)


def main():
    known = E.known_codes()
    bad = []
    n = 0
    for rel in FILES:
        path = os.path.join(REPO, rel)
        if not os.path.exists(path):
            if rel in REQUIRED:
                bad.append("%s missing (run gen_ko_names.py)" % rel)
            continue
        for ln, line in enumerate(strip_comments(io.open(path, encoding="utf-8").read()).split("\n"), 1):
            for m in LIT.finditer(line):
                bs = E.c_literal_bytes(m.group(1))
                for off, code in E.scan_codes(bs):
                    n += 1
                    if code not in known:
                        bad.append("%s:%d byte %d: code %04X not in the current glyph table"
                                   % (rel, ln, off, code))
    # sheet-2 upload lists (UPLOAD2 at +0, UPLOAD3 at +0x48): big-endian codes up to a NUL
    if os.path.exists(SHEET2):
        blob = open(SHEET2, "rb").read()
        for start in (0, 0x48):
            i = start
            while i + 1 < len(blob) and blob[i] != 0:
                code = (blob[i] << 8) | blob[i + 1]
                n += 1
                if code not in known:
                    bad.append("ko_sheet2.bin +0x%X: code %04X not in the current glyph table" % (i, code))
                i += 2
    # no hand-written KO bytes in g_RelicDefs
    cfg = io.open(os.path.join(REPO, "src/config_us.h"), encoding="utf-8").read()
    a = cfg.index("RelicDesc g_RelicDefs[] = {")
    b = cfg.index("EnemyDef g_EnemyDefs[]", a)
    if re.search(r"\\x[Ff][0-3]", strip_comments(cfg[a:b])):
        bad.append("src/config_us.h g_RelicDefs: hand-written KO bytes (use KO_RELIC_NAME_n / KO_RELIC_DESC_n)")
    if bad:
        for x in bad:
            print("FAIL " + x)
        return 1
    print("OK   KO text codes: %d codes, all cells of %s (+Latin/engtab)" % (n, E.FP.P["map_name"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
