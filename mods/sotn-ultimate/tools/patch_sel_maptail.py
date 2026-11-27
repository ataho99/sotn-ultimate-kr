#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""patch_sel_maptail.py -- put the castle map blueprint's top back after the file select overlay loads.

The castle map blueprint (F_MAP.BIN, 32 KB) lives at CASTLE_MAP_PTR = 0x801E0000 and is read once at boot. The Korean build's SEL overlay
(0x80180000, SEL.BIN) is 4.7 KB bigger than the retail one, so its end (zero-filled bss: the movie decoder buffer's tail, src/st/sel/bss.c)
reaches past 0x801E0000. Loading SEL then writes those zeros over the blueprint's first 4,780 bytes and nothing reads F_MAP.BIN again:
the top map rows lost their cells and showed as flat grey blocks (3877).

This writes the blueprint's own first bytes into the file's tail instead of zeros, so the SEL load puts them back where they were.
The tail is only that decoder buffer's tail (written before it is read), so its content does not matter to SEL.

  patch_sel_maptail.py --apply   patch build/us/SEL.BIN in place (prints "patched" when it changed the file)
  patch_sel_maptail.py --check   fail unless the tail equals the blueprint's first bytes
Run from the repository root (build_ko_trial.sh does).
"""
import sys

SEL = 'build/us/SEL.BIN'
MAP = 'build/us/F_MAP.BIN'  # the one on the disc (paint_fmap.py output)
BASE = 0x60000  # CASTLE_MAP_PTR 0x801E0000 - 0x80180000


def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else '--check'
    d = bytearray(open(SEL, 'rb').read())
    m = open(MAP, 'rb').read()
    assert len(m) == 0x8000, ('F_MAP.BIN size', len(m))
    if len(d) <= BASE:
        print('sel_maptail: SEL.BIN ends below 0x801E0000, nothing to do')
        return 0
    n = len(d) - BASE
    assert n <= len(m), ('SEL tail longer than the map buffer', n)
    want = m[:n]
    cur = bytes(d[BASE:])
    if cur == want:
        print('sel_maptail: tail already holds the map blueprint top (%d bytes)' % n)
        return 0
    if mode == '--check':
        print('FAIL sel_maptail: SEL.BIN tail (%d bytes above 0x801E0000) is not the map blueprint top' % n, file=sys.stderr)
        return 1
    # the tail must be the zero-filled bss; anything else means the overlay layout changed: stop and look
    assert not any(cur), 'SEL tail is not zero-filled: the SEL layout changed, check src/st/sel/bss.c'
    d[BASE:] = want
    open(SEL, 'wb').write(d)
    print('sel_maptail: patched %d bytes of SEL.BIN (0x801E0000..0x%08X) with F_MAP.BIN[0:%d]' % (n, 0x801E0000 + n, n))
    return 0


if __name__ == '__main__':
    sys.exit(main())
