#!/usr/bin/env bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# Korean trial: build_disc.sh copy + -DMOD_KO (menus reached by emucap
# memory writes — no TEST_START code in the game).
# Default output build/sotn_kr_P4_trial1.bin (+ auto .cue). build_disc.sh and
# build_cenp.sh are untouched. The trial never rewrites the tracked pad
# defines (MOD_DATA_PAD / MOD_DRA_PAD): the layout check below runs
# report-only, because shrinking those defines from a trial run would change
# the MOD_KO OFF build inputs. Expected result at this step: the check FAILs
# (ko_glyphs +2730B in DRA rodata does not fit the pads) — that output is the
# measurement the next step prescribes from. Do NOT run past the check.
# Copies the result to MOD_OUT (default build/sotn_kr_P4_trial1.bin).
set -euo pipefail
# Go and Rust may live in the user's home; make sure they are on PATH
if [ -f ~/.profile ]; then source ~/.profile; fi
cd "$(dirname "$0")/../../.."
# v1.5 port: game_mod.h includes this generated header (build_ptbr.py --header writes it; PT-BR is off in the KO build)
[ -f mods/sotn-ultimate/include/mod_ptbr_build.h ] ||
    printf '// Written by mods/sotn-ultimate/tools/build_ptbr.py --header\n' \
        > mods/sotn-ultimate/include/mod_ptbr_build.h
OUT=${MOD_OUT:-build/sotn_kr_P4_trial1.bin}
# 3491: a name that already exists is never overwritten (KO_ALLOW_OVERWRITE=1 to override)
if [ -e "$OUT" ] && [ "${KO_ALLOW_OVERWRITE:-0}" != 1 ]; then
    echo "ABORT: output $OUT already exists; pick a new name (KO_ALLOW_OVERWRITE=1 overrides)" >&2
    exit 4
fi
# gen_ko_splice.py defaults REPO to ${SOTN_KR} (main),
# so a worktree build spliced main's yamls, never the worktree's (NO4 KO run
# missing). Point every SOTN_KR consumer at the tree being built.
export SOTN_KR="${SOTN_KR:-$PWD}"
export KO_WK="${KO_WK:-$PWD/ko_data}"
export KO_FONTS="${KO_FONTS:-$KO_WK/fonts}"
# preflight: every static KO input problem (glyph coverage, keys, widths, slot sizes, cell budget, paths, entry
# offsets) listed at once in 1-3 min, before the long build. KO_SKIP_PREFLIGHT=1 skips; a rebuild (exec "$0") does not repeat it.
if [ "${KO_SKIP_PREFLIGHT:-0}" != 1 ] && [ "${KO_PREFLIGHT_DONE:-0}" != 1 ]; then
    python3 mods/sotn-ultimate/tools/ko_preflight.py || { echo "ABORT: ko_preflight found problems (list above; KO_SKIP_PREFLIGHT=1 skips)" >&2; exit 2; }
    export KO_PREFLIGHT_DONE=1
fi
# Korean trial flags. tools/builds/gen.py has no env passthrough for extra
# -D defines, so they are injected with a compiler wrapper first on PATH:
# only mipsel-linux-gnu-cpp evaluates #if (cc1/maspsx/as see preprocessed
# output), so wrapping cpp covers every C translation unit (DRA + overlays).
# Menus are reached by emucap memory writes, not by in-game grants.
KO_WRAPPERS=${MOD_WORK:-$PWD/build/mod-work}/ko_wrappers
mkdir -p "$KO_WRAPPERS"
# 재실행(exec "$0") 때도 PATH 선두의 ko_wrappers를 제외하고 진짜 cpp를 찾는다.
REALCPP=$(PATH="$(echo "$PATH" | tr ':' '\n' | grep -v ko_wrappers | paste -sd: -)" command -v mipsel-linux-gnu-cpp)
[ "$REALCPP" != "$KO_WRAPPERS/mipsel-linux-gnu-cpp" ] || { echo "ABORT: cpp wrapper would exec itself" >&2; exit 2; }
# KO_FIELD_VOICE=1 (default 0, ko_data/빌드레시피.env): the field voices (sound banks) are the Japanese PS1's for every Dub
# (gen_ps1jp_banks.py + game_mod.h MOD_FIELD_*_BANKS); 0 = the banks of 094_02 (PSP / JP / retail by Dub)
FV_DEFS=""
if [ "${KO_FIELD_VOICE:-0}" = 1 ]; then FV_DEFS="-DMOD_FIELD_JP_VOICE"; fi
# KO_DRAMA (095_36, ko_data/빌드레시피.env): only the DEFAULT of the one flag byte in SEL (g_ModDramaFlag, marker "DRMFLAG" + byte;
# 0 = five-line file select menu, 1 = six lines with Drama mode). The Drama code and picture are always built in; the assembling
# tool sets the byte on the disc image when the user's drama CD material is in.
DR_DEFS="-DMOD_DRAMA_DEFAULT=${KO_DRAMA:-0}"
echo "KO_DRAMA=${KO_DRAMA:-0} (flag byte default ${KO_DRAMA:-0})"
cat > "$KO_WRAPPERS/mipsel-linux-gnu-cpp" <<EOF
#!/usr/bin/env bash
exec "$REALCPP" "\$@" -DMOD_KO -DMOD_KO_EQUIP $FV_DEFS $DR_DEFS # KO trial: equipment_ko.h/accessories_ko.h gate (config_us.h); OFF builds never define it
EOF
chmod +x "$KO_WRAPPERS/mipsel-linux-gnu-cpp"
export PATH="$KO_WRAPPERS:$PATH"
# Canary: the whole KO renderer hangs on -DMOD_KO reaching
# every DRA TU. Preprocess one TU through the wrapper and abort loudly if
# the KO branch is missing (a silent no-MOD_KO build hangs the menu).
# Canary flaked once with identical wrapper+sources; retry once
# and keep stderr so a real failure is diagnosable (was 2>/dev/null).
KO_CANARY_ERR=$(mktemp)
KO_CANARY_OUT=$(mktemp)
KO_CANARY_CMD="mipsel-linux-gnu-cpp -Isrc/dra -lang-c -Iinclude -Iinclude/psxsdk -undef -Dmips -D__GNUC__=2 -D__OPTIMIZE__ -D__mips__ -D__mips -Dpsx -D__psx__ -D__psx -D_PSYQ -D__EXTENSIONS__ -D_MIPSEL -D_LANGUAGE_C -DLANGUAGE_C -DNO_LOGS -DHACKS -DUSE_INCLUDE_ASM -D_internal_version_us -DSOTN_STR -E src/dra/66590.c"
KO_CANARY_OK=0
for KO_TRY in 1 2; do
    if eval "$KO_CANARY_CMD" 2>"$KO_CANARY_ERR" >"$KO_CANARY_OUT" && grep -q "0x80099000" "$KO_CANARY_OUT"; then
        KO_CANARY_OK=1
        break
    fi
    sleep 5
done
if [ "$KO_CANARY_OK" != 1 ]; then
    echo "ABORT: MOD_KO wrapper is not applying (canary: no KO branch in 66590.c, tried x2)" >&2
    command -v mipsel-linux-gnu-cpp >&2
    head -20 "$KO_CANARY_ERR" >&2
    echo "--- wrapper: ---" >&2
    cat "$KO_WRAPPERS/mipsel-linux-gnu-cpp" >&2
    rm -f "$KO_CANARY_ERR" "$KO_CANARY_OUT"
    exit 2
fi
rm -f "$KO_CANARY_ERR" "$KO_CANARY_OUT"
# the work folder (mods/sotn-ultimate/tools/mod_work.py) and the node the builder's
# TypeScript runs on (22 or later)
export MOD_WORK=${MOD_WORK:-$PWD/build/mod-work}
export NODE=${NODE:-node}
# Retail dirt bytes are patched at fixed offsets; on the mod's moved layout
# they corrupt live data (see tools/dirt_patcher.py)
export SOTN_MOD_NO_DIRT=1
export MOD_KO_DUB=1 # KO trial: DUB spans in Korean (gen_menu_text.py)
export SYSOPT_DEFS="-DMOD_SYSOPT_JPPAD -DMOD_SYSOPT_KO $FV_DEFS" # ($FV_DEFS = -DMOD_FIELD_JP_VOICE when KO_FIELD_VOICE=1, game_mod.h) KO trial: Extra settings box uses the JP pad style (Circle = confirm, Cross = back), build_sysopt.sh
export MOD_KO_EQUIP=1 # KO trial: equipment descs in Korean (gen_ko_equip.py)
export MOD_KO=1 # KO trial: _mod_ko.h tables + check_stage_data ST0 KO allowlist

# Run a noisy build step with its output kept in build/us/mod_step.log: on a
# failure show its errors (else its tail) and stop; compile errors printed by
# a step that still succeeds are shown as a warning (before, >/dev/null hid
# them and a failed compile could look like "DRA code grew past its pad")
quiet() {
    local log=build/us/mod_step.log
    mkdir -p build/us
    if ! "$@" > "$log" 2>&1; then
        grep -iE "error|undefined reference" "$log" | head -40 >&2 ||
            tail -40 "$log" >&2
        echo "FAIL: $* (full output: $log)" >&2
        exit 1
    fi
    # cc1 reports as "file.c:123: message" (-w hides its warnings, so these
    # are errors; e.g. "`X' undeclared" dropped a statement from UpdateGame
    # and the build still passed, as the cc1 | as pipe exits with as)
    if grep -qiE "(parse|syntax) error|error:|^[^ :]+\.[ch]:[0-9]+: " "$log"; then
        echo "FAIL: $* printed errors (no image is made; 3491):" >&2
        grep -iE "(parse|syntax) error|error:|^[^ :]+\.[ch]:[0-9]+: " "$log" | head -10 >&2
        grep -iE "(parse|syntax) error|error:|^[^ :]+\.[ch]:[0-9]+: " "$log" \
            >> build/us/cc1_errors.log
        exit 1
    fi
}
rm -f build/us/cc1_errors.log

# P148f: relink bin/sotn-assets once up front. Every `go run` of the `build`
# subcommand self-copies its temp exe over bin/sotn-assets
# (tools/sotn-assets/deps/build_deps.go:108 ensureSotnAssets, via :44 <-
# build.go:21 <- main.go:53; trigger sotn.sh:2) and ninja implicit dep
# (tools/builds/gen.py:983) then re-extracts assets/done_us, clobbering
# xa_music_config.h with the retail table mid-build. Backdate bin mtime to
# done_us so ninja stays quiet (link deterministic: same go build twice ->
# same sha1 4bb3af58; Go sources unchanged since the earlier build, extract identical).
# P148m: trace dir via env (relink 로직 무수정, 경로만 환경값으로)
P148_TRACE_DIR=${P148_TRACE_DIR:-$PWD/build/p148_trace}
mkdir -p "$P148_TRACE_DIR"
date +"pre-relink %s.%N $(sha1sum bin/sotn-assets | cut -d' ' -f1) $(stat -c '%Y %i' bin/sotn-assets)" >> "$P148_TRACE_DIR/sha_trace.log"
go build -o bin/sotn-assets ./tools/sotn-assets
touch -r assets/done_us bin/sotn-assets
date +"post-relink %s.%N $(sha1sum bin/sotn-assets | cut -d' ' -f1) $(stat -c '%Y %i' bin/sotn-assets)" >> "$P148_TRACE_DIR/sha_trace.log"
: > "$P148_TRACE_DIR/relink.done"

# P148f 원인 확정 후 방법 변경: make disk(Makefile:212)가 내부에
# `SKIP_CHECK=1 ./sotn.sh build us`를 또 갖고 있어 go run 자가복사가
# 재발했다(호출 체인: build_ko_trial.sh:869 -> Makefile:212 -> sotn.sh:2
# -> build_deps.go:44/108 -> gen.py:983, 가드 ps 덤프로 확인).
# `go` 앞단에 심을 둬 `run *sotn-assets build`만 링크된 bin으로 직행시키고
# 나머지는 실제 go로 통과시킨다(리포지토리 파일 변경 없음, PATH만).
KO_GOSHIM=${MOD_WORK:-$PWD/build/mod-work}/ko_goshim
mkdir -p "$KO_GOSHIM"
# A rebuild (MOD_BUILD_RERUN exec) inherits PATH with ko_goshim first; take the real go from PATH without the shim,
# else the shim execs itself forever (build_assets 100% CPU spin)
REALGO=$(PATH="$(echo "$PATH" | tr ':' '\n' | grep -v ko_goshim | paste -sd: -)" command -v go)
KO_SOTNASSETS=$PWD/bin/sotn-assets
export KO_SOTNASSETS
cat > "$KO_GOSHIM/go" <<GOEOF
#!/bin/sh
if [ "\$1" = "run" ]; then
  case "\$2" in
    *sotn-assets) if [ "\$3" = "build" ]; then shift 2; exec "\$KO_SOTNASSETS" "\$@"; fi;;
  esac
fi
exec "$REALGO" "\$@"
GOEOF
chmod +x "$KO_GOSHIM/go"
export PATH="$KO_GOSHIM:$PATH"

# DRA and SEL compile against include/lba_us.h (the disc's file sectors),
# which only make disk regenerates: when a disc file was added or resized,
# this build used stale sectors, so it starts again once (below make disk)
LBA_BEFORE=$(sha1sum include/lba_us.h 2>/dev/null || true)

