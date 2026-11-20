// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef CUTSCENE_UNK3_RECT_X
#define CUTSCENE_UNK3_RECT_X 0
#endif

#ifndef CUTSCENE_UNK3_RECT_Y
#define CUTSCENE_UNK3_RECT_Y 384
#endif

#if defined(VERSION_HD) || defined(KO_DLG16)
/* KO_DLG16: the overlay has the Korean renderer (ko_font_params.h); plain MOD_KO keeps 12 */
#define CUTSCENE_UNK3_RECT_H 16
#else
#define CUTSCENE_UNK3_RECT_H 12
#endif

#if defined(VERSION_PSP)
extern u8 cutsceneUnk3Unknown[0x380]; // bss
#endif

static void CutsceneUnk3(s16 nextCharY) {
    RECT rect;
    rect.x = CUTSCENE_UNK3_RECT_X;
    rect.y = (nextCharY * CUTSCENE_UNK3_RECT_H) + CUTSCENE_UNK3_RECT_Y;
    rect.w = 64;
    rect.h = CUTSCENE_UNK3_RECT_H;
    ClearImage(&rect, 0, 0, 0);
#if defined(VERSION_PSP)
    memset(cutsceneUnk3Unknown, 0, LEN(cutsceneUnk3Unknown));
#endif
}
