// SPDX-License-Identifier: AGPL-3.0-or-later
// SOTN Ultimate (Korean build): the Alucard "Easy controls" code, linked into the new resident area (build_newarea.sh); ModSysOptTick
// (DRA's HandlePlay, every frame of play) calls R1Tick through BIN/ARC_F2.BIN's glue. (Earlier tries put it in the main exe: the exe BSS tail "R1" is the
// primitive pool, cleared by DRA; two stretches of Psy-Q code "nothing calls" were called while loading.)
//  * 097: R1 held + a face button = a spell: DRA's spell checks (src/dra/704D0.c CheckEasy) read the signal bit MOD_EASY_R1 this tick sets
//    while R1 is held (R1 itself is hidden from the game); R2 held + a direction = the form (the ring's input); always on.
//    The tick keeps the two forms' special on cross (wolf charge / wing smash) and the hiding of the game's L1 / R1 / R2. L2 stays the third hand.
//  * 100: the ring got a fourth place "release" (down: back to Alucard), unowned forms in grey, an open / close effect, a flash of the chosen form on
//    release and (RING_SLOW) the game running slower while it is open. (The 8px name label was tried and dropped, #3645.)
#include <game.h>
#include "ring_art.h" // tools/gen_ring_art.py: the release icon's image + palette

extern ButtonComboState g_ButtonCombo[];
extern s32 g_WingSmashButtonCounter;
extern s32 g_WingSmashButtonTimer;
extern s32 IsRelicActive(s32 relic);
extern void SubFollow(void); // newarea_sub.c: the familiars' subtitle follows the familiar
extern void RmTick(void);    // newarea_rm.c: Richter's / Maria's one-key moves

// ---- the ring's looks, switchable for the mock-ups (the choice is the user's eye)
#ifndef RING_SLOW
#define RING_SLOW 2     // the frame wait while the ring is open: 2 = half speed, 3 = a third, 0 = off (D_8003C73C is VSync's argument in DRA's frame loop)
#endif
#ifndef RING_FOLD
#define RING_FOLD 1     // open / close: 1 = the icons spread out from / fold into the centre, 0 = they grow / shrink in place
#endif
#ifndef RING_REL_GREY
#define RING_REL_GREY 0 // the release place while Alucard is human: 1 = a grey icon, 0 = hidden
#endif

// ---- the transformation ring (R2 held): up = bat, left = wolf, right = mist, down = release (back to Alucard).
extern s16 AllocPrimitives(u8 primType, s32 count);
extern void FreePrimitives(s32 index);
extern void SetPrimRect(Primitive* poly, s32 x, s32 y, s32 width, s32 height);
extern void LoadEquipIcon(s32 equipIcon, s32 palette, s32 index);
extern RelicDesc g_RelicDefs[];
extern u16 D_801374F8[]; // DRA's icon-slot caches (4AEA4.c): -1 makes the next LoadEquipIcon of the slot load again
extern u16 D_80137538[];
extern s32 D_8003C73C;
typedef struct {
    s16 dir;
    s8 dx;
    s8 dy;
    u8 relic;
    u8 act; // the button config's action index of the game's own button for the form: 4 wolf (R2), 5 mist (L1), 6 bat (R1)
} RingForm;
static const RingForm g_Ring[4] = {
    {PAD_UP, 0, -26, RELIC_SOUL_OF_BAT, 6},
    {PAD_LEFT, -26, 0, RELIC_SOUL_OF_WOLF, 4},
    {PAD_RIGHT, 26, 0, RELIC_FORM_OF_MIST, 5},
    {PAD_DOWN, 0, 44, 0, 0}, // release: the button of the form Alucard is in (lower than the others: below his feet, not over his body)
};
// equip-icon slots: the three forms' icons 0x1C..0x1E (the game's own art), the release icon 0x1B (our art, RingArt)
#define RING_SLOT_FORM 0x1C
#define RING_SLOT_REL 0x1B
static void RingDraw(ModEasyState* st, s32 pr);

// 099: linked at MOD_NEWAREA_BASE + 16 (the new resident area, build_newarea.sh); no statics: the state is the caller's (ARC_F2's ModEasyState)
void R1Tick(ModEasyState* st) {
    Pad* p = &g_pads[0];
    s32 k, b, hide, add, pr, r1, s;

    if (g_PlayableCharacter != PLAYER_ALUCARD) { // Richter / Maria: their own short tick (the ring, the spells and the slow motion are Alucard's)
        RmTick();
        SubFollow();
        return;
    }
    pr = p->pressed & ~MOD_EASY_R1; // a real L3 (stick click) is not "R1 held"
    r1 = pr & PAD_R1;
    st->eat &= pr;
    add = 0;
    b = g_Settings.buttonMask[2]; // cross
    if (r1 && (g_Player.status & 5) && (p->tapped & b)) { // the form's own special on ONE key
        st->eat |= b;
        if (g_Player.status & 4) {
            g_ButtonCombo[1].buttonsCorrect = 0xFF; // wolf charge: COMBO_QCF complete
            g_ButtonCombo[1].timer = 20;
            add = g_Settings.buttonMask[0]; // + the square tap
        } else {
            g_WingSmashButtonCounter = 7;
            g_WingSmashButtonTimer = 0x14;
        }
    }
    hide = st->eat | PAD_L1 | PAD_R1 | PAD_R2;
    if (pr & PAD_R2) {
        if (!(g_Player.status & PLAYER_STATUS_MIST_FORM)) { // (once the form is taken the held ring key must not freeze the mist: the directions go to the game)
            hide |= PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT;
        }
        for (k = 0; k < 4; k++) {
            if (!(p->tapped & g_Ring[k].dir)) {
                continue;
            }
            if (k < 3) {
                if (!IsRelicActive(g_Ring[k].relic)) {
                    continue;
                }
                add |= g_Settings.buttonMask[g_Ring[k].act];
            } else {
                s = g_Player.status;
                if (!(s & 7)) { // nothing to release
                    continue;
                }
                add |= g_Settings.buttonMask[(s & 1) ? 6 : ((s & 4) ? 4 : 5)]; // the form's own button, again, takes it off
            }
            st->sel = k + 1;
        }
    }
    RingDraw(st, pr);
    SubFollow();
#if RING_SLOW
    D_8003C73C = ((pr & PAD_R2) && !(pr & PAD_START)) ? RING_SLOW : 0; // the frame loop's VSync(D_8003C73C): all game time, same ratio
#endif
    p->pressed = (pr & ~hide) | add | (r1 ? MOD_EASY_R1 : 0);
    p->tapped = (p->tapped & ~hide) | add;
}

