// SPDX-License-Identifier: AGPL-3.0-or-later
// sihum42 (CP 929): resident port of the US BottomCornerText (src/st/popup.h) for the KO build, with Hangul
// names (12 px glyphs via KO_PopGlyph) and the per-cell advance table (KO_FONT8_ADV). The stage overlays keep only a
// call stub (popup.h, KO_POP_RESIDENT). Entry = first function at KO_POP_TEXT (dra.h).
#include "ko_equip.h"

#if defined(MOD_KO)

#define ADV ((const u8*)0x8009F200)
extern void KO_PopGlyph(u16 code, s32 n);

void KO_PopupText(u8* str, u8 leftAlign) {
    u8 toPrint[64];
    Primitive* prim;
    s32 i;
    u8 ch;
    u8* chIdx;
    u16 textWidth;
    u16 xPos;
    s16 xPos2;
    s32 a;
    s32 kN = 6; /* glyph slots from x 18 / u 72 (clear of Maria's slot x 0..17, CEN); u8 u0 -> max 15 Hangul (kN 6..20) */

    chIdx = toPrint;
    xPos = textWidth = 0;
    for (i = 0; i < 64; i++) {
        *chIdx++ = 0;
    }
    chIdx = toPrint;
    /* A3: a name pointer into the JP name bundle (RAM 0x801F0000.., ko_equip_jp) reads 00 00 while the bundle is not
     * loaded (Richter prologue) and no FF 00 terminator ever follows: treat it as an empty string */
    while (!(str[0] == 0 && str[1] == 0)) {
        if (chIdx - toPrint >= 60) { /* toPrint is 64 B: stop with room for one Hangul pair; the zero fill ends the text */
            break;
        }
        ch = *str++;
        if (ch == 0xFF) {
            ch = *str++;
            if (ch == 0) {
                break;
            }
        }
        *chIdx++ = ch;
        if ((ch & 0xFC) == 0xF0) {
            *chIdx++ = *str++;
            xPos++;
            textWidth += 12;
            continue;
        }
        if (ch) {
            xPos++;
        }
        a = (ch < 0x60) ? ADV[ch] : 0;
        textWidth += a ? a : 8;
    }

    g_unkGraphicsStruct.BottomCornerTextPrims = AllocPrimitives(PRIM_SPRT, xPos + 4);
    if (g_unkGraphicsStruct.BottomCornerTextPrims == -1) {
        return;
    }
    prim = &g_PrimBuf[g_unkGraphicsStruct.BottomCornerTextPrims];
    prim->type = PRIM_G4;
    PRED(prim) = 0;
    PGRN(prim) = 0;
    PBLU(prim) = 0;
    if (leftAlign) {
        prim->b0 = prim->b1 = 0xAF;
    } else {
        prim->g0 = prim->g1 = 0x5F;
    }
    if (leftAlign) {
        textWidth += 4;
        xPos = 7;
    } else {
        xPos = 0xD4 - textWidth;
    }
    prim->x0 = prim->x2 = xPos;
    prim->x1 = prim->x3 = xPos + textWidth + 0x20;
    prim->y0 = prim->y1 = 0xD0;
    prim->y2 = prim->y3 = 0xDF;
    prim->priority = 0x1EE;
    prim->drawMode = DRAW_TPAGE | DRAW_TRANSP;
    prim = prim->next;

    prim->tpage = 0x1F;
    prim->clut = PAL_UNK_197;
    prim->x0 = xPos - 6;
    prim->y0 = 0xCB;
    prim->u0 = 0x80;
    prim->v0 = 0;
    prim->u1 = 0x10;
    prim->v1 = 0x18;
    prim->priority = 0x1EF;
    prim->drawMode = DRAW_DEFAULT;
    prim = prim->next;

    prim->tpage = 0x1F;
    prim->clut = PAL_UNK_197;
    prim->x0 = xPos + textWidth + 0x16;
    prim->y0 = 0xCB;
    prim->u0 = 0xA8;
    prim->v0 = 0;
    prim->u1 = 0x10;
    prim->v1 = 0x18;
    prim->priority = 0x1EF;
    prim->drawMode = DRAW_DEFAULT;
    prim = prim->next;

    prim->type = PRIM_GT4;
    prim->tpage = 0x1F;
    prim->clut = PAL_UNK_197;
    prim->x0 = prim->x2 = xPos + 0xA;
    prim->x1 = prim->x3 = xPos + textWidth + 0x18;
    prim->y0 = prim->y1 = 0xCD;
    prim->y2 = prim->y3 = 0xE1;
    prim->u0 = prim->u2 = 0x98;
    prim->u1 = prim->u3 = 0x9C;
    prim->v0 = prim->v1 = 2;
    prim->v2 = prim->v3 = 0x16;
    prim->priority = 0x1EF;
    prim->drawMode = DRAW_DEFAULT;
    prim = prim->next;

    xPos2 = xPos + 0x10;
    chIdx = toPrint;
    while (prim != NULL) {
        ch = *chIdx++;
        if ((ch & 0xFC) == 0xF0) {
            KO_PopGlyph(((u16)ch << 8) | *chIdx++, kN);
            prim->tpage = 0x10;
            prim->clut = PAL_UNK_1A1;
            prim->u0 = (u8)(kN * 12);
            prim->v0 = 208; /* y 464 (eq_pop.c KO_PopGlyph) */
            prim->u1 = 12;
            prim->v1 = 16;
            prim->priority = 0x1F0;
            prim->drawMode = DRAW_DEFAULT;
            prim->x0 = xPos2;
            prim->y0 = 0xD0;
            kN++;
            prim = prim->next;
            xPos2 += 12;
            continue;
        }
        a = (ch < 0x60) ? ADV[ch] : 0;
        if (ch) {
            prim->tpage = 0x1E;
            prim->clut = PAL_UNK_196;
            prim->u0 = (s8)((ch & 0x0F) << 3);
            prim->v0 = (s8)((ch & 0xF0) >> 1);
            prim->u1 = prim->v1 = 8;
            prim->priority = 0x1F0;
            prim->drawMode = DRAW_DEFAULT;
            prim->x0 = xPos2 + (a ? 1 : 0);
            prim->y0 = 0xD4;
            prim = prim->next;
        }
        xPos2 += a ? a : 8;
    }
    g_unkGraphicsStruct.BottomCornerTextTimer = 0x130;
}

#endif
