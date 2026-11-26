// SPDX-License-Identifier: AGPL-3.0-or-later
// SOTN Ultimate (Korean build): Richter's and the Marias' command moves on ONE key: R1 held + a button (the Alucard easy controls' counterpart, r1_easy.c).
// Called by R1Tick every frame of play while the playable character is not Alucard (HandlePlay, before the player overlay's main). The command is not typed and
// the overlay is not touched: the game's own command check is put in its last state ("recognised", the final button due) and the final button is made
// tapped for this frame, so the overlay's own check starts the skill with its guards, costs and start function. The overlay's state lives at fixed addresses of the
// same build (O1_ / O2_ / O3_ symbols, build_newarea.sh looks them up in ric.elf / mar.elf / mar2.elf).
//   Richter:   R1 + square Blade Dash, R1 + cross High Jump, R1 + circle Subweapon (triangle: the game's own Item Crash, R1 is only hidden)
//   The PSP Maria (she runs with R1 / L1): the same with L2
//   The Saturn Maria (MARIA.BIN, mods/sotn-ultimate/src/maria_saturn/mar_specials.h):
//     R1 + square Guardian Knuckle, + cross High Jump, + circle Subweapon, + triangle Four Beasts;  R2 + square Genbu, + cross Byakko, + circle Seiryuu, + triangle Suzaku
#include <game.h>

extern ButtonComboState O1_g_RicComboButtons[2]; // Richter: [0] blade dash, [1] high jump
extern ButtonComboState O2_g_RicComboButtons[2]; // Saturn Maria: [1] high jump
extern ButtonComboState O3_g_MarComboButtons[2]; // PSP Maria: [0] blade dash, [1] high jump
typedef struct {
    s16 state;
    s16 timer;
    s16 facing;
} MarCombo;
extern MarCombo O2_mar_combo[6];   // Saturn: the six specials' command state (Byakko, Genbu, Seiryuu, Knuckle, Suzaku, Four Beasts)
extern s16 O2_mar_sp_cost[6];      // (declared as s16 for the symbol only; the real table is u8, read below as bytes)

#define RM_FACE (PAD_SQUARE | PAD_CROSS | PAD_CIRCLE | PAD_TRIANGLE)
#define RM_FACE3 (PAD_SQUARE | PAD_CROSS | PAD_CIRCLE)
#define PL_S_STAND_ 0
#define PL_S_CROUCH_ 2
#define PL_S_RUN_ 25

// move (0 Knuckle, 1 Genbu, 2 Byakko, 3 Seiryuu, 4 Suzaku, 5 Four Beasts) -> the command's slot in mar_combo and the recognised state's index (the sequence's length)
static const u8 g_RmSlot[6] = {3, 1, 0, 2, 4, 5};
static const u8 g_RmLen[6] = {3, 5, 4, 7, 5, 11};

static void RmForceSpecial(s32 move) {
    s32 s = g_RmSlot[move];
    MarCombo* c = &O2_mar_combo[s];
    const u8* cost = (const u8*)O2_mar_sp_cost;

    if (PLAYER.step > PL_S_CROUCH_ && !((move == 3 || move == 5) && PLAYER.step == PL_S_RUN_)) {
        return; // the game's own step gate (refused here too so the faked button cannot fire a plain shot)
    }
    if (g_Player.unk72 || g_Player.unk46 || cost[move] > g_Status.mp) {
        return;
    }
    c->state = g_RmLen[move];
    c->timer = 20;
    c->facing = PLAYER.facingLeft;
    g_pads[0].pressed |= PAD_SQUARE; // (the overlay derives its tapped bits from the pressed ones)
    g_pads[0].tapped |= PAD_SQUARE;
}

