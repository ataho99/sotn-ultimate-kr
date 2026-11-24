// SPDX-License-Identifier: AGPL-3.0-or-later
#include "st0.h"
#include <cutscene.h>

// This file uses g_Dialogue with a different struct than cutscene.c
typedef struct {
    /* 0x00 */ u8* scriptCur;
    /* 0x04 */ s16 startX;
    /* 0x06 */ s16 nextLineY;
    /* 0x08 */ s16 startY;
    /* 0x0A */ s16 nextCharX;
    /* 0x0C */ s16 nextLineX;
    /* 0x0E */ u16 nextCharY;
    /* 0x10 */ u16 portraitAnimTimer;
    /* 0x12 */ u8 unk12;
    /* 0x13 */ u8 clutIndex;
#ifdef VERSION_PSP
    /* 0x14 */ u8 nextCharTimer;
    /* 0x15 */ u8 unk17;
#endif
    /* 0x14 */ Primitive* prim;
    /* 0x18 */ u32 primIndex;
    /* 0x1C */ u16* unk20;
    /* 0x20 */ s32 : 32;
    /* 0x24 */ u16* clutIndexes;
    /* 0x28 */ s32 : 32;
    /* 0x2C */ s32 clutArrLength;
    /* 0x30 */ s32 : 32;
    /* 0x34 */ u8* script;
#ifndef VERSION_PSP
    /* 0x38 */ u16 hasEvents;
    /* 0x3A */ u16 timer;
#endif
    /* 0x3C */ u8* eventCur;
} DialoguePrologue;

// n.b. g_Dialogue is defined as the Dialogue struct, but is used in
// this file as the different type. It appears that either:
// - The devs recognized this as a bad practice and fixed it for pspeu or
// - The pspeu toolchain (probably optimization level) handled them differently
// Either way, us shares a single memory address for 3101C.c and cutscene.c
// while pspeu uses two independent memory addresses.
extern DialoguePrologue g_Dialogue;

#if defined(MOD_KO) && !defined(VERSION_PSP)
// 0.9.4 (A3): KO scroll glyphs are drawn 2x wide (24px box, 6 words); comma/period stay 12px (3 words).
// 24 slots x 0x60 u16 = same 4608 B as the old 48 x 0x30 buffer (gen_ko_scroll.py asserts <= 24 glyphs per line).
static u16 D_801BEE90[24][0x60]; // bss
static u8 D_801BEE90_ring;
#else
static u16 D_801BEE90[48][48]; // bss
#endif

u8 func_801B101C(u8* script) {
    Primitive* prim;
    s16 i;
    s16 shift;

    g_Dialogue.primIndex = g_api.AllocPrimitives(PRIM_SPRT, 0x20);
    if (g_Dialogue.primIndex != -1) {
        g_Dialogue.scriptCur = script;
        g_Dialogue.nextCharX = 0x200;
        g_Dialogue.startY = 0x21F; // 0.9.2: centred first KO line (0x200 + X of line 0)
        g_Dialogue.nextLineX = 0;
        g_Dialogue.nextCharY = 0;
        g_Dialogue.portraitAnimTimer = 0;
        g_Dialogue.unk12 = 0;
        g_Dialogue.clutIndex = 0;
        prim = g_Dialogue.prim = &g_PrimBuf[g_Dialogue.primIndex];
        for (i = 0; i < 0x20; i++) {
            shift = i & 1;
            if (shift) {
                prim->tpage = 9;
                prim->x0 = 0x100;
            } else {
                prim->tpage = 8;
                prim->x0 = 0x20;
            }
            shift = i >> 1;
            prim->u0 = 0;
            prim->v0 = shift * 0x10;
            prim->u1 = 0xF0;
            prim->v1 = 0x10;
            prim->y0 = shift * 0x16 + 0xF0;
            prim->clut = 0x221;
            prim->priority = 3;
            prim->drawMode = DRAW_DEFAULT;
            prim = prim->next;
        }

        return true;
    }
    g_Dialogue.primIndex = 0;
    return false;
}

void func_801B1198(s16 yVal) {
    RECT rect;
    rect.x = 0x200;
    rect.y = yVal * 16;
    rect.w = 0x80;
    rect.h = 0x10;

    ClearImage(&rect, 0, 0, 0);
}

