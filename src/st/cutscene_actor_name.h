// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ko_font_params.h"
#if defined(VERSION_PSP) || defined(VERSION_HD)
#include "cutscene_actor_name_psp.h"
#else
extern const char* actor_names[];

#ifndef DRAW_NAME_ACTOR_INDEX
#define DRAW_NAME_ACTOR_INDEX actorIndex
#endif
#if defined(MOD_KO) && defined(KO_HANGUL_NAMES) /* maintainer 결정 20:38: 화자 이름은 원판 영어. 한글 경로는 KO_HANGUL_NAMES 정의 시에만 */
// KO trial actor name tables (ko_dlg2 #127): F0xx raw bytes + byte lengths
extern const u8* const actor_names_ko[];
extern const u8 actor_names_ko_len[];
#endif

// Creates primitives for the actor name at the head of the dialogue
static void DrawCutsceneActorName(u16 actorIndex, Entity* self) {
    Primitive* prim;
    s16 primIndex;
    s32 x;
    u16 chCount;
    const char* actorName;
    char ch;
    s32 kAdv;

#if defined(MOD_KO) && defined(KO_HANGUL_NAMES) /* maintainer 결정 20:38: 화자 이름은 원판 영어. 한글 경로는 KO_HANGUL_NAMES 정의 시에만 */
    // KO names: ASCII keeps the 8x8 SPRT path; Hangul (F0xx lead) renders
    // 12x16 via func_80106A28 + LoadTPage (same idiom as KO dialogue).
    // SPRTs are zero-size (lifecycle only: alloc count, step-3 show,
    // NEXT_DIALOG free); pixels come from the direct upload.
    {
        const u8* nameKo;
        const u8* pKo;
        u8 cKo;
        u8 cKo2;
        u8 remKo;
        u16 nKo;
        u16 kCh;
        u_long* kGlyph;
        s32 kY;
        s32 kN;
        nameKo = actor_names_ko[DRAW_NAME_ACTOR_INDEX];
        remKo = actor_names_ko_len[DRAW_NAME_ACTOR_INDEX];
        pKo = nameKo;
        nKo = 0;
        while (remKo) {
            cKo = *pKo++;
            remKo--;
            if ((cKo & 0xFC) == 0xF0) {
                pKo++;
                remKo--;
            }
            if (cKo != CH(' ')) {
                nKo++;
            }
        }
        primIndex = g_api.AllocPrimitives(PRIM_SPRT, nKo);
        if (primIndex == -1) {
            DestroyEntity(self);
            return;
        }
        g_Dialogue.primIndex[1] = primIndex;
        prim = &g_PrimBuf[primIndex];
        pKo = nameKo;
        remKo = actor_names_ko_len[DRAW_NAME_ACTOR_INDEX];
        x = 0x38;
        kN = 6; /* slots from x 18 / u 72: clear of Maria's slot (CEN g_PlayerDraw[8], x 0..17, y 443..511) */
        while (prim != NULL && remKo) {
            cKo = *pKo++;
            remKo--;
            if (cKo == CH(' ')) {
                x += FONT_SPACE;
            } else if ((cKo & 0xFC) == 0xF0) {
                cKo2 = *pKo++;
                remKo--;
                kCh = (cKo << 8) | cKo2;
                kGlyph = (u_long*)g_api.func_80106A28(kCh, 1);
#ifdef DRAW_NAME_PRIM_Y0
                kY = DRAW_NAME_PRIM_Y0 - KO_NAME_DY;
#else
                kY = g_Dialogue.startY + 6 - KO_NAME_DY;
#endif
                /* trial29: upload into the dialogue text texture area
                 * (page 0x10, row 4 = y 448, hw x = 3*n, n from 6) and draw each glyph
                 * as a real 12x16 SPRT (the old direct write into the display
                 * buffer was erased by the next redraw). */
                if (kGlyph) {
                    /* func_80106A28 returns ONE shared glyph buffer and the
                     * upload is queued: uploading it 4x in a frame made every
                     * cell read the last glyph (trial29: 라라라라). Copy each
                     * glyph to its own slot first (A-gap F000.., 96B each,
                     * unused while a cutscene runs). DrawSync is not linked
                     * into the stage overlays. */
                    u_long* kDst = (u_long*)(0x8009F000 - 6 * 0x60 + kN * 0x60); /* still F000.. for the 1st glyph */
                    s32 kI;
                    for (kI = 0; kI < 24; kI++) {
                        kDst[kI] = kGlyph[kI];
                    }
                    LoadTPage(kDst, 0, 0, kN * 3, 448, 0xC, 0x10);
                }
                prim->type = PRIM_SPRT;
                prim->tpage = 0x10;
                prim->clut = g_Dialogue.clutIndex;
                prim->u0 = kN * 12;
                prim->v0 = 192;
                prim->u1 = 12;
                prim->v1 = 16;
                kN++;
                prim->priority = 0x1FF;
                prim->drawMode = DRAW_HIDE;
                prim->x0 = x;
                prim->y0 = kY;
                prim = prim->next;
                x += 12;
            }
            /* trial29: KO name tables are all Hangul pairs (checked); the 8x8
             * ASCII branch was dead code and its bytes are needed in the
             * stsel/stno3 overlays. */
        }
        return;
    }
#endif

    actorName = actor_names[DRAW_NAME_ACTOR_INDEX];
    chCount = 0;
    while (true) {
        ch = *actorName++;
        if (ch == DIAG_EOL) {
            ch = *actorName++;
            if (ch == DIAG_EOS) {
                break;
            }
        }
        if (ch == CH(' ')) {
            continue;
        }
        chCount++;
    }

    // Create chCount amount of sprites based on the actor name's letter count
    primIndex = g_api.AllocPrimitives(PRIM_SPRT, chCount);
    if (primIndex == -1) {
        DestroyEntity(self);
        return;
    }

    // Fill prims to render the actor name on screen
    prim = &g_PrimBuf[primIndex];
    g_Dialogue.primIndex[1] = primIndex;
    actorName = actor_names[DRAW_NAME_ACTOR_INDEX];
    x = 0x38;
    while (prim != NULL) {
        ch = *actorName++;
        if (ch == CH(' ')) {
            x += KO_NAME_SP;
        } else {
            prim->type = PRIM_SPRT;
            prim->tpage = 0x1E;
            prim->clut = PAL_UNK_196;
            prim->u0 = (ch & 0x0F) * FONT_W;
            prim->v0 = (ch & 0xF0) / (FONT_H / 4);
            prim->v1 = FONT_H;
            prim->u1 = FONT_W;
            prim->priority = 0x1FF;
            prim->drawMode = DRAW_HIDE;
            kAdv = KO_NAME_LOOK(ch);
            prim->x0 = x + KO_NAME_OFF(kAdv);
#ifdef DRAW_NAME_PRIM_Y0
            prim->y0 = DRAW_NAME_PRIM_Y0 - KO_NAME_DY;
#else
            prim->y0 = g_Dialogue.startY + 6 - KO_NAME_DY;
#endif
            prim = prim->next;
            x += KO_NAME_ADV(kAdv); /* KO variable (sihum40), retail FONT_GAP 8 */
        }
    }
}
#endif