void RmTick(void) {
    Pad* p = &g_pads[0];
    s32 pr, k, sat, psp, m1, m2, face;
    ButtonComboState* cb;

    pr = p->pressed & ~MOD_EASY_R1; // (a real L3 is not R1)
    sat = g_PlayableCharacter == PLAYER_MARIA && !MOD_MARIA_PSP();
    psp = g_PlayableCharacter == PLAYER_MARIA && MOD_MARIA_PSP();
    m1 = psp ? PAD_L2 : PAD_R1; // the modifier: the PSP Maria runs with R1 / L1 (src/maria/pl_collision.c), so she takes the free L2
    m2 = sat ? PAD_R2 : 0;      // the Saturn Maria has a second one for her other four
    k = 0;
    if (pr & m1) {
        if (p->tapped & PAD_SQUARE) {
            k = sat ? 5 : 1; // Guardian Knuckle / Blade Dash
        } else if (p->tapped & PAD_CROSS) {
            k = 2; // High Jump
        } else if (p->tapped & PAD_CIRCLE) {
            k = 3; // Subweapon
        } else if (sat && (p->tapped & PAD_TRIANGLE)) {
            k = 10; // Four Beasts
        }
    } else if (m2 && (pr & m2)) {
        if (p->tapped & PAD_SQUARE) {
            k = 6; // Genbu
        } else if (p->tapped & PAD_CROSS) {
            k = 7; // Byakko
        } else if (p->tapped & PAD_CIRCLE) {
            k = 8; // Seiryuu
        } else if (p->tapped & PAD_TRIANGLE) {
            k = 9; // Suzaku
        }
    }
    p->pressed = pr & ~(m1 | m2); // the held modifier itself is not a Richter / Maria button
    p->tapped &= ~(m1 | m2);
    if (pr & (m1 | m2)) { // the face buttons with a modifier start a skill: they are not attacks / jumps meanwhile (the Richter / PSP item crash stays the game's own: modifier + triangle)
        face = sat ? RM_FACE : RM_FACE3;
        p->pressed &= ~face;
        p->tapped &= ~face;
    }
#ifdef RM_TEST_HOOK
    ((volatile u32*)0x801FA000)[0] = 0xD0000000 | k | (sat << 8) | (psp << 9);
    ((volatile u32*)0x801FA000)[1] = pr;
    ((volatile u32*)0x801FA000)[2] = p->tapped;
    ((volatile u32*)0x801FA000)[3] = PLAYER.step | (g_Player.unk72 << 16) | (g_Player.unk46 << 20);
    ((volatile u32*)0x801FA000)[4] = g_Status.mp;
#endif
    if (k == 0) {
        return;
    }
    if (k == 2) { // high jump: the command's last state, the cross due
        cb = sat ? O2_g_RicComboButtons : (psp ? O3_g_MarComboButtons : O1_g_RicComboButtons);
        cb[1].buttonsCorrect = 2;
        cb[1].timer = 16;
        p->pressed |= PAD_CROSS;
        p->tapped |= PAD_CROSS;
    } else if (k == 1) { // blade dash: the last state, the square due
        cb = psp ? O3_g_MarComboButtons : O1_g_RicComboButtons;
        cb[0].buttonsCorrect = 3;
        cb[0].timer = 20;
        p->pressed |= PAD_SQUARE;
        p->tapped |= PAD_SQUARE;
    } else if (k == 3) { // subweapon: the chord Up + attack, the game's own check
        p->pressed |= PAD_UP;
        p->tapped |= PAD_SQUARE;
    } else if (sat && k >= 5) {
        RmForceSpecial(k - 5);
    }
}

#ifdef RM_TEST_HOOK /* memory-patch tests only: HandlePlay's ModSysOptTick call is pointed here, so the tick runs for Richter / Maria without the (not yet built) DRA hook */
void RmTestTick(void) {
    u32* ka = (u32*)0x801F3000;

    ((volatile u32*)0x801FA020)[0]++;
    if (g_PlayableCharacter != PLAYER_ALUCARD && ka[2] != 0) {
        ((volatile u32*)0x801FA020)[1]++;
        ((void (*)(s32))ka[2])(0);
    }
}
#endif
