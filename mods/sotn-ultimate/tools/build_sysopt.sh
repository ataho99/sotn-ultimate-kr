#!/usr/bin/env bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# Saturn port mod: the pause menu System screen's "Ultimate settings" row
# and popup (mods/sotn-ultimate/src/menu/sysopt.c) -> build/us/ARC_F2.BIN.
#
# DRA has no room for it. BIN/ARC_F.BIN (Alucard's sprites, 0x31BEC bytes)
# is loaded at 0x8013C000 in Alucard games only, and nothing uses the RAM
# after it below the familiar overlay (0x80170000). The code is linked at
# MOD_SYSOPT_ADDR (file offset 0x31BF0) against the built DRA's symbols and
# appended to a copy of ARC_F.BIN, filled to MOD_ARC_F_LEN (0x34000, up to
# 0x80170000): BIN/ARC_F2.BIN, a new file at the disc's end
# (config/disk.us.lba; no file before it moves), which src/dra/cd.c's
# g_LbaBinAlucard loads instead (OFF_BIN_ARC_F2). The retail ARC_F.BIN stays
# on the disc unchanged and unused.
# Run after DRA's final link (its addresses are rechecked by build_disc.sh).
set -euo pipefail
cd "$(dirname "$0")/../../.."
OUT=build/us/mod/sysopt
mkdir -p "$OUT"
ADDR=$(grep -hoE "#define MOD_SYSOPT_ADDR 0x[0-9A-Fa-f]+" include/game.h mods/sotn-ultimate/include/game_mod.h | awk '{print $3}')
MAGIC=$(grep -hoE "#define MOD_SYSOPT_MAGIC 0x[0-9A-Fa-f]+" include/game.h mods/sotn-ultimate/include/game_mod.h | awk '{print $3}')
LEN=$(grep -hoE "#define MOD_ARC_F_LEN 0x[0-9A-Fa-f]+" include/game.h mods/sotn-ultimate/include/game_mod.h | awk '{print $3}')
RETAIL=disks/us/BIN/ARC_F.BIN
OFF=$((ADDR - 0x8013C000))
# DRA's stubs (src/config_us.h, raw words) compare the header's first word
# with the base they load from, and use %lo(ADDR) = 0xDBF0
if [ $(((ADDR + 0x8000) & 0xFFFF0000)) -ne $((MAGIC)) ] ||
    [ $((ADDR & 0xFFFF)) -ne $((0xDBF0)) ]; then
    echo "FAIL sysopt: MOD_SYSOPT_ADDR / MAGIC differ from config_us.h's stubs"; exit 1
fi
# the stubs as built (maspsx may only add nops before the raw words)
python3 - "$ADDR" <<'PYEOF'
import subprocess, sys, struct
addr = int(sys.argv[1], 16)
syms = {}
for l in subprocess.run(["mipsel-linux-gnu-nm", "build/us/dra.elf"],
                        capture_output=True, text=True).stdout.splitlines():
    p = l.split()
    if len(p) == 3:
        syms[p[2]] = int(p[0], 16)
elf = open("build/us/DRA.BIN", "rb").read()
def words(sym, n):
    off = syms[sym] - 0x800A0000
    return list(struct.unpack_from("<%dI" % n, elf, off))
hi = (addr + 0x8000) >> 16
snd = words("ModSysOptSound", 4)
box = words("ModSysOptBox", 4)
inp = words("ModSysOptInput", 3)
go = [w for w in words("ModSysOptGo", 10) if w][:7]
tick = words("ModSysOptTick", 6)  # lui/addiu $13 = ModSysOptRet; b ModSysOptGo; li $11, 16; jr $31; nop
want_go = [0x3C080000 | hi, 0x8D09DBF0, 0x010B5021, 0x8D4CDBF0, 0x15280002,
           0x01806821, 0x01A00008]
ok = (snd[2:] == [0x10000008, 0x340B000C] and
      box[2:] == [0x10000004, 0x340B0004] and inp[2] == 0x340B0008 and
      syms["ModSysOptInput"] + 12 == syms["ModSysOptGo"] and
      syms["ModSysOptBox"] + 16 == syms["ModSysOptInput"] and
      syms["ModSysOptSound"] + 16 == syms["ModSysOptBox"] and go == want_go and
      syms["ModSysOptTick"] == syms["ModSysOptGo"] + 36 and
      tick[2:] == [0x1000FFF4, 0x340B0010, 0x03E00008, 0])
if not ok:
    print("FAIL sysopt: DRA's stubs are not as written (config_us.h)")
    sys.exit(1)
PYEOF
if [ "$(stat -c %s $RETAIL)" -gt $OFF ]; then
    echo "FAIL sysopt: $RETAIL ends past MOD_SYSOPT_ADDR"; exit 1
fi

