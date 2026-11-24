// SPDX-License-Identifier: AGPL-3.0-or-later
#include "stage.h"

#ifndef STAGE_FLAG
#define STAGE_FLAG NULL_STAGE_FLAG
#endif

#ifdef STAGE_NAME_BOX_MARGIN
#define STAGE_NAME_BOX_LEFT_X STAGE_NAME_BOX_MARGIN
#define STAGE_NAME_BOX_RIGHT_X 256 - STAGE_NAME_BOX_MARGIN
#else
#define STAGE_NAME_BOX_LEFT_X 12
#define STAGE_NAME_BOX_RIGHT_X 244
#endif

enum StageNameSteps {
    STAGE_NAME_INIT = 0,
    STAGE_NAME_RENDER = 1,
    STAGE_NAME_WAIT = 2,
    STAGE_NAME_DISSOLVE = 3,
    STAGE_NAME_SET_FLAG = 32,
};

enum StageNameSubSteps {
    STAGE_NAME_RENDER_INIT,
    STAGE_NAME_EXPAND_BOX,
    STAGE_NAME_TEXT_OUTLINE,
    STAGE_NAME_TEXT_FILL,
    STAGE_NAME_RENDER_COMPLETE,
};

enum StageNameDissolveSubsteps {
    STAGE_NAME_DISSOLVE_INIT,
    STAGE_NAME_DISSOLVE_PRIM_INIT = 1,
    STAGE_NAME_DISSOLVE_TEXT = 2,
    STAGE_NAME_SHRINK_BOX = 3,
};

#include "stage_name_dissolver.h"
#include "../../mods/sotn-ultimate/src/ptbr/stage_name_ptbr.h"

extern EInit g_EInitInteractable;

