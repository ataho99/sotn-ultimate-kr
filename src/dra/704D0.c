// SPDX-License-Identifier: AGPL-3.0-or-later
#include "dra.h"
#include "dra_bss.h"

#if defined(MOD_GODSPEED)
// SOTN Ultimate: Soul Steal and Dark Metamorphosis also cast during the God
// Speed Shoes sprint (its own step). Not on the Saturn (060AEC64 casts from
// a walk or a stand only), but their input ends in a walk start that the
// shoes turn into a sprint, so with the shoes on they hardly ever cast.
// One shared check, for DRA room (MOD_DRA_PAD)
static s32 ModCastStep(void) {
    s32 step = PLAYER.step;
    return step == Player_Walk || step == Player_Stand ||
           step == Player_FlameWhip;
}
#define MOD_CAST_STEP() ModCastStep()
#else
#define MOD_CAST_STEP()                                                        \
    ((PLAYER.step == Player_Walk) || (PLAYER.step == Player_Stand))
#endif

bool CheckGravityBootsInput(void) {
    switch (g_ButtonCombo[COMBO_GRAVITY_BOOTS].buttonsCorrect) {
    case 0:
        if ((g_Player.padTapped & PAD_DOWN) && (g_Player.padHeld == 0)) {
            g_ButtonCombo[COMBO_GRAVITY_BOOTS].timer = 16;
            g_ButtonCombo[COMBO_GRAVITY_BOOTS].buttonsCorrect++;
        }
        break;
    case 1:
        if (g_Player.padTapped & PAD_UP) {
            g_ButtonCombo[COMBO_GRAVITY_BOOTS].timer = 16;
            g_ButtonCombo[COMBO_GRAVITY_BOOTS].buttonsCorrect++;
        } else {
            if (--g_ButtonCombo[COMBO_GRAVITY_BOOTS].timer == 0) {
                g_ButtonCombo[COMBO_GRAVITY_BOOTS].buttonsCorrect = 0;
            }
        }
        break;
    case 2:
        if ((g_ButtonCombo[COMBO_GRAVITY_BOOTS].timer) &&
            --g_ButtonCombo[COMBO_GRAVITY_BOOTS].timer == 0) {
            g_ButtonCombo[COMBO_GRAVITY_BOOTS].buttonsCorrect = 0;
            break;
        }
        if (IsRelicActive(RELIC_GRAVITY_BOOTS) &&
            (g_Player.padTapped & PAD_CROSS) && !(g_Player.unk46 & 0x8000) &&
            ((PLAYER.step == Player_Crouch) ||
             ((PLAYER.step == Player_Jump) && (g_Player.unk44 & 1)))) {
            if (g_Player.unk72) {
                g_ButtonCombo[COMBO_GRAVITY_BOOTS].buttonsCorrect = 0;
                break;
            }
            if (HandleGravityBootsMP(REDUCE) >= 0) {
                DoGravityJump();
                g_ButtonCombo[COMBO_GRAVITY_BOOTS].buttonsCorrect = 0;
                return 1;
            }
        }
        break;
    }
    return 0;
}

// Used for wolf charge, and for weapons which have a move using this pattern
bool CheckQuarterCircleForwardInput(void) {
    s32 directionsPressed;
    s32 forward;
    s32 down_forward;
    s32 unused = 1; // Meaningless, PSP-only

    directionsPressed =
        g_Player.padPressed & (PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT);
    if (g_WasFacingLeft == 0) {
        forward = PAD_RIGHT;
    } else {
        forward = PAD_LEFT;
    }
    if (g_WasFacingLeft == 0) {
        down_forward = PAD_DOWN + PAD_RIGHT;
    } else {
        down_forward = PAD_DOWN + PAD_LEFT;
    }
    switch (g_ButtonCombo[COMBO_QCF].buttonsCorrect) {
    case 0:
        g_WasFacingLeft = PLAYER.facingLeft;
        if (directionsPressed == PAD_DOWN) {
            g_ButtonCombo[COMBO_QCF].timer = 20;
            g_ButtonCombo[COMBO_QCF].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_QCF].timer == 0) {
            g_ButtonCombo[COMBO_QCF].buttonsCorrect = 0;
        }
        break;
    case 1:
        if (directionsPressed == down_forward) {
            g_ButtonCombo[COMBO_QCF].timer = 20;
            g_ButtonCombo[COMBO_QCF].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_QCF].timer == 0) {
            g_ButtonCombo[COMBO_QCF].buttonsCorrect = 0;
        }
        break;
    case 2:
        if (directionsPressed == forward) {
            g_ButtonCombo[COMBO_QCF].timer = 20;
            g_ButtonCombo[COMBO_QCF].buttonsCorrect = COMBO_COMPLETE;
            break;
        }
        if (--g_ButtonCombo[COMBO_QCF].timer == 0) {
            g_ButtonCombo[COMBO_QCF].buttonsCorrect = 0;
        }
        break;
    default:
        if (g_ButtonCombo[COMBO_QCF].timer) {
            if (g_Player.unk72) {
                g_ButtonCombo[COMBO_QCF].buttonsCorrect = 0;
                break;
            }
            if (--g_ButtonCombo[1].timer == 0) {
                g_ButtonCombo[COMBO_QCF].buttonsCorrect = 0;
                g_WasFacingLeft = PLAYER.facingLeft;
            }
        }
    }
    return 0;
}

