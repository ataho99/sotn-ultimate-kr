// SPDX-License-Identifier: AGPL-3.0-or-later
#ifdef MOD_KO
#define KO_DLG16 1 /* Korean dialogue renderer here: 16 px line buffers (src/st/ko_font_params.h) */
#endif
#include "nz0.h"
#include "mod_psp_script.h" // the PSP dub toggle's script copies
#include "../pfn_entity_update.h"
#include <cutscene.h>

static u8 D_801813C8[] = {0x00, 0x40, 0x00, 0x00};
static u8 D_801813CC[] = {0x00, 0x00, 0x00, 0x00};
static u16 D_801813D0[] = {0x0238, 0x0240};
static u16 D_801813D4[] = {0x0000, 0x0020};
static u16 D_801813D8[] = {0x01A1, 0x01A1};
static s16 D_801813DC[] = {
    0x0008, 0x0013, 0x0011, 0x0031, 0x004F, 0x0026, 0x0036, 0x001D, 0x001B,
    0x0033, 0x002C, 0x0021, 0x0019, 0x000A, 0x0033, 0x001F, 0x0048, 0x002F,
    0x0013, 0x0019, 0x004D, 0x004B, 0x0017, 0x001D, 0x0012, 0x0002, 0x001B,
    0x002A, 0x0050, 0x0045, 0x0032, 0x000D, 0x002A, 0x004D, 0x0006, 0x0027,
    0x0007, 0x0048, 0x002F, 0x001B, 0x0036, 0x0022, 0x0039, 0x0014, 0x0039,
    0x001D, 0x000A, 0x0035, 0x0010, 0x001B, 0x003D, 0x0017, 0x002E, 0x000B,
    0x0049, 0x0042, 0x003D, 0x002A, 0x0001, 0x000C, 0x001B, 0x0034, 0x0041,
    0x0035, 0x0008, 0x000E, 0x004D, 0x0011, 0x0034, 0x0041, 0x0029, 0x0048};
static const char* actor_names[] = {_S("ALUCARD"), _S("MARIA")};
#ifdef MOD_KO
static const u8 ko_name_0[] = {0xF0, 0x6F, 0xF2, 0x06, 0xF0, 0xB2};
static const u8 ko_name_1[] = {0xF0, 0x52, 0xF0, 0x50, 0xF0, 0x6C};
static const u8* const actor_names_ko[] = {ko_name_0, ko_name_1};
static const u8 actor_names_ko_len[] = {6, 6};
#endif

extern u32 g_SkipCutscene;
extern Dialogue g_Dialogue;
extern u32 g_CutsceneFlags;
extern s32 g_IsCutsceneDone; // bss
// This file consists of EntityCutscene, and all the functions it calls.
// No function in NZ0 calls anything in there, nor does anything in here call
// any NZ0 functions, besides the ones in this file.

#include "../cutscene_unk1.h"

#include "../set_cutscene_script.h"

#include "../cutscene_unk3.h"

#include "../cutscene_unk4.h"

#include "../cutscene_actor_name.h"

#include "../set_cutscene_events.h"

#include "../cutscene_events.h"

#include "../cutscene_scale_avatar.h"

