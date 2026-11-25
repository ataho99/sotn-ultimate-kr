#!/usr/bin/env bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# Builds the Alucard "Easy controls" blob (src/easy/r1_easy.c) for two stretches of unused Psy-Q library code in the main exe
# (F1 0x80025F08..0x800264A0, F2 0x8002683C..0x80026D4C), linked against the built DRA's addresses and the exe's symbols.
# Output: build/us/mod/r1/r1a.bin (F1) and r1b.bin (F2). patch_exe_r1.py writes them into main.exe.
if [ -f ~/.profile ]; then source ~/.profile; fi
set -euo pipefail
cd "$(dirname "$0")/../../.."
OUT=build/us/mod/r1
mkdir -p "$OUT"
F1=0x80025F08; F1_END=0x800264A0
F2=0x8002683C; F2_END=0x80026D4C
DEFS="${R1_DEFS:-}"
for src in r1_easy r1_ring; do
VERSION=us mipsel-linux-gnu-cpp -Iinclude -Iinclude/psxsdk -Isrc/dra -lang-c     -undef -Wall -fno-builtin -Dmips -D__GNUC__=2 -D__OPTIMIZE__ -D__mips__ -D__mips     -Dpsx -D__psx__ -D__psx -D_PSYQ -D__EXTENSIONS__ -D_MIPSEL -D_LANGUAGE_C     -DLANGUAGE_C -DNO_LOGS -DHACKS -DUSE_INCLUDE_ASM -D_internal_version_us -DSOTN_STR -DMOD_KO $DEFS     mods/sotn-ultimate/src/easy/$src.c | tools/sotn_str/target/release/sotn_str process |
    iconv --from-code=UTF-8 --to-code=Shift-JIS |
    bin/cc1-psx-26 -G0 -w -funsigned-char -fpeephole -ffunction-cse         -fpcc-struct-return -fcommon -fverbose-asm -msoft-float -g -quiet         -mcpu=3000 -fgnu-linker -mgas -gcoff -O2 |
    python3 tools/maspsx/maspsx.py --expand-div --aspsx-version=2.34 |
    mipsel-linux-gnu-as -Iinclude -march=r3000 -mtune=r3000 -no-pad-sections         -O1 -G0 -o "$OUT/$src.o"
done
: > "$OUT/syms.txt"
mipsel-linux-gnu-nm --defined-only "$OUT/r1_easy.o" "$OUT/r1_ring.o" | awk 'NF == 3 {print $3}' | sort -u > "$OUT/defined.txt"
for s in $(mipsel-linux-gnu-nm -u "$OUT/r1_easy.o" "$OUT/r1_ring.o" | awk 'NF == 2 {print $2}' | sort -u | grep -v -x -F -f "$OUT/defined.txt"); do
    a=$(mipsel-linux-gnu-nm build/us/dra.elf | awk -v s="$s" '$3==s{print $1}')
    if [ -z "$a" ]; then
        a=$(awk -v s="$s" '$1==s{sub(/;/,"",$3); sub(/^0x/,"",$3); print $3}' config/symbols.us.txt)
    fi
    if [ -z "$a" ] || [ "$(echo "$a" | wc -l)" != 1 ]; then
        echo "FAIL r1: $s not in dra.elf / symbols.us.txt (or not once)"; exit 1
    fi
    echo "$s = 0x$a;" >> "$OUT/syms.txt"
done
cat > "$OUT/r1.ld" <<LDEOF
SECTIONS {
    . = $F1;
    .text : { *r1_easy.o(.text) }
    .rodata : { *r1_easy.o(.rodata*) *r1_easy.o(.rdata) }
    .data : { *r1_easy.o(.data) *r1_easy.o(.sdata) *r1_easy.o(.bss) *r1_easy.o(.sbss) *r1_easy.o(COMMON) }
    . = $F2;
    .textb : { *r1_ring.o(.text) *r1_ring.o(.rodata*) *r1_ring.o(.rdata) *r1_ring.o(.data) *r1_ring.o(.sdata) *r1_ring.o(.bss) *r1_ring.o(.sbss) *r1_ring.o(COMMON) }
    /DISCARD/ : { *(.reginfo) *(.pdr) *(.mdebug*) *(.comment) *(.gnu*) *(.MIPS*) }
}
LDEOF
mipsel-linux-gnu-ld -nostdlib --no-check-sections -Map "$OUT/r1.map" \
    -T "$OUT/r1.ld" -T config/undefined_syms.us.txt -T "$OUT/syms.txt" -o "$OUT/r1.elf" "$OUT/r1_easy.o" "$OUT/r1_ring.o"
mipsel-linux-gnu-objcopy -O binary -j .text -j .rodata -j .data "$OUT/r1.elf" "$OUT/r1a.bin"
mipsel-linux-gnu-objcopy -O binary -j .textb "$OUT/r1.elf" "$OUT/r1b.bin"
sa=$(stat -c %s "$OUT/r1a.bin"); sb=$(stat -c %s "$OUT/r1b.bin")
if [ "$(od -An -tx4 -N4 "$OUT/r1a.bin" | tr -d ' ')" != "31525a45" ]; then
    echo "FAIL r1: the header (magic EZR1) is not first in F1"; exit 1
fi
if [ $((F1 + sa)) -gt $((F1_END)) ]; then echo "FAIL r1: F1 blob $sa B ends past $(printf %X $F1_END)"; exit 1; fi
if [ $((F2 + sb)) -gt $((F2_END)) ]; then echo "FAIL r1: F2 blob $sb B ends past $(printf %X $F2_END)"; exit 1; fi
echo "r1: F1 $sa B of $((F1_END - F1)), F2 $sb B of $((F2_END - F2)) (unused Psy-Q code in the main exe)"
