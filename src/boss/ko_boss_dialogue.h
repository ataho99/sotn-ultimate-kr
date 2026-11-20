// SPDX-License-Identifier: AGPL-3.0-or-later
// fixBOSS: Korean dialogue entity of the boss overlays whose retail entity is
// INCLUDE_ASM (bo5 Hippogryph, bo6 Richter, rbo6 Dracula). Compiled only when
// the overlay sets KO_BOSS_C (MOD_KO, PSX US); every other build keeps the
// INCLUDE_ASM entity, byte-identical.
//
// Body: upstream sotn-decomp's decompiled PSX entity of these three overlays
// (src/boss/{bo5,bo6,rbo6}/e_cutscene_dialogue.c, the same function in all
// three), written once; what differs per overlay comes from the hooks below.
// Text: the KO glyph path of src/st/rcen/cutscene.c (KO_ENGTAB 1-byte ASCII,
// KO_PROMO 1-byte promotion 0x80..0xEF, 2-byte 0xF0..0xF3 cells via
// func_80106A28 + LoadTPage) on 16 px line buffers (KO_DLG16, 4 buffers at
// y 384..447 of the overlay's text page; src/st/ko_font_params.h).
//
// The overlay #defines KO_DLG16 and KO_BOSS_C at the top of its .c (before
// the cutscene_*.h includes) and, before including this file:
//   KO_BOSS_ENTITY                 the entity's name (EntityCutscene, ...)
//   KO_BOSS_U_COORDS, KO_BOSS_V_COORDS   u8* portrait u/v per side
//   KO_BOSS_PORTRAIT_CLUTS, KO_BOSS_TEXT_CLUTS, KO_BOSS_X_VALS   u16*
//   KO_BOSS_RED_LINES              s16* [72], the box's red draw-in steps
//   KO_BOSS_PORTRAIT_TPAGE         SET_PORTRAIT tpage
//   KO_BOSS_SKIP                   s32 lvalue: g_SkipCutscene
//   KO_BOSS_STARTED                s32 lvalue: dialogue_started /
//                                  g_IsCutsceneDone (set when a box opens)
//   KO_BOSS_SWITCH_INDEX(ptr)      SCRIPT_SWITCH's case index read
// and the static functions
//   void KoBossSkipCheck(Entity*)  START-skip test (self->step != 0)
//   s32 KoBossInit(Entity*)        step 0; 1 = fall into the run step now
//   void KoBossEnd(Entity*)        step 7 (END_CUTSCENE)
// The retail data arrays are addressed in place (fixed .data addresses in
// front of actor_names, upstream's .data e_cutscene_dialogue layout), so the
// overlay's .data does not change.

#ifndef KO_DLG16
#error "ko_boss_dialogue.h needs KO_DLG16 (16 px line buffers)"
#endif

enum KoBossDialogueSteps {
    KO_DLG_INIT = 0,
    KO_DLG_RUN = 1,
    KO_DLG_LOAD_PORTRAIT = 2,
    KO_DLG_START_TEXT = 3,
    KO_DLG_UNLOAD_PORTRAIT = 4,
    KO_DLG_OPEN_DIALOG_BOX = 5,
    KO_DLG_CLOSE_DIALOG_BOX = 6,
    KO_DLG_END = 7,
};

// PSX Dialogue: prim[0..4] text lines, prim[5] portrait (upstream's
// prim[5] + primPortrait)
#define KO_BOSS_TEXT_PRIMS (LEN(g_Dialogue.prim) - 1)
#define KO_BOSS_PORTRAIT g_Dialogue.prim[LEN(g_Dialogue.prim) - 1]
#define KO_BOSS_RED_LINE_COUNT 72

// KO_ENGTAB (94 x u16 at 0x8009EF00) and KO_PROMO_TBL (0x8009F000), DRA
// resident, as src/st/rcen/cutscene.c
#define KO_BOSS_ENGTAB ((const u16*)0x8009EF00)
#define KO_BOSS_PROMO ((const u16*)0x8009F000)
#define KO_BOSS_LINE_V(n) (((n) * 16) + CUTSCENE_UNK3_RECT_Y)