// cutscene where alucard and maria discuss castle changing
extern u8 cutscene_data[];
void EntityCutscene(Entity* self) {
    s16 uCoord;
    s16 vCoord;
    s32 primIndex;
    u16* sp44;
    s32 sp40;
    RECT rect;
    u16 nextChar;
    s32 var_s7;
    u8* var_s6;
    s32 var_s5;
    u_long ptr;
    u32 var_s3;
    Primitive* prim;
    u8* var_s1;
    s32 i;

    if (self->step) {
        if (g_IsCutsceneDone && !g_SkipCutscene &&
            (g_Settings.D_8003CB04 & 0x100 || g_GameClearFlag)) {
            // CutsceneSkip function inlined
            if (g_pads[0].tapped == PAD_START) {
                g_SkipCutscene = 1;
                g_api.FreePrimitives(self->primIndex);
                self->flags ^= FLAG_HAS_PRIMS;
                if (g_Dialogue.primIndex[1] != -1) {
                    g_api.FreePrimitives(g_Dialogue.primIndex[1]);
                }
                if (g_Dialogue.primIndex[0] != -1) {
                    g_api.FreePrimitives(g_Dialogue.primIndex[0]);
                }
                g_api.PlaySfx(SET_STOP_MUSIC);
                self->step = 1;
                self->step_s = 0;
            }
        }
    }
    if (self->step && g_Dialogue.hasEvents) {
        RunCutsceneEvents();
    }
    switch (self->step) {
    case 0:
        if (g_CastleFlags[MET_MARIA_IN_NZ0] ||
            g_PlayableCharacter != PLAYER_ALUCARD) {
            DestroyEntity(self);
            return;
        }
        if (SetCutsceneScript(cutscene_data)) {
            self->flags |= FLAG_HAS_PRIMS | FLAG_UNK_2000;
            self->primIndex = g_Dialogue.primIndex[2];
            g_CutsceneFlags = 0;
            g_IsCutsceneDone = 0;
            g_SkipCutscene = 0;
            g_CutsceneHasControl = 1;
            self->step++;
        }
        break;
    case 1:
        // this is a huge While-loop! Don't miss it!
        while (1) {
            if (g_Dialogue.nextCharTimer && !g_SkipCutscene) {
                g_Dialogue.nextCharTimer--;
                return;
            }
            nextChar = *g_Dialogue.scriptCur++;
            switch (nextChar) {
            case CSOP_END_CUTSCENE:
                self->step = 7;
                return;
            case CSOP_LINE_BREAK:
                if (g_SkipCutscene) {
                    continue;
                }
                g_Dialogue.nextCharX = g_Dialogue.nextLineX;
                if (!(g_Dialogue.unk12 & 1)) {
#ifdef MOD_KO
                    g_Dialogue.nextLineY += KO_LINE_STEP(16);
#else
                    g_Dialogue.nextLineY += 12;
#endif
                }
                g_Dialogue.nextCharY++;
#ifdef MOD_KO
                g_Dialogue.nextCharY &= 3;
#else
                if (g_Dialogue.nextCharY > 4) {
                    g_Dialogue.nextCharY = 0;
                }
#endif
                CutsceneUnk4();
                if (!(g_Dialogue.unk12 & 1)) {
#ifdef MOD_KO
                    if (KO_SCROLL_AT(g_Dialogue.nextCharY, g_Dialogue.nextCharY > 2)) {
#else
                    if (g_Dialogue.nextCharY > 3) {
#endif
                        g_Dialogue.unk12 |= 1;
                        g_Dialogue.portraitAnimTimer = 0;
                        self->step_s = 0;
                        self->step++;
                        return;
                    }
                } else {
                    g_Dialogue.portraitAnimTimer = 0;
                    self->step_s = 0;
                    self->step++;
                    return;
                }
                continue;
            case CSOP_SET_SPEED:
                g_Dialogue.unk17 = *g_Dialogue.scriptCur++;
                continue;
            case CSOP_SET_WAIT:
                g_Dialogue.nextCharTimer = *g_Dialogue.scriptCur++;
                if (g_SkipCutscene) {
                    continue;
                }
                return;
            case CSOP_HIDE_DIALOG:
                if (g_SkipCutscene) {
                    continue;
                }
                prim = g_Dialogue.prim[0];
                for (i = 0; i < LEN(g_Dialogue.prim) - 1; i++) {
                    prim->drawMode = DRAW_HIDE;
                    prim = prim->next;
                }
                return;
            case CSOP_SET_PORTRAIT:
                if (g_SkipCutscene) {
                    g_Dialogue.scriptCur += 2;
                    continue;
                }
                i = *g_Dialogue.scriptCur++;
                prim = g_Dialogue.prim[LEN(g_Dialogue.prim) - 1];
                var_s5 = *g_Dialogue.scriptCur++;
                var_s7 = var_s5 & 1;
                uCoord = D_801813C8[var_s7];
                vCoord = D_801813CC[var_s7];
                prim->clut = D_801813D0[i];
                prim->tpage = 0x90;
                if (var_s5 & 0x80) {
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
                g_Dialogue.clutIndex = D_801813D8[i];
                CutsceneUnk1();
                CutsceneUnk4();
                prim->priority = 0x1FE;
                prim->drawMode = DRAW_DEFAULT;
                DrawCutsceneActorName(i, self);
                g_Dialogue.portraitAnimTimer = 6;
                self->step = 3;
                return;
            case CSOP_NEXT_DIALOG:
                if (g_SkipCutscene) {
                    continue;
                }
                for (prim = g_Dialogue.prim[0], i = 0;
                     i < LEN(g_Dialogue.prim) - 1; i++) {
                    prim->drawMode = DRAW_HIDE;
                    prim = prim->next;
                }
                g_api.FreePrimitives(g_Dialogue.primIndex[1]);
                g_Dialogue.primIndex[1] = -1;
                g_Dialogue.portraitAnimTimer = 6;
                self->step = 4;
                return;
            case CSOP_SET_POS:
                if (g_SkipCutscene) {
                    g_Dialogue.scriptCur += 2;
                    continue;
                }
                g_Dialogue.startX = *g_Dialogue.scriptCur++;
                g_Dialogue.startY = *g_Dialogue.scriptCur++;
                prim = g_Dialogue.prim[LEN(g_Dialogue.prim) - 1];
                prim = prim->next;
                prim->y0 = prim->y1 = g_Dialogue.startY;
                prim->y2 = prim->y3 = g_Dialogue.startY + 0x48;
                prim = prim->next;
                prim->y0 = g_Dialogue.startY - 1;
                prim->u0 = 0xF6;
                g_Dialogue.portraitAnimTimer = 0x18;
                self->step = 5;
                self->step_s = 0;
                return;
            case CSOP_CLOSE_DIALOG:
                if (g_SkipCutscene) {
                    continue;
                }
                g_Dialogue.portraitAnimTimer = 0x18;
                self->step = 6;
                return;
            case CSOP_PLAY_SOUND:
                if (g_SkipCutscene) {
                    g_Dialogue.scriptCur += 2;
                    continue;
                }
                nextChar = *g_Dialogue.scriptCur++;
                nextChar <<= 4;
                nextChar |= *g_Dialogue.scriptCur++;
                g_api.PlaySfx(nextChar);
                continue;
            case CSOP_WAIT_FOR_SOUND:
                if (g_SkipCutscene) {
                    continue;
                }
                if (g_api.func_80131F68() != false) {
                    continue;
                }
                g_Dialogue.scriptCur--;
                return;
            case CSOP_SCRIPT_UNKNOWN_11:
                if (g_SkipCutscene) {
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
                g_Dialogue.scriptCur += *CS_NEXT(ptr) * 4;

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
                if (g_SkipCutscene) {
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
                    var_s5 = g_Dialogue.scriptCur++[0];
                    LoadTPage((u_long*)ptr, 1, 0, D_801813D4[var_s5], 0x100,
                              0x30, 0x48);
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
                g_SkipCutscene = 0;
                g_IsCutsceneDone = 0;
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
                if (g_SkipCutscene) {
                    continue;
                }
                g_Dialogue.nextCharTimer = g_Dialogue.unk17;
            }
            break; // This breaks the big while-loop!
        }
        if (nextChar == 0x20) {
#ifdef MOD_KO
            g_Dialogue.nextCharX += 1; /* 4px space, KO_SPACE_ADV */
#else
            g_Dialogue.nextCharX += 2;
#endif
            return;
        }
#ifdef MOD_KO
        /* ko engtab dialogue ASCII: . , ! ? digits/letters in KO dialogue use the MGalmuri12 cells via
         * KO_ENGTAB (94xu16 at 0x8009EF00), 12-col direct + kind0, advance KO_ENG_ADV (4/8 px). * A miss (0) keeps the original-font MoveImage path below. */
        if (nextChar > 0x20 && nextChar < 0x7F && ((const u16*)0x8009EF00)[nextChar - 0x21]) {
            u_long* eng_glyph = (u_long*)g_api.func_80106A28(((const u16*)0x8009EF00)[nextChar - 0x21], 1);
            if (eng_glyph) {
                LoadTPage(eng_glyph, 0, 0, g_Dialogue.nextCharX, (g_Dialogue.nextCharY * 16) + 0x180, 0xC, 0x10);
                g_Dialogue.nextCharX += KO_ENG_ADV(nextChar);
                return;
            }
        }
        if ((nextChar & 0xFC) == 0xF0 || (nextChar >= 0x80 && nextChar < 0xF0)) {
            s32 ko_ch;
            u_long* ko_glyph;
            s32 ko_y;
            if (nextChar >= 0x80 && nextChar < 0xF0) {
                /* 1-byte promotion: KO_PROMO_TBL_ADDR (dra.h), gen_ko_promo.py */
                ko_ch = ((u16*)0x8009F000)[nextChar - 0x80];
            } else {
                ko_ch = (nextChar << 8) | *g_Dialogue.scriptCur++;
            }
            ko_glyph = (u_long*)g_api.func_80106A28(ko_ch, 1);
            if (ko_glyph) {
                ko_y = (g_Dialogue.nextCharY * 16) + 0x180;
                LoadTPage(
                    ko_glyph, 0, 0, g_Dialogue.nextCharX, ko_y, 0xC, 0x10);
                g_Dialogue.nextCharX += 3;
                return;
            }
        }
#endif
        rect.x = ((nextChar & 0xF) << 1) + 0x380;
        rect.y = ((s32)(nextChar & 0xF0) >> 1) + 0xF0;
        rect.w = 2;
        rect.h = 8;
#ifdef KO_DLG16
        vCoord = (g_Dialogue.nextCharY * 16) + 0x180 + KO_DLG_FB_DY; /* 8x8 fallback on the 16 px buffer, KO baseline */
#else
        vCoord = (g_Dialogue.nextCharY * 0xC) + 0x180;
#endif
        MoveImage(&rect, g_Dialogue.nextCharX, vCoord);
        g_Dialogue.nextCharX += 2;
        break;
    case 2:
        ScaleCutsceneAvatar(2);
        if (g_Dialogue.portraitAnimTimer >= 6) {
            self->step--;
        }
        break;
    case 3:
        prim = g_Dialogue.prim[LEN(g_Dialogue.prim) - 1];
        prim->x0 = prim->x2 -= 4;
        prim->x1 = prim->x3 += 4;
        prim->y0 = prim->y1 -= 6;
        prim->y2 = prim->y3 += 6;
        g_Dialogue.portraitAnimTimer--;
        if (!g_Dialogue.portraitAnimTimer) {
            self->step = 1;
            for (prim = &g_PrimBuf[g_Dialogue.primIndex[1]]; prim != NULL;
                 prim = prim->next) {
                prim->drawMode = DRAW_DEFAULT;
            }
        }
        break;
    case 4:
        prim = g_Dialogue.prim[LEN(g_Dialogue.prim) - 1];
        prim->x0 = prim->x2 += 4;
        prim->x1 = prim->x3 -= 4;
        prim->y0 = prim->y1 += 6;
        prim->y2 = prim->y3 -= 6;
        if (prim->x1 >= (g_Dialogue.startX - 2)) {
            prim->x1 = prim->x3 = g_Dialogue.startX - 3;
        }
        g_Dialogue.portraitAnimTimer--;
        if (!g_Dialogue.portraitAnimTimer) {
            self->step = 1;
            return;
        }
        break;
    case 5:
        switch (self->step_s) {
        case 0:
            g_IsCutsceneDone = 1;
            primIndex = g_api.AllocPrimitives(PRIM_LINE_G2, 0x48);
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
                prim->x2 = D_801813DC[uCoord];
                prim->x3 = 0xF70;
                uCoord++;
            }
            self->step_s++;
            break;
        case 1:
            for (uCoord = 0, prim = &g_PrimBuf[g_Dialogue.primIndex[0]];
                 prim != NULL; prim = prim->next) {
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
                prim = g_Dialogue.prim[LEN(g_Dialogue.prim) - 1];
                prim = prim->next;
                prim->drawMode = DRAW_TPAGE | DRAW_TRANSP;
                prim = prim->next;
                prim->drawMode = DRAW_UNK_40 | DRAW_TPAGE | DRAW_TRANSP;
                self->step_s++;
            }
            break;
        case 2:
            uCoord = 0;
            prim = g_Dialogue.prim[LEN(g_Dialogue.prim) - 1];
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
                self->step = 1;
            }
        }
        break;
    case 6:
        prim = g_Dialogue.prim[LEN(g_Dialogue.prim) - 1];
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
                self->step = 1;
                prim->drawMode = DRAW_HIDE;
            } else {
                prim->y2 = (prim->y3 -= 6);
            }
            prim = prim->next;
            prim->drawMode = DRAW_HIDE;
        }
        break;
    case 7:
        DestroyEntity(self);
        g_CastleFlags[MET_MARIA_IN_NZ0] = 1;
        g_CutsceneHasControl = 0;
        g_Settings.D_8003CB04 |= 0x100;
        break;
    }
}

#ifdef MOD_SATURN
// the PSP English script: room for the lines that do not fit the retail
// script's bytes (mods/sotn-ultimate/tools/psp_script.py; empty when not
// needed)
#endif
