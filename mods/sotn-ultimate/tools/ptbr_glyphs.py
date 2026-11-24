
ORDER = "áàâãéêíóôõúçüªºÉÓÔÁÍÃÇ"
CELL0 = 0xA1          # 8x8 cells 0xA1..0xB1, then 0xB3 (avoid 0xA0 / 0xB2 =
                      # raw 0xC0 / 0xD2, MenuDrawStr's recolour marker)
KCODE0 = 0x8540       # custom Shift-JIS codes 0x8540..0x8555


def cell(i):
    """8x8 sheet cell of ORDER[i]"""
    return CELL0 + i if i < 17 else 0xB3 + i - 17

# ------------------------------------------------------------------ 8x8
ACC8 = {  # rows 0-1 above the lowercase (x-height starts at row 2)
    "acute": ["....6...", "...6...."],
    "grave": ["..6.....", "...6...."],
    "circ": ["...66...", "..6..6.."],
    "tilde": ["..66.6..", ".6..6..."],
    "uml": [".6....6.", "........"],
}


def font8(base, ch):
    """base(c) -> 8 strings of the original cell for ASCII char c."""
    kind = {"á": "acute", "é": "acute", "í": "acute", "ó": "acute", "ú": "acute",
            "à": "grave", "â": "circ", "ê": "circ", "ô": "circ",
            "ã": "tilde", "õ": "tilde", "ü": "uml"}.get(ch)
    if kind:
        plain = {"á": "a", "à": "a", "â": "a", "ã": "a", "é": "e", "ê": "e",
                 "í": "i", "ó": "o", "ô": "o", "õ": "o", "ú": "u", "ü": "u"}[ch]
        g = list(base(plain))
        a = ACC8[kind]
        if ch == "í":
            a = [".....6..", "....6..."]
        g[0], g[1] = a
        return g
    if ch == "ç":
        g = list(base("c"))
        g[7] = "...66..."
        return g
    if ch == "ª":
        return ["..66....", "....6...", "..666...", ".6..6...", "..666...",
                "........", ".6666...", "........"]
    if ch == "º":
        return ["..66....", ".6..6...", ".6..6...", "..66....", "........",
                ".6666...", "........", "........"]
    if ch == "É":  # 6-row body (rows 1-6), accent on row 0
        return ["....66..", ".666666.", ".6......", ".6666...", ".6......",
                ".6......", ".666666.", "........"]
    if ch == "Ó":
        return ["....66..", "..6666..", ".6....6.", ".6....6.", ".6....6.",
                ".6....6.", "..6666..", "........"]
    if ch == "Í":
        return ["....66..", "...666..", "....6...", "....6...", "....6...",
                "....6...", "...666..", "........"]
    if ch == "Ã":
        return ["..66.66.", "...66...", "..6..6..", ".6....6.", ".666666.",
                ".6....6.", ".6....6.", "........"]
    if ch == "Ç":
        g = list(base("C"))
        g[7] = "...66..."
        return g
    if ch == "Á":  # 6-row body like É / Ó
        return ["....66..", "...66...", "..6..6..", ".6....6.", ".666666.",
                ".6....6.", ".6....6.", "........"]
    if ch == "Ô":  # a centred hat (Ó's accent sits right of centre)
        return ["...66...", "..6666..", ".6....6.", ".6....6.", ".6....6.",
                ".6....6.", "..6666..", "........"]
    raise KeyError(ch)


# ------------------------------------------------------------------ Kanji ROM
# rows: cap top = 1, lowercase top = 5; accents live on rows 1-3, row 4 empty
ACCK = {
    "acute": {1: [8], 2: [7], 3: [6]},
    "grave": {1: [5], 2: [6], 3: [7]},
    "circ": {2: [6, 7], 3: [5, 8]},
    "tilde": {2: [5, 6, 9], 3: [4, 7, 8]},
    "uml": {3: [4, 10]},
}
SHIFT = {"a": 0, "e": 0, "o": 0, "u": 1, "i": 1}   # centre column offsets