// 1 = drawn (12x16 cell into line buffer nextCharY at nextCharX); 0 = no KO
// cell, the caller draws the original 8x8 font.
static s32 KoBossGlyph(u16 nextChar) {
    u_long* glyph;
    s32 code;

    if (nextChar > 0x20 && nextChar < 0x7F && KO_BOSS_ENGTAB[nextChar - 0x21]) {
        glyph = (u_long*)g_api.func_80106A28(KO_BOSS_ENGTAB[nextChar - 0x21], 1);
        if (glyph) {
            LoadTPage(glyph, 0, 0, g_Dialogue.nextCharX,
                      KO_BOSS_LINE_V(g_Dialogue.nextCharY), 0xC, 0x10);
            g_Dialogue.nextCharX += KO_ENG_ADV(nextChar);
            return 1;
        }
    }
    if ((nextChar & 0xFC) == 0xF0 || (nextChar >= 0x80 && nextChar < 0xF0)) {
        if (nextChar < 0xF0) {
            code = KO_BOSS_PROMO[nextChar - 0x80];
        } else {
            code = (nextChar << 8) | *g_Dialogue.scriptCur++;
        }
        glyph = (u_long*)g_api.func_80106A28(code, 1);
        if (glyph) {
            LoadTPage(glyph, 0, 0, g_Dialogue.nextCharX,
                      KO_BOSS_LINE_V(g_Dialogue.nextCharY), 0xC, 0x10);
            g_Dialogue.nextCharX += 3;
            return 1;
        }
    }
    return 0;
}

// ScaleCutsceneAvatar for the 4 KO line buffers: the line that scrolls out
// is (nextCharY + 1) & 3 (the shared header's % 5 picks the unused prim[4]
// when nextCharY is 3).
static void KoBossScaleAvatar(const u8 ySteps) {
    Primitive* prim;
    s32 primIndex;
    s32 i;

    primIndex = (g_Dialogue.nextCharY + 1) & 3;
    if (g_CurrentEntity->step_s == 0) {
        prim = g_Dialogue.prim[primIndex];
        prim->v0 += ySteps;
        prim->v1 -= ySteps;
        if (prim->v1 == 0) {
            g_CurrentEntity->step_s++;
            prim->drawMode = DRAW_HIDE;
        }
    }
    for (i = 0; i < 4; i++) {
        if (i != primIndex) {
            prim = g_Dialogue.prim[i];
            prim->y0 -= ySteps;
        }
    }
    g_Dialogue.portraitAnimTimer++;
}

