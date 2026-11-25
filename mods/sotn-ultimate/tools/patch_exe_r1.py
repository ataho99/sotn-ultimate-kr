#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
# Writes the easy-controls blob (build_r1.sh) into build/us/main.exe over two stretches of Psy-Q library code that nothing calls:
#   F1 0x80025F08..0x800264A0  SsUtKeyOn / SsUtKeyOff            (1,432 B)
#   F2 0x8002683C..0x80026D4C  SsUtKeyOffV ... SsUtGetDetVVol    (1,296 B)
# (no jal / j / pointer / lui+addiu reference in 150 disc and built files; filled with break and the game played: never executed)
# The rest of each stretch is zeroed. The exe's crt0 and everything else stay retail.
import struct, sys
EXE = 'build/us/main.exe'
FR = [(0x80025F08, 0x800264A0, 'build/us/mod/r1/r1a.bin'), (0x8002683C, 0x80026D4C, 'build/us/mod/r1/r1b.bin')]
b = bytearray(open(EXE, 'rb').read())
load = struct.unpack_from('<I', b, 0x18)[0]
assert load == 0x80010000, hex(load)
off = lambda a: 0x800 + a - load
retail = open('disks/us/SLUS_000.67', 'rb').read()
# an earlier build (095_37) patched this exe in place a different way (the crt0 BSS-clear start + a blob in the primitive pool's tail,
# which DRA clears): undo that, the exe must differ from retail only in the two stretches below
old_crt0 = off(0x80010DFC)
if struct.unpack_from('<2I', b, old_crt0) == (0x3C02800A, 0x244288A0):
    struct.pack_into('<2I', b, old_crt0, 0x3C028003, 0x244239E4)
    print('exe: crt0 words restored to retail')
b[off(0x8008B190):off(0x80096D38)] = bytes(0x80096D38 - 0x8008B190)
if '--restore' in sys.argv:
    # the exe outside the KO tail is retail: put the two stretches back (the crt0 words and R1 were restored / zeroed above)
    for lo, hi, path in FR:
        b[off(lo):off(hi)] = retail[off(lo):off(hi)]
    open(EXE, 'wb').write(bytes(b))
    print('exe: the two Psy-Q stretches, the crt0 words and R1 are retail again')
    sys.exit(0)
for lo, hi, path in FR:
    blob = open(path, 'rb').read()
    assert len(blob) <= hi - lo, (path, len(blob), hi - lo)
    seg = bytes(b[off(lo):off(hi)])
    assert seg == retail[off(lo):off(hi)] or seg[:len(blob)] == blob, 'the exe at 0x%X is neither retail nor this blob' % lo
    b[off(lo):off(hi)] = blob + bytes(hi - lo - len(blob))
    print('exe: 0x%08X..0x%08X <- %s (%d of %d B)' % (lo, hi, path.split('/')[-1], len(blob), hi - lo))
open(EXE, 'wb').write(bytes(b))
