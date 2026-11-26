#!/usr/bin/env bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# The new resident area (memory map 0x801F3000..0x801FAFFF, 32 KB). 099b: three sources linked at MOD_NEWAREA_CODE (= base + 32) against the built DRA's symbols
# (and the main exe's, config/symbols.us.txt, and the cheats' absolute symbols build_sysopt.sh computed), behind a 32 B header:
#   +0 magic 'KOR1', +4 version, +8 R1Tick (easy tick), +12 CheatTick, +16 PopDrawKo, +20 SubCmd (the familiars' subtitle), +24 CheatAct (the cheat popup's instant actions), +28 zero.
# Output: build/us/mod/newarea/newarea.bin (header + code), build/us/mod/newarea/syms.txt (rechecked after the disc is built).
# build_ko_trial.sh puts it into ARC_KO.BIN from block 17 (cd.c's CopyKoGlyphsCallback copies it to 0x801F3000 at every game load).
# Fails when it has a .bss (a zero static would not be cleared: the state lives in ARC_F2), when an entry is missing, or when it is > 32 KB - 4 (the marker).
# Needs build/us/mod/sysopt/syms.txt (run build_sysopt.sh first).
if [ -f ~/.profile ]; then source ~/.profile; fi
set -euo pipefail
cd "$(dirname "$0")/../../.."
OUT=build/us/mod/newarea
mkdir -p "$OUT"
rm -f "$OUT"/*.o
BASE=$(grep -hoE "#define MOD_NEWAREA_BASE 0x[0-9A-Fa-f]+" mods/sotn-ultimate/include/game_mod.h | awk '{print $3}')
LIMIT=$(grep -hoE "#define MOD_NEWAREA_LIMIT 0x[0-9A-Fa-f]+" mods/sotn-ultimate/include/game_mod.h | awk '{print $3}')
MAGIC=$(grep -hoE "#define MOD_NEWAREA_MAGIC 0x[0-9A-Fa-f]+" mods/sotn-ultimate/include/game_mod.h | awk '{print $3}')
if [ -z "$BASE" ] || [ -z "$LIMIT" ] || [ -z "$MAGIC" ]; then echo "FAIL newarea: MOD_NEWAREA_* missing in game_mod.h"; exit 1; fi
CODE=$((BASE + 32))
SRCS="r1_easy newarea_cheat newarea_pop newarea_sub newarea_act newarea_rm" # r1_easy first: its R1Tick is the first function
for src in $SRCS; do
VERSION=us mipsel-linux-gnu-cpp -Iinclude -Iinclude/psxsdk -Isrc/dra -lang-c \
    -undef -Wall -fno-builtin -Dmips -D__GNUC__=2 -D__OPTIMIZE__ -D__mips__ -D__mips \
    -Dpsx -D__psx__ -D__psx -D_PSYQ -D__EXTENSIONS__ -D_MIPSEL -D_LANGUAGE_C \
    -DLANGUAGE_C -DNO_LOGS -DHACKS -DUSE_INCLUDE_ASM -D_internal_version_us -DSOTN_STR ${SYSOPT_DEFS:-} \
    mods/sotn-ultimate/src/easy/$src.c | tools/sotn_str/target/release/sotn_str process |
    iconv --from-code=UTF-8 --to-code=Shift-JIS |
    bin/cc1-psx-26 -G0 -w -funsigned-char -fpeephole -ffunction-cse \
        -fpcc-struct-return -fcommon -fverbose-asm -msoft-float -g -quiet \
        -mcpu=3000 -fgnu-linker -mgas -gcoff -O2 |
    python3 tools/maspsx/maspsx.py --expand-div --aspsx-version=2.34 |
    mipsel-linux-gnu-as -Iinclude -march=r3000 -mtune=r3000 -no-pad-sections \
        -O1 -G0 -o "$OUT/$src.o"
done
OBJS=""
for src in $SRCS; do OBJS="$OBJS $OUT/$src.o"; done
: > "$OUT/syms.txt"
mipsel-linux-gnu-nm --defined-only $OBJS | awk 'NF == 3 {print $3}' | sort -u > "$OUT/defined.txt"
for s in $(mipsel-linux-gnu-nm -u $OBJS | awk 'NF == 2 {print $2}' | sort -u | grep -v -x -F -f "$OUT/defined.txt"); do
    a=""
    case "$s" in # the player overlays' own symbols (statics too): O1_ RIC.BIN, O2_ MARIA.BIN (Saturn), O3_ MARIA2.BIN (PSP)
        O1_*) a=$(mipsel-linux-gnu-nm build/us/ric.elf | awk -v s="${s#O1_}" '$3==s{print $1}') ;;
        O2_*) a=$(mipsel-linux-gnu-nm build/us/mod/mar/mar.elf | awk -v s="${s#O2_}" '$3==s{print $1}') ;;
        O3_*) a=$(mipsel-linux-gnu-nm build/us/mod/mar2/mar2.elf | awk -v s="${s#O3_}" '$3==s{print $1}') ;;
        *) a=$(mipsel-linux-gnu-nm build/us/dra.elf | awk -v s="$s" '$3==s{print $1}') ;;
    esac
    if [ -z "$a" ]; then # (main SDK functions: config/symbols.us.txt)
        a=$(awk -v s="$s" '$1==s{sub(/;/,"",$3); sub(/^0x/,"",$3); print $3}' config/symbols.us.txt)
    fi
    if [ -z "$a" ]; then # the cheats' absolute symbols (HandleDamage's entry and words, the stair branches), computed by build_sysopt.sh
        a=$(awk -v s="$s" '$1==s{sub(/;/,"",$3); sub(/^0x/,"",$3); print $3}' build/us/mod/sysopt/syms.txt)
    fi
    if [ -z "$a" ] || [ "$(echo "$a" | wc -l)" != 1 ]; then
        echo "FAIL newarea: $s not in dra.elf / symbols.us.txt / sysopt syms.txt (or not once)"; exit 1
    fi
    echo "$s = 0x$a;" >> "$OUT/syms.txt"
done
# the popup draws through DRA's 8px renderer by absolute address: the same value dra.h has
EQSEL=$(grep -hoE "#define KO_EQUIP_SEL 0x[0-9A-Fa-f]+" src/dra/dra.h | awk '{print $3}')
NASEL=$(grep -hoE "#define KO_EQUIP_SEL 0x[0-9A-Fa-f]+" mods/sotn-ultimate/src/easy/newarea_pop.c | awk '{print $3}')
if [ -z "$EQSEL" ] || [ "$((EQSEL))" -ne "$((NASEL))" ]; then
    echo "FAIL newarea: KO_EQUIP_SEL in newarea_pop.c ($NASEL) differs from src/dra/dra.h ($EQSEL)"; exit 1
fi
cat > "$OUT/newarea.ld" <<EOF
SECTIONS {
    . = $CODE;
    .text : { *(.text) }
    .rodata : { *(.rodata*) *(.rdata) }
    .data : { *(.data) *(.sdata) }
    .bss : { *(.bss) *(.sbss) *(COMMON) }
    /DISCARD/ : { *(.reginfo) *(.pdr) *(.mdebug*) *(.comment) *(.gnu*) *(.MIPS*) }
}
EOF
mipsel-linux-gnu-ld -nostdlib --no-check-sections -Map "$OUT/newarea.map" \
    -T "$OUT/newarea.ld" -T config/undefined_syms.us.txt -T "$OUT/syms.txt" \
    -o "$OUT/newarea.elf" $OBJS
sz=$(mipsel-linux-gnu-objdump -h "$OUT/newarea.elf" | awk '$2==".bss"{print $3}')
if [ -n "$sz" ] && [ $((16#$sz)) -ne 0 ]; then
    echo "FAIL newarea: .bss is not empty (no statics here: the state is ARC_F2's)"; exit 1
fi
ent() { mipsel-linux-gnu-nm "$OUT/newarea.elf" | awk -v s="$1" '$3==s{print $1}'; }
E_TICK=$(ent R1Tick); E_CHEAT=$(ent CheatTick); E_POP=$(ent PopDrawKo); E_SUB=$(ent SubCmd); E_ACT=$(ent CheatAct)
for e in "$E_TICK" "$E_CHEAT" "$E_POP" "$E_SUB" "$E_ACT"; do
    if [ -z "$e" ]; then echo "FAIL newarea: an entry (R1Tick / CheatTick / PopDrawKo / SubCmd / CheatAct) is missing"; exit 1; fi
done
if [ "0x$E_TICK" != "$(printf '0x%08x' $CODE)" ]; then
    echo "FAIL newarea: R1Tick is at 0x$E_TICK, not first at $(printf 0x%08X $CODE)"; exit 1
fi
mipsel-linux-gnu-objcopy -O binary -j .text -j .rodata -j .data "$OUT/newarea.elf" "$OUT/code.bin"
# the 32 B header: magic, version 1, the three entries, zeros
python3 - "$OUT/code.bin" "$OUT/newarea.bin" "$((MAGIC))" "$((16#$E_TICK))" "$((16#$E_CHEAT))" "$((16#$E_POP))" "$((16#$E_SUB))" "$((16#$E_ACT))" <<'PYEOF'
import struct, sys
code = open(sys.argv[1], 'rb').read()
hdr = struct.pack('<IIIIIIII', int(sys.argv[3]), 1, int(sys.argv[4]), int(sys.argv[5]), int(sys.argv[6]), int(sys.argv[7]), int(sys.argv[8]), 0)
open(sys.argv[2], 'wb').write(hdr + code)
PYEOF
size=$(stat -c %s "$OUT/newarea.bin")
if [ "$size" -gt $((LIMIT - 4)) ]; then
    echo "FAIL newarea: $size B is past the 32 KB limit ($((LIMIT - 4)) B + the marker)"; exit 1
fi
echo "newarea: $size B of $((LIMIT)) at $(printf 0x%08X $BASE) (entries tick 0x$E_TICK cheat 0x$E_CHEAT pop 0x$E_POP), $(wc -l < "$OUT/syms.txt") DRA/exe symbols"
