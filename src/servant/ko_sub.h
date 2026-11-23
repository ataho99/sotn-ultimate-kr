// SPDX-License-Identifier: AGPL-3.0-or-later
// 094_17 KO: familiar voice subtitles, design B (daegangbang #3429 / seunsang "B"): bottom centre, two rows (one row = the lower
// one), dialogue-window white, 50% black band behind, no option. The XA voice lines of the faerie (tt_002) and the demon
// (tt_003) are shown while the voice plays (ProcessSfxState: shown when the XA stream starts = xa_play_ck turns true in step 3,
// hidden when it ends in step 4). Rows: gen_ko_sub.py -> ko_sub_str.h.
// Glyphs: the resident KO glyph source (g_api.func_80106A28 returns ONE shared buffer, so every glyph is copied to a
// private 96 B buffer, uploaded with LoadImage and DrawSync(0) before the buffer is reused: a 30-slot buffer (2,880 B of bss)
// would run past 0x8017A000 where the weapon overlay WEAPON0 is loaded) into VRAM tpage 0x10 rows y 480 / 496 (slots 6..20 = 15 cells: clear of
// the dialogue buffers y 384..447, the speaker name cells y 448..463 and the pickup popup y 464, and of Maria's x 0..17).
// White = clut 0x1A1 index 8 (0xF0F0F0): the dialogue window text uses the same clut (g_Dialogue.clutIndex = 0x1A1) and the
// KO glyph full ink maps to index 8 (D_800AC934[16]); the renderer draws a SPRT without DRAW_COLORS as the raw texture.
// Band = one untextured G4 (all vertex colours 0) with DRAW_TRANSP: semi-transparency mode 0 (B/2 + F/2, F = black) = 50% darker.
// Both use DRAW_ABSPOS (no camera shake offset) and no DRAW_MENU: RenderPrimitives skips every primitive without DRAW_MENU while
// the pause menu is open (D_800973EC), so the subtitle never covers the menu. Priority: band 0x1EF < text 0x1F0 < dialogue 0x1FE.
// Failure = no subtitle (never a wait or a branch in ProcessSfxState): unknown id, too many glyphs, or a primitive that cannot be
// allocated (text or band) ends in KoSubHide(). KoSubHide() only frees a primitive that still carries OUR marker (type +
// priority [+ tpage]): after a room / stage load the primitive buffer is rebuilt and a stale index must not free another entity's.
#ifndef KO_SUB_H
#define KO_SUB_H
#define KO_SUB_ENABLE 1 /* 094_16 = 0 (A-gap fix only, no subtitles); 094_17 = 1 (design B) */
#if KO_SUB_ENABLE && defined(MOD_KO) && (defined(KO_SUB_FAERIE) || defined(KO_SUB_DEMON))
#define KO_SUBTITLE 1
typedef struct {
    u16 id;
    const char* r0;
    const char* r1;
} KoSubLine;
#include "ko_sub_str.h"

/* DrawSync is not in the servant overlay's link symbols (they are generated from the retail servant asm, which never called it):
 * call the libgpu function by its address (config/symbols.us.txt: DrawSync = 0x8001290C, 1 arg, the same lib block as LoadImage 0x80012B24). */
#define KO_SUB_DRAWSYNC() ((void)((s32 (*)(s32))0x8001290C)(0))
#define KO_SUB_ROWS 2
#define KO_SUB_MAXGLYPH 30
#define KO_SUB_PRIO_TEXT 0x1F0
#define KO_SUB_PRIO_BAND 0x1EF
#define KO_SUB_Y_ROW0 0xA8 /* 168: the upper row of a two-row subtitle */
#define KO_SUB_Y_ROW1 0xB8 /* 184: the lower row; a one-row subtitle sits here (bottom anchored) */
#define KO_SUB_BAND_PAD 4  /* band = the text block + 4 px above and below */
static u32 g_KoSubGlyph[24]; /* ONE buffer (96 B): the servant overlay ends before 0x8017A000 (WEAPON0 loads there) - no room for 30 */
static s32 g_KoSubPrim = -1; /* the text sprites */
static s32 g_KoSubBand = -1; /* the 50% black band */

// free the primitives at *idx only while the head still carries our marker, then forget the index
static void KoSubFree(s32* idx, u8 type, u16 prio, s32 needTpage) {
    s32 i = *idx;
    *idx = -1;
    if (i < 0 || i >= MAX_PRIM_COUNT) {
        return;
    }
    if (g_PrimBuf[i].type != type || g_PrimBuf[i].priority != prio) {
        return;
    }
    if (needTpage && g_PrimBuf[i].tpage != 0x10) {
        return;
    }
    g_api.FreePrimitives(i);
}

static void KoSubHide(void) {
    KoSubFree(&g_KoSubPrim, PRIM_SPRT, KO_SUB_PRIO_TEXT, 1);
    KoSubFree(&g_KoSubBand, PRIM_G4, KO_SUB_PRIO_BAND, 0);
}

// advance of one char: Hangul / ellipsis 12, space and ASCII 6
static s32 KoSubAdv(u8 ch) { return (ch & 0xFC) == 0xF0 ? 12 : 6; }