python3 mods/sotn-ultimate/tools/gen_mod_layouts.py
# the Cursed Prison's doors in NO4 / NO0 (room table copies, doorway tiles)
python3 mods/sotn-ultimate/tools/gen_prs_links.py
# the Saturn's familiar card placements (NO2, ARE layouts, in place)
python3 mods/sotn-ultimate/tools/gen_card_layouts.py
.venv/bin/python mods/sotn-ultimate/tools/gen_mod_rooms.py
# the shaft's wall sconces (art, tables): src/st/np3/mod_sconce.h
.venv/bin/python mods/sotn-ultimate/tools/gen_np3_sconce.py
# Richter's / Maria's horse ride into the castle (art, tables):
# src/st/no3/mod_horse.h; its hoofbeats in NO3's bank: build/us/SD_ZKNO3;
# Richter's / Maria's NO3 bank with its neigh: build/us/NO3RIDE.BIN (its PSP
# dub part: gen_psp_sfx.py, below)
.venv/bin/python mods/sotn-ultimate/tools/gen_no3_horse.py
.venv/bin/python mods/sotn-ultimate/tools/gen_no3_sounds.py
# the Saturn's Maria boss fight in CEN (art, tables, scripts):
# mods/sotn-ultimate/src/maria_saturn/mod_maria_boss_data.h; her sounds in CEN's bank:
# build/us/SD_ZKCEN.VH/.VB, src/st/cen/mod_cen_sfx.h (the builder's
# TypeScript, mods/sotn-ultimate/tools/builder/src/cen_sounds.ts, as gen_cen_sounds.py)
# (gen_cen_maria.py runs after build_assets below: it reads the CEN gen header)
$NODE --no-warnings mods/sotn-ultimate/tools/builder/node/cen_sounds.ts cen
.venv/bin/python mods/sotn-ultimate/tools/port_saturn_icons.py
python3 mods/sotn-ultimate/tools/gen_saturn_items.py
python3 mods/sotn-ultimate/tools/gen_saturn_enemies.py
# KO enemy names (g_EnemyDefs, Faerie Scroll popup + Library list) and relic names/descs from the
# translation table -> src/dra/gen/us/enemydefs_ko.h, enemydefs_mod_ko.h, ko_relic_str.h, ko_names_pad.h;
# review lists build/us/ko_enemy_names.tsv / ko_enemy_missing.tsv. Before gen_menu_text.py (MOD_KO_DUB
# leaves the KO enemy/relic strings alone).
python3 mods/sotn-ultimate/tools/gen_ko_names.py
# the PSP port's menu text picks: fixes in DRA's tables, and the strings that
# follow the Dub row (SEL's src/st/sel/mod_dub_text.h; checked below)
python3 mods/sotn-ultimate/tools/gen_menu_text.py
python3 mods/sotn-ultimate/tools/gen_ko_equip.py
python3 mods/sotn-ultimate/tools/gen_ko_cfg.py # System (config) screen strings -> src/dra/gen/us/ko_cfg_str.h
python3 mods/sotn-ultimate/tools/gen_ko_sub.py # familiar voice subtitles -> src/servant/ko_sub_str.h (094_15)
python3 mods/sotn-ultimate/tools/gen_ko_sub8.py # 100: the same lines as 8px rows + aux cells -> mods/sotn-ultimate/src/easy/sub_str.h (the new resident area)
python3 mods/sotn-ultimate/tools/gen_ring_art.py # 100: the transformation ring's release icon -> mods/sotn-ultimate/src/easy/ring_art.h
python3 mods/sotn-ultimate/tools/gen_ko_sound.py # Sound Mode strings -> mods/sotn-ultimate/src/menu/mod_sound_ko_str.h (094_12)
python3 mods/sotn-ultimate/tools/gen_ko_save.py # save-room texts -> src/dra/gen/us/ko_save_str.h
python3 mods/sotn-ultimate/tools/gen_ko_shop.py # Librarian shop label words / document names / descriptions -> src/st/lib/gen/ko_shop_str.h (0.9.4)
python3 mods/sotn-ultimate/tools/gen_ko_fieldmsg.py # stage message boxes (mist / sealed door / NO4 bridge) -> src/st/ko_field_msg.h
# the Saturn Librarian enemy list: src/st/lib/mod_bestiary.h
python3 mods/sotn-ultimate/tools/gen_saturn_bestiary.py
# Saturn-only music: build/us/XA_STR1.unpacked (retail + new tracks),
# mod_xa_music.h
.venv/bin/python mods/sotn-ultimate/tools/gen_saturn_music.py
# the Saturn-only stages' sound banks: the Garden's build/us/SD_ZKTE1.VH/.VB
# (mod_gdn_sfx.h), the Prison's SD_ZKTE2 (mod_prs_sfx.h), the Hell Garden's /
# the Soul Prison's SD_ZKTE3 / SD_ZKTE4 (mod_rgdn_sfx.h / mod_rprs_sfx.h).
# The builder's TypeScript (mods/sotn-ultimate/tools/builder/src/stage_sounds.ts, as
# gen_gdn / gen_prs / gen_rgdn / gen_rprs_sounds.py)
$NODE --no-warnings mods/sotn-ultimate/tools/builder/node/stage_sounds.ts
# the castle map blueprint with the Saturn's Garden cells: build/us/F_MAP.BIN
.venv/bin/python mods/sotn-ultimate/tools/paint_fmap.py disks/us/F_MAP.BIN disks/saturn/SUB_DISP.MAP build/us/F_MAP.BIN >/dev/null
.venv/bin/python mods/sotn-ultimate/tools/port_saturn_food.py
# the New Game character select's portraits: src/st/sel/mod_charsel_gfx.h
.venv/bin/python mods/sotn-ultimate/tools/gen_title_portraits.py
# the main menu's Sound mode button: src/st/sel/mod_sel_soundmode_gfx.h
MOD_PTBR=0 .venv/bin/python mods/sotn-ultimate/tools/gen_sel_soundmode.py >/dev/null
python3 mods/sotn-ultimate/tools/patch_status_asm.py
python3 mods/sotn-ultimate/tools/stale_asm_guard.py # ninja cannot see INCLUDE_ASM .s files: drop objects older than their .s
# the Song Lyrics Card: the Saturn Yousei's singing rule in TT_005
python3 mods/sotn-ultimate/tools/patch_tt005_song.py
# KO build: everything below (including build_assets' header rebuild and
# gen_cen_maria.py, which derives from the CEN gen header) must see retail
# yamls: an earlier build may have left KO-spliced yamls behind
# (incremental rebuilds do not re-extract). Restore first.
python3 mods/sotn-ultimate/tools/gen_ko_splice.py --restore-only
[ -f assets/boss/mar/cutscene_data.yaml.ko_retail ] && cp -f assets/boss/mar/cutscene_data.yaml.ko_retail assets/boss/mar/cutscene_data.yaml  # timing edits this unspliced yaml in place; psp_script reads it as retail
make build_assets >/dev/null
# v1.5 port: Richter's Ultimate stairs stances: src/ric/gen/mod_ric_stairs.h, from his own frames in assets/ric
.venv/bin/python mods/sotn-ultimate/tools/gen_ric_stairs.py
# KO build: gen_cen_maria.py derives the Saturn Maria scripts from the CEN
# cutscene gen header, so it runs here (retail-fresh after build_assets), not
# with the other gen tools above (a previous build may have left a
# KO-spliced header behind; see gen_ko_splice --restore-only).
.venv/bin/python mods/sotn-ultimate/tools/gen_cen_maria.py
# The PSP (Dracula X Chronicles) English dub and script next to the PS1 ones,
# toggled in game (Player Select "Dub" row, include/game.h MOD_PSP_DUB_ON);
# MOD_PSP_DUB=0 builds the PS1 dub and script only. Voices:
# build/us/psp_dub/dub.json (encoded from disks/psp/snd_e/XA), taken by
# repack_xa.py --dub (second streams, src/dra/mod_psp_xa.h). Script:
# psp_script.py plans the PSP copies (and the voice ids with no PSP version,
# which the dub leaves alone: hence dub, script, dub) and writes the
# overlays' mod_psp_script.h; --apply fills the copies further down
MOD_PSP_DUB=${MOD_PSP_DUB:-1}
DUB_FLAGS=""
if [ "$MOD_PSP_DUB" = 1 ]; then
    PSP_DUB_JP=${PSP_DUB_JP:-0} .venv/bin/python mods/sotn-ultimate/tools/psp_dub.py >/dev/null
    python3 mods/sotn-ultimate/tools/psp_script.py
    PSP_DUB_JP=${PSP_DUB_JP:-0} .venv/bin/python mods/sotn-ultimate/tools/psp_dub.py | grep -v "^  skipped"
    DUB_FLAGS="--dub build/us/psp_dub/dub.json"
    # v1.5 port: the prologue scroll per dub (ST0 mod_prologue_scroll.h)
    python3 mods/sotn-ultimate/tools/prologue_scroll.py
else
    python3 mods/sotn-ultimate/tools/psp_script.py --off
    python3 mods/sotn-ultimate/tools/prologue_scroll.py --off
fi
# the PSP dub's player grunts: build/us/ALK_PSP.BIN, RIH_PSP.BIN (Alucard's /
# Richter's vab 1 with the PSP clips, from disks/psp sd_alk/sd_rih.spk;
# src/dra/cd.c loads them instead of VAB/SD_ALK / SD_RIH+SD_MAR when
# MOD_PSP_DUB_ON()); MOD_PSP_DUB=0: the retail banks
if [ "$MOD_PSP_DUB" = 1 ]; then
    .venv/bin/python mods/sotn-ultimate/tools/gen_psp_grunts.py | grep -v "^  "
else
    .venv/bin/python mods/sotn-ultimate/tools/gen_psp_grunts.py --off
fi
# the PSP dub's stage / familiar banks: build/us/SFX_PSP.BIN (boss, enemy and
# familiar voices as the PSP plays them, from disks/psp snd_e/SE + PS.ELF's
# tone lists; cd.c loads them when MOD_PSP_DUB_ON()) + src/dra/mod_psp_sfx.h
if [ "$MOD_PSP_DUB" = 1 ]; then
    .venv/bin/python mods/sotn-ultimate/tools/gen_psp_sfx.py
else
    .venv/bin/python mods/sotn-ultimate/tools/gen_psp_sfx.py --off
fi
# The third dub, "JP" (Player Select Dub row; include/game.h MOD_JP_DUB_ON):
# Japanese voices with the PSP English script. Voices: build/us/jp_dub/dub.json
# (the PSP's Japanese files, disks/psp sound/XA; Maria's CEN scene: the
# Saturn's takes), repack_xa.py --dub-jp (third streams, table in SEL:
# src/st/sel/mod_jp_xa.h); banks: build/us/JP_SD.BIN (its grunts, the PSP
# Maria's voice, the PSP boss Maria's CEN bank, the stage / familiar banks
# whose Japanese clips differ: src/st/sel/mod_jp_sfx.h). MOD_PSP_DUB=0: no
# JP versions (the banks are still built: the file is on the disc)
if [ "$MOD_PSP_DUB" = 1 ]; then
    .venv/bin/python mods/sotn-ultimate/tools/psp_dub.py --set jp
    # again, now with the Japanese lines' lengths: the JP script copies (boxes
    # held for the Japanese voices, slower typing; psp_script.py jp_adjust)
    python3 mods/sotn-ultimate/tools/psp_script.py >/dev/null
    DUB_FLAGS="$DUB_FLAGS --dub-jp build/us/jp_dub/dub.json"
    .venv/bin/python mods/sotn-ultimate/tools/gen_psp_sfx.py --set jp | grep "^jp sfx:"
else
    .venv/bin/python mods/sotn-ultimate/tools/gen_psp_sfx.py --set jp --off
fi
.venv/bin/python mods/sotn-ultimate/tools/gen_psp_grunts.py --set jp | grep -v "^  "
# v1.5 port: the translation build machinery (PT-BR), off in the KO build
if [ "${MOD_PTBR:-0}" = 1 ]; then
    python3 mods/sotn-ultimate/tools/ptbr_script.py
else
    python3 mods/sotn-ultimate/tools/ptbr_script.py --off
fi
JP_MAR2=build/us/mod/mar2/sfx_jp
if [ ! -f "$JP_MAR2/SD_RIH.VB" ]; then
    cargo run --quiet --release --manifest-path tools/maria-sfx/Cargo.toml -- build \
        disks/psp/sound/SE/sd_mar2.spk "$JP_MAR2"
fi
python3 mods/sotn-ultimate/tools/gen_mar2_sounds.py "$JP_MAR2" build/us/MARIA2_SD_JP.BIN
.venv/bin/python mods/sotn-ultimate/tools/gen_cenp_sounds.py --set jp
python3 mods/sotn-ultimate/tools/gen_jp_sd.py
# The field voices (sound banks): the Japanese PS1's grafted into the US ones, for every Dub
# choice (build/us/SD_*.VH/.VB, which make disk takes over the retail files; needs JP_T1)
if [ "${KO_FIELD_VOICE:-0}" = 1 ]; then
    .venv/bin/python mods/sotn-ultimate/tools/gen_ps1jp_banks.py --check
else
    # (off: no field-voice files; a leftover of an earlier run would be taken over by make disk)
    .venv/bin/python mods/sotn-ultimate/tools/gen_ps1jp_banks.py --clean