u16* func_801B11E8(u8 ch) {
    u16 jCh;
#ifndef VERSION_PC
#if defined(MOD_KO)
    /* Latin/punct inside KO scroll lines -> MGalmuri12 (v12d
     * repurposed cells via KO_ENGTAB, 94xu16 in ARC_KO gap at 0x8009EF00).
     * Miss (0) falls through to the JP fullwidth path. TSV untouched. */
    if (ch != ' ' && ch < 0x80 && ((const u16*)0x8009EF00)[ch - 0x21]) {
        return g_api.func_80106A28(((const u16*)0x8009EF00)[ch - 0x21], 0);
    }
#endif
     if (ch >= 'a') {
        jCh = ('ａ' - 'a') + ch;
    } else if (ch >= 'A') {
        jCh = ('Ａ' - 'A') + ch;
    } else if (ch == ',') {
        jCh = '，';
    } else if (ch == '.') {
        jCh = '．';
    } else if (ch == '\'') {
        jCh = '’';
    } else if (ch == ' ') {
        return NULL;
#if defined(MOD_SATURN) && defined(VERSION_US)
    } else if (ch == '-') {
        // Saturn port: the PSP scroll's "full-moon" (retail's text has no
        // hyphen: the arithmetic below lands it on an empty code, 0x824C).
        // (Not MOD_PROLOGUE_SCROLL: mod_prologue_scroll.h, which defines
        // it, is included below this function; same condition as there)
        jCh = 0x817C; // Shift-JIS '－' (the char literal does not parse)
#endif
    } else {
        jCh = ('Ａ' - 'A') + ch;
        if (ch >= 'a') {
            ++jCh;
        }
    }
#else
    jCh = 'A';
#endif

    return g_api.func_80106A28(jCh, 0);
}

#include "prologue_cutscene_script.h"
#include "prologue_scroll_ko.h"

#if defined(MOD_KO) && !defined(VERSION_PSP)
// Draw one scroll glyph into the VRAM line ring. wide: 4bpp column-doubled 24px (6 words); narrow (, .): raw 12px (3 words).
// A glyph never crosses word 0x3C of its line (SPRT0 width 240px): it jumps to word 0x40 (same rule as gen_ko_scroll.py).
static void ScrollPutGlyph(u16* glyph, s32 narrow, u16 slot) {
    u16 w = narrow ? 3 : 6;
    u16 p;
    u16* buf;
    u8* src;
    u8* dst;
    s32 i;

    p = g_Dialogue.startY - g_Dialogue.nextCharX;
    if (p < 0x3C && p + w > 0x3C) {
        g_Dialogue.startY = g_Dialogue.nextCharX + 0x40;
    }
    buf = D_801BEE90[slot];
    if (narrow) {
        for (i = 0; i < 0x30; i++) {
            buf[i] = glyph[i];
        }
    } else {
        src = (u8*)glyph;
        dst = (u8*)buf;
        for (i = 0; i < 0x60; i++) {
            u8 b = src[i];
            *dst++ = (b & 0xF) * 0x11;
            *dst++ = (b >> 4) * 0x11;
        }
    }
    LoadTPage((u_long*)buf, 0, 0, g_Dialogue.startY, g_Dialogue.nextLineX * 16, w * 4, 16);
    g_Dialogue.startY += w;
    p = g_Dialogue.startY - g_Dialogue.nextCharX;
    if (0x38 <= p && p < 0x40) {
        g_Dialogue.startY += 8;
    }
}
#endif
// Saturn port: the scroll per dub (JP: the English retranslation; PSP: the
// PSP English scroll), generated by mods/sotn-ultimate/tools/prologue_scroll.py
#include "mod_prologue_scroll.h"

