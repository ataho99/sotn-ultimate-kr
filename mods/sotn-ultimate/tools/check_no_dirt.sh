#!/bin/bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# Saturn port mod: check that build/us/DRA.BIN is exactly the linked
# build/us/dra.elf, i.e. no retail dirt bytes were patched over it (see
# tools/dirt_patcher.py).
cd "$(dirname "$0")/../../.."
linked=$(mktemp)
trap 'rm -f "$linked"' EXIT
mipsel-linux-gnu-objcopy -O binary build/us/dra.elf "$linked"
if cmp -s "$linked" build/us/DRA.BIN; then
    echo "OK   DRA.BIN matches dra.elf (no dirt patches)"
else
    # 3877: the one allowed difference: the castle map-top table patch_dra_maptop.py writes into the start of g_ModDraPad (the DRA room
    # pad, dead space). Put the same table into the linked image and compare again: any other difference still fails.
    linked2=$(mktemp)
    trap 'rm -f "$linked" "$linked2"' EXIT
    cp "$linked" "$linked2"
    if python3 mods/sotn-ultimate/tools/patch_dra_maptop.py --apply-to "$linked2" && cmp -s "$linked2" build/us/DRA.BIN; then
        echo "OK   DRA.BIN matches dra.elf plus the map-top table in g_ModDraPad (no dirt patches)"
    else
        echo "FAIL DRA.BIN differs from dra.elf:"
        cmp -l "$linked" build/us/DRA.BIN | head -5
        exit 1
    fi
fi