void KO_BOSS_ENTITY(Entity* self) {
    Primitive* prim;
    s32 primIndex;
    s32 i, j;
    s16 uCoord, vCoord;
    u16 nextChar;
    RECT rect;
    u_long ptr;

    if (self->step) {
        KoBossSkipCheck(self);
    }
    if (self->step && g_Dialogue.hasEvents) {
        RunCutsceneEvents();
    }

    switch (self->step) {
    case KO_DLG_INIT:
        if (!KoBossInit(self)) {
            break;
        }
        // rbo6 only: falls into the run step in the same frame
    case KO_DLG_RUN:
        nextChar = 0;
        while (1) {
            if (g_Dialogue.nextCharTimer && !KO_BOSS_SKIP) {
                g_Dialogue.nextCharTimer--;
                return;
            }
            nextChar = *g_Dialogue.scriptCur++;
            switch (nextChar) {
            case CSOP_END_CUTSCENE:
                self->step = KO_DLG_END;
                return;
            case CSOP_LINE_BREAK:
                if (KO_BOSS_SKIP) {
                    continue;
                }
                g_Dialogue.nextCharX = g_Dialogue.nextLineX;
                if (!(g_Dialogue.unk12 & 1)) {
                    g_Dialogue.nextLineY += KO_LINE_STEP(CS_LINE_SPACING);
                }
                g_Dialogue.nextCharY++;
                g_Dialogue.nextCharY &= 3; // 4 KO line buffers (y 384..447)
                CutsceneUnk4();
                if (!(g_Dialogue.unk12 & 1)) {
                    if (KO_SCROLL_AT(g_Dialogue.nextCharY,
                                     g_Dialogue.nextCharY > CS_LINE_MAX - 1)) {
                        g_Dialogue.unk12 |= 1;
                        g_Dialogue.portraitAnimTimer = 0;
                        self->step_s = 0;
                        self->step++;
                        return;
                    }
                    continue;
                }
                g_Dialogue.portraitAnimTimer = 0;
                self->step_s = 0;
                self->step++;
                return;
            case CSOP_SET_SPEED:
                g_Dialogue.unk17 = *g_Dialogue.scriptCur++;
                continue;
            case CSOP_SET_WAIT:
                g_Dialogue.nextCharTimer = *g_Dialogue.scriptCur++;
                if (KO_BOSS_SKIP) {
                    continue;
                }
                return;
            case CSOP_HIDE_DIALOG:
                if (KO_BOSS_SKIP) {
                    continue;
                }
                prim = g_Dialogue.prim[0];
                for (i = 0; i < KO_BOSS_TEXT_PRIMS; i++) {
                    prim->drawMode = DRAW_HIDE;
                    prim = prim->next;
                }
                return;
            case CSOP_SET_PORTRAIT:
                if (KO_BOSS_SKIP) {
                    g_Dialogue.scriptCur += 2;
                    continue;
                }
                i = *g_Dialogue.scriptCur++;
                prim = KO_BOSS_PORTRAIT;
                j = *g_Dialogue.scriptCur++;
                uCoord = KO_BOSS_U_COORDS[j & 1];
                vCoord = KO_BOSS_V_COORDS[j & 1];
                prim->clut = KO_BOSS_PORTRAIT_CLUTS[i];
                prim->tpage = KO_BOSS_PORTRAIT_TPAGE;
                if (j & 0x80) {
                    prim->u0 = prim->u2 = uCoord + 0x2F;
                    prim->u1 = prim->u3 = uCoord;
                } else {
                    prim->u0 = prim->u2 = uCoord;
                    prim->u1 = prim->u3 = uCoord + 0x2F;
                }
                prim->v0 = prim->v1 = vCoord;
                prim->v2 = prim->v3 = vCoord + 0x48;
                prim->x0 = prim->x1 = prim->x2 = prim->x3 =
                    g_Dialogue.startX - 0x1E;
                prim->y0 = prim->y1 = prim->y2 = prim->y3 =
                    g_Dialogue.startY + 0x24;
                g_Dialogue.clutIndex = KO_BOSS_TEXT_CLUTS[i];
                CutsceneUnk1();
                CutsceneUnk4();
                prim->priority = 0x1FE;
                prim->drawMode = DRAW_DEFAULT;
                DrawCutsceneActorName((u16)i, self);
                g_Dialogue.portraitAnimTimer = 6;
                self->step = KO_DLG_START_TEXT;
                return;
            case CSOP_NEXT_DIALOG:
                if (KO_BOSS_SKIP) {
                    continue;
                }
                prim = g_Dialogue.prim[0];
                for (i = 0; i < KO_BOSS_TEXT_PRIMS; i++) {
                    prim->drawMode = DRAW_HIDE;
                    prim = prim->next;
                }
                g_api.FreePrimitives(g_Dialogue.primIndex[1]);
                g_Dialogue.primIndex[1] = -1;
                g_Dialogue.portraitAnimTimer = 6;
                self->step = KO_DLG_UNLOAD_PORTRAIT;
                return;
            case CSOP_SET_POS:
                if (KO_BOSS_SKIP) {
                    g_Dialogue.scriptCur += 2;
                    continue;
                }
                g_Dialogue.startX = *g_Dialogue.scriptCur++;
                g_Dialogue.startY = *g_Dialogue.scriptCur++;
                prim = KO_BOSS_PORTRAIT;
                prim = prim->next;
                prim->y0 = prim->y1 = g_Dialogue.startY;
                prim->y2 = prim->y3 = g_Dialogue.startY + 0x48;
                prim = prim->next;
                prim->y0 = g_Dialogue.startY - 1;
                prim->u0 = 0xF6;
                g_Dialogue.portraitAnimTimer = 0x18;
                self->step = KO_DLG_OPEN_DIALOG_BOX;
                self->step_s = 0;
                return;
            case CSOP_CLOSE_DIALOG:
                if (KO_BOSS_SKIP) {
                    continue;
                }
                g_Dialogue.portraitAnimTimer = 0x18;
                self->step = KO_DLG_CLOSE_DIALOG_BOX;
                return;
            case CSOP_PLAY_SOUND:
                if (KO_BOSS_SKIP) {
                    g_Dialogue.scriptCur += 2;
                    continue;
                }
                nextChar = *g_Dialogue.scriptCur++;
                nextChar <<= 4;
                nextChar |= *g_Dialogue.scriptCur++;
                g_api.PlaySfx(nextChar);
                continue;
            case CSOP_WAIT_FOR_SOUND:
                if (KO_BOSS_SKIP) {
                    continue;
                }
                if (g_api.func_80131F68() != false) {
                    continue;
                }
                g_Dialogue.scriptCur--;
                return;
            case CSOP_SCRIPT_UNKNOWN_11:
                if (KO_BOSS_SKIP) {
                    continue;
                }
                if (g_api.func_80131F68() != true) {
                    continue;
                }
                g_Dialogue.scriptCur--;
                return;
            case CSOP_SET_EVENTS:
                ptr = (u_long)*g_Dialogue.scriptCur++;
                ptr <<= 4;
                ptr |= (u_long)*g_Dialogue.scriptCur++;
                ptr <<= 4;
                ptr |= (u_long)*g_Dialogue.scriptCur++;
                ptr <<= 4;
                ptr |= (u_long)*g_Dialogue.scriptCur++;
                SetCutsceneEvents((u8*)ptr);
                continue;
            case CSOP_SCRIPT_UNKNOWN_13:
                continue;
            case CSOP_SCRIPT_SWITCH:
                ptr = (u_long)*g_Dialogue.scriptCur++;
                ptr <<= 4;
                ptr |= (u_long)*g_Dialogue.scriptCur++;
                ptr <<= 4;
                ptr |= (u_long)*g_Dialogue.scriptCur++;
                ptr <<= 4;
                ptr |= (u_long)*g_Dialogue.scriptCur++;
                g_Dialogue.scriptCur += KO_BOSS_SWITCH_INDEX(ptr) << 2;

                ptr = (u_long)*g_Dialogue.scriptCur++;
                ptr <<= 4;
                ptr |= (u_long)*g_Dialogue.scriptCur++;
                ptr <<= 4;
                ptr |= (u_long)*g_Dialogue.scriptCur++;
                ptr <<= 4;
                ptr |= (u_long)*g_Dialogue.scriptCur;
                g_Dialogue.scriptCur = CS_PTR(ptr);
                continue;
            case CSOP_SCRIPT_UNKNOWN_15:
                ptr = (u_long)*g_Dialogue.scriptCur++;
                ptr <<= 4;
                ptr |= (u_long)*g_Dialogue.scriptCur++;
                ptr <<= 4;
                ptr |= (u_long)*g_Dialogue.scriptCur++;
                ptr <<= 4;
                ptr |= (u_long)*g_Dialogue.scriptCur;
                g_Dialogue.scriptCur = CS_PTR(ptr);
                continue;
            case CSOP_WAIT_FOR_FLAG:
                if (!((g_CutsceneFlags >> *g_Dialogue.scriptCur) & 1)) {
                    g_Dialogue.scriptCur--;
                    return;
                }
                g_CutsceneFlags &= ~(1 << *g_Dialogue.scriptCur);
                g_Dialogue.scriptCur++;
                continue;
            case CSOP_SET_FLAG:
                g_CutsceneFlags |= 1 << *g_Dialogue.scriptCur++;
                continue;
            case CSOP_STOP_EVENTS:
                g_Dialogue.hasEvents = 0;
                continue;
            case CSOP_LOAD_PORTRAIT:
                if (KO_BOSS_SKIP) {
                    g_Dialogue.scriptCur += 5;
                } else {
                    ptr = (u_long)*g_Dialogue.scriptCur++;
                    ptr <<= 4;
                    ptr |= (u_long)*g_Dialogue.scriptCur++;
                    ptr <<= 4;
                    ptr |= (u_long)*g_Dialogue.scriptCur++;
                    ptr <<= 4;
                    ptr |= (u_long)*g_Dialogue.scriptCur++;
                    ptr = (u_long)CS_PTR(ptr);
                    j = *g_Dialogue.scriptCur++;
                    LoadTPage((u_long*)ptr, 1, 0, KO_BOSS_X_VALS[j], 0x100,
                              48, 72);
                }
                continue;
            case CSOP_SCRIPT_UNKNOWN_20:
                nextChar = *g_Dialogue.scriptCur++;
                nextChar <<= 4;
                nextChar |= *g_Dialogue.scriptCur++;
                g_api.PlaySfx(nextChar);
                continue;
            case CSOP_SCRIPT_UNKNOWN_21:
                g_CutsceneFlags = 0;
                KO_BOSS_SKIP = 0;
                KO_BOSS_STARTED = 0;
                continue;
            case CSOP_SCRIPT_UNKNOWN_22:
                g_CutsceneFlags &= ~(1 << *g_Dialogue.scriptCur++);
                continue;
            case CSOP_SCRIPT_UNKNOWN_23:
                return;
            case CSOP_WAIT_FOR_FLAG_RESET:
                if (!((g_CutsceneFlags >> *g_Dialogue.scriptCur) & 1)) {
                    g_Dialogue.scriptCur--;
                    return;
                }
                g_Dialogue.scriptCur++;
                continue;
            default:
                if (KO_BOSS_SKIP) {
                    // a skipped 2-byte KO cell: its 2nd byte is not an op
                    if ((nextChar & 0xFC) == 0xF0) {
                        g_Dialogue.scriptCur++;
                    }
                    continue;
                }
                g_Dialogue.nextCharTimer = g_Dialogue.unk17;
            }
            break;
        }
        if (nextChar == ' ') {
            g_Dialogue.nextCharX += 1; // 4 px space (KO_SPACE_ADV, as rcen)
            return;
        }
        if (KoBossGlyph(nextChar)) {
            break;
        }
        // no KO cell: original 8x8 font on the KO baseline of the buffer
        rect.x = ((nextChar & 0xF) * 2) + 0x380;
        rect.y = ((u32)(nextChar & 0xF0) >> 1) + 0xF0;
        rect.w = 2;
        rect.h = 8;
        vCoord = KO_BOSS_LINE_V(g_Dialogue.nextCharY) + KO_DLG_FB_DY;
        MoveImage(&rect, g_Dialogue.nextCharX, vCoord);
        g_Dialogue.nextCharX += 2;
        break;
    case KO_DLG_LOAD_PORTRAIT:
        KoBossScaleAvatar(2);
        if (g_Dialogue.portraitAnimTimer >= 6) {
            self->step--;
        }
        break;
    case KO_DLG_START_TEXT:
        prim = KO_BOSS_PORTRAIT;
        prim->x0 = prim->x2 -= 4;
        prim->x1 = prim->x3 += 4;
        prim->y0 = prim->y1 -= 6;
        prim->y2 = prim->y3 += 6;
        g_Dialogue.portraitAnimTimer--;
        if (!g_Dialogue.portraitAnimTimer) {
            self->step = KO_DLG_RUN;
            for (prim = &g_PrimBuf[g_Dialogue.primIndex[1]]; prim != NULL;
                 prim = prim->next) {
                prim->drawMode = DRAW_DEFAULT;
            }
        }
        break;
    case KO_DLG_UNLOAD_PORTRAIT:
        prim = KO_BOSS_PORTRAIT;
        prim->x0 = prim->x2 += 4;
        prim->x1 = prim->x3 -= 4;
        prim->y0 = prim->y1 += 6;
        prim->y2 = prim->y3 -= 6;
        if (prim->x1 >= (g_Dialogue.startX - 2)) {
            prim->x1 = prim->x3 = g_Dialogue.startX - 3;
        }
        g_Dialogue.portraitAnimTimer--;
        if (!g_Dialogue.portraitAnimTimer) {
            self->step = KO_DLG_RUN;
        }
        break;
    case KO_DLG_OPEN_DIALOG_BOX:
        switch (self->step_s) {
        case 0:
            KO_BOSS_STARTED = 1;
            primIndex =
                g_api.AllocPrimitives(PRIM_LINE_G2, KO_BOSS_RED_LINE_COUNT);
            if (primIndex == -1) {
                DestroyEntity(self);
                return;
            }
            g_Dialogue.primIndex[0] = primIndex;
            for (prim = &g_PrimBuf[primIndex], uCoord = 0; prim != NULL;
                 prim = prim->next) {
                prim->r0 = prim->r1 = 0x7F;
                prim->b0 = prim->b1 = prim->g0 = prim->g1 = 0;
                prim->x0 = prim->x1 = 0xF7;
                prim->y0 = prim->y1 = g_Dialogue.startY + uCoord;
                prim->priority = 0x1FE;
                prim->drawMode = DRAW_DEFAULT;
                prim->x2 = KO_BOSS_RED_LINES[uCoord];
                prim->x3 = 0xF70;
                uCoord++;
            }
            self->step_s++;
            break;
        case 1:
            uCoord = 0;
            for (prim = &g_PrimBuf[g_Dialogue.primIndex[0]]; prim != NULL;
                 prim = prim->next) {
                prim->x3 -= prim->x2;
                prim->x2 += 2;
                prim->x0 = prim->x3 / 16;
                if (prim->x0 < 5) {
                    prim->x0 = 4;
                } else {
                    uCoord = 1;
                }
            }
            if (!uCoord) {
                g_api.FreePrimitives(g_Dialogue.primIndex[0]);
                g_Dialogue.primIndex[0] = -1;
                prim = KO_BOSS_PORTRAIT;
                prim = prim->next;
                prim->drawMode = DRAW_TPAGE | DRAW_TRANSP;
                prim = prim->next;
                prim->drawMode = DRAW_UNK_40 | DRAW_TPAGE | DRAW_TRANSP;
                self->step_s++;
            }
            break;
        case 2:
            uCoord = 0;
            prim = KO_BOSS_PORTRAIT;
            prim = prim->next;
            if (prim->r0 < 0x10) {
                prim->r0 = prim->r1 = prim->r2 = prim->r3 = 0;
            } else {
                prim->r0 = prim->r1 = prim->r2 = prim->r3 -= 0x10;
                uCoord = 1;
            }
            if (prim->b0 >= 0x7F) {
                prim->b0 = prim->b1 = 0x7F;
            } else {
                prim->b0 = prim->b1 += 8;
                uCoord = 1;
            }
            prim = prim->next;
            if (prim->r0 < 0x10) {
                prim->r0 = prim->g0 = prim->b0 = 0x10;
            } else {
                prim->r0 = prim->g0 = prim->b0 -= 0xF;
                uCoord = 1;
            }
            if (!uCoord) {
                self->step = KO_DLG_RUN;
            }
            break;
        }
        break;
    case KO_DLG_CLOSE_DIALOG_BOX:
        prim = KO_BOSS_PORTRAIT;
        prim = prim->next;
        g_Dialogue.portraitAnimTimer--;
        if (g_Dialogue.portraitAnimTimer >= 0xC) {
            prim = prim->next;
            prim->u0 -= 20;
            if (g_Dialogue.portraitAnimTimer & 1) {
                prim->u0--;
            }
        } else {
            if (!g_Dialogue.portraitAnimTimer) {
                self->step = KO_DLG_RUN;
                prim->drawMode = DRAW_HIDE;
            } else {
                prim->y2 = (prim->y3 -= 6);
            }
            prim = prim->next;
            prim->drawMode = DRAW_HIDE;
        }
        break;
    case KO_DLG_END:
        KoBossEnd(self);
        break;
    }
}