void func_801B1298(Entity* self) {
    s16 y0;
    Primitive* prim;
    s32 i;
    u16* glyph;
    u16 nextChar;
    u16* glyphPtr;
    u16 glyphIndex;
    u16 y;

    switch (self->step) {
    case 0:
#if defined(MOD_KO) && !defined(VERSION_PSP)
        if (func_801B101C((u8*)prologue_scroll_ko_psx)) {
#elif defined(MOD_PROLOGUE_SCROLL)
        if (func_801B101C(ModPrologueScript((u8*)prologue_script_en))) {
            g_Dialogue.startY = g_Dialogue.nextCharX + g_ModScrollX;
#else
        if (func_801B101C((u8*)prologue_script_en)) {
#endif
            self->flags |= FLAG_HAS_PRIMS;
            self->primIndex = g_Dialogue.primIndex;
            ++self->step;
            func_801B1198(0);
            glyphIndex = 0;
            while (1) {
                nextChar = *g_Dialogue.scriptCur++;
                if (nextChar == 1) {
                    // Gets weirdly relocated in the asm.
                    g_Dialogue.startY =
                        g_Dialogue.nextCharX + *g_Dialogue.scriptCur++;
                    ++g_Dialogue.nextLineX;
                    break;
                }
                if (nextChar & 0x80) {
                    nextChar = (nextChar << 8) + *g_Dialogue.scriptCur++;
                    glyph = g_api.func_80106A28(nextChar, 0);
                } else {
                    glyph = func_801B11E8(nextChar);
                }
                if (glyph) {
#if defined(MOD_KO) && !defined(VERSION_PSP)
                    ScrollPutGlyph(glyph, nextChar == ',' || nextChar == '.',
                                   glyphIndex);
                    glyphIndex++;
#else
                    glyphPtr = D_801BEE90[glyphIndex];
                    for (i = 0; i < 0x30; ++i) {
                        glyphPtr++[0] = glyph++[0];
                    }
                    glyph = D_801BEE90[glyphIndex];
                    y = g_Dialogue.nextLineX * 16;
                    LoadTPage(
                        (u_long*)glyph, 0, 0, g_Dialogue.startY, y, 12, 16);
                    g_Dialogue.startY += 3;
                    nextChar = g_Dialogue.startY - g_Dialogue.nextCharX;
                    if (0x38 <= nextChar && nextChar < 0x40) {
                        g_Dialogue.startY += 8;
                    }
                    glyphIndex++;
#endif
                } else {
                    g_Dialogue.startY += 2;
                    nextChar = g_Dialogue.startY - g_Dialogue.nextCharX;
                    if (0x38 <= nextChar && nextChar < 0x40) {
                        g_Dialogue.startY += 8;
                    }
                }
            }
        }
        break;
    case 1:
        if (!g_Dialogue.nextCharY) {
            break;
        }
        nextChar = 0;
        glyphIndex = 0;
        do {
            nextChar = *g_Dialogue.scriptCur++;
            switch (nextChar) {
            case 0:
                self->step = 7;
                return;
            case 1:
                g_Dialogue.startY =
                    g_Dialogue.nextCharX + *g_Dialogue.scriptCur++;
                g_Dialogue.nextLineX++;
                if (g_Dialogue.nextLineX > 15) {
                    g_Dialogue.nextLineX = 0;
                }
                g_Dialogue.nextCharY = 0;
                return;
            case 2:
                g_Dialogue.startY =
                    g_Dialogue.nextCharX + *g_Dialogue.scriptCur++;
                g_Dialogue.nextLineX++;
                if (g_Dialogue.nextLineX > 15) {
                    g_Dialogue.nextLineX = 0;
                }

                prim = g_Dialogue.prim;
                for (i = 0; i < g_Dialogue.nextLineX; ++i) {
                    prim = prim->next;
                    prim = prim->next;
                }

                y0 = prim->y0;
                for (prim = g_Dialogue.prim; prim != NULL; prim = prim->next) {
                    if (y0 <= prim->y0) {
                        prim->y0 += 0x16;
                        prim->p1 += 0x16;
                    }
                }
                g_Dialogue.unk12 += 0x58;
                g_Dialogue.portraitAnimTimer += 0x16;
                g_Dialogue.nextCharY = 0;
                return;
            }
        } while (0);
        if (nextChar & 0x80) {
            nextChar = (nextChar << 8) + *g_Dialogue.scriptCur++;
            glyph = g_api.func_80106A28(nextChar, 0);
        } else {
            glyph = func_801B11E8(nextChar);
        }

#if defined(MOD_KO) && !defined(VERSION_PSP)
        if (glyph) {
            ScrollPutGlyph(glyph, nextChar == ',' || nextChar == '.',
                           D_801BEE90_ring);
            D_801BEE90_ring = (D_801BEE90_ring + 1) % 24;
        } else {
            g_Dialogue.startY += 2;
            nextChar = g_Dialogue.startY - g_Dialogue.nextCharX;
            if (0x38 <= nextChar && nextChar < 0x40) {
                g_Dialogue.startY += 8;
            }
        }
#else
        if (glyph) {
            y = g_Dialogue.nextLineX * 16;
            LoadTPage((u_long*)glyph, 0, 0, g_Dialogue.startY, y, 0xC, 0x10);
            g_Dialogue.startY += 3;
        } else {
            g_Dialogue.startY += 2;
        }
        nextChar = g_Dialogue.startY - g_Dialogue.nextCharX;
        if (0x38 <= nextChar && nextChar < 0x40) {
            g_Dialogue.startY += 8;
        }
#endif
        break;
    case 2:
        break;
    case 7:
        if (g_Dialogue.nextCharY) {
            g_Dialogue.nextLineX++;
            if (g_Dialogue.nextLineX > 15) {
                g_Dialogue.nextLineX = 0;
            }
            g_Dialogue.nextCharY = 0;
        }
    }

    if (!g_Dialogue.unk12) {
        func_801B1198(g_Dialogue.nextLineX);
        g_Dialogue.nextCharY = 1;
        g_Dialogue.unk12 = 88;
    }
    --g_Dialogue.unk12;
    if (!g_Dialogue.clutIndex) {
        for (prim = g_Dialogue.prim, i = 0; i < 32; ++i) {
            prim->y0--;
            if (prim->y0 == -22) {
                prim->y0 = g_Dialogue.portraitAnimTimer - prim->p1 + 330;
                prim->p1 = g_Dialogue.portraitAnimTimer;
            }
            prim = prim->next;
        }
        g_Dialogue.clutIndex = 4;
    }
    --g_Dialogue.clutIndex;
}
