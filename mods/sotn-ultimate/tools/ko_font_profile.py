#!/usr/bin/env python3
# ko_font_profile.py -- the ONE place for font-dependent values of the KO build.
#
# A different dialogue font later means: edit (or add) a profile here, rerun
#   gen_ko_glyph16.py  (cells)      gen_ko_wrap.py (line breaks / windows)
# and mirror the two C macros in src/st/ko_font_params.h (check_ko_glyph.py
# fails if they disagree). Nothing else may hold these numbers.
#
# Branch record: see 인계문서 「폰트 분기표」.
import os
from ko_paths import KO_FONTS  # noqa: E402

PROFILES = {
    "isunshinR16": dict(
        label="이순신 Regular_ots 16px",
        ttf=(KO_FONTS + "/이순신Regular_ots.ttf"),
        size=16,
        supersample=16,
        coverage_threshold=0.30,
        vert_offset=-1,        # glyph rows: cell row = font row + vert_offset
        name_dy=0,             # speaker name moved UP by this many px (C: KO_NAME_DY); 4 earlier (workaround), 0 since the trailing-LINE_BREAK scroll cause is fixed
        panel_right=248,       # inner right border of the dialogue panel (G4 prim x1, cutscene_script_text.h)
        text_dx=12,            # text line sprite starts this many px further left (C: KO_TEXT_DX)
        text_extra_w=8,        # ... and is this many px wider (C: KO_TEXT_EXTRA_W); sprite width 0xC0 + this
        start_x=60,            # SET_POS x of every cutscene script
        window_px=188,         # = panel_right - (start_x + 4 + 8 - text_dx); earlier builds used 192 (text clipped/over the border)
        window_lines=3,        # lines per dialogue window (then NEXT_DIALOG)
        space_px=4,            # space advance (cutscene render hooks: nextCharX += 1)
        ascii_px=8,            # ASCII glyph advance (8x8 plate, nextCharX += 2)
        glyph_px=12,           # Hangul / symbol cell advance (nextCharX += 3)
        cell_bytes=30,         # 16x15 1bpp cells, original squeeze path (KO_GLYPH_12COL off)
        blk_name="ko_glyph_block_isunshin16_v13p.bin",
        map_name="ko_glyph_map_v13p.tsv",
    ),
    "mgalmuri12": dict(
        label="MGalmuri12 12px, an-na (12-col direct) + a (kind0)",
        ttf=(KO_FONTS + "/MonadABXY/mgalmuri-font/_ttf/MGalmuri12.ttf"),
        size=12,
        supersample=1,
        coverage_threshold=0.5,
        vert_offset=1,         # mono render: cell row = font row + 1 (v12 layout)
        name_dy=0,
        panel_right=248,
        text_dx=12,
        text_extra_w=8,
        start_x=60,
        window_px=188,
        window_lines=4,        # 4 static lines (KO_LINE_STEP 13; scroll only from the 5th line)
        line_step=13,          # px between lines (C: KO_LINE_STEP, src/st/ko_font_params.h)
        line_up=4,             # first line moves up this many px (C: KO_LINE_UP)
        reflow=True,           # rows over window_lines are re-flowed ignoring the translator's "/" (gen_ko_wrap)
        space_px=4,
        ascii_px=8,
        glyph_px=12,           # 12-col cell, advance 12
        ascii_adv={".": 4, ",": 4, "!": 4},  # KO_ENG_ADV (ko_font_params.h): engtab cells, per-glyph advance (px)
        cell_bytes=23,         # 12 cols x 15 rows 1bpp, 180 bits (KO_GLYPH_12COL on)
        blk_name="ko_glyph_block_mgalmuri12_v14h.bin",  # v14g + 6 Hangul cells F3F8..F3FD (꿀낄뎌떤앉픔, song subtitles; gen_ko_glyph_h.py)
        map_name="ko_glyph_map_v14h.tsv",
    ),
}
ACTIVE = os.environ.get("KO_FONT_PROFILE", "mgalmuri12")
P = PROFILES[ACTIVE]


def char_px(ch):
    """advance of one character on the dialogue line"""
    if ch == " ":
        return P["space_px"]
    if ord(ch) < 128:
        return P.get("ascii_adv", {}).get(ch, P["ascii_px"])
    return P["glyph_px"]


def text_px(s):
    return sum(char_px(c) for c in s)


def norm(s):
    """same dot-run rule as gen_ko_splice.ell: 9 dots -> two cells, 3 dots -> one cell"""
    s = s.replace("\u2026", "...")
    return s.replace(".........", "\u2026\u2026").replace("...", "\u2026")


def text_px_norm(s):
    return text_px(norm(s))
