#!/usr/bin/env bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# quick_sel.sh -- rebuild ST/SEL/SEL.BIN and BIN/SELRIC2.BIN after a SOURCE change without the full build and
# without make disk.  SEL depends on include/lba_us.h only for the stage audio file lengths (see quick_sel.py), so
# the last full build's header is used as it is and, with an image, checked against that image.  Steps: ninja link
# of stsel.elf, psp_script copies, build_selric2, the Japanese-movie patch (6 bytes) the full build's last step adds.
# Results only in $QUICK_OUT (default /tmp/quick); build/us/*.BIN are not written.
#
#   quick_sel.sh [--verify FULL.bin] [--base BASE.bin --image-out OUT.bin]
#
# The image given by --verify / --base is also where the movie sector number is read from (else the size of
# build/sotn.us.bin).  New SEL must fit its sectors for --base.
# Translation changes of the ending dialogue (sel_ending rows, spliced into a yaml and from it into a generated
# header) are NOT covered: run the generators / full build.  Environment: SOTN_KR, KO_WK, QUICK_OUT,
# QUICK_NO_NINJA=1 (post-link steps on the elf as it is).
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
        -h|--help) sed -n '2,16p' "$0"; exit 0;;
        *) echo "unknown argument $1" >&2; exit 2;;
    esac
done
. "$TOOLS/quick_env.sh"
W=$OUTDIR/sel_work
rm -rf "$W"; mkdir -p "$W" "$OUTDIR"
T0=$(date +%s.%N)
MOVIE_IMG=${VERIFY:-$BASE}
[ -z "$MOVIE_IMG" ] || python3 "$TOOLS/quick_sel.py" lba "$MOVIE_IMG" || { echo "ABORT: lba_us.h is not the image's: full build needed" >&2; exit 3; }
elf=$(ninja -t query build/us/SEL.BIN | awk '/input:/{getline; print $1}')
if [ -n "${QUICK_RAW_SEL:-}" ]; then      # a freshly linked SEL.BIN made elsewhere (control runs)
    echo "== SEL: raw BIN $QUICK_RAW_SEL"
    cp "$QUICK_RAW_SEL" "$W/SEL.BIN"
else
    if [ "${QUICK_NO_NINJA:-0}" = 1 ]; then
        echo "== SEL: elf as is $elf"
    else
        echo "== SEL: ninja $elf"
        ninja "$elf" 2>&1 | grep -vE 'Warning: used \$at|^\{standard input\}' | tail -3
    fi
    mipsel-linux-gnu-objcopy -O binary "$elf" "$W/SEL.BIN"
fi
python3 "$TOOLS/quick_sel.py" post "$W" "$MOVIE_IMG"
ARGS=()
for n in SEL SELRIC2; do
    cp "$W/$n.BIN" "$OUTDIR/$n.BIN"; sha1sum "$OUTDIR/$n.BIN"
    if [ -n "$VERIFY" ]; then QUICK_VERIFY_FILE="$OUTDIR/$n.BIN" python3 "$TOOLS/quick_bin.py" verify "$n" "$VERIFY" || true; fi
    ARGS+=("$(python3 "$TOOLS/quick_bin.py" diskpath "$n")=$OUTDIR/$n.BIN")
done
if [ -n "$BASE" ]; then python3 "$TOOLS/quickpatch.py" "$BASE" "$IMGOUT" "${ARGS[@]}"; fi
T1=$(date +%s.%N)
echo "quick_sel: SEL SELRIC2 done in $(echo "$T1 - $T0" | bc) s"
