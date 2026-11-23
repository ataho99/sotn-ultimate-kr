#!/usr/bin/env python3
# gen_drama_sub.py -- the Drama cd screen's subtitle TEST strings (094_21): the prologue scroll's own Korean lines, no transcription.
# -> mods/sotn-ultimate/src/menu/mod_drama_sub_str.h  (12px glyph codes F0xx big endian, ASCII raw, space 0x20; plain NUL-terminated
# C strings in SEL's .rodata: the ARC_KO A-gap that holds the other sound strings has too little room)
# Not run by the build (the header is checked in); run it by hand after editing LINES:  python3 mods/sotn-ultimate/tools/gen_drama_sub.py
import io
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from ko_paths import KO_WK  # noqa: E402
import ko_font_profile as FP  # noqa: E402

REPO = os.path.abspath(os.path.join(HERE, '..', '..', '..'))
OUT = os.path.join(REPO, 'mods/sotn-ultimate/src/menu/mod_drama_sub_str.h')
P4 = KO_WK + '/P4시험/'
# (start second after the track starts, text)
LINES = [
    (0, '쓰러뜨리는 데 성공했다.'),
    (5, '그러나 4년 뒤 어느 날 밤,'),
    (10, '휘영청 밝은 보름달 아래'),
    (15, '리히터는 수수께끼처럼 사라졌다.'),
    (20, '어디서부터 찾아야 할지'),
    (25, '모른 채 마리아 레넷드는'),
    (30, '그를 찾아 길을 나섰다.'),
    (35, '그때 운명이 끼어들었다.'),
]


def rd(path):
    m = {}
    for ln in io.open(path, encoding='utf-8'):
        q = ln.rstrip('\n').split('\t')
        if len(q) >= 2 and len(q[0]) == 1:
            try:
                m[q[0]] = int(q[1], 16)
            except ValueError:
                pass
    return m


def esc(b):
    return ''.join('\\x%02X' % x for x in b)


mp = rd(P4 + FP.P['map_name'])
out = ['// gen_drama_sub.py: sound-mode subtitle test strings (do not edit)']
names = []
for i, (t, s) in enumerate(LINES):
    b = bytearray()
    for ch in s:
        if ch == ' ':
            b.append(0x20)
        elif ord(ch) < 128:
            b.append(ord(ch))
        else:
            assert ch in mp, ('no glyph code', ch, s)
            b += bytes([mp[ch] >> 8, mp[ch] & 0xFF])
    n = 'ko_dsub%d' % i
    out.append('static const char %s[] = "%s";' % (n, esc(b)))
    names.append((t, n))
out.append('#define MOD_DSUB_N %d' % len(LINES))
out.append('static const char* const g_ModDsubStr[MOD_DSUB_N] = {%s};' % ', '.join(n for _, n in names))
out.append('static const u16 g_ModDsubSec[MOD_DSUB_N] = {%s};' % ', '.join(str(t) for t, _ in names))
io.open(OUT, 'w', encoding='utf-8', newline='').write('\n'.join(out) + '\n')
print('wrote', OUT, len(LINES), 'lines')