// This is the banner version which fades the text in and then dissolves it away
void EntityStageNamePopup(Entity* self) {
    u8 pad[100];
    Primitive* prim;
    s32 i;
    u8 primsSet;
    s16 primIndex;
    s32 drawMode;
    SN_LOCAL;

    switch (self->step) {
    case STAGE_NAME_INIT:
        if (g_DemoMode != Demo_None || g_CastleFlags[STAGE_FLAG]) {
            DestroyEntity(self);
            return;
        }

        InitializeEntity(g_EInitInteractable);
        ModPtbrStageName(self);
#if defined(MOD_PTBR) && !defined(MOD_PTBR_NAME_NONE)
        SN_LOAD();
#endif
        primIndex = g_api.AllocPrimitives(PRIM_GT4, 7 + 2 * SN_STRIPS);
        if (primIndex == -1) {
            DestroyEntity(self);
            return;
        }
        self->primIndex = primIndex;
        self->flags |= FLAG_HAS_PRIMS;
        prim = &g_PrimBuf[primIndex];
        self->ext.stpopup.firstTextPrim = prim;
        while (prim != NULL) {
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;
        }
        prim = self->ext.stpopup.firstTextPrim;
        for (i = 0; i < 2; i++) {
            prim->tpage = 17;
            prim->clut = PAL_UNK_19F; // Black to white gradient
            prim->u0 = prim->u2 = 4;
            prim->u1 = prim->u3 = 120;
            prim->v0 = prim->v1 = 0;
            prim->v2 = prim->v3 = SN_ROWS;
            prim->x2 = prim->x0 = 12;
            prim->y1 = prim->y0 = SN_TOP;
            prim->x3 = prim->x1 = 128;
            prim->y3 = prim->y2 = SN_TOP + SN_ROWS;
            prim->priority = 192 - i;
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;

            prim->tpage = 17;
            prim->clut = PAL_UNK_19F;
            prim->u0 = prim->u2 = 8;
            prim->u1 = prim->u3 = 124;
            prim->v0 = prim->v1 = 64;
            prim->v2 = prim->v3 = 64 + SN_ROWS;
            prim->x2 = prim->x0 = 128;
            prim->y1 = prim->y0 = SN_TOP;
            prim->x3 = prim->x1 = 244;
            prim->y3 = prim->y2 = SN_TOP + SN_ROWS;
            prim->priority = 192 - i;
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;
        }

        i = 0;
        self->ext.stpopup.firstOutlinePrim = prim;
        prim->tpage = 17;
        prim->clut = PAL_UNK_19D; // Light grey to black gradient
        prim->u0 = prim->u2 = 4;
        prim->u1 = prim->u3 = 120;
        prim->v0 = prim->v1 = 0;
        prim->v2 = prim->v3 = SN_ROWS;
        prim->x2 = prim->x0 = 12;
        prim->y1 = prim->y0 = SN_TOP;
        prim->x3 = prim->x1 = 128;
        prim->y3 = prim->y2 = SN_TOP + SN_ROWS;
        prim->priority = 191;
        prim->drawMode = DRAW_HIDE;
        prim = prim->next;
        prim->clut = PAL_UNK_19D;
        prim->tpage = 17;
        prim->u0 = prim->u2 = 8;
        prim->u1 = prim->u3 = 124;
        prim->v0 = prim->v1 = 64;
        prim->v2 = prim->v3 = 64 + SN_ROWS;
        prim->x2 = prim->x0 = 128;
        prim->y1 = prim->y0 = SN_TOP;
        prim->x3 = prim->x1 = 244;
        prim->y3 = prim->y2 = SN_TOP + SN_ROWS;
        prim->priority = 191;
        prim->drawMode = DRAW_HIDE;
        prim = prim->next;
        self->ext.stpopup.boxPrim = prim;
        prim->type = PRIM_G4;
        prim->tpage = 26;
        prim->clut = PAL_FILL_WHITE; // All white
        prim->u0 = 0;
        prim->u1 = 64;
        prim->u2 = prim->u0;
        prim->u3 = prim->u1;
        prim->v0 = 192;
        prim->v1 = prim->v0;
        prim->v2 = 255;
        prim->v3 = prim->v2;
        prim->x2 = prim->x0 = STAGE_NAME_BOX_LEFT_X;
        prim->y1 = prim->y0 = SN_TOP - 2;
        prim->x3 = prim->x1 = STAGE_NAME_BOX_RIGHT_X;
        prim->y3 = prim->y2 = SN_TOP + SN_ROWS + 2;
        prim->r0 = 16;
        prim->g0 = 40;
        prim->b0 = 0;
        LOW(prim->r1) = LOW(prim->r0);
        prim->r2 = 8;
        prim->g2 = 0;
        prim->b2 = 56;
        LOW(prim->r3) = LOW(prim->r2);
        prim->priority = 176;
        prim->drawMode = DRAW_TRANSP | DRAW_TPAGE | DRAW_COLORS | DRAW_HIDE;
        prim = prim->next;
        self->ext.stpopup.firstDissolvePrim = prim;
        for (i = 0; i < SN_STRIPS; i++) {
            prim->tpage = 17;
            prim->clut = PAL_UNK_19F; // Black to white gradient
            prim->v2 = prim->v3 = (i << SN_SH) + (1 << SN_SH);
            prim->y1 = prim->y0 = (i << SN_SH) + SN_TOP;
            prim->r0 = 120;
            prim->g0 = 120;
            prim->b0 = 168;
            prim->y3 = prim->y2 = (i << SN_SH) + SN_TOP + (1 << SN_SH);
            prim->v0 = prim->v1 = i << SN_SH;
            prim->x2 = prim->x0 = 12;
            prim->x3 = prim->x1 = 128;
            prim->u2 = prim->u0 = 4;
            prim->u1 = prim->u3 = 120;
            primIndex = 120; // FAKE
            prim->priority = 192;
            prim->drawMode = DRAW_COLORS | DRAW_HIDE;
            LOW(prim->r1) = LOW(prim->r0);
            LOW(prim->r2) = LOW(prim->r0);
            LOW(prim->r3) = LOW(prim->r0);
            prim = prim->next;
        }
        for (i = 0; i < SN_STRIPS; i++) {
            prim->tpage = 17;
            prim->clut = PAL_UNK_19F;
            prim->y1 = prim->y0 = (i << SN_SH) + SN_TOP;
            prim->y3 = prim->y2 = (i << SN_SH) + SN_TOP + (1 << SN_SH);
            prim->v0 = prim->v1 = (i << SN_SH) + 64;
            prim->x2 = prim->x0 = 128;
            prim->x3 = prim->x1 = 244;
            prim->u2 = prim->u0 = 8;
            prim->u1 = prim->u3 = 124;
            prim->v2 = prim->v3 = (i << SN_SH) + 64 + (1 << SN_SH);
            prim->r0 = 120;
            prim->g0 = 120;
            prim->b0 = 168;
            prim->priority = 192;
            prim->drawMode = DRAW_COLORS | DRAW_HIDE;
            LOW(prim->r1) = LOW(prim->r0);
            LOW(prim->r2) = LOW(prim->r0);
            LOW(prim->r3) = LOW(prim->r0);
            prim = prim->next;
        }
        self->ext.stpopup.timer = 32;
    case STAGE_NAME_RENDER:
        switch (self->step_s) {
        case STAGE_NAME_RENDER_INIT:
            if (self->ext.stpopup.timer) {
                self->ext.stpopup.timer--;
            } else {
                prim = self->ext.stpopup.boxPrim;
                prim->x0 = prim->x2 = 128;
                prim->x1 = prim->x3 = 128;
                prim->y0 = prim->y1 = 177;
                prim->y2 = prim->y3 = 177;
                prim->drawMode = DRAW_TPAGE | DRAW_COLORS | DRAW_TRANSP;
                self->step_s++;
            }
            break;
        case STAGE_NAME_EXPAND_BOX:
            primsSet = 0;
            prim = self->ext.stpopup.boxPrim;
            if (prim->y0 > SN_TOP - 2) {
                prim->y0 -= 4;
                prim->y1 = prim->y0;
                prim->y2 += 4;
                prim->y3 = prim->y2;
                primsSet = 1;
            }
            if (prim->x0 > STAGE_NAME_BOX_LEFT_X) {
                prim->x0 -= 8;
                prim->x2 = prim->x0;
                prim->x1 += 8;
                prim->x3 = prim->x1;
                primsSet |= 1;
            }
            if (!primsSet) {
                self->ext.stpopup.timer = 0;
                self->ext.stpopup.textOutlineBrightness = 0;
                self->ext.stpopup.textFillBrightness = 0;
                self->step_s++;
            }
            break;
        case STAGE_NAME_TEXT_OUTLINE:
            self->ext.stpopup.textOutlineBrightness += 3;
            if (self->ext.stpopup.textOutlineBrightness >= 128) {
                self->step_s++;
            }
            prim = self->ext.stpopup.firstOutlinePrim;
            for (i = 0; i < 2; i++) {
                prim->drawMode = DRAW_UNK_40 | DRAW_TPAGE2 | DRAW_TPAGE |
                                 DRAW_COLORS | DRAW_TRANSP;
                PGREY(prim, 0) = PGREY(prim, 1) = PGREY(prim, 2) =
                    PGREY(prim, 3) = self->ext.stpopup.textOutlineBrightness;
                prim = prim->next;
            }
            break;
        case STAGE_NAME_TEXT_FILL:
            self->ext.stpopup.textOutlineBrightness -= 2;
            if (self->ext.stpopup.textOutlineBrightness < 0) {
                self->ext.stpopup.textOutlineBrightness = 0;
            }
            // Fade out outline as fill fades in
            prim = self->ext.stpopup.firstOutlinePrim;
            for (i = 0; i < 2; i++) {
                prim->drawMode =
                    DRAW_TPAGE2 | DRAW_TPAGE | DRAW_COLORS | DRAW_TRANSP;
                PGREY(prim, 0) = PGREY(prim, 1) = PGREY(prim, 2) =
                    PGREY(prim, 3) = self->ext.stpopup.textOutlineBrightness;
                prim = prim->next;
            }
            if (self->ext.stpopup.textOutlineBrightness) {
                self->ext.stpopup.textFillBrightness =
                    128 - self->ext.stpopup.textOutlineBrightness;
            } else {
                self->ext.stpopup.textFillBrightness += 4;
            }
            if (self->ext.stpopup.textFillBrightness >= 128) {
                self->ext.stpopup.textFillBrightness = 128;
                self->step_s++;
            }
            prim = self->ext.stpopup.firstTextPrim;
            for (i = 0; i < 2; i++) {
                prim->drawMode =
                    DRAW_TPAGE2 | DRAW_TPAGE | DRAW_COLORS | DRAW_TRANSP;
                PGREY(prim, 0) = PGREY(prim, 1) = PGREY(prim, 2) =
                    PGREY(prim, 3) = self->ext.stpopup.textFillBrightness;
                prim = prim->next;
            }
            for (i = 0; i < 2; i++) {
                prim->drawMode = DRAW_TPAGE | DRAW_COLORS | DRAW_TRANSP;
                PGREY(prim, 0) = PGREY(prim, 1) = PGREY(prim, 2) =
                    PGREY(prim, 3) = self->ext.stpopup.textFillBrightness;
                prim = prim->next;
            }
            break;
        case STAGE_NAME_RENDER_COMPLETE:
            prim = self->ext.stpopup.firstOutlinePrim;
            prim->drawMode = DRAW_HIDE;
            self->ext.stpopup.timer = 64;
            self->step = STAGE_NAME_WAIT;
            self->step_s = 0;
            g_CastleFlags[STAGE_FLAG] = true;
            break;
        }
        break;
    case STAGE_NAME_WAIT:
        if (!--self->ext.stpopup.timer) {
            self->step_s = STAGE_NAME_DISSOLVE_INIT;
            self->step = STAGE_NAME_DISSOLVE;
        }
        break;
    case STAGE_NAME_DISSOLVE:
        switch (self->step_s) {
        case STAGE_NAME_DISSOLVE_INIT:
            prim = self->ext.stpopup.firstTextPrim;
            prim = prim->next;
            prim = prim->next;
            // Fades the second pair of prims before setting up dissolve prims
            for (i = 0; i < 2; i++) {
                prim->drawMode = DRAW_TPAGE | DRAW_COLORS | DRAW_TRANSP;
                PGREY(prim, 0) = PGREY(prim, 1) = PGREY(prim, 2) =
                    PGREY(prim, 3) = self->ext.stpopup.textFillBrightness;
                prim = prim->next;
            }
            self->ext.stpopup.textFillBrightness -= 4;
            if (self->ext.stpopup.textFillBrightness < 0) {
                self->ext.stpopup.textFillBrightness = 0;
                self->step_s++;
                break;
            }
            break;
        case STAGE_NAME_DISSOLVE_PRIM_INIT:
            prim = self->ext.stpopup.firstTextPrim;
            // Hide all text prims
            for (i = 0; i < 4; i++) {
                prim->drawMode = DRAW_HIDE;
                prim = prim->next;
            }

            prim = self->ext.stpopup.firstDissolvePrim;
#if defined(MOD_PTBR) && !defined(MOD_PTBR_NAME_NONE)
            for (i = 0; i < 2 * SN_STRIPS; i++) {
                primIndex = i < SN_STRIPS ? i : i - SN_STRIPS;
                prim->r0 = 128;
                prim->g0 = 128;
                prim->b0 = 128;
                LOW(prim->r1) = LOW(prim->r0);
                LOW(prim->r2) = LOW(prim->r0);
                LOW(prim->r3) = LOW(prim->r0);
                prim->p1 = primIndex < (SN_STRIPS >> 1)
                               ? primIndex * 4
                               : -((SN_STRIPS - primIndex) * 4);
                prim->p2 = Random() & 3;
                prim->p3 = 0;
                prim->drawMode =
                    DRAW_TPAGE2 | DRAW_TPAGE | DRAW_COLORS | DRAW_TRANSP;
                prim = prim->next;
            }
#else
            // Left side, upper half
            for (i = 0; i < (SN_ROWS >> 1); i++) {
                prim->r0 = 128;
                prim->g0 = 128;
                prim->b0 = 128;
                drawMode = DRAW_TPAGE2 | DRAW_TPAGE | DRAW_COLORS | DRAW_TRANSP;
                LOW(prim->r1) = LOW(prim->r0);
                LOW(prim->r2) = LOW(prim->r0);
                LOW(prim->r3) = LOW(prim->r0);
                prim->p1 = i * 4;
                prim->p2 = Random() & 3;
                prim->p3 = 0;
                prim->drawMode = drawMode;
                prim = prim->next;
            }
            // Left side, lower half
            for (i = 0; i < SN_ROWS - (SN_ROWS >> 1); i++) {
                prim->r0 = 128;
                prim->g0 = 128;
                prim->b0 = 128;
                LOW(prim->r1) = LOW(prim->r0);
                LOW(prim->r2) = LOW(prim->r0);
                LOW(prim->r3) = LOW(prim->r0);
                prim->p1 = -((SN_ROWS - (SN_ROWS >> 1) - i) * 4);
                prim->p2 = Random() & 3;
                prim->p3 = 0;
                prim->drawMode =
                    DRAW_TPAGE2 | DRAW_TPAGE | DRAW_COLORS | DRAW_TRANSP;
                prim = prim->next;
            }
            // Right side, upper half
            for (i = 0; i < (SN_ROWS >> 1); i++) {
                prim->r0 = 128;
                prim->g0 = 128;
                prim->b0 = 128;
                drawMode = DRAW_TPAGE2 | DRAW_TPAGE | DRAW_COLORS | DRAW_TRANSP;
                LOW(prim->r1) = LOW(prim->r0);
                LOW(prim->r2) = LOW(prim->r0);
                LOW(prim->r3) = LOW(prim->r0);
                prim->p1 = i * 4;
                prim->p2 = Random() & 3;
                prim->p3 = 0;
                prim->drawMode = drawMode;
                prim = prim->next;
            }
            // Right side, lower half
            for (i = 0; i < SN_ROWS - (SN_ROWS >> 1); i++) {
                prim->r0 = 128;
                prim->g0 = 128;
                prim->b0 = 128;
                LOW(prim->r1) = LOW(prim->r0);
                LOW(prim->r2) = LOW(prim->r0);
                LOW(prim->r3) = LOW(prim->r0);
                prim->p1 = -((SN_ROWS - (SN_ROWS >> 1) - i) * 4);
                prim->p2 = Random() & 3;
                prim->p3 = 0;
                prim->drawMode =
                    DRAW_TPAGE2 | DRAW_TPAGE | DRAW_COLORS | DRAW_TRANSP;
                prim = prim->next;
            }

#endif
            self->ext.stpopup.timer = 112;
            self->step_s++;
            break;
        case STAGE_NAME_DISSOLVE_TEXT:
            prim = self->ext.stpopup.firstDissolvePrim;
            while (prim != NULL) {
                StageNamePopupDissolver(prim);
                prim = prim->next;
            }
            if (!--self->ext.stpopup.timer) {
                self->step_s++;
            }
            break;
        case STAGE_NAME_SHRINK_BOX:
            prim = self->ext.stpopup.firstDissolvePrim;
            while (prim != NULL) {
                StageNamePopupDissolver(prim);
                prim = prim->next;
            }
            prim = self->ext.stpopup.boxPrim;
            prim->y0 += 2;
            prim->y1 = prim->y0;
            prim->y2 -= 2;
            prim->y3 = prim->y2;
            prim->x0 -= 2;
            prim->x1 += 2;
            prim->x2 -= 2;
            prim->x3 += 2;
            if (prim->y2 < prim->y0) {
                DestroyEntity(self);
                return;
            }
            break;
        }
        break;
    }
#if !defined(MOD_PTBR) || defined(MOD_PTBR_NAME_NONE)
    FntPrint("t_step   %x\n", self->step);
    FntPrint("t_step_s %x\n", self->step_s);
#endif
}