bool CheckBackForwardInput(void) {
    s32 directionsPressed;
    s32 forward;
    s32 backward;
    s32 unused = 2; // Meaningless, PSP-only

    directionsPressed =
        g_Player.padPressed & (PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT);
    if (!g_WasFacingLeft2) {
        forward = PAD_RIGHT;
    } else {
        forward = PAD_LEFT;
    }
    if (!g_WasFacingLeft2) {
        backward = PAD_LEFT;
    } else {
        backward = PAD_RIGHT;
    }
    switch (g_ButtonCombo[COMBO_BF].buttonsCorrect) {
    case 0:
        g_WasFacingLeft2 = PLAYER.facingLeft;
        if (!g_WasFacingLeft2) {
            backward = PAD_LEFT;
        } else {
            backward = PAD_RIGHT;
        }
        if (directionsPressed == backward) {
            g_ButtonCombo[COMBO_BF].timer = 15;
            g_ButtonCombo[COMBO_BF].buttonsCorrect++;
        } else if (--g_ButtonCombo[COMBO_BF].timer == 0) {
            g_ButtonCombo[COMBO_BF].buttonsCorrect = 0;
        }
        break;
    case 1:
        if (directionsPressed == forward) {
            g_ButtonCombo[COMBO_BF].timer = 15;
            g_ButtonCombo[COMBO_BF].buttonsCorrect = COMBO_COMPLETE;
            break;
        } else if (--g_ButtonCombo[COMBO_BF].timer == 0) {
            g_ButtonCombo[COMBO_BF].buttonsCorrect = 0;
        }
        break;
    case 0xFE: // Indicates FE might have special flag meaning. Never gets set
               // though.
    default:
        if (g_Player.unk72) {
            g_ButtonCombo[COMBO_BF].buttonsCorrect = 0;
            break;
        }
        if (g_ButtonCombo[COMBO_BF].timer &&
            --g_ButtonCombo[COMBO_BF].timer == 0) {
            g_ButtonCombo[COMBO_BF].buttonsCorrect = 0;
            g_WasFacingLeft2 = PLAYER.facingLeft;
        }
    }
    return 0;
}

#if defined(MOD_SATURN)
// 097: the six spell commands are "R1 held (the tick's MOD_EASY_R1 bit) + a face button + the posture"; the casting is the game's own
// (CastSpell, Perform*, LearnSpell). steps: 1 = standing / walking (MOD_CAST_STEP), 2 = crouching, 4 = not while g_Player.unk72.
// The face button is used up when the spell goes off (as the retail commands' last button was).
void PerformDarkMetamorphosis(void);
void PerformSummonSpirit(void);
void PerformHellfire(void);
void PerformTetraSpirit(void);
void PerformSoulSteal(void);
void PerformSwordBrothers(void);
extern s32 D_800ACE00[]; // 692E8.c: the logical bit of each button-config action
// 3958: the face button is the PHYSICAL one (the spell list draws R1 + the shape): tested in the pad's own tapped bits, not in g_Player.padTapped (which holds
// the config's logical buttons: with the circle action moved to L2, R1 + circle did not cast and R1 + L2 did). On a cast every action bound to that
// physical button is used up (its logical bit, tapped and held), so an attack on the same button does not go off with the spell.
static bool CheckEasy(u32 btn, s32 spell, void (*perform)(void), s32 steps) {
    s32 i;

    if (!(g_Player.padPressed & MOD_EASY_R1) || !(g_pads[0].tapped & btn) || (g_Player.unk46 & 0x8000) ||
        ((steps & 4) && g_Player.unk72)) {
        return 0;
    }
    if (!(((steps & 1) && MOD_CAST_STEP()) || ((steps & 2) && PLAYER.step == Player_Crouch)) || !CastSpell(spell)) {
        return 0;
    }
    perform();
    LearnSpell(spell);
    for (i = 0; i < 8; i++) {
        if (g_Settings.buttonMask[i] & btn) {
            g_Player.padTapped &= ~D_800ACE00[i];
            g_Player.padPressed &= ~D_800ACE00[i];
        }
    }
    return 1;
}

