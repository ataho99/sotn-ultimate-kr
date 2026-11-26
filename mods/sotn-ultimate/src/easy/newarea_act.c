// SPDX-License-Identifier: AGPL-3.0-or-later
// SOTN Ultimate (Korean build): the cheat popup's instant actions (Extra settings -> cheats, the rows after the toggles), linked into the new resident area
// (build_newarea.sh). CheatAct(k) is called by ARC_F2's popup input (header entry +24) when X is pressed on an action row; it runs once and the
// game's own state is changed through its own functions where the game keeps lists (AddToInventory, CheckAndDoLevelUp).
// k: 0 gold 999999, 1 all relics, 2 all spells, 3 all items x9, 4 all equipment x9, 5 stats 99, 6 level up, 7 familiar level up (the Sprite and the
// Nosedevil too: castle flag bytes).
#include <game.h>

extern void AddToInventory(u16 id, EquipKind kind);
extern u32 CheckAndDoLevelUp(void);
extern s32 g_ExpNext[];
extern s32 g_Servant; // 0: none, else 1 + the familiar's index

#define ACT_COUNT 9

void CheatAct(s32 k) {
    s32 i, j;
    FamiliarStats* fs;

    switch (k) {
    case 0:
        g_Status.gold = 999999;
        break;
    case 1:
        for (i = 0; i < NUM_RELICS; i++) {
            if (i >= RELIC_BAT_CARD && i <= RELIC_JP_1) { // the familiar cards: only found (which one is summoned is the player's choice)
                if (g_Status.relics[i] == RELIC_FLAG_DISABLE) {
                    g_Status.relics[i] = RELIC_FLAG_FOUND;
                }
            } else {
                g_Status.relics[i] = RELIC_FLAG_FOUND | RELIC_FLAG_ACTIVE;
            }
        }
        break;
    case 2:
        g_Status.spellsLearnt = 0xFF;
        break;
    case 3: // every hand item (weapons, shields, usable items) up to 9
        for (i = 1; i < HAND_ITEMS_CAP; i++) {
            for (j = g_Status.equipHandCount[i]; j < ACT_COUNT; j++) {
                AddToInventory(i, EQUIP_HAND);
            }
        }
        break;
    case 4: // every body item (armour, helmets, cloaks, accessories) up to 9
        for (i = 1; i < BODY_ITEMS_CAP; i++) {
            for (j = g_Status.equipBodyCount[i]; j < ACT_COUNT; j++) {
                AddToInventory(i, EQUIP_ARMOR);
            }
        }
        break;
    case 5:
        for (i = 0; i < 4; i++) {
            g_Status.statsBase[i] = 99;
        }
        break;
    case 6:
        if (g_Status.level < 99) {
            if (g_Status.exp < (u32)g_ExpNext[g_Status.level + 1]) {
                g_Status.exp = g_ExpNext[g_Status.level + 1];
            }
            CheckAndDoLevelUp();
        }
        break;
    case 7:
        if (g_Servant == 6 || g_Servant == 7) { // the Sprite / the Nosedevil: their stats are castle flag bytes (game.h MOD_CF_FAMILIARS: exp as a little-endian u16, 4 B each; level = exp / 100 + 1)
            u8* f = &g_CastleFlags[MOD_CF_FAMILIARS + (g_Servant - 6) * 4];
            s32 e = f[0] | (f[1] << 8);

            if (e < 9800) {
                e += 100;
                f[0] = e & 0xFF;
                f[1] = e >> 8;
            }
        } else if (g_Servant > 0 && g_Servant <= FAMILIARS_STORED) {
            fs = &g_Status.statsFamiliars[g_Servant - 1];
            if (fs->level < 99) {
                fs->level++;
                fs->exp = (fs->level - 1) * 100;
            }
        }
        break;
    }
}