fi
# Pack XA_STR1 (needs the fresh xa_music_config.h): build/us/XA_STR1.unpacked
# -> build/us/XA_STR1., rewrites the retail and mod XA configs; verifies
# itself (fails the build on any mismatch). --keep-all keeps the JP-only
# lines, --no-mono keeps dual-mono voices stereo
# The 15 PSP-Japanese voice slots take their sectors from the official Ultimate v1.4 image
# (fixed table ko_assets/voice_v14/table.tsv, sha-pinned) — a build step that runs AFTER psp_dub.py's
# cache every time, so a cache rebuild can never bring the old voice back.  VOICE_V14=0 skips.
if [ "$MOD_PSP_DUB" = 1 ] && [ "${PSP_DUB_JP:-0}" = 1 ] && [ "${VOICE_V14:-1}" != 0 ]; then
    V14_BIN="${V14_BIN:-build/v14/v14.bin}"
    [ -f "$V14_BIN" ] || { echo "ABORT: voice step needs V14_BIN ($V14_BIN missing); VOICE_V14=0 skips" >&2; exit 3; }
    python3 mods/sotn-ultimate/tools/voice_v14.py build/us/psp_dub/dub.json "$V14_BIN" mods/sotn-ultimate/ko_assets/voice_v14/table.tsv || { echo "ABORT: voice v14 step failed" >&2; exit 3; }
fi
# 0.9.2: the prologue Moonlight Nocturne takes the JP run 0x33D (the narration is mixed into that run);
# BGM_JP_RUN=0 keeps the US music-only run.  Needs the JP PS1 XA span (ko_data/recipe/p110_jp_xa_span.bin).
BGM_FLAGS=""
if [ "${BGM_JP_RUN:-1}" != 0 ]; then
    [ -f ko_data/recipe/p110_jp_xa_span.bin ] || { echo "ABORT: --swap-bgm needs ko_data/recipe/p110_jp_xa_span.bin (BGM_JP_RUN=0 skips)" >&2; exit 3; }
    BGM_FLAGS="--swap-bgm --swap-span ko_data/recipe/p110_jp_xa_span.bin"
fi
PSP_DUB_JP=${PSP_DUB_JP:-0} .venv/bin/python mods/sotn-ultimate/tools/repack_xa.py $DUB_FLAGS $BGM_FLAGS ${REPACK_XA_FLAGS:-}
# Guard vs stale GEN_H: snapshot repack's xa_music_config.h right after
# its self-verify. The ninja assets-build rewrites this file from retail yaml;
# the freshness assert before check_psp_dub catches a late clobber.
GENH_SHA=$(sha1sum src/dra/gen/us/xa_music_config.h | cut -d' ' -f1)
cp src/dra/gen/us/xa_music_config.h build/us/xa_music_config.h.repack # BGM fix: repack's song table, put back before DRA is compiled
# Saturn balance (equipment + enemy fields) as a runtime patch list for
# ModApplyBalance (src/dra/game_handlers.c, the Player Select Balance row)
python3 mods/sotn-ultimate/tools/gen_saturn_balance.py
# v1.5 port: the PT-BR build header (stub when PT-BR is off)
.venv/bin/python mods/sotn-ultimate/tools/build_ptbr.py --header
# KO build: cutscene KO splice. psp_script.py planning above reads the
# retail cutscene yamls, so this runs after all yaml readers: gen_ko_dlg.py
# bakes/verifies the 145 lines, gen_ko_splice.py rewrites the 11 stage
# cutscene yamls from v13 (0B pad + mod_data_edits.json), gen_ko_mar.py the
# Saturn Maria fight scripts, then gen headers are rebuilt from spliced yamls.
# The rebuild touches .assets_build_done_us after repack_xa, which would trip
# the restart guard below as a false positive (no Go tool changed, no
# re-extract, XA configs untouched -- verified by content-sha at line ~731);
# touch the XA outputs newer so only a genuine clobber restarts.
python3 mods/sotn-ultimate/tools/gen_ko_dlg.py > build/us/ko_dlg_bake.log
python3 mods/sotn-ultimate/tools/gen_ko_wrap.py
python3 mods/sotn-ultimate/tools/gen_ko_sheet2.py
# every KO string header + sheet-2 upload lists: glyph codes must be cells of the current glyph table
python3 mods/sotn-ultimate/tools/check_ko_text_codes.py || { echo "check_ko_text_codes FAILED" >&2; exit 1; }
python3 mods/sotn-ultimate/tools/gen_ko_promo.py
python3 mods/sotn-ultimate/tools/gen_ko_splice.py
# Voice-linked windows >= longest Dub lane (psp_dub + jp_dub) + margin; before the ST0 header snapshot
python3 mods/sotn-ultimate/tools/gen_ko_timing.py
# Every C-side entry offset (RCEN +0x129 pre-fight Shaft, DRE +0x49D, ...) still on its retail command after splice + timing (fail closed)
python3 mods/sotn-ultimate/tools/ko_entry_offsets.py || { echo "ko_entry_offsets FAILED (a KO tool moved a C entry offset: the scene would freeze)" >&2; exit 1; }
python3 mods/sotn-ultimate/tools/gen_ko_st0_snapshot.py
# KO cutscene windows keep the retail window count / speaker order / voice per window, <= window_lines lines (fail closed)
python3 mods/sotn-ultimate/tools/check_ko_cutscene_struct.py || { echo "check_ko_cutscene_struct FAILED" >&2; exit 1; }
python3 mods/sotn-ultimate/tools/gen_ko_mar.py
# Saturn Maria fight scripts get the same window rule (longest voice + margin): the copy gen_ko_mar.py patched, then this tree's copy (skipped when it has no MOD_KO branch)
python3 mods/sotn-ultimate/tools/gen_ko_timing.py --mar --hdr ${SOTN_KR}/mods/sotn-ultimate/src/maria_saturn/mod_maria_boss_data.h
python3 mods/sotn-ultimate/tools/gen_ko_timing.py --mar
make build_assets >/dev/null
touch build/us/XA_STR1. build/us/XA_STR1.unpacked
# BGM fix (silent BGM): "make build_assets" above rewrote xa_music_config.h from the RETAIL
# yaml, so DRA was compiled with the retail song table (file/channel/LBA of the retail disc) while the
# disc's XA_STR1 is the repacked one -> CdlSetfilter file mismatch -> every XA song silent since an earlier build.
# Put repack's table back BEFORE DRA is compiled and fail if it is not what repack wrote.
cp build/us/xa_music_config.h.repack src/dra/gen/us/xa_music_config.h
if [ "$(sha1sum src/dra/gen/us/xa_music_config.h | cut -d' ' -f1)" != "$GENH_SHA" ]; then
    echo "FAIL xa_music_config.h restore (sha != repack's)" >&2
    exit 1
fi

# Build, then rebalance DRA's two pads: MOD_DATA_PAD keeps sound_data (and
# g_SfxScripts) at its retail address, MOD_DRA_PAD keeps the BSS there.
# The data pad goes first because it also moves everything after it.
SKIP_CHECK=1 quiet bin/sotn-assets build us
# v1.5 port: v1.5 changed the asset tool (tools/sotn-assets), so this build extracts the generated headers again and
# replaces repack_xa.py's XA configs. That case already has its restart below ("start again once, the tool is current
# then"), but the sha check right after this line failed first. Restart here, before the check.
if [ src/.assets_build_done_us -nt build/us/XA_STR1. ]; then
    echo "the asset build replaced the XA configs: building again"
    exec bash mods/sotn-ultimate/tools/build_ko_trial.sh "$@"
fi
# the compile must have used repack's table, not a regenerated retail one
if [ "$(sha1sum src/dra/gen/us/xa_music_config.h | cut -d' ' -f1)" != "$GENH_SHA" ]; then
    echo "FAIL xa_music_config.h changed during the DRA build (sha != repack's)" >&2
    exit 1
fi
python3 mods/sotn-ultimate/tools/check_xa_table.py dra || { echo "FAIL check_xa_table dra" >&2; exit 1; }
# A rebuilt asset tool (its Go source changed) makes that build extract the
# generated headers again, xa_music_config.h too, over repack_xa.py's
# configs: start again once (the tool is current then)
if [ src/.assets_build_done_us -nt build/us/XA_STR1. ]; then
    echo "the asset build replaced the XA configs: building again"
    exec bash mods/sotn-ultimate/tools/build_ko_trial.sh "$@"
fi
# Never balance pads blind: stop if the checker itself can't run
if ! { bash mods/sotn-ultimate/tools/check_dra_layout.sh 2>&1 || true; } | grep -q "dra_BSS_START"; then
    echo "check_dra_layout.sh did not run; refusing to continue" >&2
    exit 1
fi
dpad=$(bash mods/sotn-ultimate/tools/check_dra_layout.sh | grep -oE "MOD_DATA_PAD in [^ ]+ to -?[0-9]+" | grep -oE "\-?[0-9]+$" || true)
if [ -n "$dpad" ]; then
    if [ "$dpad" -lt 0 ]; then
        echo "DRA data grew past its pad: free more data first" >&2
        exit 1
    fi
    # KO trial: report-only (do NOT sed the tracked MOD_DATA_PAD define).
    echo "KO trial: check wants MOD_DATA_PAD -> $dpad (not applied, report only)"
fi
if bash mods/sotn-ultimate/tools/check_dra_layout.sh | grep -q "FAIL dra_DATA_END"; then
    bash mods/sotn-ultimate/tools/check_dra_layout.sh || true
    echo "KO trial: dra_DATA_END moved (ko_glyphs 2730B in DRA rodata); stop here for measurement" >&2
    exit 1
fi
pad=$(bash mods/sotn-ultimate/tools/check_dra_layout.sh | grep -oE "MOD_DRA_PAD in [^ ]+ to -?[0-9]+" | grep -oE "\-?[0-9]+$" || true)
if [ -n "$pad" ]; then
    if [ "$pad" -lt 0 ]; then
        echo "DRA code grew past its pad: free more dead code first" >&2
        exit 1
    fi
    # KO trial: report-only (do NOT sed the tracked MOD_DRA_PAD define).
    echo "KO trial: check wants MOD_DRA_PAD -> $pad (not applied, report only)"
fi
# The KO renderer only exists with -DMOD_KO in every DRA TU.
# A silent no-MOD_KO link hangs the menu: abort here, not at the emulator.
# NOTE: never 'objdump | grep -q' here: script runs under 'set -o pipefail',
# so grep -q quitting early SIGPIPEs objdump and the check false-aborts.
# Dump to a file first, then grep the file.
KO_CHK_TMP=$(mktemp)
mipsel-linux-gnu-objdump -d build/us/src/dra/66590.c.o 2>/dev/null >"$KO_CHK_TMP" || { echo "ABORT: objdump 66590 failed" >&2; exit 2; }
if ! grep -q "4032" "$KO_CHK_TMP"; then
    echo "ABORT: 66590.c.o lacks the KO branch (stale or no-MOD_KO objects)" >&2
    ls -la build/us/src/dra/66590.c.o build/us/src/dra/menu.c.o >&2
    sha1sum build/us/src/dra/66590.c.o >&2
    head -20 "$KO_CHK_TMP" >&2
    rm -f "$KO_CHK_TMP"
    exit 2
fi
mipsel-linux-gnu-objdump -d build/us/src/dra/menu.c.o 2>/dev/null >"$KO_CHK_TMP" || { echo "ABORT: objdump menu failed" >&2; exit 2; }
if ! grep -q "0x8009" "$KO_CHK_TMP"; then
    echo "ABORT: menu.c.o lacks the KO hook call (stale or no-MOD_KO objects)" >&2
    rm -f "$KO_CHK_TMP"
    exit 2
fi
rm -f "$KO_CHK_TMP"
echo "OK   KO code in DRA objects (66590 wrapper + menu hook)"
# KO bytes of the asm boss scripts (bo2/bo5/bo6/rbo2/rbo6) into the
# linked BINs (plan from gen_ko_splice.py; empty while their rows are
# placeholders), before psp_script checks them and BO6RIC2 copies BO6
python3 mods/sotn-ultimate/tools/gen_ko_splice.py --apply-boss
# the PSP English script into the built overlays (before BO6RIC2 copies BO6)
if [ "$MOD_PSP_DUB" = 1 ]; then
    python3 mods/sotn-ultimate/tools/psp_script.py --apply
    python3 mods/sotn-ultimate/tools/prologue_scroll.py --apply
fi

# The Saturn's new stages (TE1.BIN / F_TE1.BIN slots); needs the WRP build
bash mods/sotn-ultimate/tools/build_gdn.sh
# the Cursed Prison (TE2.BIN / F_TE2.BIN slots)
bash mods/sotn-ultimate/tools/build_prs.sh
# the Soul Prison, the inverted castle's (TE4.BIN / F_TE4.BIN slots)
bash mods/sotn-ultimate/tools/build_rprs.sh
# the Hell Garden, the inverted castle's Garden (TE3.BIN / F_TE3.BIN slots)
bash mods/sotn-ultimate/tools/build_rgdn.sh
# KO area name cards (ko_data/cards/*.png): Korean cmpgfx bytes over the English slot of the linked stage BINs,
# zero padded to the same length; registered in mod_data_edits.json for check_stage_data (idempotent)
python3 mods/sotn-ultimate/tools/gen_ko_cards.py
# Left-hand copies of the weapons the mod changes (WEAPON1.BIN; the decomp
# only builds WEAPON0.BIN). Weapon 21 throws food; weapon 17 (Pentagram /
# Bat Pentagram) follows the Balance row. 37 (Jewel / Stone sword: gold),
# 44 (Heart Refresh: hearts) and 52 (Alucard Shield: its Shield Rod spell
# spends MP) touch g_Status fields the mod moved by 0x14 (HAND_ITEMS_CAP):
# the retail left-hand copies would read the old offsets
# 000 (swords): the Saturn's Terminus Est / Mourneblade subtypes
bash mods/sotn-ultimate/tools/build_weapon1.sh 021 017 037 044 052 001 004 005 014 031 033 000
python3 mods/sotn-ultimate/tools/check_weapon_hands.py
# the Saturn's new weapons 60-63: build/us/WEAPON2.BIN
python3 mods/sotn-ultimate/tools/build_saturn_weapons.py
# the Saturn's bestiary pictures: build/us/MONSTER2.BIN (its 18 new enemies) and
# build/us/MONSTER.BIN (retail, same size, 3 Saturn touch-ups). The builder's
# TypeScript (mods/sotn-ultimate/tools/builder, also run by the player's builder page); the
# internal Ghost Skull build still needs the Python (its extra record)
if grep -q "^#define MOD_OUTER_WALL_GHOST *$" include/game.h mods/sotn-ultimate/include/game_mod.h; then
    python3 mods/sotn-ultimate/tools/build_monster2.py