static void KoSubShow(u16 id) {
    const KoSubLine* ln = NULL;
    const u8* row[KO_SUB_ROWS];
    s32 i, r, n, w, x, slot, idx, rowsN, y, yTop;
    const u8* p;
    u16 code;
    u_long* glyph;
    Primitive* prim;
    RECT rc;

    for (i = 0; i < (s32)(sizeof(g_KoSubLines) / sizeof(g_KoSubLines[0])); i++) {
        if (g_KoSubLines[i].id == id) {
            ln = &g_KoSubLines[i];
            break;
        }
    }
    if (ln == NULL) {
        return;
    }
    KoSubHide();
    row[0] = (const u8*)ln->r0;
    row[1] = (const u8*)ln->r1;
    n = 0;
    for (r = 0; r < KO_SUB_ROWS; r++) {
        for (p = row[r]; *p; p += ((*p & 0xFC) == 0xF0) ? 2 : 1) {
            if (*p != 0x20) {
                n++;
            }
        }
    }
    if (n == 0 || n > KO_SUB_MAXGLYPH) {
        return;
    }
    rowsN = (*row[1] != 0) ? 2 : 1;
    g_KoSubPrim = g_api.AllocPrimitives(PRIM_SPRT, n);
    if (g_KoSubPrim == -1) {
        return;
    }
    g_KoSubBand = g_api.AllocPrimitives(PRIM_G4, 1);
    if (g_KoSubBand == -1) {
        KoSubHide(); // no band, no subtitle
        return;
    }
    // the band: one black G4 over the text block, DRAW_TRANSP = mode 0 (B/2 + F/2) = 50% darker
    yTop = (rowsN == 2 ? KO_SUB_Y_ROW0 : KO_SUB_Y_ROW1) - KO_SUB_BAND_PAD;
    prim = &g_PrimBuf[g_KoSubBand];
    prim->r0 = prim->g0 = prim->b0 = 0;
    prim->r1 = prim->g1 = prim->b1 = 0;
    prim->r2 = prim->g2 = prim->b2 = 0;
    prim->r3 = prim->g3 = prim->b3 = 0;
    prim->x0 = 0;
    prim->y0 = yTop;
    prim->x1 = 256;
    prim->y1 = yTop;
    prim->x2 = 0;
    prim->y2 = KO_SUB_Y_ROW1 + 16 + KO_SUB_BAND_PAD;
    prim->x3 = 256;
    prim->y3 = KO_SUB_Y_ROW1 + 16 + KO_SUB_BAND_PAD;
    prim->priority = KO_SUB_PRIO_BAND;
    prim->drawMode = DRAW_ABSPOS | DRAW_TRANSP;

    prim = &g_PrimBuf[g_KoSubPrim];
    idx = 0;
    for (r = 0; r < KO_SUB_ROWS; r++) {
        w = 0;
        for (p = row[r]; *p; p += ((*p & 0xFC) == 0xF0) ? 2 : 1) {
            w += KoSubAdv(*p);
        }
        x = (256 - w) / 2;
        y = (r || rowsN == 1) ? KO_SUB_Y_ROW1 : KO_SUB_Y_ROW0;
        slot = 6;
        for (p = row[r]; *p; p += ((*p & 0xFC) == 0xF0) ? 2 : 1) {
            if (*p == 0x20) {
                x += 6;
                continue;
            }
            if ((*p & 0xFC) == 0xF0) {
                code = (u16)((p[0] << 8) | p[1]);
            } else {
                code = ((const u16*)0x8009EF00)[*p - 0x21]; // the KO engtab cell of this ASCII char
            }
            if (code == 0 || prim == NULL) {
                x += KoSubAdv(*p);
                continue;
            }
            glyph = (u_long*)g_api.func_80106A28(code, 1);
            for (i = 0; i < 24; i++) {
                g_KoSubGlyph[i] = glyph[i];
            }
            rc.x = slot * 3;
            rc.y = r ? 496 : 480;
            rc.w = 3;
            rc.h = 16;
            LoadImage(&rc, (u_long*)g_KoSubGlyph);
            KO_SUB_DRAWSYNC(); // the transfer must finish before the buffer is reused
            prim->tpage = 0x10;
            prim->clut = PAL_UNK_1A1; /* the dialogue window's clut: ink = index 8 = 0xF0F0F0 */
            prim->r0 = prim->g0 = prim->b0 = 0x80;
            prim->u0 = (u8)(slot * 12);
            prim->v0 = r ? 240 : 224;
            prim->u1 = 12;
            prim->v1 = 16;
            prim->priority = KO_SUB_PRIO_TEXT;
            prim->drawMode = DRAW_ABSPOS;
            prim->x0 = x;
            prim->y0 = y;
            prim = prim->next;
            idx++;
            slot++;
            x += KoSubAdv(*p);
        }
    }
    while (prim != NULL) { // (ASCII chars with no cell)
        prim->drawMode = DRAW_HIDE;
        prim = prim->next;
    }
}
#define KO_SUB_SHOW(id) KoSubShow(id)
#define KO_SUB_HIDE() KoSubHide()
#endif
#endif
