#!/usr/bin/env python3
# gen_ko_glyph_f.py -- glyph block v14e (950 cells) -> v14f: append ONE new Hangul cell (폐, used by the CHI place name 폐광).
# Same render() recipe as gen_ko_glyph_lat.py (MGalmuri12 12x15, 23 B cell). Existing cells and codes are untouched;
# the new cell is appended behind the last cell (no reuse of a freed cell). The low byte of the new code must not be
# 0x00 / 0xFF (NUL / terminator inside hand-coded strings). Limit: 960 cells (KO_POP_TEXT at 0x5640).
import sys, io, os, hashlib
TOOLS = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, TOOLS)
import gen_ko_glyph_lat as G  # noqa: E402
P4, CB, BASE, LIMIT = G.P4, 23, 0xF040, 960
NEW = sys.argv[1] if len(sys.argv) > 1 else "폐"
blk = open(P4 + "ko_glyph_block_mgalmuri12_v14e.bin", "rb").read()
assert len(blk) % CB == 0
ncell = len(blk) // CB
mp = {}
for ln in io.open(P4 + "ko_glyph_map_v14e.tsv", encoding="utf-8"):
    q = ln.rstrip("\n").split("\t")
    if len(q) >= 2 and q[1].startswith("0x") and len(q[0]) == 1:
        mp[q[0]] = int(q[1], 16)
out = bytearray(blk)
for k, c in enumerate(NEW):
    assert c not in mp, ("already mapped", c)
    code = BASE + ncell + k
    assert (code & 0xFF) not in (0x00, 0xFF), hex(code)
    d = G.render(c)
    assert d.any(), ("no ink", c)
    out += G.pack(d)
    mp[c] = code
    print("new glyph %s U+%04X code 0x%04X cell %d" % (c, ord(c), code, ncell + k))
assert len(out) // CB <= LIMIT, len(out) // CB
assert out[:len(blk)] == blk
open(P4 + "ko_glyph_block_mgalmuri12_v14f.bin", "wb").write(bytes(out))
with io.open(P4 + "ko_glyph_map_v14f.tsv", "w", encoding="utf-8", newline="") as f:
    f.write("글자\t코드\tn\t오프셋\n")
    for ch, code in sorted(mp.items(), key=lambda x: x[1]):
        f.write("%s\t0x%04X\t%d\t%d\n" % (ch, code, code - BASE, (code - BASE) * CB))
print("v14f block %d cells %d B sha256 %s" % (len(out) // CB, len(out), hashlib.sha256(bytes(out)).hexdigest()))
print("NEXT: dra.h KO_GLYPH_COUNT %d; build_ko_trial.sh KO_SHA = sha above" % (len(out) // CB))