// Our own art goes into one more equip-icon slot (4 bit 16x16 at x 0x280 + (slot & 7) * 4, y 0x180 + (slot / 8) * 16), its palette into g_Clut like LoadEquipIcon
// does. The slot's caches are set to -1 so the pause menu, which loads its own icons there, does not take our picture for its own.
static void RingArt(void) {
    s32 i;

    LoadTPage((u_long*)g_RingRel, 0, 0, ((RING_SLOT_REL & 7) * 4) + 0x280, (RING_SLOT_REL / 8) * 0x10 + 0x180, 16, 16);
    for (i = 0; i < 5; i++) {
        g_Clut[1][RING_SLOT_REL * 16 + 0xD00 + i] = g_RingPalRel[i];
    }
    LoadClut((u_long*)&g_Clut[1][0xE00], 0, 0xFE);
    D_801374F8[RING_SLOT_REL] = -1;
    D_80137538[RING_SLOT_REL] = -1;
}

// slot's image into the prim: v offset and height inside the 16x16 slot
static void RingTex(Primitive* p, s32 s, s32 vOff, s32 h, s32 col, s32 mode) {
    p->u0 = p->u2 = (s & 7) * 16;
    p->u1 = p->u3 = p->u0 + 15;
    p->v0 = p->v1 = (s >> 3) * 16 + 128 + vOff;
    p->v2 = p->v3 = p->v0 + h - 1;
    p->r0 = p->g0 = p->b0 = p->r1 = p->g1 = p->b1 = p->r2 = p->g2 = p->b2 = p->r3 = p->g3 = p->b3 = col;
    p->tpage = 0x1A;
    p->clut = 0x1D0 + s;
    p->priority = 0x1F0; // in front of the room's layers and Alucard
    p->drawMode = mode;
}

// The icons live in the equip-icon slots; the prim's clut is an INDEX into g_ClutIds (0x1D0 + slot).
// pr = the pad as held: R2 opens the ring, a held direction makes its icon bigger and brighter. A form Alucard lacks is drawn grey.
// st->ringPrim = the first of the ring's 4 prims (the 4 places; -1: none), st->anim = the open / close step (0..3), st->sel = the place
// chosen while open (index + 1), shown once more, big, for the closing frames (the other places vanish at once); no new DRA data (dra_DATA_END is checked)
static void RingDraw(ModEasyState* st, s32 pr) {
    Primitive* p;
    s32 i, s, id, k, cur, closing, sc, x, y, col, own, held, mode;

    cur = st->ringPrim;
    closing = !(pr & PAD_R2);
    if (closing) {
        if (cur < 0) {
            return;
        }
        if (st->anim <= 0) {
            FreePrimitives(cur);
            st->ringPrim = -1;
            st->sel = 0;
            return;
        }
        sc = st->anim--;
    } else {
        if (cur < 0) {
            if ((cur = AllocPrimitives(PRIM_GT4, 4)) < 0) {
                return;
            }
            st->ringPrim = cur;
            st->sel = 0;
            st->anim = 0;
            RingArt();
        }
        if (st->anim < 3) {
            st->anim++;
        }
        sc = st->anim;
    }
    for (i = 0, p = &g_PrimBuf[cur]; i < 4; i++, p = p->next) {
        id = g_Ring[i].relic;
        if (i < 3) {
            s = RING_SLOT_FORM + i;
            LoadEquipIcon(g_RelicDefs[id].icon, g_RelicDefs[id].iconPalette, s);
            own = IsRelicActive(id);
        } else {
            s = RING_SLOT_REL;
            own = g_Player.status & 7;
        }
        held = !closing && own && (pr & g_Ring[i].dir);
        x = g_Ring[i].dx;
        y = g_Ring[i].dy;
        k = held ? 12 : 8;
        mode = DRAW_COLORS; // the prim's colour is applied (0x80 = as drawn): grey, normal, bright
        if (closing && st->sel) {
            if (st->sel == i + 1) {
                k = 16; // the chosen one, once more, big
            } else {
                mode = DRAW_HIDE; // the others are gone at once
            }
        } else if (sc < 3) { // opening / closing
#if RING_FOLD
            x = x * sc / 3;
            y = y * sc / 3;
#else
            k = k * sc / 3;
#endif
        }
        col = !own ? 0x40 : ((held || (closing && st->sel)) ? 0xFF : 0x80);
        if (i == 3 && !own) {
#if !RING_REL_GREY
            mode = DRAW_HIDE;
#endif
        }
        x += PLAYER.posX.i.hi;
        y += PLAYER.posY.i.hi - 24;
        SetPrimRect(p, x - k, y - k, k * 2, k * 2);
        RingTex(p, s, 0, 16, col, mode);
    }
}