bool CheckDarkMetamorphosisInput(void) {
    return CheckEasy(PAD_TRIANGLE, SPELL_DARK_METAMORPHOSIS, PerformDarkMetamorphosis, 1);
}

bool CheckSummonSpiritInput(void) {
    return CheckEasy(PAD_CROSS, SPELL_SUMMON_SPIRIT, PerformSummonSpirit, 2);
}

bool CheckHellfireInput(void) {
    return CheckEasy(PAD_SQUARE, SPELL_HELLFIRE, PerformHellfire, 1 | 2 | 4);
}

bool CheckTetraSpiritInput(void) {
    return CheckEasy(PAD_TRIANGLE, SPELL_TETRA_SPIRIT, PerformTetraSpirit, 2);
}

bool CheckSoulStealInput(void) {
    return CheckEasy(PAD_CIRCLE, SPELL_SOUL_STEAL, PerformSoulSteal, 1);
}

bool CheckSwordBrothersInput(void) {
    ButtonComboState* c = &g_ButtonCombo[COMBO_SWORD_BROTHERS];

    if (c->buttonsCorrect == COMBO_COMPLETE) { // the familiars' state, as the retail check leaves it
        if (--c->timer == 0) {
            c->buttonsCorrect = 0;
            g_Player.unk70 = 1;
        }
        return 0;
    }
    if (CheckEasy(PAD_CIRCLE, SPELL_SWORD_BROTHERS, PerformSwordBrothers, 2)) {
        c->buttonsCorrect = COMBO_COMPLETE;
        c->timer = 20;
        g_Player.unk70 = 1;
        return 1;
    }
    return 0;
}
#else
bool CheckDarkMetamorphosisInput(void) {
    s32 directionsPressed;
    s32 up_forward;
    s32 up_back;
    s32 forward;
    s32 up;

    directionsPressed =
        g_Player.padPressed & (PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT);
    if (!g_WasFacingLeft3) {
        up_forward = PAD_UP + PAD_RIGHT;
        forward = PAD_RIGHT;
        up_back = PAD_UP + PAD_LEFT;
    } else {
        up_forward = PAD_UP + PAD_LEFT;
        forward = PAD_LEFT;
        up_back = PAD_UP + PAD_RIGHT;
    }
    up = PAD_UP;

    switch (g_ButtonCombo[COMBO_DARK_METAMORPH].buttonsCorrect) {
    case 0:
        if (PLAYER.facingLeft == 0) {
            if (g_Player.padTapped & PAD_LEFT) {
                g_ButtonCombo[COMBO_DARK_METAMORPH].timer = 20;
                g_ButtonCombo[COMBO_DARK_METAMORPH].buttonsCorrect++;
                g_WasFacingLeft3 = 0;
            }
            break;
        }
        if (g_Player.padTapped & PAD_RIGHT) {
            g_ButtonCombo[COMBO_DARK_METAMORPH].timer = 20;
            g_ButtonCombo[COMBO_DARK_METAMORPH].buttonsCorrect++;
            g_WasFacingLeft3 = 1;
        }
        break;
    case 1:
        if ((g_Player.padPressed & up_back) == up_back) {
            g_ButtonCombo[COMBO_DARK_METAMORPH].timer = 20;
            g_ButtonCombo[COMBO_DARK_METAMORPH].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_DARK_METAMORPH].timer == 0) {
            g_ButtonCombo[COMBO_DARK_METAMORPH].buttonsCorrect = 0;
        }
        break;
    case 2:
        if (g_Player.padPressed == up) {
            g_ButtonCombo[COMBO_DARK_METAMORPH].timer = 20;
            g_ButtonCombo[COMBO_DARK_METAMORPH].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_DARK_METAMORPH].timer == 0) {
            g_ButtonCombo[COMBO_DARK_METAMORPH].buttonsCorrect = 0;
        }
        break;
    case 3:
        if ((directionsPressed & up_forward) == up_forward) {
            g_ButtonCombo[COMBO_DARK_METAMORPH].timer = 20;
            g_ButtonCombo[COMBO_DARK_METAMORPH].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_DARK_METAMORPH].timer == 0) {
            g_ButtonCombo[COMBO_DARK_METAMORPH].buttonsCorrect = 0;
        }
        break;
    case 4:
        if (directionsPressed == forward) {
            g_ButtonCombo[COMBO_DARK_METAMORPH].timer = 20;
            g_ButtonCombo[COMBO_DARK_METAMORPH].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_DARK_METAMORPH].timer == 0) {
            g_ButtonCombo[COMBO_DARK_METAMORPH].buttonsCorrect = 0;
        }
        break;
    case 5:
        if (--g_ButtonCombo[COMBO_DARK_METAMORPH].timer == 0) {
            g_ButtonCombo[COMBO_DARK_METAMORPH].buttonsCorrect = 0;
            break;
        }

        if ((g_Player.padTapped & (PAD_SQUARE | PAD_CIRCLE)) &&
            !(g_Player.unk46 & 0x8000) &&
            MOD_CAST_STEP() && (CastSpell(SPELL_DARK_METAMORPHOSIS) != 0)) {
            PerformDarkMetamorphosis();
            g_ButtonCombo[COMBO_DARK_METAMORPH].buttonsCorrect = 0;
            LearnSpell(SPELL_DARK_METAMORPHOSIS);
            return 1;
        }
        break;
    }
    return 0;
}

