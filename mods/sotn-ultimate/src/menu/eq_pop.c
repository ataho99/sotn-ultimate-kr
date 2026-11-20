// SPDX-License-Identifier: AGPL-3.0-or-later
// Item-pickup popup (src/st/popup.h BottomCornerText) Hangul glyph upload.
// Resident in ARC_KO at KO_POP_GLYPH (dra.h), must stay the first/only function (offset 0 = entry).
// The stage overlays draw every popup glyph as an SPRT; a Hangul name code (F0xx pair) is uploaded here as one
// 12x16 4bpp glyph into VRAM slot n (halfword x = 3n, n >= 6 from eq_popt.c -> x >= 18, y = 464: tpage 0x10 below the 4 KO dialogue line buffers
// y 384..447 and the speaker-name tag cells y 448..463, cutscene_actor_name.h) and the overlay points its SPRT at it
// (u = 12n, v = 208, clut 0x1A1). Was y 448 / v 192 (shared the name tag row).
// DrawSync(0): func_80106A28 returns ONE shared scratch buffer and LoadTPage only queues the upload (ko_blob.c).
#include "ko_equip.h"

#if defined(MOD_KO)

extern u16* func_80106A28(u32 ch, u16 kind);

void KO_PopGlyph(u16 code, s32 n) {
    u_long* g = (u_long*)func_80106A28(code, 0);
    LoadTPage(g, 0, 0, n * 3, 464, 12, 16);
    DrawSync(0);
}

#endif
