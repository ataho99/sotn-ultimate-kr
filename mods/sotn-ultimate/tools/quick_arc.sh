#!/usr/bin/env bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# quick_arc.sh -- rebuild ARC_KO.BIN (glyph block + resident KO functions + book blob + field messages
# + JP bundle + 8px block) without the full build.  The assembly code is NOT copied: it is cut out of
# build_ko_trial.sh at run time (from the ko_blob step to the line that writes ARC_KO.BIN), the paths of
# its outputs are redirected into $QUICK_OUT/arc_work, so nothing under build/us is written.
#
#   quick_arc.sh [--verify IMAGE.bin] [--base BASE.bin --image-out OUT.bin]
#
# Result: $QUICK_OUT/ARC_KO.BIN (default /tmp/quick).  With --base/--image-out the file is also written into a
# copy of a finished image: both the BIN/ARC_KO.BIN file and the 34816-byte tail of the SLUS exe, where the
# full build appends the same bytes.
# Inputs read (not generated here): src/dra/dra.h, build/us/dra.elf (symbol addresses), the glyph block /
# ko_libdesc.bin / ko_field.bin / promo / sheet2 / engtab files in ko_data, src/dra/gen/us/ko_jp_bundle.bin.
# After changing a generator or table that feeds them, run that generator first (this script does not).
# Environment: KO_WK, QUICK_OUT, QUICK_MENU_SRC (optional: a directory with a copy of mods/sotn-ultimate/src/menu,
# to compile the resident functions from another version of the sources, e.g. for a control run).
set -euo pipefail
if [ -f ~/.profile ]; then source ~/.profile; fi
cd "${SOTN_KR:-$(dirname "$0")/../../..}"
TOOLS=$PWD/mods/sotn-ultimate/tools
OUTDIR=${QUICK_OUT:-/tmp/quick}
BASE=""; IMGOUT=""; VERIFY=""
while [ $# -gt 0 ]; do
    case "$1" in
        --base) BASE=$2; shift 2;;
        --image-out) IMGOUT=$2; shift 2;;
        --verify) VERIFY=$2; shift 2;;
        -h|--help) sed -n '2,17p' "$0"; exit 0;;
        *) echo "unknown argument $1" >&2; exit 2;;
    esac
done
. "$TOOLS/quick_env.sh"
W=$OUTDIR/arc_work
rm -rf "$W"; mkdir -p "$W"
T0=$(date +%s.%N)
SRC=$TOOLS/build_ko_trial.sh
awk '/^BLOBOUT=build\/us\/mod\/kobob/{p=1} p{print} p && /^print\(.ARC_KO\.BIN: glyphs/{exit}' "$SRC" > "$W/steps.sh"
echo PYEOF >> "$W/steps.sh"
grep -q "^print('ARC_KO.BIN: glyphs" "$W/steps.sh" || { echo "ABORT: ARC_KO step markers not found in build_ko_trial.sh" >&2; exit 3; }
sed -i -e "s#build/us/mod/kobob#$W/kobob#g" -e "s#build/us/mod/koequip#$W/koequip#g" -e "s#build/us/ARC_KO\.BIN#$W/ARC_KO.BIN#g"     -e "s#mods/sotn-ultimate/src/menu/#${QUICK_MENU_SRC:-mods/sotn-ultimate/src/menu}/#g" "$W/steps.sh"
# nothing of the cut-out may still write into build/us
if grep -nE "(>|>>|-o|open\()[^|]*build/us/" "$W/steps.sh" | grep -v "dra.elf" | grep -vE "open\('build/us/[^']*', 'rb'"; then
    echo "ABORT: the cut-out step would still write into build/us" >&2; exit 3
fi
bash -e -o pipefail "$W/steps.sh" > "$W/steps.log" 2>&1 || { tail -20 "$W/steps.log" >&2; echo "ABORT: ARC_KO step failed" >&2; exit 1; }
tail -4 "$W/steps.log"
cp "$W/ARC_KO.BIN" "$OUTDIR/ARC_KO.BIN"
sha1sum "$OUTDIR/ARC_KO.BIN"
python3 - "$OUTDIR/ARC_KO.BIN" "$VERIFY" "$BASE" "$IMGOUT" <<'PYEOF'
import hashlib, os, sys
sys.path.insert(0, os.path.join(os.environ["SOTN_KR"], "mods/sotn-ultimate/tools"))
import quickpatch as Q
arc, verify, base, out = sys.argv[1:5]
mine = open(arc, "rb").read()
if verify:
    with open(verify, "rb") as f:
        t = Q.walk(f)
        e = t["BIN/ARC_KO.BIN"]
        data = bytes(Q.user(f, e["lba"], (e["size"] + 2047) // 2048))[:e["size"]]
        s = t["SLUS_000.67"]
        exe = bytes(Q.user(f, s["lba"], (s["size"] + 2047) // 2048))[:s["size"]]
    print("quick_arc verify: ARC_KO quick %d B vs image file %d B -> %s" % (
        len(mine), len(data), "IDENTICAL" if mine == data else "DIFFERENT"))
    print("quick_arc verify: image SLUS tail == image ARC_KO file: %s" % (exe[-len(data):] == data))
    if mine != data:
        d = [i for i in range(min(len(mine), len(data))) if mine[i] != data[i]]
        print("  differing bytes: %d, first %s, last %s" % (len(d), hex(d[0]) if d else "-", hex(d[-1]) if d else "-"))
        runs = []
        for i in d:
            if runs and i - runs[-1][1] <= 16:
                runs[-1][1] = i
            else:
                runs.append([i, i])
        print("  runs (file offset, ram 0x80099000+off):", ", ".join("%x-%x" % (a, b) for a, b in runs[:20]))
if base:
    import shutil, struct
    shutil.copyfile(base, out)
    with open(out, "r+b") as f:
        t = Q.walk(f)
        for path in ("BIN/ARC_KO.BIN",):
            e = t[path]
            assert (e["size"] + 2047) // 2048 == len(mine) // 2048
            for si in range(len(mine) // 2048):
                Q.write_user(f, e["lba"] + si, mine[si * 2048:(si + 1) * 2048])
        s = t["SLUS_000.67"]
        first = s["lba"] + (s["size"] - len(mine)) // 2048
        assert (s["size"] - len(mine)) % 2048 == 0
        for si in range(len(mine) // 2048):
            Q.write_user(f, first + si, mine[si * 2048:(si + 1) * 2048])
    print("quick_arc: wrote %s (ARC_KO file + SLUS tail)" % out)
PYEOF
T1=$(date +%s.%N)
echo "quick_arc: done in $(echo "$T1 - $T0" | bc) s"
