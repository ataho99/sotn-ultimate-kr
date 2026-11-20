// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
// Font-dependent values of the KO dialogue, C side. The python side lives in
// mods/sotn-ultimate/tools/ko_font_profile.py (check_ko_glyph.py fails when the
// two disagree). A different dialogue font => new profile + update this file.
//   KO_NAME_DY: speaker name (8px plate) is drawn this many px higher than
//               retail, so the first dialogue line (glyph rows start at row 0
//               of the cell with the Isunshin 16px font) does not touch it.
//   profile isunshinR16: KO_NAME_DY = 0 (sihum34 used 4 as a workaround; the real cause, trailing LINE_BREAK rows scrolling the box, is fixed in gen_ko_splice.py)
//   KO_TEXT_DX / KO_TEXT_EXTRA_W: dialogue text line sprite starts DX px further left
//               and is EXTRA_W px wider. Retail text starts at startX+4+8 (nextLineX 2
//               words = 8 px blank) = 72 px and the panel's inner right border is at x
//               248, so only 176 px were usable and a 188 px line spilled over the border.
//               profile isunshinR16: DX 12 -> text starts at x 60 (portrait right edge
//               ~x 53, 7 px gap), EXTRA_W 8 -> sprite width 200 (text capacity 192),
//               window_px 188 = 248 - 60.
#ifdef MOD_KO
/* 4 static dialogue lines (sihum39): line step 13 px (glyph cells are 15 rows; the 16 px text sprites
 * overlap on blank rows), the first line moves up KO_LINE_UP px so the gap below the speaker name and the
 * gap above the window's lower border are about equal, and the scroll mode (unk12 bit 0) starts only when
 * the line counter wraps to 0 = the 5th line (the original KO/JP code started it at the 4th line). */
#define KO_LINE_STEP(dflt) 13
#define KO_LINE_UP 4
#define KO_SCROLL_AT(y, orig) ((y) == 0)
/* ASCII in KO dialogue (KO_ENGTAB cells): advance in 4 px units; . , ! carry ink in the
 * left 1-2 columns only -> 4 px, everything else 8 px (% M m w have 9 px ink but never occur in dialogue). Mirror: ko_font_profile.py ascii_adv. */
#define KO_ENG_ADV(c) (((c) == '.' || (c) == ',' || (c) == '!') ? 1 : 2)
#define KO_NAME_DY 0
#define KO_TEXT_DX 12
#define KO_TEXT_EXTRA_W 8
#else
#define KO_LINE_STEP(dflt) (dflt)
#define KO_LINE_UP 0
#define KO_SCROLL_AT(y, orig) (orig)
#define KO_ENG_ADV(c) 2
#define KO_NAME_DY 0
#define KO_TEXT_DX 0
#define KO_TEXT_EXTRA_W 0
#endif

/* KO_DLG16 (per overlay, opt-in): the overlay draws its dialogue with the Korean renderer (12x16 LoadTPage cells),
 * so its line buffers are 16 rows: CutsceneUnk3 clears n*16 + y, CutsceneUnk4 / cutscene_script_box.h sample
 * v = n*16 + offset, 16 rows high, lines rotate &= 3 (4 buffers, y 384..447 on page 16). Define it as
 *     #ifdef MOD_KO
 *     #define KO_DLG16 1
 *     #endif
 * at the top of the overlay's dialogue .c BEFORE the cutscene_unk3.h / cutscene_unk4.h / cutscene_script_box.h
 * includes, and ONLY together with the Korean renderer (glyph writes at n*16 + base). An overlay without it keeps the
 * retail 12 px buffers in every build (the boss overlays mar/bo2/bo5/bo6/rbo2/rbo6 write retail 8x8 glyphs at
 * n*12 + 0x180; a 16 px buffer there garbles line 2+). fixBOSS: bo5/bo6/rbo6 now have the renderer
 * (src/boss/ko_boss_dialogue.h) and set KO_DLG16 in MOD_KO; bo2/rbo2 still retail 12 px.
 * KO_DLG_FB_DY: the original-font 8x8 fallback (MoveImage) inside a 16 px buffer is drawn this many rows down so it
 * sits on the Korean cells' baseline (n*16 + base + 5). */
#if defined(KO_DLG16) && !defined(MOD_KO)
#error "KO_DLG16 needs MOD_KO (Korean renderer)"
#endif
#ifdef KO_DLG16
#define KO_DLG_PITCH 16
#define KO_DLG_FB_DY 5
#else
#define KO_DLG_PITCH 12
#define KO_DLG_FB_DY 0
#endif

/* sihum42: resident Hangul glyph uploader of the pickup popup (mods/sotn-ultimate/src/menu/eq_pop.c, dra.h KO_POP_GLYPH) */
#ifdef MOD_KO
#define KO_PopGlyph ((void (*)(u16, s32))0x8009F280)
#define KO_PopupText ((void (*)(u8*, u8))0x8009E640)
#endif

/* sihum40 (CP 904/920): speaker-name tag (cutscene_actor_name.h) and bottom-corner popup (popup.h) use the same
 * per-cell advance table as the menu (0x8009F200 = KO_FONT8_ADV_ADDR, ARC_KO): re-baked ASCII cells advance ink
 * width + 2 and are drawn 1 px right of the pen, space 4, table 0 = 8 px. ONE table read per character: the caller
 * stores KO_NAME_LOOK(ch) in a local and passes it to KO_NAME_OFF / KO_NAME_ADV.
 * Per-overlay switch: `#define KO_NAME_FIXED 1` BEFORE the first include of the overlay's cutscene.c / popup.c
 * falls back to the fixed 6 px advance (space 3) when that overlay has no bytes for the table code
 * (list: tools/build_ko_trial.sh KO_NAME_FIXED_OVERLAYS; stsel 8 B margin). */
#ifdef MOD_KO
#ifndef KO_NAME_FIXED
#define KO_NAME_LOOK(ch) (((const u8*)0x8009F200)[(u8)(ch)])
#define KO_POP_LOOK(ch) (((u8)(ch) < 0x60) ? ((const u8*)0x8009F200)[(u8)(ch)] : 0)
#define KO_NAME_ADV(a) ((a) ? (a) : 8)
#define KO_NAME_OFF(a) ((a) ? 1 : 0)
#define KO_NAME_SP 4
#else
#define KO_NAME_LOOK(ch) ((ch) ? 6 : 3)
#define KO_POP_LOOK(ch) ((ch) ? 6 : 3)
#define KO_NAME_ADV(a) (a)
#define KO_NAME_OFF(a) 0
#define KO_NAME_SP 3
#endif
#else
#define KO_NAME_LOOK(ch) 0
#define KO_POP_LOOK(ch) 0
#define KO_NAME_ADV(a) FONT_GAP
#define KO_NAME_OFF(a) 0
#define KO_NAME_SP FONT_SPACE
#endif