def krom(base, ch, variant=3):
    """base(c) -> 15 strings of 16 chars ("#"/".") for ASCII letter c."""
    acc = {"á": ("a", "acute"), "à": ("a", "grave"), "â": ("a", "circ"),
           "ã": ("a", "tilde"), "é": ("e", "acute"), "ê": ("e", "circ"),
           "í": ("i", "acute"), "ó": ("o", "acute"), "ô": ("o", "circ"),
           "õ": ("o", "tilde"), "ú": ("u", "acute"), "ü": ("u", "uml")}
    if ch in acc:
        plain, kind = acc[ch]
        g = [list(r) for r in base(plain)]
        if plain == "i":
            for r in (1, 2):
                g[r] = ["."] * 16
        dx = SHIFT[plain]
        for r, cols in ACCK[kind].items():
            for c in cols:
                g[r][c + dx] = "#"
        return ["".join(r) for r in g]
    if ch == "ç":  # c body one row shorter (rows 5-12), cedilla rows 13-14
        g = list(base("c"))
        body = g[5:9] + g[10:14]
        return g[:5] + body + ["......#.........", ".....##........."] 
    if ch in "ªº":
        small = {"ª": ["....###.........", ".......#........", ".....####.......",
                       "....#...#.......", ".....####.......", "................",
                       "....#####......."],
                 "º": [".....###........", "....#...#.......", "....#...#.......",
                       "....#...#.......", ".....###........", "................",
                       "....#####......."]}[ch]
        return ["." * 16] + small + ["." * 16] * 7
    if ch in "ÍÃÇ":
        b = {"Í": "I", "Ã": "A", "Ç": "C"}[ch]
        out = [list(r) for r in shorten(base(b), b)]
        if ch == "Í":
            out[0][9] = out[1][8] = "#"
        elif ch == "Ã":
            for c in (6, 7, 10):
                out[0][c] = "#"
            for c in (5, 8, 9):
                out[1][c] = "#"
        else:
            out[14][6] = out[14][7] = "#"
        return ["".join(r) for r in out]
    if ch in "ÉÓÔÁ":
        g = list(base({"É": "E", "Á": "A"}.get(ch, "O")))
        body = g[1:14]                      # 13 rows
        cut = 3 if variant == 2 else 2      # rows removed from the body
        if ch == "Á":                       # the apex and leg repeats
            drop = [1, 8]
        elif ch == "É":                     # drop rows from the two stems
            drop = ({2: [4, 9], 3: [3, 9, 10]}[cut] if variant != 3 else
                    [9, 10])                # middle bar stays centred
        else:                               # drop from the straight sides
            drop = {2: [6, 7], 3: [5, 6, 7]}[cut]
        body = [r for i, r in enumerate(body) if i not in drop]
        top = 1 + cut
        out = ["." * 16] * top + body + ["." * 16]
        out = out[:15]
        a = [list("." * 16) for _ in range(top)]
        if variant == 1:                    # one-row accent on row 1
            a[1][8] = a[1][9] = "#"
        elif variant == 2:                  # two-row accent rows 1-2
            a[1][9] = "#"
            a[2][8] = "#"
        elif ch == "Ô":                     # circumflex, rows 0-1
            a[0][7] = a[0][8] = "#"
            a[1][6] = a[1][9] = "#"
        else:                               # accent rows 0-1, one row
            a[0][9] = "#"                   # above the capitals top
            a[1][8] = "#"
        for r in range(0 if variant == 3 else 1, top):
            out[r] = "".join(a[r])
        return out
    raise KeyError(ch)


KROM2_FIRST, KROM2_COUNT = 0x824F, 43
# rows (0 = cap top) removed from each 13-row body; None = automatic
DROP = {}


def shorten(rows, ch=None):
    """15 rows (cap top on row 1) -> body moved down to rows 3-13."""
    body = rows[1:14]
    pick = DROP.get(ch)
    if pick is None:
        rep = [i for i in range(1, 13) if body[i] == body[i - 1]]
        if len(rep) >= 2:
            up = [i for i in rep if i <= 6] or rep
            lo = [i for i in rep if i > 6] or rep
            a = up[len(up) // 2]
            b = lo[len(lo) // 2]
            if b == a:
                b = [i for i in rep if i != a][0]
            pick = [a, b]
        else:
            pick = sorted(range(1, 13), key=lambda i: sum(
                x != y for x, y in zip(body[i], body[i - 1])))[:2]
    body = [r for i, r in enumerate(body) if i not in pick]
    return ["." * 16] * 3 + body + ["." * 16]


def krom2(base_code):
    """The 43 KROM2 glyphs; base_code(code) -> 15 rows of the ROM glyph."""
    out = []
    for k in range(KROM2_COUNT):
        code = KROM2_FIRST + k
        if 0x8259 <= code <= 0x825F:          # no characters there
            out.append(["." * 16] * 15)
            continue
        ch = chr(ord("0") + code - 0x824F) if code < 0x8259 else chr(ord("A") + code - 0x8260)
        out.append(shorten(base_code(code), ch))
    return out
