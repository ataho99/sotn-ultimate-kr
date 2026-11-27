#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""patch_dra_maptop.py -- the castle map blueprint's top, for ModRestoreMapTop (src/dra/game_handlers.c).

The new game's opening movie (the Video Playback state) writes its decoded frames into the Korean SEL overlay's image buffer, whose tail
runs past 0x801E0000 (the castle map blueprint buffer) because the overlay is bigger than the retail one. The blueprint's first
L = stsel_BSS_END - 0x801E0000 bytes (4,780 at the time of writing) are lost for that session, and patch_sel_maptail.py (which refills them
when SEL loads) runs before the movie. DRA's func_800E4970 (every way out of a movie, and the common state change into loading)
calls ModRestoreMapTop, which copies the bytes back from a table at the start of g_ModDraPad (the DRA room pad, dead space).

This writes that table into build/us/DRA.BIN: 'R' '1', u16 LE length L, then (count 1..255, value) run-length pairs of F_MAP.BIN[0:L].
Nothing of the game is stored in the source tree: the bytes come from the disc's own F_MAP.BIN at build time.

  patch_dra_maptop.py --apply   write the table (prints "patched" when it changed the file)
  patch_dra_maptop.py --check   fail unless the table is in DRA.BIN as expected
Run from the repository root (build_ko_trial.sh does)."""
import subprocess
import sys

DRA = 'build/us/DRA.BIN'
MAP = 'build/us/F_MAP.BIN'
DRA_ELF = 'build/us/dra.elf'
SEL_ELF = 'build/us/stsel.elf'
DRA_BASE = 0x800A0000
MAP_BASE = 0x801E0000
NM = 'mipsel-linux-gnu-nm'
PAD_USE_MAX = 1024  # the pad is 3,680 bytes; the table is about 300


def sym(elf, name):
    out = subprocess.check_output([NM, elf], universal_newlines=True)
    for line in out.splitlines():
        p = line.split()
        if len(p) == 3 and p[2] == name:
            return int(p[0], 16)
    raise SystemExit('symbol %s not found in %s' % (name, elf))


def rle(b):
    out = bytearray()
    i = 0
    while i < len(b):
        v = b[i]
        n = 1
        while i + n < len(b) and b[i + n] == v and n < 255:
            n += 1
        out += bytes([n, v])
        i += n
    return bytes(out)


def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else '--check'
    m = open(MAP, 'rb').read()
    assert len(m) == 0x8000, ('F_MAP.BIN size', len(m))
    L = sym(SEL_ELF, 'stsel_BSS_END') - MAP_BASE
    assert 0 < L <= 0x4000, ('overlap length', L)
    table = b'R1' + L.to_bytes(2, 'little') + rle(m[:L])
    assert len(table) <= PAD_USE_MAX, ('table too big for the pad use limit', len(table))
    off = sym(DRA_ELF, 'g_ModDraPad') - DRA_BASE
    if mode == '--apply-to':
        # check_no_dirt.sh: put the same table into a copy of the linked dra.elf image, to compare it with DRA.BIN
        path = sys.argv[2]
        t = bytearray(open(path, 'rb').read())
        cur = bytes(t[off:off + len(table)])
        assert cur[0] == 1 and not any(cur[1:]), 'the linked pad is not the placeholder pad'
        t[off:off + len(table)] = table
        open(path, 'wb').write(t)
        return 0
    d = bytearray(open(DRA, 'rb').read())
    cur = bytes(d[off:off + len(table)])
    if cur == table:
        print('dra_maptop: table already in DRA.BIN (%d bytes, restores %d bytes)' % (len(table), L))
        return 0
    if mode == '--check':
        print('FAIL dra_maptop: the map-top table is not in DRA.BIN', file=sys.stderr)
        return 1
    # the pad is {1} followed by zeros: refuse to write over anything else
    assert cur[0] == 1 and not any(cur[1:]), 'g_ModDraPad is not the placeholder pad: the DRA layout changed, check src/dra/42398.c'
    d[off:off + len(table)] = table
    open(DRA, 'wb').write(d)
    print('dra_maptop: patched %d bytes into DRA.BIN at pad offset 0x%X (restores %d bytes of the blueprint top)' % (len(table), off, L))
    return 0


if __name__ == '__main__':
    sys.exit(main())
