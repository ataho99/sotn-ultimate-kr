// SPDX-License-Identifier: AGPL-3.0-or-later
#include "sel.h"

// BSS
static s32 D_801BB010;
static s32 D_801BB014;
static u8 D_801BB018[0xE0];
static s32 D_801804D0 = 0;

// DATA
static u8 D_801804D4[] = {
    STAGE_MEMORYCARD,
    STAGE_CAT,
};
static char* D_801804D8[] = {
    "NORMAL      ",
    "SPECIAL     ",
};
const char D_801A7B80[] = "SELECT ！！";

#if defined(MOD_PTBR)
s32 ModPtbrLoad(s32 block);
#endif

#if defined(MOD_KO)
// The title screen's "ULTIMATUM" light sweep. The letters are painted with CLUT0 indices 172..243 (9 phases x 8 tones, the same colours in every phase; texture
// F_TITLE0.BIN, CLUT0 at VRAM (512, 240)); every SW_PERIOD frames of the title a soft light band crosses the word from the left to the right in SW_LEN frames by
// rewriting those 72 entries (VRAM (512 + 172, 240), 72 x 1) with each phase's tones lifted toward white by the band's profile; one frame after it the plain
// colours are restored. Stateless: the band's position is a function of g_Timer. The version line (244..246), "by smash99" (252, 253) and the side lines keep
// their colours.
#define SW_PERIOD 300
#define SW_LEN 36
#define SW_BAND 34  /* half width of the band in letter pixels */
#define SW_BOOST 100 /* percent lifted toward white at the band's centre */
static u16 D_ModSwBuf[72];
static const u16 D_ModSwTone[8] = {0x7fde, 0x7b9b, 0x7338, 0x6ad4, 0x5a50, 0x49cc, 0x3949, 0x28e6};

static void ModTitleSweep(void) {
    RECT rc;
    s32 t, p, k, pos, xc, d, b, c, v, ch, x;

    t = g_Timer % SW_PERIOD;
    if (t > SW_LEN + 1) {
        return;
    }
    pos = t > SW_LEN ? -1000 : (t * (179 + 2 * SW_BAND)) / SW_LEN - SW_BAND;
    for (p = 0; p < 9; p++) {
        xc = (p * 2 + 1) * 179 / 18;
        d = xc - pos;
        if (d < 0) {
            d = -d;
        }
        b = d >= SW_BAND ? 0 : (SW_BAND - d) * SW_BOOST / SW_BAND;
        for (k = 0; k < 8; k++) {
            v = D_ModSwTone[k];
            c = 0;
            for (ch = 0; ch < 3; ch++) {
                x = (v >> (ch * 5)) & 31;
                x = x + (31 - x) * b / 100;
                c |= x << (ch * 5);
            }
            D_ModSwBuf[p * 8 + k] = c;
        }
    }
    rc.x = 512 + 172;
    rc.y = 240;
    rc.w = 72;
    rc.h = 1;
    LoadImage(&rc, (u_long*)D_ModSwBuf);
}
#endif