else
    $NODE mods/sotn-ultimate/tools/builder/node/monster2.ts
fi
# Richter's Saturn alternate costume: build/us/RIC2.BIN (RIC.BIN + new sheet)
.venv/bin/python mods/sotn-ultimate/tools/build_ric2.py
# boss Richter in that coat: build/us/BO6RIC2.BIN (BO6.BIN + the RIC2 sheet)
.venv/bin/python mods/sotn-ultimate/tools/build_bo6ric2.py
# ... and the kneeling Richter after Shaft's orb (TOP): g_ModTopRic2
(cd mods/sotn-ultimate/tools && python3 build_topric2.py)
# the Saturn's playable Maria: build/us/MARIA.BIN (src/ric + -DMOD_MARIA_SAT);
# her packed graphics only when the generator or Saturn data changed (slow)
if [ ! -f build/us/mod/mar/mar_sprites.h ] ||
    [ mods/sotn-ultimate/tools/gen_mar_assets.py -nt build/us/mod/mar/mar_sprites.h ]; then
    .venv/bin/python mods/sotn-ultimate/tools/gen_mar_assets.py
fi
# her Saturn voice and sounds: build/us/MARIA_SD.BIN (her vab 1 and vab 2,
# loaded by src/dra/cd.c) and build/us/mod/mar/mar_sfx.h. The builder's
# TypeScript (mods/sotn-ultimate/tools/builder/src/mar_sounds.ts, also run by the player's
# builder page; ~1 s), ported from gen_mar_sounds.py
$NODE --no-warnings mods/sotn-ultimate/tools/builder/node/mar_sounds.ts build/us/MARIA_SD.BIN \
    build/us/mod/mar/mar_sfx.h | tail -1
# the same banks with the PSP dub's English lines: build/us/MARIA_SD_PSP.BIN
$NODE --no-warnings mods/sotn-ultimate/tools/builder/node/mar_sounds.ts --psp build/us/MARIA_SD_PSP.BIN | tail -1
# her PS1-dub bank: a copy of the PSP one (her select panel offers no PS1 dub)
cp build/us/MARIA_SD_PSP.BIN build/us/MARIA_SD_PS1.BIN
bash mods/sotn-ultimate/tools/build_mar.sh
# the PSP Maria (Maria "PSP" on the select screen): build/us/MARIA2.BIN
# (src/maria + mods/sotn-ultimate/src/maria_psp glue) and her PSP voice build/us/MARIA2_SD.BIN
bash mods/sotn-ultimate/tools/build_mar2.sh
# MARIA_SD_PS1 + the PSP Maria's PS1-dub bank after it, once build_mar2.sh
# has made MARIA2_SD.BIN (same layout, 0x42 sectors each;
# src/dra/cd.c MOD_MAR_SD_SECTORS): a copy of her PSP bank.
# The file is cut back to one bank first
cp build/us/MARIA2_SD.BIN build/us/MARIA2_SD_PS1.BIN
for f in MARIA_SD_PSP MARIA2_SD_PS1; do
    if [ "$(stat -c %s build/us/$f.BIN)" -ne $((0x42 * 0x800)) ]; then
        echo "FAIL build/us/$f.BIN is not 0x42 sectors"; exit 1
    fi
done
truncate -s $((0x42 * 0x800)) build/us/MARIA_SD_PS1.BIN
cat build/us/MARIA2_SD_PS1.BIN >> build/us/MARIA_SD_PS1.BIN
# the PSP Maria boss fight (Alucard's "Boss Maria: PSP" on the select
# screen): build/us/CENP.BIN, CEN with it in the Saturn fight's place
bash mods/sotn-ultimate/tools/build_cenp.sh
# her Saturn HUD art in F_GAME2's empty VRAM: build/us/F_GAME2.BIN
.venv/bin/python mods/sotn-ultimate/tools/gen_mar_hud.py
.venv/bin/python mods/sotn-ultimate/tools/build_ptbr.py --apply
# Original 8x8 English menu sheet re-baked with the small Hangul font (F_GAME/F_GAME2 +0x30000)
python3 mods/sotn-ultimate/tools/gen_ko_font8.py --patch
# the pause menu System screen's "Ultimate settings" (Alucard games): code
# after Alucard's sprites in build/us/ARC_F2.BIN (mods/sotn-ultimate/src/menu/sysopt.c),
# against DRA's addresses (checked again after make disk below)
bash mods/sotn-ultimate/tools/build_sysopt.sh
# 099: the new resident area (r1_easy at 0x801F3000+16) -> build/us/mod/newarea/newarea.bin, placed by the ARC_KO step below
bash mods/sotn-ultimate/tools/build_newarea.sh
# v1.5 port: Richter's / the Marias' pause menu drawing (Ultimate balance), read into g_Pix[1] when it opens
# the Richter / Maria pause menu in Korean: strings from 번역_리히터마리아메뉴.tsv (8px plate cells + info bar font)
.venv/bin/python mods/sotn-ultimate/tools/gen_ko_cm.py
CHARMENU_DEFS="-DMOD_CHARMENU_KO" bash mods/sotn-ultimate/tools/build_charmenu.sh
# VAB/SD_MAR.VB grows with the field voices on: the unused retail ARC_F.BIN gives its sectors back (build_sysopt.sh removed any earlier one)
if [ "${KO_FIELD_VOICE:-0}" = 1 ]; then .venv/bin/python mods/sotn-ultimate/tools/gen_ps1jp_banks.py --arcf; fi
# 776-glyph ARC_KO.BIN (A-gap) + blob renderer. Trial-only disc
# file at the disc END (the end-append still shifts post-VAB files +1, so
# cd.c's shifted literals use KO_LBA (check_cd_lba resolves it per image);
# the manifest and lba.go are patched here and restored at script
# exit (OFF builds never see either).
MANIFEST=config/disk.us.lba
LBAGO=tools/sotn-assets/mods/lba.go
[ -f "$MANIFEST.bak_trial10" ] || cp "$MANIFEST" "$MANIFEST.bak_trial10"
[ -f "$LBAGO.bak_trial10" ] || cp "$LBAGO" "$LBAGO.bak_trial10"
restore_trial10() {
    [ -f "$MANIFEST.bak_trial10" ] && { cp "$MANIFEST.bak_trial10" "$MANIFEST"; rm -f "$MANIFEST.bak_trial10"; }
    [ -f "$LBAGO.bak_trial10" ] && { cp "$LBAGO.bak_trial10" "$LBAGO"; rm -f "$LBAGO.bak_trial10"; }
}
trap restore_trial10 EXIT
grep -q "BIN/ARC_KO.BIN" "$MANIFEST" || echo 'BIN/ARC_KO.BIN;1,N,19970901-134338-36' >> "$MANIFEST"
grep -q "BIN/KO_JP.BIN" "$MANIFEST" || echo 'BIN/KO_JP.BIN;1,N,19970901-134338-36' >> "$MANIFEST"
grep -q 'OFF_BIN_KO' "$LBAGO" || python3 - "$LBAGO" <<'PYEOF'
import io, sys
p = sys.argv[1]
s = io.open(p, encoding='utf-8').read()
anchor = '\t{"OFF_BIN_NO3RIDE", "BIN/NO3RIDE.BIN", lbaOffset},\n'
assert s.count(anchor) == 1, 'lba.go anchor'
s = s.replace(anchor, anchor + '\t// Trial10: ARC_KO.BIN (A-gap glyphs+blob; trial manifest only)\n\t{"OFF_BIN_KO", "BIN/ARC_KO.BIN", lbaOffset},\n')
kojp_anchor = '\t{"OFF_BIN_KO", "BIN/ARC_KO.BIN", lbaOffset},\n'
assert s.count(kojp_anchor) == 1, 'lba.go kojp anchor'
s = s.replace(kojp_anchor, kojp_anchor + '\t// Trial17d: KO_JP.BIN (equip JP-bundle 1 sector; trial manifest only)\n\t{"OFF_BIN_KO_JP", "BIN/KO_JP.BIN", lbaOffset},\n')
io.open(p, 'w', encoding='utf-8').write(s)
print('lba.go: OFF_BIN_KO entry added')
print('lba.go: OFF_BIN_KO_JP entry added')
PYEOF
# ko_equip: KO_JP.BIN (menu-open bundle, 1 sector) at disc end.
# Source: gen_ko_equip.py bake_mod() output (runs above); padded to 2048.
python3 - <<'PYEOF'
import io
b = io.open('src/dra/gen/us/ko_jp_bundle.bin', 'rb').read()
assert 1 <= len(b) <= 2048, len(b)
open('build/us/KO_JP.BIN', 'wb').write(bytes(b) + bytes(2048 - len(b)))
print('KO_JP.BIN: bundle %d -> 2048 (1 sector)' % len(b))
PYEOF
# KO_8P.BIN (equipment-name 8px block, 2 sectors) spliced
# into the 17d3 zero tail at LBA 0x492CA (after the 58-sector EXE at
# 0x49290-0x492C9; readback-asserted, emu-verified -- not manifest-packed).
# Source: P4시험/ko_8p_block_249.bin (gen_ko_8p.py v246 = 371 cells, + gen_ko_8p_ext.py: v247 385 cells (+ the Extra settings popup 14) -> v248 390 cells (+ 추 U R O F: 추가 설정 row, SATURN, ON/OFF); 371 cells = the 283-cell block 243 [260H + 8 name-ASCII + 15
# desc-ASCII reserve; Mgalmuri8 @8px, 1:1, sha-pinned).
python3 - <<'PYEOF'
import hashlib, io
b = io.open(__import__('os').environ['KO_WK'] + '/P4시험/ko_8p_block_249.bin', 'rb').read()
assert hashlib.sha256(b).hexdigest() == 'c1692763d58a381e6e1249f494ef23b80f40689855b0261a7ebf25dbac0f7095', hashlib.sha256(b).hexdigest()
assert len(b) <= 4096, len(b)
open('build/us/KO_8P.BIN', 'wb').write(bytes(b) + bytes(4096 - len(b)))
print('KO_8P.BIN: block %d -> 4096 (2 sectors)' % len(b))
PYEOF
# Blob: MenuDrawKoHook at KO_BLOB_ADDR, linked against this build's
# DRA (same pipeline as build_sysopt.sh). Fails on .bss/.data (scratch is a
# fixed A-gap address, never cleared) or text over its 512B slot.
BLOBOUT=build/us/mod/kobob
mkdir -p "$BLOBOUT"
BLOB_ADDR=$(grep -hoE "#define KO_BLOB_ADDR 0x[0-9A-Fa-f]+" src/dra/dra.h | awk '{print $3}')
BLOB_SLOT=$(grep -hoE "#define KO_BLOB_SLOT [0-9]+" src/dra/dra.h | awk '{print $3}')
VERSION=us mipsel-linux-gnu-cpp -Iinclude -Iinclude/psxsdk -Isrc/dra -lang-c \
    -undef -Wall -fno-builtin -Dmips -D__GNUC__=2 -D__OPTIMIZE__ -D__mips__ -D__mips \
    -Dpsx -D__psx__ -D__psx -D_PSYQ -D__EXTENSIONS__ -D_MIPSEL -D_LANGUAGE_C \
    -DLANGUAGE_C -DNO_LOGS -DHACKS -DUSE_INCLUDE_ASM -D_internal_version_us -DSOTN_STR \
    -DMOD_KO -E mods/sotn-ultimate/src/menu/ko_blob.c | tools/sotn_str/target/release/sotn_str process |
    iconv --from-code=UTF-8 --to-code=Shift-JIS |
    bin/cc1-psx-26 -G0 -w -funsigned-char -fpeephole -ffunction-cse \
        -fpcc-struct-return -fcommon -fverbose-asm -msoft-float -g -quiet \
        -mcpu=3000 -fgnu-linker -mgas -gcoff -O2 |
    python3 tools/maspsx/maspsx.py --expand-div --aspsx-version=2.34 |
    mipsel-linux-gnu-as -Iinclude -march=r3000 -mtune=r3000 -no-pad-sections \
        -O1 -G0 -o "$BLOBOUT/ko_blob.o"