bool CheckSummonSpiritInput(void) {
    s32 directionsPressed;
    s32 forward;

    directionsPressed =
        g_Player.padPressed & (PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT);
    if (g_WasFacingLeft4) {
        forward = directionsPressed & PAD_LEFT;
    } else {
        forward = directionsPressed & PAD_RIGHT;
    }
    switch (g_ButtonCombo[COMBO_SUMMON_SPIRIT].buttonsCorrect) {
    case 0:
        if (PLAYER.facingLeft == 0) {
            if (g_Player.padTapped == PAD_LEFT) {
                g_ButtonCombo[COMBO_SUMMON_SPIRIT].timer = 20;
                g_ButtonCombo[COMBO_SUMMON_SPIRIT].buttonsCorrect++;
                g_WasFacingLeft4 = 0;
            }
            break;
        }
        if (g_Player.padTapped == PAD_RIGHT) {
            g_ButtonCombo[COMBO_SUMMON_SPIRIT].timer = 20;
            g_ButtonCombo[COMBO_SUMMON_SPIRIT].buttonsCorrect++;
            g_WasFacingLeft4 = 1;
        }
        break;
    case 1:
        if (directionsPressed == forward) {
            g_ButtonCombo[COMBO_SUMMON_SPIRIT].timer = 20;
            g_ButtonCombo[COMBO_SUMMON_SPIRIT].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_SUMMON_SPIRIT].timer == 0) {
            g_ButtonCombo[COMBO_SUMMON_SPIRIT].buttonsCorrect = 0;
        }
        break;

    case 2:
        if (directionsPressed == PAD_UP) {
            g_ButtonCombo[COMBO_SUMMON_SPIRIT].timer = 20;
            g_ButtonCombo[COMBO_SUMMON_SPIRIT].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_SUMMON_SPIRIT].timer == 0) {
            g_ButtonCombo[COMBO_SUMMON_SPIRIT].buttonsCorrect = 0;
        }
        break;
    case 3:
        if (directionsPressed == PAD_DOWN) {
            g_ButtonCombo[COMBO_SUMMON_SPIRIT].timer = 20;
            g_ButtonCombo[COMBO_SUMMON_SPIRIT].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_SUMMON_SPIRIT].timer == 0) {
            g_ButtonCombo[COMBO_SUMMON_SPIRIT].buttonsCorrect = 0;
        }
        break;
    case 4:
        if (--g_ButtonCombo[COMBO_SUMMON_SPIRIT].timer == 0) {
            g_ButtonCombo[COMBO_SUMMON_SPIRIT].buttonsCorrect = 0;
        }
        if ((g_Player.padTapped & (PAD_SQUARE | PAD_CIRCLE)) &&
            !(g_Player.unk46 & 0x8000) && (PLAYER.step == Player_Crouch) &&
            (CastSpell(SPELL_SUMMON_SPIRIT) != 0)) {
            PerformSummonSpirit();
            g_ButtonCombo[COMBO_SUMMON_SPIRIT].buttonsCorrect = 0;
            LearnSpell(SPELL_SUMMON_SPIRIT);
            return 1;
        }
        break;
    }
    return 0;
}

