# SPDX-License-Identifier: AGPL-3.0-or-later
# quick_env.sh -- sourced by quick_bin.sh / quick_arc.sh / quick_boss.sh / quick_sel.sh: the full Korean
# build's compile environment (the `export` lines of build_ko_trial.sh + its cpp wrapper), so a
# partial rebuild compiles the same bytes.  Expects cwd = tree root and $TOOLS set.
export KO_WK=${KO_WK:-$PWD/ko_data}
if [ -n "${SOTN_U_IMAGE:-}" ] && [ -f "$KO_WK/빌드레시피.env" ]; then . "$KO_WK/빌드레시피.env"; fi
MOD_WORK=${MOD_WORK:-$PWD/build/mod-work}
KO_WRAPPERS=$MOD_WORK/ko_wrappers
[ -x "$KO_WRAPPERS/mipsel-linux-gnu-cpp" ] || { echo "ABORT: $KO_WRAPPERS/mipsel-linux-gnu-cpp missing: run the full build once" >&2; exit 3; }
grep -q -- '-DMOD_KO' "$KO_WRAPPERS/mipsel-linux-gnu-cpp" || { echo "ABORT: cpp wrapper has no -DMOD_KO" >&2; exit 3; }
_QEXP=$(mktemp)
sed -n '1,200p' "$TOOLS/build_ko_trial.sh" | grep -E '^export [A-Za-z_]+=' | sed 's/ #.*$//' |
    grep -v '^export PATH=' > "$_QEXP"
. "$_QEXP"
rm -f "$_QEXP"
export PATH="$KO_WRAPPERS:$PATH"
export SOTN_KR=$PWD MOD_WORK
python3 "$TOOLS/stale_asm_guard.py" # drop objects older than their INCLUDE_ASM .s (ninja cannot see them)
