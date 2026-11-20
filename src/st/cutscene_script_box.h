// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ko_font_params.h"
// The CUTSCENE_UNK3_RECT_X and CUTSCENE_UNK4_TPAGE macros must be defined in
// the overlay

static void CutsceneUnk3(s16 nextCharY) {
    RECT rect;
    rect.x = CUTSCENE_UNK3_RECT_X;
#ifdef KO_DLG16
    /* Korean renderer: 16-row line buffers (glyphs at n*16 + 384, ko_font_params.h) */
    rect.y = (nextCharY * 16) + 384;
    rect.w = 64;
    rect.h = 16;
#else
    rect.y = (nextCharY * 12) + 384;
    rect.w = 64;
    rect.h = 12;
#endif
    ClearImage(&rect, 0, 0, 0);
#if defined(VERSION_PSP)
    memset(cutsceneUnk3Unknown, 0, LEN(cutsceneUnk3Unknown));
#endif
}

#ifdef CUTSCENE_UNK4_HAS_ARGS
static void CutsceneUnk4(s32 nextLineY) {
#else
static void CutsceneUnk4(void) {
#endif
    Primitive* prim;

    CutsceneUnk3(g_Dialogue.nextCharY);
    prim = g_Dialogue.prim[g_Dialogue.nextCharY];
    prim->tpage = CUTSCENE_UNK4_TPAGE;
#ifdef CUTSCENE_UNK4_HAS_ARGS
    prim->clut = PAL_UNK_1A1;
    prim->x0 = 56;
    prim->u0 = 0;
    prim->y0 = nextLineY + 44;
#ifdef KO_DLG16
    prim->v0 = g_Dialogue.nextCharY * 16 + 128;
#else
    prim->v0 = nextLineY - 128;
#endif
#else
    prim->clut = g_Dialogue.clutIndex;
    prim->y0 = g_Dialogue.nextLineY;
    prim->u0 = 0;
    prim->x0 = g_Dialogue.startX + 4 - KO_TEXT_DX;
#ifdef KO_DLG16
    prim->v0 = g_Dialogue.nextCharY * 16 + 128;
#else
    prim->v0 = g_Dialogue.nextCharY * 12 + 128;
#endif
#endif
    prim->u1 = 192 + KO_TEXT_EXTRA_W;
#ifdef KO_DLG16
    prim->v1 = 16;
#else
    prim->v1 = 12;
#endif
    prim->priority = 0x1FF;
    prim->drawMode = DRAW_DEFAULT;
}
