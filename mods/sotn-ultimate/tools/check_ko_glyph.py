#!/usr/bin/env python3
# check_ko_glyph.py -- KO glyph block consistency checker (fail closed).
#   1. block: 30 B cells (original 16x15 1bpp), size == cells*30, cells == dra.h
#      KO_GLYPH_COUNT, sha256 == build_ko_trial KO_SHA, 776 + extension cells
#      <= 776 + 29 (the extension lives in the 896 B dead blob slot)
#   2. needed cells <= available: distinct codes in the map <= cells <= 960
#      (code space F040..F3FF) and every code < F040 + cells
#   3. glyph map: code <-> cell unique (no hangul/symbol cell sharing: the
#      nyeom / jim class of bug), no code outside the block
#   4. 1-byte promotion table == ko_promo_map.tsv, slots 0x80..0xEF, glyph codes
#      valid, table address inside the ARC zero gap and not on the engtab
#   5. hand-coded sources: every F0xx code decodes to a hangul that the
#      hand-coded character list expects (stale v13n/v12g code => FAIL)
import hashlib
from ko_paths import KO_WK  # noqa: E402
import io
import os
import re
import sys

WK = (KO_WK + "/P4시험/")
ROOT = os.environ.get("SOTN_KR", os.getcwd())
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ko_font_profile as _FP  # noqa: E402
CELL = _FP.P["cell_bytes"]  # 30 = original squeeze path, 23 = 12-col direct path
BLKNAME = _FP.P["blk_name"]
MAPNAME = _FP.P["map_name"]
BLOCK_BYTES = 0x5640  # 960 cells x 23 B: the glyph block slot behind it (0x8009E640..0x8009EAF0) holds the resident KO_PopupText
BASE = 0xF040
MAX_CELLS = 960
BASE_CELLS = 776
EXT_MAX = 29


def rd(p):
    m = []
    for ln in io.open(p, encoding="utf-8"):
        q = ln.rstrip(chr(10)).split(chr(9))
        if len(q) >= 2 and q[1].startswith("0x") and len(q[0]) == 1:
            m.append((q[0], int(q[1], 16)))
    return m


def fail(msg):
    print("FAIL", msg)
    sys.exit(1)


