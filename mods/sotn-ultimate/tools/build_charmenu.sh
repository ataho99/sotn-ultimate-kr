#!/usr/bin/env bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# Saturn port mod: the Ultimate balance's pause menu for Richter and the
# Marias, its drawing code (mods/sotn-ultimate/src/menu/charmenu.c) ->
# 12 sectors after build/us/ARC_F2.BIN's MOD_ARC_F_LEN bytes (Alucard's
# sprites + the Extra settings code, all g_LbaBinAlucard reads; a new file
# in BIN/ would move the retail ones). Run after build_sysopt.sh, which
# makes ARC_F2.BIN.
#
# DRA has no room for it. The menu reads it each time it opens into
# g_Pix[1] to [3] (24 KB, idle while the menu is open: see charmenu.c),
# so it is linked at g_Pix + 0x2000 (this build's DRA) against DRA's
# symbols, at most 24 KB: the demo-key load it borrows reads 12 sectors
# for it (game_mod.h MOD_CHARMENU_SIZE). Their portraits come from
# gen_title_portraits.py (run before it by build_disc.sh).
# Run after DRA's final link (build_disc.sh checks the addresses again).
set -euo pipefail
cd "$(dirname "$0")/../../.."
OUT=build/us/mod/charmenu
mkdir -p "$OUT"
MAGIC=$(grep -hoE "#define MOD_CHARMENU_MAGIC 0x[0-9A-Fa-f]+" mods/sotn-ultimate/include/game_mod.h | awk '{print $3}')
PIX=$(mipsel-linux-gnu-nm build/us/dra.elf | awk '$3=="g_Pix"{print $1}')
if [ -z "$PIX" ]; then
    echo "FAIL charmenu: g_Pix not in dra.elf"; exit 1
fi
ADDR=$(printf "0x%08X" $((0x$PIX + 0x2000)))
echo "$ADDR" > "$OUT/addr.txt" # (build_disc.sh checks it after make disk)

# KO: the 8px renderer is called by its absolute address (charmenu.c KO_EQUIP_SEL)
if grep -q "MOD_CHARMENU_KO" <<<"${CHARMENU_DEFS:-}"; then
    EQSEL=$(grep -hoE "#define KO_EQUIP_SEL 0x[0-9A-Fa-f]+" src/dra/dra.h | awk '{print $3}')
    CMSEL=$(grep -hoE "#define KO_EQUIP_SEL 0x[0-9A-Fa-f]+" mods/sotn-ultimate/src/menu/charmenu.c | awk '{print $3}')
    if [ -z "$EQSEL" ] || [ "$EQSEL" != "$CMSEL" ]; then
        echo "FAIL charmenu: KO_EQUIP_SEL in charmenu.c ($CMSEL) differs from src/dra/dra.h ($EQSEL)"; exit 1
    fi
    EQOV=$(grep -hoE "#define KO_EQUIP_OV 0x[0-9A-Fa-f]+" src/dra/dra.h | awk '{print $3}')
    CMOV=$(grep -hoE "#define KO_EQUIP_OV 0x[0-9A-Fa-f]+" mods/sotn-ultimate/src/menu/charmenu.c | awk '{print $3}')
    if [ -z "$EQOV" ] || [ "$EQOV" != "$CMOV" ]; then
        echo "FAIL charmenu: KO_EQUIP_OV in charmenu.c ($CMOV) differs from src/dra/dra.h ($EQOV)"; exit 1
    fi
fi
VERSION=us mipsel-linux-gnu-cpp -Iinclude -Iinclude/psxsdk -Isrc/dra -lang-c \
    -undef -Wall -fno-builtin -Dmips -D__GNUC__=2 -D__OPTIMIZE__ -D__mips__ -D__mips \
    -Dpsx -D__psx__ -D__psx -D_PSYQ -D__EXTENSIONS__ -D_MIPSEL -D_LANGUAGE_C \
    -DLANGUAGE_C -DNO_LOGS -DHACKS -DUSE_INCLUDE_ASM -D_internal_version_us -DSOTN_STR ${CHARMENU_DEFS:-} \
    mods/sotn-ultimate/src/menu/charmenu.c | tools/sotn_str/target/release/sotn_str process |
    iconv --from-code=UTF-8 --to-code=Shift-JIS |
    bin/cc1-psx-26 -G0 -w -funsigned-char -fpeephole -ffunction-cse \
        -fpcc-struct-return -fcommon -fverbose-asm -msoft-float -g -quiet \
        -mcpu=3000 -fgnu-linker -mgas -gcoff -O2 |
    python3 tools/maspsx/maspsx.py --expand-div --aspsx-version=2.34 |
    mipsel-linux-gnu-as -Iinclude -march=r3000 -mtune=r3000 -no-pad-sections \
        -O1 -G0 -o "$OUT/charmenu.o"