: > "$BLOBOUT/syms.txt"
for s in func_80106A28 MenuDrawImg LoadTPage g_Timer g_CurrentBuffer g_GpuUsage g_Vram SetDrawMode AddPrim; do
    a=$(mipsel-linux-gnu-nm build/us/dra.elf | awk -v s="$s" '$3==s{print $1}')
    if [ -z "$a" ]; then # (a main SDK function: config/symbols.us.txt)
        a=$(awk -v s="$s" '$1==s{sub(/;/,"",$3); sub(/^0x/,"",$3); print $3}' config/symbols.us.txt)
    fi
    if [ -z "$a" ] || [ "$(echo "$a" | wc -l)" != 1 ]; then
        echo "FAIL ko_blob: $s not in dra.elf (or not once)"; exit 1
    fi
    echo "$s = 0x$a;" >> "$BLOBOUT/syms.txt"
done
cat > "$BLOBOUT/ko_blob.ld" <<EOF
SECTIONS {
    . = $BLOB_ADDR;
    .text : { *(.text) }
    .rodata : { *(.rodata*) *(.rdata) }
    .data : { *(.data) *(.sdata) }
    .bss : { *(.bss) *(.sbss) *(COMMON) }
    /DISCARD/ : { *(.reginfo) *(.pdr) *(.mdebug*) *(.comment) *(.gnu*) *(.MIPS*) }
}
EOF
mipsel-linux-gnu-ld -nostdlib --no-check-sections -Map "$BLOBOUT/ko_blob.map" \
    -T "$BLOBOUT/ko_blob.ld" -T config/undefined_syms.us.txt -T "$BLOBOUT/syms.txt" \
    -o "$BLOBOUT/ko_blob.elf" "$BLOBOUT/ko_blob.o"