VERSION=us mipsel-linux-gnu-cpp -Iinclude -Iinclude/psxsdk -Isrc/dra -lang-c \
    -undef -Wall -fno-builtin -Dmips -D__GNUC__=2 -D__OPTIMIZE__ -D__mips__ -D__mips \
    -Dpsx -D__psx__ -D__psx -D_PSYQ -D__EXTENSIONS__ -D_MIPSEL -D_LANGUAGE_C \
    -DLANGUAGE_C -DNO_LOGS -DHACKS -DUSE_INCLUDE_ASM -D_internal_version_us -DSOTN_STR $SYSOPT_DEFS \
    mods/sotn-ultimate/src/menu/sysopt.c | tools/sotn_str/target/release/sotn_str process |
    iconv --from-code=UTF-8 --to-code=Shift-JIS |
    bin/cc1-psx-26 -G0 -w -funsigned-char -fpeephole -ffunction-cse \
        -fpcc-struct-return -fcommon -fverbose-asm -msoft-float -g -quiet \
        -mcpu=3000 -fgnu-linker -mgas -gcoff -O2 |
    python3 tools/maspsx/maspsx.py --expand-div --aspsx-version=2.34 |
    mipsel-linux-gnu-as -Iinclude -march=r3000 -mtune=r3000 -no-pad-sections \
        -O1 -G0 -o "$OUT/sysopt.o"

# DRA's functions / statics it calls (this build's addresses)
: > "$OUT/syms.txt"
for s in func_800F5E68 MenuDrawStr MenuHandleCursorInput D_8013784C \
    g_Entities MenuSoundConfigDraw g_Menus g_MenuInit g_LbaSdAlkVh \
    g_LbaSdAlkVb g_CdStep g_IsUsingCd g_LoadFile g_ServantLoaded g_MenuStep     g_LoadOvlIdx g_StageId SsUtAllKeyOff     g_Status g_Player g_GameTimer FlushCache func_80109990 HandleDamage; do
    a=$(mipsel-linux-gnu-nm build/us/dra.elf | awk -v s="$s" '$3==s{print $1}')
    if [ -z "$a" ]; then # (main SDK functions: config/symbols.us.txt)
        a=$(awk -v s="$s" '$1==s{sub(/;/,"",$3); sub(/^0x/,"",$3); print $3}' config/symbols.us.txt)
    fi
    if [ -z "$a" ] || [ "$(echo "$a" | wc -l)" != 1 ]; then
        echo "FAIL sysopt: $s not in dra.elf (or not once)"; exit 1
    fi
    echo "$s = 0x$a;" >> "$OUT/syms.txt"
done
# the KO popup calls DRA's 8px renderer by absolute address (sysopt.c KO_EQUIP_SEL)
if grep -q "MOD_SYSOPT_KO" <<<"$SYSOPT_DEFS"; then
    EQSEL=$(grep -hoE "#define KO_EQUIP_SEL 0x[0-9A-Fa-f]+" src/dra/dra.h | awk '{print $3}')
    SYSSEL=$(grep -hoE "#define KO_EQUIP_SEL 0x[0-9A-Fa-f]+" mods/sotn-ultimate/src/menu/sysopt.c | awk '{print $3}')
    if [ -z "$EQSEL" ] || [ "$((EQSEL))" -ne "$((SYSSEL))" ]; then
        echo "FAIL sysopt: KO_EQUIP_SEL in sysopt.c ($SYSSEL) differs from src/dra/dra.h ($EQSEL)"; exit 1
    fi
fi
# the cheats' absolute symbols: HandleDamage's entry and first two words as
# built, the two stair-slowdown branches (upper halfword 0x1482) in func_80109990
python3 - "$OUT/syms.txt" <<'PYEOF2'
import re, struct, sys
t = open(sys.argv[1]).read()
a = int(re.search(r"HandleDamage = (0x[0-9a-fA-F]+);", t).group(1), 16)
b = int(re.search(r"func_80109990 = (0x[0-9a-fA-F]+);", t).group(1), 16)
dra = open("build/us/DRA.BIN", "rb").read()
w0, w1 = struct.unpack_from("<II", dra, a - 0x800A0000)
sa, sb = b + 0x2D2, b + 0x35E
if (w0, w1) != (0x27BDFFD8, 0xAFB20018):
    sys.exit("FAIL sysopt: HandleDamage starts 0x%08X 0x%08X, not the words src/dra/cd.c ModCheatUnpatch puts back" % (w0, w1))
for x in (sa, sb):
    h = struct.unpack_from("<H", dra, x - 0x800A0000)[0]
    if h != 0x1482:
        sys.exit("FAIL sysopt: the stair branch halfword at 0x%08X is 0x%04X (not 0x1482)" % (x, h))
lines = ["ModHdEntry = 0x%08X;" % a, "ModHdOrig0 = 0x%08X;" % w0,
         "ModHdOrig1 = 0x%08X;" % w1, "ModStairsA = 0x%08X;" % sa,
         "ModStairsB = 0x%08X;" % sb]