void HandleTitleScreen(void) {
    Primitive* prim;
    Primitive* prim15;
    s32 i;

    func_801B1F34();
#if defined(MOD_PTBR)
    if (g_GameEngineStep - 1 < 5U && ModPtbrLoad(1)) {
        return;
    }
#endif
    switch (g_GameEngineStep) {
    case 0:
        if (g_UseDisk != 0) {
            g_CdStep = 1;
            g_LoadFile = 0x8003;
            g_LoadOvlIdx = 0xFF;
        }
        D_801BB010 = g_api.AllocPrimitives(PRIM_SPRT, 0x11);
        prim = &g_PrimBuf[D_801BB010];
        for (i = 0; i < 3; i++, prim = prim->next) {
            prim->x0 = i * 0x80 + 0x70;
            prim->y0 = 0xA8;
            prim->u0 = 0x80;
            prim->v0 = i * 0x10 + 0xB0;
            prim->u1 = 0x80;
            if (i == 2) {
                prim->u1 = 0x20;
            }
            prim->v1 = 0x10;
            prim->tpage = 0x18;
            prim->clut = 0x20;
            prim->priority = 1;
            prim->drawMode = DRAW_HIDE;
        }
        for (i = 0; i < 4; i++, prim = prim->next) {
            prim->x0 = i * 0x80 + 0xC;
            prim->y0 = 0xD6;
            prim->u0 = 0x80;
            prim->v0 = i * 8 + 0x90;
            prim->u1 = 0x80;
            if (i == 3) {
                prim->u1 = 0x64;
            }
            prim->v1 = 8;
            prim->tpage = 0x18;
            prim->clut = 0x21;
            prim->priority = 1;
            prim->drawMode = DRAW_HIDE;
        }
        for (i = 0; i < 4; i++, prim = prim->next) {
            prim->x0 = i * 0x80;
            prim->u1 = 0x80;
            prim->v1 = 0xF0;
            prim->tpage = i + 0x88;
            prim->clut = 0x10;
            prim->drawMode = DRAW_HIDE;
        }
        for (i = 0; i < 4; i++, prim = prim->next) {
            prim->x0 = i * 0x80;
            prim->u1 = 0x80;
            prim->y0 = 0x18;
            prim->v1 = 0x88;
            prim->tpage = i + 0x8C;
            prim->priority = 2;
            prim->drawMode = DRAW_HIDE;
        }
        prim->x0 = 0x101;
        prim->y0 = 0x60;
        prim->u1 = 0xFF;
        prim->v1 = 0x90;
        prim->tpage = 0x18;
        prim->clut = 0x22;
        prim->priority = 1;
        prim->drawMode = DRAW_HIDE;
        prim = prim->next;

        prim->v0 = 0x90;
        prim->u1 = 0x78;
        prim->v1 = 0x40;
        prim->tpage = 0x18;
        prim->clut = 0x22;
        prim->priority = 1;
        prim->drawMode = DRAW_HIDE;
        func_801B18F4();
        D_801BB014 = 0;
        g_GameEngineStep++;
        break;

    case 1:
        SetTitleDisplayBuffer();
        prim = &g_PrimBuf[D_801BB010];
        i = 0;
        D_801BB014 += 8;
        while (prim != NULL) {
            prim->drawMode = DRAW_COLORS;
#if defined(MOD_PTBR)
            if (i < 7) {
                prim->drawMode = DRAW_HIDE;
            }
#endif
            SetPrimGrey(prim, D_801BB014);
            if (i == 7 || i == 8 || i == 9 || i == 10) {
                prim->r0 = D_801BB014 * 3 / 4;
                prim->g0 = D_801BB014 * 7 / 8;
                prim->b0 = D_801BB014 * 3 / 4;
            }
            if (i == 15 || i == 16) {
                prim->drawMode = DRAW_TPAGE | DRAW_COLORS | DRAW_TRANSP;
            }
            prim = prim->next;
            i++;
        }
        if (D_801BB014 == 0x80) {
            D_801804D0 = 0x800;
            g_GameEngineStep++;
        }
        break;

    case 2:
#if defined(MOD_KO)
        ModTitleSweep();
#endif
        prim = &g_PrimBuf[D_801BB010];
        for (i = 0; prim != NULL; prim = prim->next, i++) {
            if (i < 3) {
                prim->drawMode = DRAW_HIDE;
                if (g_Timer & 0x30) {
                    prim->drawMode = DRAW_DEFAULT;
                }
            }
            if ((i != 15) && (i != 16)) {
                continue;
            }
            if (i == 15) {
                prim15 = prim;
            }
            switch (prim->p1) {
            case 0:
                if (i == 0xF) {
                    if (!(rand() & 0x7F)) {
                        prim->p2 = 6;
                        prim->tpage = 0x19;
                        prim->p1++;
                    }
                } else if (
                    (prim15->p1 >= 2) || (prim15->p1 && (prim15->p2 < 3))) {
                    prim->p2 = (rand() & 3) + 6;
                    prim->tpage = 0x19;
                    prim->p1++;
                }
                break;

            case 1:
                if (!--prim->p2) {
                    prim->p2 = 6;
                    prim->tpage = 0x1A;
                    prim->p1++;
                }
                break;

            case 2:
                if (!--prim->p2) {
                    prim->p2 = (rand() & 7) + 6;
                    prim->tpage = 0x1B;
                    prim->p1++;
                }
                break;

            case 3:
                if (!--prim->p2) {
                    prim->p2 = 5;
                    prim->tpage = 0x1A;
                    prim->p1++;
                }
                break;

            case 4:
                if (!--prim->p2) {
                    prim->p2 = 5;
                    prim->tpage = 0x19;
                    prim->p1++;
                }
                break;

            case 5:
                if (!--prim->p2) {
                    prim->p2 = 6;
                    prim->tpage = 0x18;
                    prim->p1++;
                }
                break;

            case 6:
                if (!--prim->p2) {
                    if (i == 0x10) {
                        prim->p1 = 0;
                    } else {
                        prim->p1++;
                        prim->p2 = 6;
                        if (rand() & 3) {
                            prim->p1 = 1;
                            prim->tpage = 0x19;
                        }
                    }
                }
                break;

            case 7:
                if (!--prim->p2) {
                    prim->p1 = 0;
                }
                break;
            }
            if (i == 0x10) {
                if (prim->tpage == 0x18) {
                    prim->u1 = 0x78;
                } else {
                    prim->u1 = 0x70;
                }
            }
        }
        if (g_pads[0].tapped & PAD_START) {
            g_api.PlaySfx(SFX_START_SLAM_B);
            g_GameEngineStep++;
        }
        break;

    case 3:
        g_GameEngineStep = 6;
        break;

    case 4:
        if (g_pads[0].tapped & (PAD_RIGHT | PAD_DOWN)) {
            g_api.PlaySfx(SFX_DEBUG_SELECT);
            D_800987B4++;
            if (D_800987B4 >= 2) {
                D_800987B4 = 0;
            }
        }
        if (g_pads[0].tapped & (PAD_UP | PAD_LEFT)) {
            g_api.PlaySfx(SFX_DEBUG_SELECT);
            D_800987B4--;
            if (D_800987B4 < 0) {
                D_800987B4 = 1;
            }
        }
        prim = &g_PrimBuf[D_801BB010];
        if (g_Timer & 0x1C) {
            prim->drawMode = DRAW_DEFAULT;
        } else {
            prim->drawMode = DRAW_HIDE;
        }
        g_StageId = D_801804D4[D_800987B4];
        func_801B1F4C(1);
        func_801B259C(D_801804D8[D_800987B4], 1);
        if (g_pads[0].tapped & (PAD_START | PAD_CIRCLE)) {
            g_api.PlaySfx(SFX_START_SLAM_B);
            g_GameEngineStep++;
        }
        break;

    case 5:
        func_801B18F4();
        func_801B19F4();
        func_801B1DA8();
        g_api.FreePrimitives(D_801BB010);
        strcpy(g_Status.saveName, "alucard");
        if (g_StageId == STAGE_ST0) {
            SetGameState(Game_VideoPlayback);
        } else if (g_StageId == STAGE_MEMORYCARD) {
            SetGameState(Game_MainMenu);
        } else {
            SetGameState(Game_NowLoading);
            g_GameStep = 2;
        }
        break;

    case 6:
        prim = &g_PrimBuf[D_801BB010];
        i = 0;
        D_801BB014 -= 8;
        while (prim != NULL) {
            prim->drawMode = DRAW_COLORS;
            SetPrimGrey(prim, D_801BB014);
            if (i == 15 || i == 16) {
                prim->drawMode = DRAW_TPAGE | DRAW_COLORS | DRAW_TRANSP;
            }
            prim = prim->next;
            i++;
        }
        if (D_801BB014 == 0) {
            g_StageId = STAGE_MEMORYCARD;
            g_GameEngineStep = 5;
        }
        return;
    }
}
