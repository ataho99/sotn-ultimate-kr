#!/usr/bin/env bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# quick_bin.sh -- rebuild ONE overlay BIN of the Korean build after a source change, byte-identical
# to what build_ko_trial.sh leaves in build/us/<NAME>.BIN, without the 6-12 minute full build.
#
#   quick_bin.sh NAME [NAME...] [--base BASE.bin --image-out OUT.bin] [--verify FULL.bin]
#     e.g.  quick_bin.sh LIB
#           quick_bin.sh LIB CHI --base finished_disc.bin --image-out quick_disc.bin
#
# What it does (per NAME):
#   1. recreates the full build's compile environment: the `export` lines of build_ko_trial.sh and
#      its cpp wrapper (-DMOD_KO -DMOD_KO_EQUIP; the wrapper file the last full build wrote into
#      $MOD_WORK/ko_wrappers is used as is).  A plain `ninja` WITHOUT this environment compiles a
#      different overlay (no KO branch): never run ninja for these BINs by hand.
#   2. ninja build/us/NAME.BIN  (recompiles only what changed, then links)
#   3. the post-link steps that touch this BIN (quick_bin.py post): area-name card
#      (gen_ko_cards.py) and PSP script copies (psp_script.py --apply), same code as the full build
#   4. copies the result to $QUICK_OUT (default /tmp/quick) and, with --base, writes a disc image
#      with the file replaced (quickpatch.py)
#
# Prerequisites: one finished full build in this tree (build/us objects, generated headers
# src/**/gen, build/us/psp_script/patches.json, build/mod-work/ko_wrappers).  The generated
# headers are NOT regenerated: after changing translation tables / generators run the full build
# (or the gen_ko_* steps) first.  See quick_bin.py for the BINs that need the full build.
#
# Environment: SOTN_KR (tree root, default: this script's tree), KO_WK, QUICK_OUT.
set -euo pipefail
if [ -f ~/.profile ]; then source ~/.profile; fi
cd "${SOTN_KR:-$(dirname "$0")/../../..}"
ROOT=$PWD
TOOLS=$ROOT/mods/sotn-ultimate/tools
OUTDIR=${QUICK_OUT:-/tmp/quick}
NAMES=(); BASE=""; IMGOUT=""; VERIFY=""
while [ $# -gt 0 ]; do
    case "$1" in
        --base) BASE=$2; shift 2;;
        --image-out) IMGOUT=$2; shift 2;;
        --verify) VERIFY=$2; shift 2;;
        -h|--help) sed -n '2,25p' "$0"; exit 0;;
        -*) echo "unknown option $1" >&2; exit 2;;
        *) NAMES+=("${1%.BIN}"); shift;;
    esac
done
[ ${#NAMES[@]} -gt 0 ] || { sed -n '2,12p' "$0" >&2; exit 2; }
if [ -n "$BASE$IMGOUT" ] && { [ -z "$BASE" ] || [ -z "$IMGOUT" ]; }; then
    echo "--base and --image-out go together" >&2; exit 2
fi

# --- the full build's environment ---------------------------------------------------------
# External inputs of build_ko_trial.sh (recipe file is optional: ko_data/*.env needs paths of
# the build machine; nothing in it reaches the C compile of an overlay)
export KO_WK=${KO_WK:-$ROOT/ko_data}
if [ -n "${SOTN_U_IMAGE:-}" ] && [ -f "$KO_WK/빌드레시피.env" ]; then . "$KO_WK/빌드레시피.env"; fi
# the exports of build_ko_trial.sh itself (lines before the first generator step), evaluated
# as the script does -- single source of truth, no copy to drift
MOD_WORK=${MOD_WORK:-$ROOT/build/mod-work}
KO_WRAPPERS=$MOD_WORK/ko_wrappers
KO_GOSHIM=$MOD_WORK/ko_goshim
[ -x "$KO_WRAPPERS/mipsel-linux-gnu-cpp" ] || { echo "ABORT: $KO_WRAPPERS/mipsel-linux-gnu-cpp missing: run the full build once" >&2; exit 3; }
grep -q -- '-DMOD_KO' "$KO_WRAPPERS/mipsel-linux-gnu-cpp" || { echo "ABORT: cpp wrapper has no -DMOD_KO" >&2; exit 3; }
EXPORTS=$(mktemp)
trap 'rm -f "$EXPORTS"' EXIT
sed -n '1,200p' "$TOOLS/build_ko_trial.sh" | grep -E '^export [A-Za-z_]+=' | sed 's/ #.*$//' |
    grep -v '^export PATH=' > "$EXPORTS"
. "$EXPORTS"
export PATH="$KO_WRAPPERS:$PATH"
export SOTN_KR=$ROOT MOD_WORK

run_one() {
    local name=$1
    python3 "$TOOLS/quick_bin.py" plan "$name" || { echo "ABORT: $name is not supported by the quick path" >&2; return 3; }
    ninja -n "build/us/$name.BIN" > /dev/null 2>&1 || { echo "ABORT: no ninja target build/us/$name.BIN" >&2; return 3; }
    echo "== $name: ninja"
    ninja "build/us/$name.BIN" 2>&1 | grep -vE 'Warning: used \$at|^\{standard input\}' | tail -5
    python3 "$TOOLS/quick_bin.py" post "$name"
    mkdir -p "$OUTDIR"
    cp "build/us/$name.BIN" "$OUTDIR/$name.BIN"
    sha1sum "$OUTDIR/$name.BIN"
    if [ -n "$VERIFY" ]; then python3 "$TOOLS/quick_bin.py" verify "$name" "$VERIFY"; fi
}

T0=$(date +%s.%N)
ARGS=()
for n in "${NAMES[@]}"; do
    run_one "$n"
    ARGS+=("$(python3 "$TOOLS/quick_bin.py" diskpath "$n")=$OUTDIR/$n.BIN")
done
if [ -n "$BASE" ]; then
    python3 "$TOOLS/quickpatch.py" "$BASE" "$IMGOUT" "${ARGS[@]}"
fi
T1=$(date +%s.%N)
echo "quick_bin: ${NAMES[*]} done in $(echo "$T1 - $T0" | bc) s"