for sec in .bss .data .sdata .sbss; do
    z=$(mipsel-linux-gnu-objdump -h "$BLOBOUT/ko_blob.elf" | awk -v s="$sec" '$2==s{print $3}')
    if [ -n "$z" ] && [ $((16#$z)) -ne 0 ]; then
        echo "FAIL ko_blob: $sec is not empty (scratch must be absolute)"; exit 1
    fi
done
# .rodata holds the 5 status strings (34B) + sheet tables (52B); asserted below.
tsz=$(mipsel-linux-gnu-objdump -h "$BLOBOUT/ko_blob.elf" | awk '$2==".text"{print $3}')
if [ -z "$tsz" ] || [ $((16#$tsz)) -gt "$BLOB_SLOT" ]; then
    echo "FAIL ko_blob: text 0x$tsz over slot $BLOB_SLOT"; exit 1
fi
hook=$(mipsel-linux-gnu-nm "$BLOBOUT/ko_blob.elf" | awk '$3=="MenuDrawKoHook"{print $1}')
want_hook=$(echo "$BLOB_ADDR" | sed 's/^0x//' | tr A-F a-f)
if [ "$hook" != "$want_hook" ]; then
    echo "FAIL ko_blob: MenuDrawKoHook at $hook, entry is $BLOB_ADDR"; exit 1
fi
mipsel-linux-gnu-objcopy -O binary -j .text "$BLOBOUT/ko_blob.elf" "$BLOBOUT/ko_blob_text.bin"
mipsel-linux-gnu-objcopy -O binary -j .rodata "$BLOBOUT/ko_blob.elf" "$BLOBOUT/ko_blob_ro.bin"
echo "ko_blob: text 0x$tsz / slot $BLOB_SLOT, rodata $(stat -c %s "$BLOBOUT/ko_blob_ro.bin")B, hook at $BLOB_ADDR"
# ko_equip_jp: equipment-name tail, 4 entries + core + menu-open
# loader. DRA calls the entries with absolute macros (same cost as ordinary
# calls); DRA keeps no KO code/BSS. 5 TUs -> one link with fixed section
# addresses. Links against this build's DRA (one direction).
EQOUT=build/us/mod/koequip
mkdir -p "$EQOUT"
EQ_SEL=$(grep -hoE "#define KO_EQUIP_SEL 0x[0-9A-Fa-f]+" src/dra/dra.h | awk '{print $3}')
EQ_OV=$(grep -hoE "#define KO_EQUIP_OV 0x[0-9A-Fa-f]+" src/dra/dra.h | awk '{print $3}')
EQ_ENTER=$(grep -hoE "#define KO_EQUIP_ENTER 0x[0-9A-Fa-f]+" src/dra/dra.h | awk '{print $3}')
EQ_CORE=$(grep -hoE "#define KO_EQUIP_CORE 0x[0-9A-Fa-f]+" src/dra/dra.h | awk '{print $3}')
EQ_LOAD=$(grep -hoE "#define KO_EQUIP_LOAD 0x[0-9A-Fa-f]+" src/dra/dra.h | awk '{print $3}')
EQ_SIZE=$(grep -hoE "#define KO_EQUIP_SIZE 0x[0-9A-Fa-f]+" src/dra/dra.h | awk '{print $3}')
for t in eq_sel eq_ov eq_enter eq_core eq_pop eq_popt; do
VERSION=us mipsel-linux-gnu-cpp -Iinclude -Iinclude/psxsdk -Isrc/dra -lang-c \
    -undef -Wall -fno-builtin -Dmips -D__GNUC__=2 -D__OPTIMIZE__ -D__mips__ -D__mips \
    -Dpsx -D__psx__ -D__psx -D_PSYQ -D__EXTENSIONS__ -D_MIPSEL -D_LANGUAGE_C \
    -DLANGUAGE_C -DNO_LOGS -DHACKS -DUSE_INCLUDE_ASM -D_internal_version_us -DSOTN_STR \
    -DMOD_KO -DMOD_KO_EQUIP -E mods/sotn-ultimate/src/menu/$t.c | tools/sotn_str/target/release/sotn_str process |
    iconv --from-code=UTF-8 --to-code=Shift-JIS |
    bin/cc1-psx-26 -G0 -w -funsigned-char -fpeephole -ffunction-cse \
        -fpcc-struct-return -fcommon -fverbose-asm -msoft-float -g -quiet \
        -mcpu=3000 -fgnu-linker -mgas -gcoff -O2 |
    python3 tools/maspsx/maspsx.py --expand-div --aspsx-version=2.34 |
    mipsel-linux-gnu-as -Iinclude -march=r3000 -mtune=r3000 -no-pad-sections \
        -O1 -G0 -o "$EQOUT/$t.o"
done
VERSION=us mipsel-linux-gnu-cpp -Iinclude -Iinclude/psxsdk -Isrc/dra -lang-c \
    -undef -Wall -fno-builtin -Dmips -D__GNUC__=2 -D__OPTIMIZE__ -D__mips__ -D__mips \
    -Dpsx -D__psx__ -D__psx -D_PSYQ -D__EXTENSIONS__ -D_MIPSEL -D_LANGUAGE_C \
    -DLANGUAGE_C -DNO_LOGS -DHACKS -DUSE_INCLUDE_ASM -D_internal_version_us -DSOTN_STR \
    -DMOD_KO -DMOD_KO_EQUIP -E mods/sotn-ultimate/src/menu/ko_equip_jp_load.c | tools/sotn_str/target/release/sotn_str process |
    iconv --from-code=UTF-8 --to-code=Shift-JIS |
    bin/cc1-psx-26 -G0 -w -funsigned-char -fpeephole -ffunction-cse \
        -fpcc-struct-return -fcommon -fverbose-asm -msoft-float -g -quiet \
        -mcpu=3000 -fgnu-linker -mgas -gcoff -O2 |
    python3 tools/maspsx/maspsx.py --expand-div --aspsx-version=2.34 |
    mipsel-linux-gnu-as -Iinclude -march=r3000 -mtune=r3000 -no-pad-sections \
        -O1 -G0 -o "$EQOUT/eq_load.o"
: > "$EQOUT/syms.txt"
for sm in func_800F98AC func_800F66BC MenuDrawImg LoadTPage DrawSync func_80106A28 AllocPrimitives g_PrimBuf g_unkGraphicsStruct CdIntToPos CdControl CdSync CdGetSector; do
    a=$(mipsel-linux-gnu-nm build/us/dra.elf | awk -v s="$sm" '$3==s{print $1}')
    if [ -z "$a" ] || [ "$(echo "$a" | wc -l)" != "1" ]; then
        echo "FAIL ko_equip_jp: $sm not in dra.elf (or not once)"; exit 1
    fi
    echo "$sm = 0x$a;" >> "$EQOUT/syms.txt"
done
cat > "$EQOUT/ko_equip.ld" <<EOF
SECTIONS {
    . = $EQ_SEL; .eq_sel : { $EQOUT/eq_sel.o(.text) }
    . = $EQ_OV; .eq_ov : { $EQOUT/eq_ov.o(.text) }
    . = $EQ_ENTER; .eq_enter : { $EQOUT/eq_enter.o(.text) }
    . = $EQ_CORE; .eq_core : { $EQOUT/eq_core.o(.text) }
    . = 0x8009F280; .eq_pop : { $EQOUT/eq_pop.o(.text) }
    . = 0x8009E640; .eq_popt : { $EQOUT/eq_popt.o(.text) }
    . = $EQ_LOAD; .eq_load : { $EQOUT/eq_load.o(.text) }
    .rodata : { *(.rodata*) *(.rdata) }
    .data : { *(.data) *(.sdata) }
    .bss : { *(.bss) *(.sbss) *(COMMON) }
    /DISCARD/ : { *(.reginfo) *(.pdr) *(.mdebug*) *(.comment) *(.gnu*) *(.MIPS*) }
}
EOF
mipsel-linux-gnu-ld -nostdlib --no-check-sections -Map "$EQOUT/ko_equip.map" \
    -T "$EQOUT/ko_equip.ld" -T "$EQOUT/syms.txt" \
    -o "$EQOUT/ko_equip.elf"
for sec in .bss .data .sdata .sbss; do
    z=$(mipsel-linux-gnu-objdump -h "$EQOUT/ko_equip.elf" | awk -v s="$sec" '$2==s{print $3}')
    if [ -n "$z" ] && [ $((16#$z)) -ne 0 ]; then
        echo "FAIL ko_equip_jp: $sec is not empty"; exit 1
    fi
done
if mipsel-linux-gnu-objdump -h "$EQOUT/ko_equip.elf" | awk '$2 ~ /rodata|rdata/ && $3 != "00000000" {found=1} END {exit !found}'; then
    echo "FAIL ko_equip_jp: rodata is not empty (tables must be computed)"; exit 1
fi
for pair in "KO_EqSelDraw $EQ_SEL .eq_sel 0x40" "KO_EqOvDraw $EQ_OV .eq_ov 0x40" "KO_EqOnMenuEnter $EQ_ENTER .eq_enter 0x80" "KO_EquipJpLoad $EQ_LOAD .eq_load 0xC4"; do
    set -- $pair
    a=$(mipsel-linux-gnu-nm "$EQOUT/ko_equip.elf" | awk -v s="$1" '$3==s{print $1}')
    want=$(echo "$2" | sed 's/^0x//' | tr A-F a-f)
    if [ "$a" != "$want" ]; then
        echo "FAIL ko_equip_jp: $1 at $a, want $2"; exit 1
    fi
    sz=$(mipsel-linux-gnu-objdump -h "$EQOUT/ko_equip.elf" | awk -v s="$3" '$2==s{print $3}')
    if [ -z "$sz" ] || [ $((16#$sz)) -gt "$(( $4 ))" ]; then
        echo "FAIL ko_equip_jp: $3 size 0x$sz over slot $4"; exit 1
    fi
done
coresz=$(mipsel-linux-gnu-objdump -h "$EQOUT/ko_equip.elf" | awk '$2==".eq_core"{print $3}')
corevma=$(mipsel-linux-gnu-objdump -h "$EQOUT/ko_equip.elf" | awk '$2==".eq_core"{print $4}')
if [ -z "$coresz" ] || [ $((16#$coresz)) -gt "$((0xA00))" ]; then
    echo "FAIL ko_equip_jp: .eq_core size 0x$coresz over slot 0xA00"; exit 1
fi
if [ $((16#$corevma + 16#$coresz)) -gt "$((0x8009FF40))" ]; then
    echo "FAIL ko_equip_jp: core end over KO_EQUIP_LOAD"; exit 1
fi
loadsz=$(mipsel-linux-gnu-objdump -h "$EQOUT/ko_equip.elf" | awk '$2==".eq_load"{print $3}')
loadvma=$(mipsel-linux-gnu-objdump -h "$EQOUT/ko_equip.elf" | awk '$2==".eq_load"{print $4}')
if [ $((16#$loadvma + 16#$loadsz)) -gt "$((0x800A0000))" ]; then
    echo "FAIL ko_equip_jp: load end over 0x800A0000"; exit 1
fi
# Popup Hangul glyph uploader: entry at KO_POP_GLYPH, <= 0x80 B (ARC gap 0x8009F280..0x8009F300)
popsz=$(mipsel-linux-gnu-objdump -h "$EQOUT/ko_equip.elf" | awk '$2==".eq_pop"{print $3}')
popa=$(mipsel-linux-gnu-nm "$EQOUT/ko_equip.elf" | awk '$3=="KO_PopGlyph"{print $1}')
if [ -z "$popsz" ] || [ $((16#$popsz)) -gt 128 ] || [ "$popa" != "8009f280" ]; then
    echo "FAIL ko_equip_jp: KO_PopGlyph size 0x$popsz at $popa (want <= 0x80 at 8009f280)"; exit 1
fi
echo "ko_pop: KO_PopGlyph 0x$popsz B at $popa"
poptsz=$(mipsel-linux-gnu-objdump -h "$EQOUT/ko_equip.elf" | awk '$2==".eq_popt"{print $3}')
popta=$(mipsel-linux-gnu-nm "$EQOUT/ko_equip.elf" | awk '$3=="KO_PopupText"{print $1}')
if [ -z "$poptsz" ] || [ $((16#$poptsz)) -gt 1200 ] || [ "$popta" != "8009e640" ]; then
    echo "FAIL ko_equip_jp: KO_PopupText size 0x$poptsz at $popta (want <= 0x4B0 at 8009e640)"; exit 1
fi
echo "ko_pop: KO_PopupText 0x$poptsz B at $popta"
for sec in eq_sel eq_ov eq_enter eq_core eq_load eq_pop eq_popt; do
    mipsel-linux-gnu-objcopy -O binary --only-section=.$sec "$EQOUT/ko_equip.elf" "$EQOUT/$sec.bin"
done
echo "ko_equip_jp: sel/ov/enter at $EQ_SEL/$EQ_OV/$EQ_ENTER, core $corevma size 0x$coresz, load $loadvma size 0x$loadsz"
# ARC_KO.BIN: v11 glyphs (sha-pinned) + blob slot + ko_equip tail,
# sector-padded.
# ARC_F2's old 178-bake region is zeroed (same length, no LBA moves).
# v12e (v12c Korean restored in F12A..F28A, English 32 moved
# to free-code cells) + engtab250. Same 776x30 size, ARC offsets unchanged.
# v14g (gen_ko_glyph_add.py): v14f + 1 Hangul cell F3F7 (켬, the Extra settings popup; 952 cells). v14f (gen_ko_glyph_f.py): v14e + 1 Hangul cell F3F6 (951 in all, limit 960); v14e (gen_ko_glyph_e.py): v14d + 91 more cells (950); KO_SHA = the block's sha256 (check_ko_glyph.py prints it).
export KO_BLK=${KO_WK}/P4시험/ko_glyph_block_mgalmuri12_v14g.bin
export KO_SHA=e8013d637b19c70a23d646e5cb66b988a9ea3e5fe74f166e756bc07f485f5f90
# name tag / popup advance table lookup (src/st/ko_font_params.h) is per overlay. KO_NAME_FIXED_OVERLAYS (switches, one
# `#define KO_NAME_FIXED 1` as first line of the files): sel(cutscene.c) no3(cutscene.c, popup.c) no4(cutscene.c, popup.c) mad(popup.c: MAD.BIN must keep its retail size, check_status_shift).
# If a later build reports `FAIL stXXX: overlay ends ... past 0x801e0000`, add the define to that overlay's
# cutscene.c / popup.c (fixed 6 px, space 3) and rebuild; if one of the three has margin, delete its define.
export KO_FONT8_ADV=${KO_WK}/P4시험/ko_font8_adv.bin
export KO_SHEET2=${KO_WK}/P4시험/ko_sheet2.bin
export KO_PROMO=${KO_WK}/P4시험/ko_promo_tbl.bin
export KO_ENGTAB=${KO_WK}/P4시험/ko_engtab_253.bin # v14d: 252 + 40 ASCII entries (existing entries unchanged)
export KO_FIELD=${KO_WK}/P4시험/ko_field.bin
export KO_LIBDESC=${KO_WK}/P4시험/ko_libdesc.bin # 0.9.2: Saturn bestiary KO rows
python3 mods/sotn-ultimate/tools/check_ko_glyph.py || { echo 'check_ko_glyph FAILED' >&2; exit 1; }
python3 - <<'PYEOF'
import hashlib, io, os, re
dra_h = io.open('src/dra/dra.h', encoding='utf-8').read()
def define(name):
    m = re.search(r"#define %s (0x[0-9A-Fa-f]+|\d+)" % name, dra_h)
    assert m, name
    return int(m.group(1), 0)
assert define('KO_GLYPH_BASE') == 0xF040 and 819 <= define('KO_GLYPH_COUNT') <= 960, define('KO_GLYPH_COUNT')  # v14d: 819 + new ASCII cells
slot, arc_size = define('KO_BLOB_SLOT'), define('KO_ARC_SIZE')
eq_base, eq_size = define('KO_EQUIP_BASE'), define('KO_EQUIP_SIZE')
eq_slots = {'sel': (define('KO_EQUIP_SEL'), 0x40),
            'ov': (define('KO_EQUIP_OV'), 0x40),
            'enter': (define('KO_EQUIP_ENTER'), 0x80),
            'core': (define('KO_EQUIP_CORE'), 0xA00)}
eq_load = define('KO_EQUIP_LOAD')
raw = open(os.environ['KO_BLK'], 'rb').read()
assert len(raw) == define('KO_GLYPH_COUNT') * 23 <= 0x5640, len(raw)  # v14d (v14c 819 + ASCII cells): MGalmuri12 12x15 23 B cells, must end before KO_POP_TEXT (0x5640)
assert hashlib.sha256(raw).hexdigest() == os.environ['KO_SHA'], hashlib.sha256(raw).hexdigest()
assert define('KO_GLYPH_CELL_BYTES') == 23
# the extension cells (776..799) live in the dead blob slot at 0x8009EAF0 (896 B)
assert 0x8009EAF0 - 0x80099000 == 23280 and len(raw) - 23280 <= slot
ext_cells = raw[23280:]  # empty: 23 B cells fit the block
raw = raw[:23280].ljust(23280, bytes(1))
# Resident KO_PopupText in the glyph block slot behind the last possible cell (960 x 23 B = 0x5640)
popt = open('build/us/mod/koequip/eq_popt.bin', 'rb').read()
pt_off = define('KO_POP_TEXT') - 0x80099000
assert pt_off == 0x5640 and 0 < len(popt) <= 0x4B0 and pt_off + 0x4B0 <= 0x5AF0, (hex(pt_off), len(popt))
assert len(open(os.environ['KO_BLK'], 'rb').read()) <= pt_off, 'glyph cells overlap KO_PopupText'
assert raw[pt_off:pt_off + 0x4B0] == bytes(0x4B0)
raw = raw[:pt_off] + popt + raw[pt_off + len(popt):]
# cutscene 1-byte promotion table (gen_ko_promo.py): ARC zero gap behind the engtab
promo = open(os.environ['KO_PROMO'], 'rb').read()
assert len(promo) == define('KO_PROMO_TBL_SLOTS') * 2 == 224, len(promo)
promo_off = define('KO_PROMO_TBL_ADDR') - 0x80099000
assert promo_off == 0x6000 and promo_off >= 24320 + 188 and promo_off + 224 <= 0x8009F500 - 0x80099000
# KO_ENGTAB (94xu16 ASCII->v12d code, 0=miss) in ARC gap at file+24320
# (RAM 0x8009EF00). ARC_KO.BIN size unchanged (28672). KO_FLAG(24308) set later.
engtab = open(os.environ['KO_ENGTAB'], 'rb').read()
assert len(engtab) == 94 * 2 == 188, len(engtab)
ENGTAB_OFF = 0x8009EF00 - 0x80099000
assert ENGTAB_OFF == 24320, ENGTAB_OFF
# the dead MenuDrawKoHook blob text is replaced by the extension glyph cells
# 0.9.2: the Saturn bestiary KO descriptions (17 rows) live in this slot (RAM 0x8009EAF0, KO_LIBDESC_TBL);
# the cells above 1012 would overwrite it, so the extension must stay empty
libdesc = open(os.environ['KO_LIBDESC'], 'rb').read()
assert len(libdesc) <= 896, len(libdesc)
assert len(ext_cells) == 0, len(ext_cells)
blob = libdesc + bytes(slot - len(libdesc))
assert len(blob) == slot, (len(blob), slot)
ro = open('build/us/mod/kobob/ko_blob_ro.bin', 'rb').read()
exp = bytes.fromhex('f080f05eff00 f046f067ff00 f052f141f046ff00 f1abf081ff00 f0abf1ccf052ff00'.replace(' ', ''))
# Hand labels appended (upload +오/른/손/왼 cells 14-17, indices 0e0f10ff 오른손 + 1110ff 왼손)
sheet = bytes.fromhex('f080f05ef046f067f052f141f1ab f081f0abf1ccf0a9f0b0f0caf071f077f0c5f066f2d2 00 0001ff0203ff040502ff0607ff 080904ff0a0bff0c0dff101112ff1312ff'.replace(' ', ''))
# Measured: KO_BOXALL 34B (incl NUL) + 2B linker pad + KO_SHEET_ALL
# 67B + 1B compiler NUL = 104B. Sheet starts at +36 (KO_STR_BASE+36).
assert ro[:34] == exp and ro[34:36] == bytes(2) and ro[36:103] == sheet and ro[103:] == bytes(1) and len(ro) == 104, (ro.hex(), len(ro))
ro = ro + bytes(104 - len(ro))
assert arc_size == 43008, arc_size
assert eq_base == 0x8009F500 and eq_size == 0xB00, (hex(eq_base), hex(eq_size))
assert len(raw) + len(blob) + len(ro) == 24280, (len(raw), len(blob), len(ro))
gap_end = eq_base - 0x80099000  # 25856: tail base offset in ARC (RAM 0x8009F500)
gap_pre = bytes(ENGTAB_OFF - (len(raw) + len(blob) + len(ro)))
assert len(gap_pre) == 40, len(gap_pre)
gap_post = bytes(gap_end - (ENGTAB_OFF + len(engtab)))
assert len(gap_post) == 1348, len(gap_post)
gap_post = bytearray(gap_post)
assert bytes(gap_post[promo_off - (ENGTAB_OFF + len(engtab)):][:224]) == bytes(224)
gap_post[promo_off - (ENGTAB_OFF + len(engtab)):promo_off - (ENGTAB_OFF + len(engtab)) + 224] = promo
# big-label sheet tables (gen_ko_sheet2.py): ARC offset 0x6100 = 0x8009F100
sheet2 = open(os.environ['KO_SHEET2'], 'rb').read()
sh_off = define('KO_SHEET2_ADDR') - 0x80099000
assert sh_off == 0x6100 and sh_off >= 0x6000 + 224 and sh_off + len(sheet2) <= 0x8009F500 - 0x80099000, (hex(sh_off), len(sheet2))
s2 = sh_off - (ENGTAB_OFF + len(engtab))
assert bytes(gap_post[s2:s2 + len(sheet2)]) == bytes(len(sheet2))
gap_post[s2:s2 + len(sheet2)] = sheet2
# Per-cell advance table of the re-baked 8x8 sheet (gen_ko_font8.py), ARC offset 0x6200 = 0x8009F200
f8adv = open(os.environ['KO_FONT8_ADV'], 'rb').read()
a_off = define('KO_FONT8_ADV_ADDR') - 0x80099000
assert a_off == 0x6200 and len(f8adv) == 96 and a_off >= sh_off + len(sheet2), (hex(a_off), len(f8adv))
s3 = a_off - (ENGTAB_OFF + len(engtab))
assert bytes(gap_post[s3:s3 + 96]) == bytes(96)
gap_post[s3:s3 + 96] = f8adv
# KO_PopGlyph (popup Hangul uploader) at ARC offset 0x6280 = 0x8009F280
pop = open('build/us/mod/koequip/eq_pop.bin', 'rb').read()
p_off = define('KO_POP_GLYPH') - 0x80099000
assert p_off == 0x6280 and 0 < len(pop) <= 128 and p_off >= a_off + 96, (hex(p_off), len(pop))
s4 = p_off - (ENGTAB_OFF + len(engtab))
assert bytes(gap_post[s4:s4 + 128]) == bytes(128)
gap_post[s4:s4 + len(pop)] = pop
# 56: stage message-box labels (gen_ko_fieldmsg.py), 4 slots x 0x80 at 0x8009F300
fld = open(os.environ['KO_FIELD'], 'rb').read()
f_off = define('KO_FIELD_BASE') - 0x80099000
assert f_off == 0x6300 and len(fld) == 0x200 and f_off >= p_off + 128 and f_off + 0x200 <= 0x8009F500 - 0x80099000, (hex(f_off), len(fld))
s5 = f_off - (ENGTAB_OFF + len(engtab))
assert bytes(gap_post[s5:s5 + 0x200]) == bytes(0x200)
gap_post[s5:s5 + 0x200] = fld
gap_post = bytes(gap_post)
gap = gap_pre + engtab + gap_post
assert len(gap) == gap_end - (len(raw) + len(blob) + len(ro)), len(gap)
# ko_equip_jp: tail = sel+ov+enter+core+load, zero-padded to slots.
# LOAD is 0x8009FF40 per dra.h + link + e_shop/w_021 literals
# (0x8009FF3C was a stale pre-link value; the link already placed it at FF40).
assert (eq_slots['sel'][0], eq_slots['ov'][0], eq_slots['enter'][0], eq_slots['core'][0], eq_load) == (0x8009F500, 0x8009F540, 0x8009F580, 0x8009F600, 0x8009FF40)
parts = {}
for nm in ['sel', 'ov', 'enter', 'core', 'load']:
    parts[nm] = open('build/us/mod/koequip/eq_%s.bin' % nm, 'rb').read()
img = bytearray(raw + blob + ro + gap)
assert len(img) == gap_end, len(img)
# Place each part at its LINKED offset (tail base 0x8009F500).
# Concatenating full slots overflows: core slot 0xA00 already reaches 0x800A0000,
# the loader lives at 0x8009FF40 (offset 0xA40, 0xC0 slot per dra.h).
tail = bytearray(eq_size)
assert len(parts['sel']) <= 0x40 and len(parts['ov']) <= 0x40 and len(parts['enter']) <= 0x80, (len(parts['sel']), len(parts['ov']), len(parts['enter']))
assert len(parts['core']) <= 0xA00 and 0x100 + len(parts['core']) <= 0xA40, len(parts['core'])
assert len(parts['load']) <= 0xC0, len(parts['load'])
for nm, off in [('sel', 0x0), ('ov', 0x40), ('enter', 0x80), ('core', 0x100), ('load', 0xA40)]:
    tail[off:off+len(parts[nm])] = parts[nm]
tail = bytes(tail)
assert len(tail) == eq_size, (len(tail), hex(eq_size))
img = img + tail
# JP bundle (121 rows, 1669B -> 1 sector) + 8px block
# (283 cells, 2832B -> 2 sectors) ride the ARC_KO boot chain (cd.c blocks
# 14/15-16 into 0x801F0000/0x801F2000). Same bytes as the retired menu-time
# files (KO_JP.BIN at disc end, KO_8P.BIN splice at DUMMY 0x18).
KO_BUNDLE = 'src/dra/gen/us/ko_jp_bundle.bin'
bundle = open(KO_BUNDLE, 'rb').read()
assert 1 <= len(bundle) <= 2048, len(bundle)
KO_8PBLK = __import__('os').environ['KO_WK'] + '/P4시험/ko_8p_block_249.bin'
blk8 = open(KO_8PBLK, 'rb').read()
assert hashlib.sha256(blk8).hexdigest() == 'c1692763d58a381e6e1249f494ef23b80f40689855b0261a7ebf25dbac0f7095', hashlib.sha256(blk8).hexdigest()
assert len(blk8) <= 4096, len(blk8)
img = img + bytes(bundle) + bytes(2048 - len(bundle)) + bytes(blk8) + bytes(4096 - len(blk8))
# 099b: blocks 17-18 = the new resident area's first two sectors (cd.c's copy puts them at 0x801F3000 / 0x801F3800: KO_8P_BASE + 0x1000); header + code, zero padded
newarea = open('build/us/mod/newarea/newarea.bin', 'rb').read()
assert 32 < len(newarea) <= 8192 and newarea[:4] == bytes.fromhex('31524f4b'), len(newarea) # 100: four sectors (blocks 17-20 -> 0x801F3000..0x801F4FFF)
img = img + newarea + bytes(8192 - len(newarea))
assert len(img) == arc_size, (len(img), arc_size)
# 094_12 Sound Mode text (gen_ko_sound.py): strings live in the A-gap zero runs (resident from the title on), SEL only holds
# their addresses (SEL has no room). Every run must still be zero (nothing else uses it).
import json as _js
_sj = 'src/dra/gen/us/ko_sound_arc.json'
if os.path.exists(_sj):
    _done = []  # byte ranges already patched in this loop: text, never a branch (a run can hold several strings back to back)
    for _a, _h in _js.load(open(_sj))['patches']:
        _ad = int(_a, 16); _b = bytes.fromhex(_h); _o = _ad - 0x80099000
        assert 0 <= _o and _o + len(_b) <= 0x7000, ('sound text outside the A-gap', _a)
        assert not any(img[_o:_o + len(_b)]), ('A-gap run is not zero', _a)
        # 094_16: a zero is not always free. 094_14 put text on the delay slot (a 0 = nop) of a `jr ra` of the equipment tail code
        # and the pause menu went black. So: a run must not start at the word that follows a jump/branch, and a run that starts
        # unaligned must follow a NUL (the terminator of a label / string that precedes it).
        _w = _o & ~3
        if _w >= 4 and not any(_s < _w and _w - 4 < _e for _s, _e in _done):
            _pw = __import__("struct").unpack('<I', bytes(img[_w - 4:_w]))[0]
            _op = _pw >> 26
            assert not (_op in (1, 2, 3, 4, 5, 6, 7, 0x14, 0x15, 0x16, 0x17) or (_op == 0 and (_pw & 0x3F) in (8, 9))), \
                ('A-gap run starts in a branch delay slot', _a, hex(_pw))
        # label slots (KO_FIELD_BASE 0x8009F300..F4FF): the run must follow the NUL of the label that precedes it (F414 / F4B0 are
        # 4-aligned, so the alignment test alone missed them); an unaligned run anywhere must follow a NUL as well
        assert not (0x8009F300 < _ad < 0x8009F500 and img[_o - 1] != 0), ('A-gap run in a label slot does not follow the label NUL', _a)
        assert _o % 4 == 0 or img[_o - 1] == 0, ('unaligned A-gap run does not follow a NUL', _a)
        # no run may overlap the real code length of a ko_equip slot (a loader that is ON, e.g. the JP bundle, makes the part longer)
        for _nm, _sb in (('sel', eq_slots['sel'][0]), ('ov', eq_slots['ov'][0]), ('enter', eq_slots['enter'][0]), ('core', eq_slots['core'][0]), ('load', eq_load)):
            _ce = _sb + len(parts[_nm])  # a part is the linked section as is: it already holds the delay slot nop of its last jr ra (JP ON: core 0x8FC, load 8); a run right after a branch is guard (1)'s job
            assert not (_ad < _ce and _ad + len(_b) > _sb), ('A-gap run overlaps the ko_equip code', _nm, _a, hex(_sb), hex(_ce))
        img[_o:_o + len(_b)] = _b
        _done.append((_o, _o + len(_b)))
    print('ARC_KO.BIN: sound mode text placed in the A-gap zero runs')
img[24308] = 1  # KO_FLAG_ADDR default on (10q-dbg layout)
open('build/us/ARC_KO.BIN', 'wb').write(bytes(img))
print('ARC_KO.BIN: glyphs %d + blob slot %d + strings 104 + gap %d + equip %d+%d+%d+%d+%d + bundle %d + 8p %d = %d' % (len(raw), slot, gap_end - 24280, len(parts['sel']), len(parts['ov']), len(parts['enter']), len(parts['core']), len(parts['load']), len(bundle), len(blk8), arc_size))
arc_len = None
gm = io.open('mods/sotn-ultimate/include/game_mod.h', encoding='utf-8').read()
arc_len = int(re.search(r"#define MOD_ARC_F_LEN (0x[0-9A-Fa-f]+)", gm).group(1), 0)
sysopt_addr = int(re.search(r"#define MOD_SYSOPT_ADDR (0x[0-9A-Fa-f]+)", gm).group(1), 0)
sysopt = open('build/us/mod/sysopt/sysopt.bin', 'rb').read()
off = (sysopt_addr - 0x8013C000) + len(sysopt)
p = 'build/us/ARC_F2.BIN'
b = bytearray(open(p, 'rb').read())
# v1.5 port: build_charmenu.sh appends the Richter/Maria pause-menu code (24576 B) after MOD_ARC_F_LEN; the part zeroed below is inside the first part
assert len(b) in (arc_len, arc_len + 24576), len(b)
end = min(off + 5340, arc_len - 16)  # v1.5: the pause-menu blob follows arc_len and the holder table is the last 16 bytes: the zeroing must not reach either
b[off:end] = bytes(end - off)
open(p, 'wb').write(bytes(b))
print('ARC_F2: old 178-bake zeroed at file+%d, len still %d' % (off, len(b)))
PYEOF
# ARC_KO.BIN (34816B = +17 sectors) would push every post-SLUS file +17
# (check_cd_lba measured uniform +0xC), so: SLUS_000.67 goes last on the disc
# (boot is name-based via SYSTEM.CNF) and a 275-sector dummy keeps slot #2,
# leaving every other file at its earlier LBA. No literal changes needed.
python3 - <<'PYEOF'
import struct
exe, arc = 'build/us/main.exe', 'build/us/ARC_KO.BIN'
blob = open(arc, 'rb').read()
assert len(blob) == 43008, (arc, len(blob))
blob34 = blob[:34816]  # 099: the 17-sector part (the new area's sector is appended last)
b = bytearray(open(exe, 'rb').read())
tsize = struct.unpack('<I', b[0x1C:0x20])[0]
if len(b) == 563200 and tsize == 0x89000:
    b = b + blob34
    struct.pack_into('<I', b, 0x1C, 0x91800)
    open(exe, 'wb').write(bytes(b))
    print('main.exe: 598016, t_size 0x91800')
elif len(b) == 587776 and tsize == 0x8F000 and bytes(b[-24576:]) == bytes(blob[:24576]):
    b = b[:-24576] + blob34
    struct.pack_into('<I', b, 0x1C, 0x91800)
    open(exe, 'wb').write(bytes(b))
    print('main.exe: re-extended 587776->598016, t_size 0x91800')
elif len(b) == 591872 and tsize == 0x90000:
    # 14-sector tail (any bytes: tail TUs changed) replaced
    # by the 17-sector blob.
    b = b[:-28672] + blob34
    struct.pack_into('<I', b, 0x1C, 0x91800)
    open(exe, 'wb').write(bytes(b))
    print('main.exe: 14-sector tail replaced (591872->598016, t_size 0x91800)')
elif len(b) == 598016 and tsize == 0x91800 and bytes(b[-34816:]) == blob34:
    print('main.exe: 17-sector tail already in (598016, t_size 0x91800)')
elif len(b) == 598016 and tsize == 0x91800:
    b = b[:-34816] + blob34
    open(exe, 'wb').write(bytes(b))
    print('main.exe: stale tail refreshed (598016, t_size 0x91800)')
elif len(b) == 606208 and tsize == 0x93800:
    # 099 rerun (the build runs twice when the disc geometry moved): fold the tail back to the 17-sector geometry, the 099 stage below grows it again
    b = b[:-43008] + blob34
    struct.pack_into('<I', b, 0x1C, 0x91800)
    open(exe, 'wb').write(bytes(b))
    print('main.exe: 099 rerun, folded back to 598016 / t_size 0x91800')
else:
    assert False, (len(b), hex(tsize))
# 100: the tail grows by four sectors (the new resident area's): 598016 -> 606208, t_size 0x91800 -> 0x93800
b = bytearray(open(exe, 'rb').read())
tsize = struct.unpack('<I', b[0x1C:0x20])[0]
if len(b) == 598016 and tsize == 0x91800:
    b = b[:-34816] + blob
    struct.pack_into('<I', b, 0x1C, 0x93800)
    open(exe, 'wb').write(bytes(b))
    print('main.exe: 100 tail +4 sectors (606208, t_size 0x93800)')
elif len(b) == 606208 and tsize == 0x93800:
    b = b[:-43008] + blob
    open(exe, 'wb').write(bytes(b))
    print('main.exe: 100 tail refreshed (606208, t_size 0x93800)')
else:
    assert False, ('099 tail', len(b), hex(tsize))
open('build/us/DUMMY.BIN', 'wb').write(bytes(563200))  # 275 sectors = retail SLUS's size: it holds SLUS's old slot, so the files after it keep their LBA (SLUS itself is last now, any size; 099's +1 sector there moves nothing)
mf = 'config/disk.us.lba'
lines = open(mf).read().splitlines()
slus = [l for l in lines if l.startswith('SLUS_000.67;1')][0]
rest = [l for l in lines if l != slus and not l.startswith('DUMMY.BIN;1')]  # rerun-safe: exec skips EXIT-trap restore
si = next(i for i, l in enumerate(rest) if l.startswith('SYSTEM.CNF;1'))
rest.insert(si + 1, 'DUMMY.BIN;1,N,' + slus.split(',', 2)[2])
rest.append(slus)
open(mf, 'w').write('\n'.join(rest) + '\n')
print('manifest: DUMMY.BIN at slot #2, SLUS_000.67 last')
PYEOF
# 095_39: main.exe is retail again outside the KO tail: undo what the earlier builds (095_37 / 095_38) patched into it in place
python3 mods/sotn-ultimate/tools/patch_exe_r1.py --restore
# Richter in the coat in the endings: build/us/SELRIC2.BIN (SEL.BIN + the
# RIC2 art); made again below from the final SEL
(cd mods/sotn-ultimate/tools && python3 build_selric2.py)
quiet make disk
if [ "$(sha1sum include/lba_us.h)" != "$LBA_BEFORE" ]; then
    if [ -z "${MOD_BUILD_RERUN:-}" ]; then
        echo "include/lba_us.h changed (disc files moved): building again"
        MOD_BUILD_RERUN=1 exec bash "$0" "$@"
    fi
    echo "FAIL include/lba_us.h changed again on the second run" >&2
    exit 1
fi
# make disk relinks the overlays that include lba_us.h (SEL): patch them
# again and remake the image (BO6, copied into BO6RIC2 above, must not be
# among them: --check compares both)
if [ "$MOD_PSP_DUB" = 1 ]; then
    out=$(python3 mods/sotn-ultimate/tools/psp_script.py --apply;
          python3 mods/sotn-ultimate/tools/prologue_scroll.py --apply)
    echo "$out"
    if echo "$out" | grep -q "BIN"; then
        VERSION=us quiet ./sotn.sh disk
    fi
    python3 mods/sotn-ultimate/tools/psp_script.py --check
    python3 mods/sotn-ultimate/tools/prologue_scroll.py --check
fi
# SELRIC2 from the final SEL (relinked and patched above)
out=$(cd mods/sotn-ultimate/tools && python3 build_selric2.py)
echo "$out"
if echo "$out" | grep -q changed; then
    VERSION=us quiet ./sotn.sh disk
fi
# Raw-copied XA sectors keep retail addresses; fix them or music/voices break
python3 mods/sotn-ultimate/tools/fix_sector_headers.py build/sotn.us.bin
# cd.c's / SEL's literal sector numbers (not from lba_us.h) vs the image
python3 mods/sotn-ultimate/tools/check_cd_lba.py build/sotn.us.bin
# BGM fix: the disc's XA sectors (file/channel at XA_STR1+cd_addr+channel) must match the song table
python3 mods/sotn-ultimate/tools/check_xa_table.py image build/sotn.us.bin || { echo "FAIL check_xa_table image" >&2; exit 1; }

bash mods/sotn-ultimate/tools/check_dra_layout.sh
# MARIA2.BIN was linked against DRA symbols (build/us/mod/mar2/mod_syms.txt:
# g_ModSubwpnIcons, D_800A3B70, any moved one) before make disk relinked
# DRA: none may have moved
while read -r s _ a; do
    a=${a%;}
    want=$(mipsel-linux-gnu-nm build/us/dra.elf | awk -v s="$s" '$3==s{print "0x" $1}')
    if [ "$want" != "$a" ]; then
        echo "FAIL MARIA2 linked against $s $a (DRA has $want): run build_disc.sh again"
        exit 1
    fi
done < build/us/mod/mar2/mod_syms.txt
echo "OK   MARIA2 DRA symbols ($(wc -l < build/us/mod/mar2/mod_syms.txt))"
# 099: the same for the new resident area (r1_easy linked at 0x801F3010 against dra.elf symbols; the main exe's ones do not move)
while read -r s _ a; do
    a=${a%;}
    want=$(mipsel-linux-gnu-nm build/us/dra.elf | awk -v s="$s" '$3==s{print "0x" $1}')
    if [ -n "$want" ] && [ "$want" != "$a" ]; then
        echo "FAIL newarea linked against $s $a (DRA has $want): run the build again"
        exit 1
    fi
done < build/us/mod/newarea/syms.txt
echo "OK   newarea DRA symbols ($(wc -l < build/us/mod/newarea/syms.txt)), $(stat -c %s build/us/mod/newarea/newarea.bin) B of $((0x8000))"
# the same for the System screen options' code in ARC_F2.BIN
# (syms.txt also holds the cheats' derived values: ModHdEntry = HandleDamage, ModStairsA/B = func_80109990 + 0x2D2 / + 0x35E,
# and the two instruction words ModHdOrig0/1, which are not addresses: build_sysopt.sh already checked them against the built DRA)
while read -r s _ a; do
    a=${a%;}
    off=0; w=$s
    case "$s" in
        ModHdOrig0|ModHdOrig1) continue ;;
        ModHdEntry) w=HandleDamage ;;
        ModStairsA) w=func_80109990; off=$((0x2D2)) ;;
        ModStairsB) w=func_80109990; off=$((0x35E)) ;;
    esac
    want=$(mipsel-linux-gnu-nm build/us/dra.elf | awk -v s="$w" '$3==s{print "0x" $1}')
    if [ -z "$want" ]; then # (a main SDK function: config/symbols.us.txt)
        want=$(awk -v s="$w" '$1==s{sub(/;/,"",$3); print tolower($3)}' config/symbols.us.txt)
    fi
    if [ -n "$want" ]; then want=$(printf '0x%08x' $((want + off))); fi
    a=$(printf '0x%08x' $((a)))
    if [ "$want" != "$a" ]; then
        echo "FAIL sysopt linked against $s $a (DRA has $want): run build_disc.sh again"
        exit 1
    fi
done < build/us/mod/sysopt/syms.txt
echo "OK   sysopt DRA symbols ($(wc -l < build/us/mod/sysopt/syms.txt)), ARC_F2.BIN $(stat -c %s build/us/ARC_F2.BIN) bytes"
# v1.5 port: the same for the character menu's code (its address is g_Pix + 0x2000)
while read -r s _ a; do
    a=${a%;}
    want=$(mipsel-linux-gnu-nm build/us/dra.elf | awk -v s="$s" '$3==s{print "0x" $1}')
    if [ -z "$want" ]; then
        want=$(awk -v s="$s" '$1==s{sub(/;/,"",$3); print tolower($3)}' config/symbols.us.txt)
        a=$(echo "$a" | tr A-F a-f)
    fi
    if [ "$want" != "$a" ]; then
        echo "FAIL charmenu linked against $s $a (DRA has $want): run build_disc.sh again"
        exit 1
    fi
done < build/us/mod/charmenu/syms.txt
pix=$(printf "0x%08X" $((0x$(mipsel-linux-gnu-nm build/us/dra.elf | awk '$3=="g_Pix"{print $1}') + 0x2000)))
if [ "$pix" != "$(cat build/us/mod/charmenu/addr.txt)" ]; then
    echo "FAIL charmenu linked at $(cat build/us/mod/charmenu/addr.txt) (g_Pix[1] is $pix): run build_disc.sh again"
    exit 1
fi
echo "OK   charmenu DRA symbols ($(wc -l < build/us/mod/charmenu/syms.txt))"
# The same for the KO blob in ARC_KO.BIN (A-gap)
while read -r s _ a; do
    a=${a%;}
    want=$(mipsel-linux-gnu-nm build/us/dra.elf | awk -v s="$s" '$3==s{print "0x" $1}')
    if [ -z "$want" ]; then # (a main SDK function: config/symbols.us.txt)
        want=$(awk -v s="$s" '$1==s{sub(/;/,"",$3); print tolower($3)}' config/symbols.us.txt)
        a=$(echo "$a" | tr A-F a-f)
    fi
    if [ "$want" != "$a" ]; then
        echo "FAIL ko_blob linked against $s $a (DRA has $want): run build_disc.sh again"
        exit 1
    fi
done < build/us/mod/kobob/syms.txt
echo "OK   ko_blob DRA symbols ($(wc -l < build/us/mod/kobob/syms.txt)), ARC_KO.BIN $(stat -c %s build/us/ARC_KO.BIN) bytes"
bash mods/sotn-ultimate/tools/check_no_dirt.sh
python3 mods/sotn-ultimate/tools/check_stage_data.py
if [ "$MOD_PSP_DUB" = 1 ]; then
    # Freshness assert: GEN_H must still be repack output. A late
    # assets-build clobbers it with retail values -> 483 cryptic FAILs.
    # Repack once more (deterministic: same XA bytes, no re-disk needed).
    if [ "$(sha1sum src/dra/gen/us/xa_music_config.h | cut -d' ' -f1)" != "$GENH_SHA" ] || [ src/.assets_build_done_us -nt build/us/XA_STR1. ]; then
        echo "GEN_H changed after repack (marker/XA/GENH: $(stat -c %y src/.assets_build_done_us build/us/XA_STR1. src/dra/gen/us/xa_music_config.h | tr '\n' ' ')): repacking once"
        PSP_DUB_JP=${PSP_DUB_JP:-0} .venv/bin/python mods/sotn-ultimate/tools/repack_xa.py $DUB_FLAGS $BGM_FLAGS ${REPACK_XA_FLAGS:-}
        GENH_SHA=$(sha1sum src/dra/gen/us/xa_music_config.h | cut -d' ' -f1)
    fi
    if [ "$(sha1sum src/dra/gen/us/xa_music_config.h | cut -d' ' -f1)" != "$GENH_SHA" ]; then
        echo "GEN_H still stale after repack: refusing check_psp_dub" >&2
        exit 1
    fi
    python3 mods/sotn-ultimate/tools/check_psp_dub.py
fi
python3 mods/sotn-ultimate/tools/check_status_shift.py | tail -1
# KO trial only: menu text check is report-only. The trial replaces
# relic/spell names (Sprite Card, Nosedevil Card slots) with KO codes
# (F040..F09A, bytes f0 40..f0 9a), so a mismatch vs retail bytes is
# expected. Tag each mismatch row: [KO expected] when the DRA bytes contain
# a KO code, else [UNEXPECTED] (any UNEXPECTED still fails the build).
if ! menu_out=$(python3 mods/sotn-ultimate/tools/check_menu_text.py 2>&1); then
    echo "$menu_out"
    if ! echo "$menu_out" | python3 -c '
import re, sys
out = sys.stdin.read()
body = out.split("FAIL menu text:", 1)[1] if "FAIL menu text:" in out else out
print("KO trial: menu text mismatches (expected: relic/spell names replaced):")
unexpected = False
for row in body.strip().split(";"):
    row = row.strip()
    if not row:
        continue
    ko = False
    try:
        import json as _json
        _br = _json.load(open("src/dra/gen/us/ko_jp_bundle_rows.json", encoding="utf-8"))
        _bset = {("0", i * 52) for i in _br.get("equip", [])} | {("1", i * 32) for i in _br.get("acc", [])}
        _tm = re.search(r"table (\d+) \+0x([0-9A-Fa-f]+)", row)
        if _tm and (_tm.group(1), int(_tm.group(2), 16)) in _bset:
            ko = True  # #240: KO bundle-pointer row (name lives in KO_JP.BIN, loaded at menu-open)
    except Exception:
        pass
    m = re.search(r"DRA has ([0-9a-fA-F]+)", row)
    if m:
        hx = m.group(1).lower()
        for i in range(0, len(hx) - 3, 2):
            hi = hx[i:i+2]
            if hi in ("f0", "f1", "f2", "f3"):
                lo = int(hx[i+2:i+4], 16)
                if (hi == "f0" and 0x40 <= lo) or hi in ("f1", "f2") or (hi == "f3" and lo <= 0x47):
                    ko = True  # #240: full KO map F040-F347 (names use F1xx/F2xx too)
                    break
    if not ko:
        unexpected = True
    print(("[KO expected] " if ko else "[UNEXPECTED] ") + row)
sys.exit(1 if unexpected else 0)
'; then
        echo "KO trial: UNEXPECTED menu text mismatch (not a KO replacement)" >&2
        exit 1
    fi
else
    echo "$menu_out"
fi
# splat's absolute addresses inside moved symbols (stale = broken asm
# cutscenes; fix: check_abs_syms.py --write, then build again)
python3 mods/sotn-ultimate/tools/check_abs_syms.py || true
echo "--- files that differ from retail:"
sha1sum -c config/check.us.sha 2>/dev/null | grep -v ": OK$" || true

if [ -s build/us/cc1_errors.log ]; then
    echo "FAIL: compiler errors before the image copy (build/us/cc1_errors.log); no image is made" >&2
    sort -u build/us/cc1_errors.log | head -10 >&2
    exit 1
fi
# an emulator holding the disc open blocks the copy: write a second name
if ! cp build/sotn.us.bin "$OUT" 2>/dev/null; then
    echo "$OUT is in use: writing ${OUT%.bin}2.bin instead"
    OUT="${OUT%.bin}2.bin"
    cp build/sotn.us.bin "$OUT"
fi
# its cue sheet names the bin (the main disc's .cue already exists)
cue="${OUT%.bin}.cue"
if [ ! -f "$cue" ]; then
    printf 'FILE "%s" BINARY\r\n  TRACK 01 MODE2/2352\r\n    FLAGS DCP\r\n    INDEX 01 00:00:00\r\n' \
        "$(basename "$OUT")" > "$cue"
fi
# 3491: runs on EVERY build (it used to sit inside the "no cue yet" block and was skipped on a same-name rebuild)
# The linked image must carry the KO fetch (else the menu hangs).
# addiu v0,a0,4032 (n = ch-0xF040) near sltiu *,*,KO_GLYPH_COUNT (dra.h; v14c 819 = 0x333) in func_80106A28.
if ! python3 - build/sotn.us.bin <<'PYEOF'
import re, sys
d = open(sys.argv[1], 'rb').read()
cnt = int(re.search(r"#define KO_GLYPH_COUNT (\d+)", open('src/dra/dra.h', encoding='utf-8').read()).group(1))
a = d.find(bytes.fromhex('c00f8224'))
b = d.find(cnt.to_bytes(2, 'little') + bytes.fromhex('a22c'))
ok = a >= 0 and b >= 0 and abs(a - b) < 128
sys.exit(0 if ok else 1)
PYEOF
then
    echo "ABORT: image lacks the KO fetch pair (bad link)" >&2
    exit 2
fi
echo "OK   KO fetch pair in image"
echo "copied to $OUT"
# The Japanese Konami logo movie (LOGO15XA.STR) is the LAST build step, in the repo
# (it used to be an out-of-repo post step and silently dropped out of the next build).
# JP_T1 = the Japanese disc's Track 1 (.bin, or the .ecm — decoded with tools/unecm.c).
if [ "${JP_MOVIE:-1}" != 0 ]; then
    JP_T1="${JP_T1:-build/jp_t1/jp_t1.bin}"
    [ -f "$JP_T1" ] || { echo "ABORT: JP movie step needs JP_T1 ($JP_T1 missing); JP_MOVIE=0 skips" >&2; exit 3; }
    python3 mods/sotn-ultimate/tools/jp_movie.py "$OUT" "$JP_T1" || { echo "ABORT: JP movie step failed" >&2; exit 3; }
fi
# The shipped title / loading / 1792-caption art (F_TITLE0 F_TITLE1 F_SEL F_GAME F_GAME2, sha-pinned in ko_assets/title46)
# goes in as a build step too — no replacement may live outside the build.  TITLE_RESTORE=0 skips.
if [ "${TITLE_RESTORE:-1}" != 0 ]; then
    python3 mods/sotn-ultimate/tools/title_restore.py "$OUT" mods/sotn-ultimate/ko_assets/title46 || { echo "ABORT: title restore failed" >&2; exit 3; }
fi
if [ -s build/us/cc1_errors.log ]; then
    echo "CC1 ERRORS: $(sort -u build/us/cc1_errors.log | wc -l) lines (build/us/cc1_errors.log)"
else
    echo "OK   no compiler errors in the build steps"
fi
