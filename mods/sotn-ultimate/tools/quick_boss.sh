#!/usr/bin/env bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# quick_boss.sh -- rebuild BO2 BO5 BO6 RBO2 RBO6 (and BO6RIC2 with BO6) after a source OR translation change, same
# bytes as the full build, without it.  Steps = the full build's: ninja link of the overlay's elf, gen_ko_wrap.py
# (4-line table), gen_ko_splice.py boss plan + --apply-boss, psp_script --apply, build_bo6ric2.py.  Results only in
# $QUICK_OUT (default /tmp/quick); build/us/*.BIN are NOT written (ninja still refreshes objects / the elf).
#
#   quick_boss.sh NAME [NAME...] [--verify FULL.bin] [--base BASE.bin --image-out OUT.bin]
#     e.g.  quick_boss.sh BO6
#
# Generated headers / yamls are not regenerated (sources only; same limit as quick_bin.sh); the translation path
# is complete: the boss rows are re-wrapped (gen_ko_wrap.py into a work dir) and re-spliced on every run.
# Environment: SOTN_KR, KO_WK, QUICK_OUT, KO_WRAP_SRC (optional: another source table).
set -euo pipefail
if [ -f ~/.profile ]; then source ~/.profile; fi
cd "${SOTN_KR:-$(dirname "$0")/../../..}"
TOOLS=$PWD/mods/sotn-ultimate/tools
OUTDIR=${QUICK_OUT:-/tmp/quick}
NAMES=(); BASE=""; IMGOUT=""; VERIFY=""
while [ $# -gt 0 ]; do
    case "$1" in
        --base) BASE=$2; shift 2;;
        --image-out) IMGOUT=$2; shift 2;;
        --verify) VERIFY=$2; shift 2;;
        -h|--help) sed -n '2,13p' "$0"; exit 0;;
        -*) echo "unknown option $1" >&2; exit 2;;
        *) NAMES+=("$(echo "${1%.BIN}" | tr a-z A-Z)"); shift;;
    esac
done
[ ${#NAMES[@]} -gt 0 ] || { sed -n '2,13p' "$0" >&2; exit 2; }
. "$TOOLS/quick_env.sh"
W=$OUTDIR/boss_work
rm -rf "$W"; mkdir -p "$W/wrap" "$OUTDIR"
T0=$(date +%s.%N)
# 1. translation: the 4-line table into the work dir (gen_ko_wrap.py honours KO_OUTD / KO_WRAP_SRC)
SRCDIR=$KO_WK/검수
cp "$SRCDIR"/줄끊기_수정_*.tsv "$W/wrap/" 2>/dev/null || true
KO_OUTD="$W/wrap/" KO_WRAP_SRC="${KO_WRAP_SRC:-$SRCDIR/번역_v14.tsv}" python3 "$TOOLS/gen_ko_wrap.py" | tail -2
export KO_TSV=$(ls "$W"/wrap/번역_v14_*줄.tsv | head -1)
# 2. link each overlay (ninja, objects of this tree) and copy the raw BIN
for n in "${NAMES[@]}"; do
    rawvar=QUICK_RAW_$n   # QUICK_RAW_<NAME>=file: use this freshly linked BIN instead of linking (control runs)
    if [ -n "${!rawvar:-}" ]; then cp "${!rawvar}" "$W/$n.BIN"; echo "== $n: raw BIN ${!rawvar}"; continue; fi
    elf=$(ninja -t query "build/us/$n.BIN" | awk '/input:/{getline; print $1}')
    [ -n "$elf" ] || { echo "ABORT: no ninja target for $n" >&2; exit 3; }
    if [ "${QUICK_NO_NINJA:-0}" = 1 ]; then   # post-link steps only, on the elf as it is
        echo "== $n: elf as is $elf"
    else
        echo "== $n: ninja $elf"
        ninja "$elf" 2>&1 | grep -vE 'Warning: used \$at|^\{standard input\}' | tail -3
    fi
    mipsel-linux-gnu-objcopy -O binary "$elf" "$W/$n.BIN"
done
# 3. boss splice + psp copies + BO6RIC2 on the copies
python3 "$TOOLS/quick_boss.py" "$W" "${NAMES[@]}"
OUTS=("${NAMES[@]}"); for n in "${NAMES[@]}"; do if [ "$n" = BO6 ]; then OUTS+=(BO6RIC2); fi; done
ARGS=()
for n in "${OUTS[@]}"; do
    cp "$W/$n.BIN" "$OUTDIR/$n.BIN"; sha1sum "$OUTDIR/$n.BIN"
    if [ -n "$VERIFY" ]; then
        QUICK_VERIFY_FILE="$OUTDIR/$n.BIN" python3 "$TOOLS/quick_bin.py" verify "$n" "$VERIFY" || true
    fi
    ARGS+=("$(python3 "$TOOLS/quick_bin.py" diskpath "$n")=$OUTDIR/$n.BIN")
done
if [ -n "$BASE" ]; then python3 "$TOOLS/quickpatch.py" "$BASE" "$IMGOUT" "${ARGS[@]}"; fi
T1=$(date +%s.%N)
echo "quick_boss: ${OUTS[*]} done in $(echo "$T1 - $T0" | bc) s"
