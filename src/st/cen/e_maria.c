// SPDX-License-Identifier: AGPL-3.0-or-later
#include "cen.h"
#include "game.h"

#if defined(MOD_MARIA_BOSS) && !defined(MOD_CENP)
#include "../../../mods/sotn-ultimate/src/maria_saturn/mod_maria_boss.h"
#endif

#if defined(MOD_CENP)
// Saturn port: BIN/CENP.BIN (cen.h MOD_CENP), the PSP's Maria fight. She is
// the PSP boss's player engine from the start
// (mods/sotn-ultimate/src/maria_psp/mod_cenp.c); after the scene
// (MET_MARIA_IN_CEN) she is gone as in the retail CEN.
void CenpInit(void);
void EntityMaria(Entity* self) {
    if (self->step == 0) {
        if (g_CastleFlags[MET_MARIA_IN_CEN] != 0 || !MOD_MARIA_BOSS_ON()) {
            DestroyEntity(self);
            return;
        }
        CenpInit();
        InitializeEntity(g_EInitSpawner);
        self->flags = FLAG_POS_CAMERA_LOCKED;
        self->animCurFrame = 0;
    }
    EntityMariaPsp(self);
}
#else
void EntityMaria(Entity* self) {
#if defined(MOD_MARIA_BOSS)
    // Saturn 0x18 (060DE850): when the first script ends (the platform sets
    // g_ModMariaFight) she becomes the boss
    if (self->step == 1 && MOD_MARIA_BOSS_ON() && g_ModMariaFight) {
        g_ModMariaFight = 0;
        CreateEntityFromCurrentEntity(E_MOD_MARIA_BOSS, &g_Entities[MB_SLOT]);
        g_Entities[MB_SLOT].params = MB_ID(MB_BODY, 0);
        g_api.TimeAttackController(
            TIMEATTACK_EVENT_GET_HOLYGLASSES, TIMEATTACK_SET_VISITED);
        self->animCurFrame = 0;
        self->step = 16;
        return;
    }
#endif
    if (self->step == 0) {
#if defined(MOD_MARIA_BOSS)
        g_ModMariaFight = 0;
#endif
#if !defined(VERSION_HD)
        /* Has player seen Maria Holy Glasses Cutscene? */
        if (CEN_MARIA_DONE()) {
            DestroyEntity(self);
            return;
        }
#endif
        InitializeEntity(g_EInitSpawner);
        self->flags = FLAG_POS_CAMERA_LOCKED;
        self->animSet = ANIMSET_OVL(1);
        self->animCurFrame = 10;
        self->unk5A = 0x48;
        self->palette = 0x210;
        self->zPriority = 0x80;
    }
}
#endif