open(sys.argv[1], "a").write(chr(10).join(lines) + chr(10))
PYEOF2
cat > "$OUT/sysopt.ld" <<EOF
SECTIONS {
    . = $ADDR;
    .text : { *(.text) }
    .rodata : { *(.rodata*) *(.rdata) }
    .data : { *(.data) *(.sdata) }
    .bss : { *(.bss) *(.sbss) *(COMMON) }
    /DISCARD/ : { *(.reginfo) *(.pdr) *(.mdebug*) *(.comment) *(.gnu*) *(.MIPS*) }
}
EOF
mipsel-linux-gnu-ld -nostdlib --no-check-sections -Map "$OUT/sysopt.map" \
    -T "$OUT/sysopt.ld" -T config/undefined_syms.us.txt -T "$OUT/syms.txt" \
    -o "$OUT/sysopt.elf" "$OUT/sysopt.o"
# its state (.data) is loaded with the file; .bss would not be cleared
sz=$(mipsel-linux-gnu-objdump -h "$OUT/sysopt.elf" | awk '$2==".bss"{print $3}')
if [ -n "$sz" ] && [ $((16#$sz)) -ne 0 ]; then
    echo "FAIL sysopt: .bss is not empty (initialise its statics)"; exit 1
fi
mipsel-linux-gnu-objcopy -O binary -j .text -j .rodata -j .data "$OUT/sysopt.elf" "$OUT/sysopt.bin"
size=$(stat -c %s "$OUT/sysopt.bin")
if [ "$(od -An -tx4 -N4 "$OUT/sysopt.bin" | tr -d ' ')" != "$(printf %08x $MAGIC)" ]; then
    echo "FAIL sysopt: the header is not first"; exit 1
fi
# the info bar text: header + 20 (include/game.h MOD_SYSOPT_BAR_TEXT)
BARHEX="426f737320"; BARLEN=5 # "Boss "
if grep -q "MOD_SYSOPT_KO" <<<"$SYSOPT_DEFS"; then BARHEX=""; fi
if [ -n "$BARHEX" ]; then
    if [ "$(od -An -tx1 -j20 -N$BARLEN "$OUT/sysopt.bin" | tr -d ' ' | tr -d '
')" != "$BARHEX" ]; then
        echo "FAIL sysopt: the info bar text is not at the header + 20"; exit 1
    fi
else
    # KO: the WHOLE bar text (sysopt_ko_str.h KO_XSET_BAR_ASM, C octal escapes) must equal the bytes at header + 20
    python3 - "$OUT/sysopt.bin" "mods/sotn-ultimate/src/menu/sysopt_ko_str.h" <<'PY' || { echo "FAIL sysopt: the info bar text is not the KO_XSET_BAR_ASM bytes at header + 20"; exit 1; }
import re, sys
d = open(sys.argv[1], "rb").read()
m = re.search(r'#define KO_XSET_BAR_ASM "(.*)"', open(sys.argv[2], encoding="utf-8").read())
want = bytes(int(x, 8) if x else 0 for x in re.findall(r'\\\\([0-7]{3})', m.group(1)))
lit = re.sub(r'\\\\[0-7]{3}', "", m.group(1))
assert m and not lit, ("unparsed text in KO_XSET_BAR_ASM", lit)
assert len(want) > 8, len(want)
assert d[20:20 + len(want)] == want, (d[20:20 + len(want)].hex(), want.hex())
print("sysopt: KO info bar %d bytes match KO_XSET_BAR_ASM" % len(want))
PY
fi
if [ $((OFF + size)) -gt $((LEN)) ]; then
    echo "FAIL sysopt: $size bytes, $((OFF + size - LEN)) past MOD_ARC_F_LEN"
    exit 1
fi
# (the file the earlier build patched in place: the disc takes retail again)
rm -f build/us/ARC_F.BIN
cp $RETAIL build/us/ARC_F2.BIN
truncate -s $OFF build/us/ARC_F2.BIN
cat "$OUT/sysopt.bin" >> build/us/ARC_F2.BIN
truncate -s $((LEN)) build/us/ARC_F2.BIN
# the other player files loaded at the same address must not hold the magic
# there (the stubs would call into them)
for f in disks/us/BIN/RIC.BIN build/us/RIC2.BIN build/us/MARIA.BIN build/us/MARIA2.BIN; do
    if [ -f "$f" ] && [ "$(od -An -tx4 -j $OFF -N4 "$f" | tr -d ' ')" = "$(printf %08x $MAGIC)" ]; then
        echo "FAIL sysopt: $f holds the magic at 0x$(printf %X $OFF)"; exit 1
    fi
done
echo "sysopt: $size bytes at $ADDR, ARC_F2.BIN free $((LEN - OFF - size))"