def main():
    blk = open(WK + BLKNAME, "rb").read()
    cells = len(blk) // CELL
    if len(blk) != cells * CELL:
        fail("block size %d not a multiple of %d" % (len(blk), CELL))
    if CELL == 30 and not (BASE_CELLS <= cells <= BASE_CELLS + EXT_MAX):
        fail("cells %d outside 776..%d (extension limited to the blob slot)" % (cells, BASE_CELLS + EXT_MAX))
    if CELL == 23 and not (BASE_CELLS <= cells and cells * CELL <= BLOCK_BYTES):
        fail("23 B cells: %d cells x 23 B must be >= 776 cells and fit the %d B block" % (cells, BLOCK_BYTES))
    sh = hashlib.sha256(blk).hexdigest()
    bs = io.open(os.path.join(ROOT, "mods/sotn-ultimate/tools/build_ko_trial.sh"), encoding="utf-8").read()
    m = re.search(r"KO_SHA=([0-9a-f]{64})", bs)
    if not m or m.group(1) != sh:
        fail("KO_SHA in build_ko_trial.sh != block sha %s" % sh)
    dra = io.open(os.path.join(ROOT, "src/dra/dra.h"), encoding="utf-8").read()
    cnt = int(re.search(r"#define KO_GLYPH_COUNT (\d+)", dra).group(1))
    cb = int(re.search(r"#define KO_GLYPH_CELL_BYTES (\d+)", dra).group(1))
    if cnt != cells or cb != CELL:
        fail("dra.h KO_GLYPH_COUNT %d / CELL_BYTES %d != block %d / %d" % (cnt, cb, cells, CELL))
    has12 = bool(re.search(r"(?m)^#define KO_GLYPH_12COL", dra))
    if has12 != (CELL == 23):
        fail("KO_GLYPH_12COL defined=%s but profile cell_bytes=%d" % (has12, CELL))
    mp = rd(WK + MAPNAME)
    codes = {}
    for ch, code in mp:
        if not (BASE <= code < BASE + cells):
            fail("code %04X of %s outside cells" % (code, ch))
        codes.setdefault(code, []).append(ch)
    dup = {c: v for c, v in codes.items() if len(v) > 1}
    if dup:
        fail("cell shared by several chars: %s" % {hex(c): "".join(v) for c, v in dup.items()})
    need = len(codes)
    print("cells %d (needed distinct %d) <= available %d (code space %d); extension cells %d <= %d"
          % (cells, need, cells, MAX_CELLS, cells - BASE_CELLS, EXT_MAX))
    if need > cells or cells > MAX_CELLS:
        fail("needed cells > available")
    # promotion table
    tbl = open(WK + "ko_promo_tbl.bin", "rb").read()
    if len(tbl) != 224:
        fail("promo table size")
    chars = dict(mp)
    n = 0
    for ln in io.open(WK + "ko_promo_map.tsv", encoding="utf-8").read().split(chr(10))[1:]:
        q = ln.split(chr(9))
        if len(q) < 4:
            continue
        slot = int(q[1], 16)
        code = int(q[2], 16)
        if not (0x80 <= slot <= 0xEF):
            fail("promo slot %02X" % slot)
        if chars.get(q[0]) != code:
            fail("promo %s code %04X != map" % (q[0], code))
        if int.from_bytes(tbl[(slot - 0x80) * 2:(slot - 0x80) * 2 + 2], "little") != code:
            fail("promo table slot %02X" % slot)
        n += 1
    pa = int(re.search(r"#define KO_PROMO_TBL_ADDR (0x[0-9A-Fa-f]+)", dra).group(1), 16)
    if not (0x8009EFBC <= pa and pa + 224 <= 0x8009F500):
        fail("promo table 0x%08X not inside the ARC zero gap 0x8009EFBC..0x8009F500" % pa)
    for f in os.listdir(os.path.join(ROOT, "src/st")):
        for g in ("cutscene.c", "e_cutscene_dialogue.c"):
            p = os.path.join(ROOT, "src/st", f, g)
            if os.path.exists(p):
                t = io.open(p, encoding="utf-8").read()
                if "KO_PROMO" in t or "1-byte promotion" in t:
                    if ("0x%08X" % pa).replace("0x", "0x") not in t and ("0x%08x" % pa) not in t:
                        fail("%s hook address != KO_PROMO_TBL_ADDR 0x%08X" % (p, pa))
    print("promotion slots", n, "OK, table at 0x%08X" % pa)
    # hand-coded sources
    exp = set(io.open(WK + "ko_handcoded_chars.txt", encoding="utf-8").read().strip())
    inv = {v: k for k, v in mp}
    pat = re.compile(re.escape(chr(92)) + "x([Ff][0-3])" + re.escape(chr(92)) + "x([0-9A-Fa-f]{2})")
    seen = set()
    for f in ("src/st/lib/e_shop.c", "src/st/lib/ko_lib_names.h", "src/st/sel/2C048.c"):
        t = io.open(os.path.join(ROOT, f), encoding="utf-8").read()
        for mm in pat.finditer(t):
            c = (int(mm.group(1), 16) << 8) | int(mm.group(2), 16)
            if not (BASE <= c <= 0xF3FF):
                continue
            ch = inv.get(c)
            if ch is None:
                fail("%s: code %04X is not in the profile glyph map" % (f, c))
            seen.add(ch)
    bad = [c for c in seen if "가" <= c <= "힣" and c not in exp]
    if bad:
        fail("hand-coded sources decode to hangul outside the expected set: %s" % "".join(bad))
    print("hand-coded sources: %d distinct hangul, all expected" % len([c for c in seen if '가' <= c <= '힣']))
    # LIB hand-coded string forms (the generators are outside this repo).
    # (a) KO bestiary descriptions (D_us_80180FB4, func_us_801B0C40 reads every
    #     non-letter byte as an SJIS pair): punctuation/digits must be full-width.
    # (b) KO enemy names (func_us_801B1064): space \x00, \xFF\x00 terminator.
    esc = re.compile(re.escape(chr(92)) + "x[0-9A-Fa-f]{2}")
    t = io.open(os.path.join(ROOT, "src/st/lib/e_shop.c"), encoding="utf-8").read()
    m = re.search(r"D_us_80180FB4\[\] = \{(.*?)\n\};", t, re.S)
    if not m:
        fail("e_shop.c: D_us_80180FB4 not found")
    for ln in m.group(1).split("\n"):
        q = re.match(r'\s*"(.*)",?\s*$', ln)
        if q and re.search(re.escape(chr(92)) + "x[Ff][0-3]", q.group(1)):
            if re.search(r"[0-9.,?!():]", esc.sub("", q.group(1))):
                fail("e_shop.c KO description has half-width punctuation/digit: %s" % ln.strip())
    t = io.open(os.path.join(ROOT, "src/st/lib/ko_lib_names.h"), encoding="utf-8").read()
    for q in re.finditer(r'^\s*"(.+)", //', t, re.M):
        s = q.group(1)
        if not s.endswith(chr(92) + "xFF" + chr(92) + "x00") or (chr(92) + "x20") in s:
            fail("ko_lib_names.h entry not in func_us_801B1064 form (\\x00 space, \\xFF\\x00 end): %s" % s)
    print("LIB KO descriptions full-width, enemy names FF00-terminated")
    # 6. font profile <-> C macros (one place for font-dependent values)
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import ko_font_profile as FP
    hdr = io.open(os.path.join(ROOT, "src/st/ko_font_params.h"), encoding="utf-8").read()
    mdy = re.search(r"#define KO_NAME_DY ([0-9]+)", hdr)
    if not mdy or int(mdy.group(1)) != FP.P["name_dy"]:
        fail("src/st/ko_font_params.h KO_NAME_DY != profile name_dy %d" % FP.P["name_dy"])
    mdx = re.search(r"#define KO_TEXT_DX ([0-9]+)", hdr)
    mew = re.search(r"#define KO_TEXT_EXTRA_W ([0-9]+)", hdr)
    if not mdx or int(mdx.group(1)) != FP.P["text_dx"]:
        fail("ko_font_params.h KO_TEXT_DX != profile text_dx %d" % FP.P["text_dx"])
    if not mew or int(mew.group(1)) != FP.P["text_extra_w"]:
        fail("ko_font_params.h KO_TEXT_EXTRA_W != profile text_extra_w %d" % FP.P["text_extra_w"])
    want_w = FP.P["panel_right"] - (FP.P["start_x"] + 4 + 8 - FP.P["text_dx"])
    if FP.P["window_px"] != want_w:
        fail("window_px %d != panel_right - text start = %d" % (FP.P["window_px"], want_w))
    if FP.P["window_px"] > 0xC0 + FP.P["text_extra_w"] - 8:
        fail("window_px %d exceeds the text sprite capacity %d" % (FP.P["window_px"], 0xC0 + FP.P["text_extra_w"] - 8))
    # ASCII advance: C macro KO_ENG_ADV (4 px units) == profile ascii_adv / ascii_px, and every dialogue file uses it
    adv = FP.P.get("ascii_adv", {})
    mm = re.search(r"#define KO_ENG_ADV[(]c[)](.*?)(?=#else)", hdr, re.S)
    if adv and not mm:
        fail("ko_font_params.h lacks KO_ENG_ADV but the profile has ascii_adv")
    if adv:
        body = mm.group(1)
        for ch, px in adv.items():
            lit = "'%s'" % ch
            if lit not in body:
                fail("KO_ENG_ADV lacks %s (profile advance %d px)" % (lit, px))
        if sorted(set(adv.values())) != [4]:
            fail("ascii_adv values must be 4 px (macro encodes 1 unit, default 2): %s" % adv)
        nuse = 0
        for f2 in os.listdir(os.path.join(ROOT, "src/st")):
            for g in ("cutscene.c", "e_cutscene_dialogue.c"):
                p2 = os.path.join(ROOT, "src/st", f2, g)
                if os.path.exists(p2):
                    t2 = io.open(p2, encoding="utf-8").read()
                    if "0x8009F000" in t2:
                        if "KO_ENG_ADV" not in t2 or "0x8009EF00" not in t2:
                            fail("%s: promotion hook present but no KO_ENG_ADV engtab hook" % p2)
                        nuse += 1
        print("ascii advance: KO_ENG_ADV ok in %d dialogue files, profile %s" % (nuse, adv))
    # 4 static dialogue lines: C macros (ko_font_params.h) == profile, every dialogue copy uses them
    mls = re.search(r"#define KO_LINE_STEP[(]dflt[)] ([0-9]+)", hdr)
    mlu = re.search(r"#define KO_LINE_UP ([0-9]+)", hdr)
    if not mls or int(mls.group(1)) != FP.P.get("line_step", 16):
        fail("ko_font_params.h KO_LINE_STEP != profile line_step %s" % FP.P.get("line_step"))
    if not mlu or int(mlu.group(1)) != FP.P.get("line_up", 0):
        fail("ko_font_params.h KO_LINE_UP != profile line_up %s" % FP.P.get("line_up"))
    if "#define KO_SCROLL_AT(y, orig) ((y) == 0)" not in hdr:
        fail("ko_font_params.h KO_SCROLL_AT is not the wrap-to-0 trigger")
    copies = ["cen/cutscene.c", "rcen/cutscene.c", "dai/e_cutscene_dialogue.c", "dre/e_cutscene_dialogue.c", "st0/cutscene.c",
              "lib/cutscene.c", "top/cutscene.c", "nz0/cutscene.c", "sel/cutscene.c", "no3/cutscene.c"]
    for cp in copies:
        t3 = io.open(os.path.join(ROOT, "src/st", cp), encoding="utf-8").read()
        if "KO_LINE_STEP(" not in t3 or "KO_SCROLL_AT(" not in t3:
            fail("%s: dialogue line break does not use KO_LINE_STEP / KO_SCROLL_AT" % cp)
    for hf in ("cutscene_script_text.h", "cutscene_unk1.h"):
        t3 = io.open(os.path.join(ROOT, "src/st", hf), encoding="utf-8").read()
        if "KO_LINE_UP" not in t3:
            fail("%s: first line y does not use KO_LINE_UP" % hf)
    print("dialogue lines: %d overlay copies use KO_LINE_STEP %d / KO_SCROLL_AT, first line up %d px, window_lines %d" % (len(copies), FP.P.get("line_step"), FP.P.get("line_up"), FP.P["window_lines"]))
    t4 = io.open(os.path.join(ROOT, "src/st/cutscene_unk4.h"), encoding="utf-8").read()
    if "prim->u1 = 0xC0 + KO_TEXT_EXTRA_W;" not in t4 or "startX + 4 - KO_TEXT_DX" not in t4:
        fail("cutscene_unk4.h text sprite no longer uses KO_TEXT_DX / KO_TEXT_EXTRA_W")
    # 7. big-label sheet (gen_ko_sheet2.py): dra.h addresses <-> ko_sheet2.bin, glyphs in the map
    sh2 = open(WK + "ko_sheet2.bin", "rb").read()
    base = int(re.search(r"#define KO_SHEET2_ADDR (0x[0-9A-Fa-f]+)", dra).group(1), 16)
    up2 = int(re.search(r"#define KO_SHEET_UPLOAD2 [(][(]const char[*][)](0x[0-9A-Fa-f]+)[)]", dra).group(1), 16)
    if up2 != base:
        fail("KO_SHEET_UPLOAD2 0x%X != KO_SHEET2_ADDR 0x%X" % (up2, base))
    codes2 = []
    i = 0
    while sh2[i] != 0:
        codes2.append(int.from_bytes(sh2[i:i + 2], "big"))
        i += 2
    inv2 = {v: k for k, v in mp}
    if not codes2 or any(c not in inv2 for c in codes2) or len(codes2) > 16:
        fail("sheet2 upload list: %d codes, some not in the glyph map" % len(codes2))
    seen = []
    for nm in ("ATK", "DEF", "HEAD", "BODY", "CLOAK", "OTHER", "HOLDER"):
        m5 = re.search(r"#define KO_SHEET_%s [(][(]const char[*][)](0x[0-9A-Fa-f]+)[)]" % nm, dra)
        if not m5:
            fail("dra.h lacks KO_SHEET_%s" % nm)
        off = int(m5.group(1), 16) - base
        if not (0 < off < len(sh2)):
            fail("KO_SHEET_%s outside ko_sheet2.bin" % nm)
        j = off
        idxs = []
        while sh2[j] != 0xFF:
            idxs.append(sh2[j]); j += 1
        if not idxs or any(x > 0x1F for x in idxs) or any(0x10 <= x and x - 0x10 >= len(codes2) for x in idxs):
            fail("KO_SHEET_%s index string %r out of range" % (nm, idxs))
        seen.append((nm, idxs))
    print("big-label sheet2: %d upload glyphs, labels %s OK" % (len(codes2), ",".join(n for n, _ in seen)))
    print("font profile %s: name_dy %d, window %d px x %d lines, space %d px OK" % (
        FP.ACTIVE, FP.P["name_dy"], FP.P["window_px"], FP.P["window_lines"], FP.P["space_px"]))
    # 8px equipment-name plate: every name character must have a plate cell whose code is its glyph-map code
    # (a missing one is drawn as a blank, a stale code as another glyph: 숏 / 발 / 멜 bugs)
    import struct
    pb = open(WK + "ko_8p_block_246.bin", "rb").read()
    pn = struct.unpack("<H", pb[:2])[0]
    pcodes = set(struct.unpack("<%dH" % pn, pb[2:2 + pn * 2]))
    eqc = io.open(os.path.join(ROOT, "mods/sotn-ultimate/src/menu/eq_core.c"), encoding="utf-8").read()
    pinned = re.search(r"if [(]n == ([0-9]+)[)]", eqc)
    if not pinned or int(pinned.group(1)) != pn or len(pb) != 2 + pn * 10 or len(pb) > 4096:
        fail("8px plate: count %d / size %d, eq_core.c KO_8pValid pins %s" % (pn, len(pb), pinned and pinned.group(1)))
    codes13 = dict(mp)
    miss = []
    for ln in io.open((KO_WK + "/번역/번역_v4_장비.tsv"), encoding="utf-8"):
        q = ln.rstrip(chr(10)).split(chr(9))
        if len(q) > 3 and q[0].endswith(":name"):
            for ch in q[3]:
                if ch == " ":
                    continue
                code = codes13.get(ch) if "가" <= ch <= "힣" else ord(ch)
                if code is None or code not in pcodes:
                    miss.append((q[0].split(":")[2], q[3], ch))
    if miss:
        fail("equipment names with characters missing from the 8px plate: %s" % miss[:12])
    print("8px plate: %d cells," % pn + " every equipment-name character has a cell with its v13p code")
    print("check_ko_glyph OK sha", sh[:12])


if __name__ == "__main__":
    main()