bool CheckHellfireInput(void) {
    s32 directionsPressed;
    s32 down_forward;
    s32 forward;

    directionsPressed =
        g_Player.padPressed & (PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT);
    if (g_WasFacingLeft5 == 0) {
        forward = PAD_RIGHT;
    } else {
        forward = PAD_LEFT;
    }

    if (g_WasFacingLeft5 == 0) {
        down_forward = PAD_DOWN + PAD_RIGHT;
    } else {
        down_forward = PAD_DOWN + PAD_LEFT;
    }
    switch (g_ButtonCombo[COMBO_HELLFIRE].buttonsCorrect) {
    case 0:
        if (g_Player.padTapped == PAD_UP) {
            g_ButtonCombo[COMBO_HELLFIRE].timer = 20;
            g_ButtonCombo[COMBO_HELLFIRE].buttonsCorrect++;
        }
        break;
    case 1:
        if (directionsPressed == PAD_DOWN) {
            g_ButtonCombo[COMBO_HELLFIRE].timer = 21;
            g_ButtonCombo[COMBO_HELLFIRE].buttonsCorrect++;
            g_WasFacingLeft5 = PLAYER.facingLeft;
            break;
        }
        if (--g_ButtonCombo[COMBO_HELLFIRE].timer == 0) {
            g_ButtonCombo[COMBO_HELLFIRE].buttonsCorrect = 0;
        }
        break;
    case 2:
        if ((directionsPressed & down_forward) == down_forward) {
            g_ButtonCombo[COMBO_HELLFIRE].timer = 20;
            g_ButtonCombo[COMBO_HELLFIRE].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_HELLFIRE].timer == 0) {
            g_ButtonCombo[COMBO_HELLFIRE].buttonsCorrect = 0;
        }
        break;
    case 3:
        if (directionsPressed == forward) {
            g_ButtonCombo[COMBO_HELLFIRE].timer = 20;
            g_ButtonCombo[COMBO_HELLFIRE].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_HELLFIRE].timer == 0) {
            g_ButtonCombo[COMBO_HELLFIRE].buttonsCorrect = 0;
        }
        break;
    case 4:
        if (g_ButtonCombo[COMBO_HELLFIRE].timer &&
            --g_ButtonCombo[COMBO_HELLFIRE].timer == 0) {
            g_ButtonCombo[COMBO_HELLFIRE].buttonsCorrect = 0;
            break;
        }
        FntPrint("pl_pose:%02x\n", PLAYER.pose);
        if ((g_Player.padTapped & (PAD_SQUARE | PAD_CIRCLE)) &&
            !(g_Player.unk46 & 0x8000) &&
            ((PLAYER.step == Player_Crouch) ||
             ((PLAYER.step == Player_Stand) || (PLAYER.step == Player_Walk)))) {
            if (g_Player.unk72) {
                g_ButtonCombo[COMBO_HELLFIRE].buttonsCorrect = 0;
                break;
            }
            if (CastSpell(SPELL_HELLFIRE)) {
                PerformHellfire();
                g_ButtonCombo[COMBO_HELLFIRE].buttonsCorrect = 0;
                LearnSpell(SPELL_HELLFIRE);
                return 1;
            }
        }
    }
    return 0;
}