# every DRA symbol it uses (this build's addresses)
: > "$OUT/syms.txt"
for s in $(mipsel-linux-gnu-nm -u "$OUT/charmenu.o" | awk '{print $2}'); do
    a=$(mipsel-linux-gnu-nm build/us/dra.elf | awk -v s="$s" '$3==s{print $1}')
    if [ -z "$a" ]; then # (main / SDK symbols: config/symbols.us.txt)
        a=$(awk -v s="$s" '$1==s{sub(/;/,"",$3); sub(/^0x/,"",$3); print $3}' config/symbols.us.txt)
    fi
    if [ -z "$a" ] || [ "$(echo "$a" | wc -l)" != 1 ]; then
        echo "FAIL charmenu: $s not in dra.elf (or not once)"; exit 1
    fi
    echo "$s = 0x$a;" >> "$OUT/syms.txt"
done
cat > "$OUT/charmenu.ld" <<EOF
SECTIONS {
    . = $ADDR;
    .text $ADDR : SUBALIGN(4) { *(.text) }
    .rodata : { *(.rodata*) *(.rdata) }
    .data : { *(.data) *(.sdata) }
    .bss : { *(.bss) *(.sbss) *(COMMON) }
    /DISCARD/ : { *(.reginfo) *(.pdr) *(.mdebug*) *(.comment) *(.gnu*) *(.MIPS*) }
}
EOF
mipsel-linux-gnu-ld -nostdlib --no-check-sections -Map "$OUT/charmenu.map" \
    -T "$OUT/charmenu.ld" -T "$OUT/syms.txt" \
    -o "$OUT/charmenu.elf" "$OUT/charmenu.o"
# it is copied to g_Pix[1] as is: .text must start exactly there (g_Pix[1]
# is not 16-byte aligned; an aligned .text would put every address off)
vma=$(mipsel-linux-gnu-objdump -h "$OUT/charmenu.elf" | awk '$2==".text"{print $4}')
if [ "$((16#$vma))" -ne "$((ADDR))" ]; then
    echo "FAIL charmenu: .text linked at 0x$vma, not $ADDR"; exit 1
fi
sz=$(mipsel-linux-gnu-objdump -h "$OUT/charmenu.elf" | awk '$2==".bss"{print $3}')
if [ -n "$sz" ] && [ $((16#$sz)) -ne 0 ]; then
    echo "FAIL charmenu: .bss is not empty (initialise its statics)"; exit 1
fi
mipsel-linux-gnu-objcopy -O binary -j .text -j .rodata -j .data "$OUT/charmenu.elf" "$OUT/charmenu.bin"
size=$(stat -c %s "$OUT/charmenu.bin")
if [ "$(od -An -tx4 -N4 "$OUT/charmenu.bin" | tr -d ' ')" != "$(printf %08x $MAGIC)" ]; then
    echo "FAIL charmenu: the header is not first"; exit 1
fi
if [ "$size" -gt 24576 ]; then
    echo "FAIL charmenu: $size bytes, $((size - 24576)) over g_Pix[1..3]"; exit 1
fi
LEN=$(grep -hoE "#define MOD_ARC_F_LEN 0x[0-9A-Fa-f]+" mods/sotn-ultimate/include/game_mod.h | awk '{print $3}')
if [ "$(stat -c %s build/us/ARC_F2.BIN)" -ne $((LEN)) ]; then
    echo "FAIL charmenu: build/us/ARC_F2.BIN is not MOD_ARC_F_LEN long (run build_sysopt.sh first)"; exit 1
fi
cat "$OUT/charmenu.bin" >> build/us/ARC_F2.BIN
truncate -s $((LEN + 24576)) build/us/ARC_F2.BIN
echo "charmenu: $size bytes at $ADDR, $((24576 - size)) free"
