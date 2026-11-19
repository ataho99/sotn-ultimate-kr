// SPDX-License-Identifier: AGPL-3.0-or-later
// sihum42 (CP 926): item-pickup popup (src/st/popup.h BottomCornerText) Hangul glyph upload.
// Resident in ARC_KO at KO_POP_GLYPH (dra.h), must stay the first/only function (offset 0 = entry).
// The stage overlays draw every popup glyph as an SPRT; a Hangul name code (F0xx pair) is uploaded here as one
// 12x16 4bpp glyph into VRAM slot n (halfword x = 3n, y = 448: the dialogue text texture area, tpage 0x10 row 4,
// unused during play) and the overlay points its SPRT at it (u = 12n, v = 192, clut 0x1A1).
// DrawSync(0): func_80106A28 returns ONE shared scratch buffer and LoadTPage only queues the upload (ko_blob.c).
#include "ko_equip.h"

#if defined(MOD_KO)

extern u16* func_80106A28(u32 ch, u16 kind);

void KO_PopGlyph(u16 code, s32 n) {
    u_long* g = (u_long*)func_80106A28(code, 0);
    LoadTPage(g, 0, 0, n * 3, 448, 12, 16);
    DrawSync(0);
}

#endif