bool CheckTetraSpiritInput(void) {
    s32 directionsPressed;
    s32 down_forward;
    s32 up_forward;
    s32 forward;

    directionsPressed =
        g_Player.padPressed & (PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT);
    if (!g_WasFacingLeft6) {
        forward = PAD_RIGHT;
    } else {
        forward = PAD_LEFT;
    }
    if (!g_WasFacingLeft6) {
        down_forward = PAD_DOWN + PAD_RIGHT;
    } else {
        down_forward = PAD_DOWN + PAD_LEFT;
    }
    if (!g_WasFacingLeft6) {
        up_forward = PAD_UP + PAD_RIGHT;
    } else {
        up_forward = PAD_UP + PAD_LEFT;
    }
    switch (g_ButtonCombo[COMBO_TETRA_SPIRIT].buttonsCorrect) {
    case 0:
        // Must hold UP for 32 frames
        if (g_Player.padTapped == PAD_UP) {
            g_ButtonCombo[COMBO_TETRA_SPIRIT].timer = 32;
            g_ButtonCombo[COMBO_TETRA_SPIRIT].buttonsCorrect++;
            break;
        }
        break;
    case 1:
        if (directionsPressed != PAD_UP) {
            g_ButtonCombo[COMBO_TETRA_SPIRIT].buttonsCorrect = 0;
            break;
        }
        // Counts down the required 32 frames before you go to step 2
        if (--g_ButtonCombo[COMBO_TETRA_SPIRIT].timer == 0) {
            g_ButtonCombo[COMBO_TETRA_SPIRIT].buttonsCorrect++;
            g_WasFacingLeft6 = PLAYER.facingLeft;
        }
        break;
    case 2:
        // After holding UP, you must let go for one frame to move to step 3
        if (directionsPressed != PAD_UP) {
            g_ButtonCombo[COMBO_TETRA_SPIRIT].buttonsCorrect++;
            g_ButtonCombo[COMBO_TETRA_SPIRIT].timer = 32;
        }
        // Precheck on case 3, so you can skip it to 4.
        if ((directionsPressed & up_forward) == up_forward) {
            g_ButtonCombo[COMBO_TETRA_SPIRIT].timer = 20;
            g_ButtonCombo[COMBO_TETRA_SPIRIT].buttonsCorrect = 4;
            break;
        }
        break;
    case 3:
        if ((directionsPressed & up_forward) == up_forward) {
            g_ButtonCombo[COMBO_TETRA_SPIRIT].timer = 20;
            g_ButtonCombo[COMBO_TETRA_SPIRIT].buttonsCorrect = 4;
            break;
        }
        if (--g_ButtonCombo[COMBO_TETRA_SPIRIT].timer == 0) {
            g_ButtonCombo[COMBO_TETRA_SPIRIT].buttonsCorrect = 0;
        }
        break;
    case 4:
        if (directionsPressed == forward) {
            g_ButtonCombo[COMBO_TETRA_SPIRIT].timer = 20;
            g_ButtonCombo[COMBO_TETRA_SPIRIT].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_TETRA_SPIRIT].timer == 0) {
            g_ButtonCombo[COMBO_TETRA_SPIRIT].buttonsCorrect = 0;
        }
        break;
    case 5:
        if ((directionsPressed & down_forward) == down_forward) {
            g_ButtonCombo[COMBO_TETRA_SPIRIT].timer = 20;
            g_ButtonCombo[COMBO_TETRA_SPIRIT].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_TETRA_SPIRIT].timer == 0) {
            g_ButtonCombo[COMBO_TETRA_SPIRIT].buttonsCorrect = 0;
        }
        break;
    case 6:
        if (directionsPressed == PAD_DOWN) {
            g_ButtonCombo[COMBO_TETRA_SPIRIT].timer = 20;
            g_ButtonCombo[COMBO_TETRA_SPIRIT].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_TETRA_SPIRIT].timer == 0) {
            g_ButtonCombo[COMBO_TETRA_SPIRIT].buttonsCorrect = 0;
        }
        break;
    case 7:
        if (--g_ButtonCombo[COMBO_TETRA_SPIRIT].timer == 0) {
            g_ButtonCombo[COMBO_TETRA_SPIRIT].buttonsCorrect = 0;
        }
        if ((g_Player.padTapped & (PAD_SQUARE | PAD_CIRCLE)) &&
            !(g_Player.unk46 & 0x8000) && (PLAYER.step == Player_Crouch) &&
            (CastSpell(SPELL_TETRA_SPIRIT) != 0)) {
            PerformTetraSpirit();
            g_ButtonCombo[COMBO_TETRA_SPIRIT].buttonsCorrect = 0;
            LearnSpell(SPELL_TETRA_SPIRIT);
            return 1;
        }
        break;
    }
    return 0;
}

