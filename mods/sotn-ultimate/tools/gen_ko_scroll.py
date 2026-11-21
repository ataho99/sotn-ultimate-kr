# gen_ko_scroll.py -- writes src/st/st0/prologue_scroll_ko.h (KO prologue scroll blob) and the first-line X in 3101C_psx.c
# from ko_data/번역/프롤로그스크롤_JP계열.tsv (39 lines, 10 paragraphs, same line structure as the Japanese scroll).
# Width model 0.9.4 (A3): glyph 6 words, space 2, comma/period 3, 0x3C jump, +8 when a position lands in 0x38..0x3F.
# usage: python3 mods/sotn-ultimate/tools/gen_ko_scroll.py
import os, sys, re
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ko_text_enc as K

import os
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../.."))
H = os.path.join(ROOT, "src/st/st0/prologue_scroll_ko.h")
C = os.path.join(ROOT, "src/st/st0/3101C_psx.c")
TSV = os.path.join(ROOT, "ko_data/번역/프롤로그스크롤_JP계열.tsv")
SUBST = {"곧": "즉"}   # '곧' has no cell in the v14f glyph block; '즉' has one (same meaning here)

gm = K.glyph_map()


GW, SW, PW = 6, 2, 3   # 0.9.4 (A3): Hangul/Latin glyph 6 words (24px box), space 2 (8px), comma/period 3 (12px)
PUNCT = ",."


def layout(text, start):
    """(positions, end X) in words for a line starting at X=start. Same rules as 3101C_psx.c ScrollPutGlyph:
    a glyph never crosses word 0x3C (SPRT0 width): it jumps to 0x40; after each char a position inside 0x38..0x3F is pushed +8"""
    x = start
    pos = []
    for ch in text:
        if ch == " ":
            x += SW
        else:
            w = PW if ch in PUNCT else GW
            if x < 0x3C and x + w > 0x3C:
                x = 0x40
            pos.append((ch, x, w))
            x += w
        if 0x38 <= x < 0x40:
            x += 8
    return pos, x


def width_end(text, start):
    return layout(text, start)[1]


def best_start(text):
    best = None
    for s in range(0, 0x3C):
        e = width_end(text, s)
        if e > 0x78:
            continue
        err = abs((s + e) / 2 - 0x3C)
        if best is None or err < best[0] - 1e-9:
            best = (err, s, e)
    return best


def old_blob_lines():
    t = open(H, encoding="utf-8").read()
    m = re.search(r"prologue_scroll_ko_psx\[\] = \{(.*?)\};", t, re.S)
    v = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", m.group(1))]
    lines, cur, i = [], [], 0
    starts = [0x16]
    while i < len(v):
        c = v[i]
        if 0xF0 <= c <= 0xF3:
            cur.append(("G", v[i:i + 2])); i += 2
        elif c >= 0x20:
            cur.append(("A", chr(c))); i += 1
        elif c in (1, 2):
            lines.append(cur); cur = []; starts.append(v[i + 1]); i += 2
        else:
            i += 1
    lines.append(cur)
    return lines, starts


if __name__ == "__main__":
    rows = [l.rstrip("\n").split("\t") for l in open(TSV, encoding="utf-8")][1:]
    assert len(rows) == 39
    paras = [r[5].strip() != "" for r in rows]   # last column marks paragraph end (¶)
    print("paragraph-end rows:", [i for i, p in enumerate(paras) if p], "count", sum(paras))
    lines = []
    for r in rows:
        txt = r[3]
        for a, b in SUBST.items():
            txt = txt.replace(a, b)
        lines.append(txt)
    # encode
    data, starts = [], []
    CTRL = []
    for i, txt in enumerate(lines):
        bs = []
        for ch in txt:
            if K.is_hangul(ch):
                code = gm[ch]
                if (code & 0xFF) in (0, 1, 2): CTRL.append((i, ch, hex(code)))
                bs += [code >> 8, code & 0xFF]
            else:
                assert ch in " ,.4C", "unexpected char %r" % ch
                bs.append(ord(ch))
        assert sum(1 for ch in txt if ch != ' ') <= 24, 'line %d has more than 24 glyphs (render buffer slots)' % i
        b = best_start(txt)
        assert b is not None, "line %d does not fit" % i
        starts.append(b[1])
        data.append((bs, b))
    print('control-valued low bytes:', CTRL)
    # the scroll renderer (3101C_psx.c func_801B1298) reads a glyph as first byte >= 0x80 + any second byte, so low bytes 0/1/2 are safe here
    out = []
    for i, (bs, b) in enumerate(data):
        out += bs
        if i < len(data) - 1:
            out += [2 if paras[i] else 1, starts[i + 1]]
    out.append(0)
    slots = len(lines) + sum(paras[:-1])
    print("lines", len(lines), "slots", slots, "bytes", len(out), "(old 902)")
    print("start offsets:", " ".join("%02X" % s for s in starts))
    worst = max(b[0] for _, b in data)
    print("start offsets hex:", " ".join(hex(x) for x in starts))
    print("max centre error (words)", worst, "=", worst * 4, "px; max end X", max(b[2] for _, b in data))
    t = open(H, encoding="utf-8").read()
    m = re.search(r"(prologue_scroll_ko_psx\[\] = \{)(.*?)(\};)", t, re.S)
    body = "\n" + ",\n".join(",".join("0x%02X" % x for x in out[i:i + 24]) for i in range(0, len(out), 24)) + "\n"
    t2 = t[:m.start(2)] + body + t[m.end(2):]
    t2 = re.sub(r"// X offsets \(0\.9\.2\):[^\n]*", "// 0.9.2: JP-line-structure translation (39 lines, 10 paragraphs), per-line centred X offsets (screen 0x78 words, glyph 6, space 2, comma/period 3; A3).", t2, count=1)
    open(H, "w", encoding="utf-8").write(t2)
    c = open(C, encoding="utf-8").read()
    m2 = re.search(r"g_Dialogue\.startY = 0x[0-9A-Fa-f]+;[^\n]*", c)
    c2 = c[:m2.start()] + "g_Dialogue.startY = 0x%X; // 0.9.2: centred first KO line (0x200 + X of line 0)" % (0x200 + starts[0]) + c[m2.end():]
    open(C, "w", encoding="utf-8").write(c2)
    print("written; line0 X", hex(starts[0]), "startY", hex(0x200 + starts[0]))
