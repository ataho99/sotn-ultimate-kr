// SPDX-License-Identifier: AGPL-3.0-or-later
// SOTN Ultimate (Korean build): the cheats' tick (099b: moved here from ARC_F2's ModSysOptTick), linked into the new resident area (build_newarea.sh).
// CheatTick(mask) is called every frame of play by ARC_F2's glue with its g_CheatMask; the bit numbers CHEAT_* are game_mod.h's.
#include <game.h>

// absolute symbols from build_sysopt.sh (this build's DRA): HandleDamage's entry and its first two words as built, the upper halfwords of the two
// stair-slowdown branches in func_80109990 (bne 0x1482, made b 0x1000)
extern char ModHdEntry;
extern char ModHdOrig0;
extern char ModHdOrig1;
extern char ModStairsA;
extern char ModStairsB;
void FlushCache(void); // (main SDK: code changed in place)

static void CodeWord(u32* p, u32 v) {
    if (*p != v) {
        *p = v;
        FlushCache();
    }
}

static void CodeHalf(u16* p, u16 v) {
    if (*p != v) {
        *p = v;
        FlushCache();
    }
}

// (must be the area's first function in its object: build_newarea.sh checks the entries it puts in the header)
void CheatTick(s32 m) {
    u32* hd = (u32*)&ModHdEntry;

    if (m & (1 << CHEAT_HP) && g_Status.hp > 0) {
        g_Status.hp = g_Status.hpMax;
    }
    if (m & (1 << CHEAT_MP)) {
        g_Status.mp = g_Status.mpMax;
    }
    if (m & (1 << CHEAT_HEARTS)) {
        g_Status.hearts = g_Status.heartsMax;
    }
    if (m & (1 << CHEAT_GOLD)) {
        g_Status.gold = 9999999;
    }
    // the retail regen is 1 MP per 64 frames (HUD update): 8 times as fast
    if (m & (1 << CHEAT_MPREGEN) && !(g_GameTimer & 7) && !(g_Status.D_80097BF8 & 1)) {
        g_Status.mp++;
        if (g_Status.mp > g_Status.mpMax) {
            g_Status.mp = g_Status.mpMax;
        }
    }
    // potions, food: the heal waiting for the next player update (healKind 1; the Healing Mail's trickle is 2) becomes a full heal
    if (m & (1 << CHEAT_FOOD) && g_Player.healKind == 1 && g_Player.healAmount) {
        g_Player.healAmount = g_Status.hpMax;
    }
    // invincible: HandleDamage returns 0 at once ("jr ra; move v0, zero"); off: the built words back (also after a game load left it patched)
    if (m & (1 << CHEAT_INVINC)) {
        CodeWord(hd, 0x03E00008);
        CodeWord(hd + 1, 0x00001021);
    } else {
        CodeWord(hd, (u32)&ModHdOrig0);
        CodeWord(hd + 1, (u32)&ModHdOrig1);
    }
    // stairs: no slowdown
    CodeHalf((u16*)&ModStairsA, m & (1 << CHEAT_STAIRS) ? 0x1000 : 0x1482);
    CodeHalf((u16*)&ModStairsB, m & (1 << CHEAT_STAIRS) ? 0x1000 : 0x1482);
    // jumps: the double jump (Leap Stone) is allowed again in the air; the moon jump holds the rise while Cross is held
    if (PLAYER.step == Player_Jump || PLAYER.step == Player_Fall) {
        if (m & (1 << CHEAT_JUMP)) {
            g_Player.unk44 &= ~1;
        }
        if (m & (1 << CHEAT_MOON) && (g_Player.padPressed & PAD_CROSS) && PLAYER.velocityY > FIX(-3)) {
            PLAYER.velocityY = FIX(-3);
        }
    }
}