bool CheckSoulStealInput(void) {
    s32 directionsPressed;
    s32 down_forward;
    s32 down_backward;
    s32 forward;
    s32 backward;
    s32 down;

    directionsPressed =
        g_Player.padPressed & (PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT);
    if (!g_WasFacingLeft7) {
        down_forward = PAD_DOWN + PAD_RIGHT;
        forward = PAD_RIGHT;
        backward = PAD_LEFT;
        down_backward = PAD_DOWN + PAD_LEFT;
    } else {
        down_forward = PAD_DOWN + PAD_LEFT;
        forward = PAD_LEFT;
        backward = PAD_RIGHT;
        down_backward = PAD_DOWN + PAD_RIGHT;
    }
    down = PAD_DOWN;
    switch (g_ButtonCombo[COMBO_SOUL_STEAL].buttonsCorrect) {
    case 0:
        if (PLAYER.facingLeft == 0) {
            if (g_Player.padTapped == PAD_LEFT) {
                g_ButtonCombo[COMBO_SOUL_STEAL].timer = 24;
                g_ButtonCombo[COMBO_SOUL_STEAL].buttonsCorrect++;
                g_WasFacingLeft7 = 0;
            }
            break;
        }
        if (g_Player.padTapped == PAD_RIGHT) {
            // @bug - Most spells use a 20-frame button timing. Soul Steal uses
            // 24. It appears that this spot was forgotten, meaning Soul Steal
            // is easier if you start facing right than left!
            g_ButtonCombo[COMBO_SOUL_STEAL].timer = 20;
            g_ButtonCombo[COMBO_SOUL_STEAL].buttonsCorrect++;
            g_WasFacingLeft7 = 1;
        }
        break;
    case 1:
        if (directionsPressed == forward) {
            g_ButtonCombo[COMBO_SOUL_STEAL].timer = 24;
            g_ButtonCombo[COMBO_SOUL_STEAL].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_SOUL_STEAL].timer == 0) {
            g_ButtonCombo[COMBO_SOUL_STEAL].buttonsCorrect = 0;
        }
        break;
    case 2:
        if ((directionsPressed & down_forward) == down_forward) {
            g_ButtonCombo[COMBO_SOUL_STEAL].timer = 24;
            g_ButtonCombo[COMBO_SOUL_STEAL].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_SOUL_STEAL].timer == 0) {
            g_ButtonCombo[COMBO_SOUL_STEAL].buttonsCorrect = 0;
        }
        break;
    case 3:
        if (directionsPressed == down) {
            g_ButtonCombo[COMBO_SOUL_STEAL].timer = 24;
            g_ButtonCombo[COMBO_SOUL_STEAL].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_SOUL_STEAL].timer == 0) {
            g_ButtonCombo[COMBO_SOUL_STEAL].buttonsCorrect = 0;
        }
        break;
    case 4:
        if ((directionsPressed & down_backward) == down_backward) {
            g_ButtonCombo[COMBO_SOUL_STEAL].timer = 24;
            g_ButtonCombo[COMBO_SOUL_STEAL].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_SOUL_STEAL].timer == 0) {
            g_ButtonCombo[COMBO_SOUL_STEAL].buttonsCorrect = 0;
        }
        break;
    case 5:
        if (directionsPressed == backward) {
            g_ButtonCombo[COMBO_SOUL_STEAL].timer = 24;
            g_ButtonCombo[COMBO_SOUL_STEAL].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_SOUL_STEAL].timer == 0) {
            g_ButtonCombo[COMBO_SOUL_STEAL].buttonsCorrect = 0;
        }
        break;
    case 6:
        if (directionsPressed == forward) {
            g_ButtonCombo[COMBO_SOUL_STEAL].timer = 24;
            g_ButtonCombo[COMBO_SOUL_STEAL].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_SOUL_STEAL].timer == 0) {
            g_ButtonCombo[COMBO_SOUL_STEAL].buttonsCorrect = 0;
        }
        break;
    case 7:
        if (--g_ButtonCombo[COMBO_SOUL_STEAL].timer == 0) {
            g_ButtonCombo[COMBO_SOUL_STEAL].buttonsCorrect = 0;
            break;
        }
        if ((g_Player.padTapped & (PAD_SQUARE | PAD_CIRCLE)) &&
            !(g_Player.unk46 & 0x8000) &&
            MOD_CAST_STEP() && (CastSpell(SPELL_SOUL_STEAL) != 0)) {
            PerformSoulSteal();
            g_ButtonCombo[COMBO_SOUL_STEAL].buttonsCorrect = 0;
            LearnSpell(SPELL_SOUL_STEAL);
            return 1;
        }
        break;
    }
    return 0;
}

bool CheckSwordBrothersInput(void) {
    s32 directionsPressed;
    s32 down_forward;
    s32 forward;
    s32 up_forward;

    directionsPressed =
        g_Player.padPressed & (PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT);
    if (!PLAYER.facingLeft) {
        down_forward = PAD_DOWN + PAD_RIGHT;
        forward = PAD_RIGHT;
        up_forward = PAD_UP + PAD_RIGHT;
    } else {
        down_forward = PAD_DOWN + PAD_LEFT;
        forward = PAD_LEFT;
        up_forward = PAD_UP + PAD_LEFT;
    }

    // Check if sword familiar is currently active. If not, prevent having
    // buttons correct
    if (g_Servant != FAM_ACTIVE_SWORD) {
        g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect = 0;
    }

    switch (g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect) {
    case 0:
        if (directionsPressed == PAD_DOWN) {
            g_ButtonCombo[COMBO_SWORD_BROTHERS].timer = 20;
            g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect++;
        }
        break;
    case 1:
        if (directionsPressed == down_forward) {
            g_ButtonCombo[COMBO_SWORD_BROTHERS].timer = 20;
            g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_SWORD_BROTHERS].timer == 0) {
            g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect = 0;
        }
        break;
    case 2:
        if (directionsPressed == forward) {
            g_ButtonCombo[COMBO_SWORD_BROTHERS].timer = 20;
            g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_SWORD_BROTHERS].timer == 0) {
            g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect = 0;
        }
        break;
    case 3:
        if (directionsPressed == up_forward) {
            g_ButtonCombo[COMBO_SWORD_BROTHERS].timer = 20;
            g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_SWORD_BROTHERS].timer == 0) {
            g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect = 0;
        }
        break;
    case 4:
        if (directionsPressed == PAD_UP) {
            g_ButtonCombo[COMBO_SWORD_BROTHERS].timer = 64;
            g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_SWORD_BROTHERS].timer == 0) {
            g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect = 0;
        }
        break;
    case 5:
        if (--g_ButtonCombo[COMBO_SWORD_BROTHERS].timer == 0) {
            g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect++;
            break;
        }
        if (!(directionsPressed & PAD_UP)) {
            g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect = 0;
        }
        break;
    case 6:
        if (!(directionsPressed & PAD_UP)) {
            g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect++;
            g_ButtonCombo[COMBO_SWORD_BROTHERS].timer = 20;
        }
        break;
    case 7:
        if (directionsPressed == PAD_DOWN) {
            g_ButtonCombo[COMBO_SWORD_BROTHERS].timer = 20;
            g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect++;
            break;
        }
        if (--g_ButtonCombo[COMBO_SWORD_BROTHERS].timer == 0) {
            g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect = 0;
        }
        break;
    case 8:
        if (--g_ButtonCombo[COMBO_SWORD_BROTHERS].timer == 0) {
            g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect = 0;
        }
        if ((g_Player.padTapped & (PAD_SQUARE | PAD_CIRCLE)) &&
            !(g_Player.unk46 & 0x8000)) {
            if (PLAYER.step == Player_Crouch) {
                if (CastSpell(SPELL_SWORD_BROTHERS) != 0) {
                    PerformSwordBrothers();
                    g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect =
                        COMBO_COMPLETE;
                    g_ButtonCombo[COMBO_SWORD_BROTHERS].timer = 20;
                    LearnSpell(SPELL_SWORD_BROTHERS);
                    g_Player.unk70 = 1;
                    return 1;
                }
                break;
            }
            FntPrint("command ok\n");
            break;
        }
        break;
    case COMBO_COMPLETE:
        FntPrint("100sword set ok\n");
        if (--g_ButtonCombo[COMBO_SWORD_BROTHERS].timer == 0) {
            g_ButtonCombo[COMBO_SWORD_BROTHERS].buttonsCorrect = 0;
            g_Player.unk70 = 1;
        }
        break;
    }
    return 0;
}

#endif

void CheckSpecialMoveInputs(void) {
    s32 result;
    s32 i;

    for (i = 0; i < 16; i++) {
        switch (i) {
        case COMBO_GRAVITY_BOOTS:
            result = CheckGravityBootsInput();
            break;
        case COMBO_QCF:
            result = CheckQuarterCircleForwardInput();
            break;
        case COMBO_BF:
            result = CheckBackForwardInput();
            break;
        case COMBO_HELLFIRE:
            result = CheckHellfireInput();
            break;
        case COMBO_TETRA_SPIRIT:
            result = CheckTetraSpiritInput();
            break;
        case COMBO_SOUL_STEAL:
            result = CheckSoulStealInput();
            break;
        case COMBO_SWORD_BROTHERS:
            result = CheckSwordBrothersInput();
            break;
        case COMBO_SUMMON_SPIRIT:
            result = CheckSummonSpiritInput();
            break;
        case COMBO_DARK_METAMORPH:
            result = CheckDarkMetamorphosisInput();
            break;
        default:
            result = 0;
            break;
        }
        if (result != 0) {
            return;
        }
    }
}
