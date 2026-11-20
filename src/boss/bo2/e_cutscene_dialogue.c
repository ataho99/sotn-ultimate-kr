// SPDX-License-Identifier: AGPL-3.0-or-later
#if defined(MOD_KO) && !defined(VERSION_PSP) && !defined(VERSION_PC) &&        \
    !defined(VERSION_HD)
/* 시험56 fixBOSS2: Korean dialogue entity in C (src/boss/ko_boss_dialogue.h)
 * with 16 px line buffers (src/st/ko_font_params.h); other builds: INCLUDE_ASM */
#define KO_BOSS_C 1
#define KO_DLG16 1
#endif
#include "bo2.h"
#include "mod_psp_script.h" // the PSP dub toggle's script copies
#include <cutscene.h>

extern Dialogue g_Dialogue;
extern const char* actor_names[];
#ifdef MOD_KO
static const u8 ko_name_0[] = {0xF0, 0x6F, 0xF2, 0x06, 0xF0, 0xB2};
static const u8 ko_name_1[] = {0xF0, 0x50, 0xF2, 0x4E, 0xF2, 0x1E};
static const u8* const actor_names_ko[] = {ko_name_0, ko_name_1};
static const u8 actor_names_ko_len[] = {6, 6};
#endif

#include "../../st/cutscene_unk1.h"

#include "../../st/set_cutscene_script.h"

#include "../../st/cutscene_unk3.h"

#include "../../st/cutscene_unk4.h"

#include "../../st/cutscene_actor_name.h"

#include "../../st/set_cutscene_events.h"

#include "../../st/cutscene_events.h"

#include "../../st/cutscene_skip.h"

#include "../../st/cutscene_scale_avatar.h"

INCLUDE_RODATA("boss/bo2/nonmatchings/e_cutscene_dialogue", D_us_801A1F6C);

INCLUDE_RODATA("boss/bo2/nonmatchings/e_cutscene_dialogue", D_us_801A1F78);

#ifndef KO_BOSS_C
INCLUDE_ASM("boss/bo2/nonmatchings/e_cutscene_dialogue", EntityCutsceneDialogue);

#else
// Minotaurus/Werewolf: Richter's summon (params 0) and Alucard's remark after
// the fight (params 1)
#define KO_BOSS_ENTITY EntityCutsceneDialogue
// the retail entity's data, in place (actor_names - 0xA8 = a flag word, then
// u/v coords, portrait cluts, x_vals, text cluts, red lines[72], actor_names)
#define KO_BOSS_DATA(off) ((u8*)actor_names - (off))
#define KO_BOSS_U_COORDS KO_BOSS_DATA(0xA4)
#define KO_BOSS_V_COORDS KO_BOSS_DATA(0xA0)
#define KO_BOSS_PORTRAIT_CLUTS ((u16*)KO_BOSS_DATA(0x9C))
#define KO_BOSS_X_VALS ((u16*)KO_BOSS_DATA(0x98))
#define KO_BOSS_TEXT_CLUTS ((u16*)KO_BOSS_DATA(0x94))
#define KO_BOSS_RED_LINES ((s16*)KO_BOSS_DATA(0x90))
#define KO_BOSS_PORTRAIT_TPAGE 0x90
#define KO_BOSS_FLAG (*(s32*)KO_BOSS_DATA(0xA8)) // D_us_80180B78
#define KO_BOSS_SKIP g_SkipCutscene
// g_IsCutsceneDone = g_CutsceneFlags + 4 (D_us_801B68F0)
#define KO_BOSS_STARTED (((s32*)&g_CutsceneFlags)[1])
#define KO_BOSS_SWITCH_INDEX(ptr) (*(u16*)(ptr))
extern u8 D_us_80181CCC[]; // the scripts (ko_entry_offsets.BOSS_SCRIPTS bo2)
extern u8 D_us_80181DC5[];

static void KoBossSkipCheck(Entity* self) {
    if (KO_BOSS_STARTED && !KO_BOSS_SKIP) {
        if (g_GameClearFlag) {
            CutsceneSkip(self);
        } else if (self->params) {
            if (g_Settings.D_8003CB04 & 0x4000) {
                CutsceneSkip(self);
            }
        } else if (g_Settings.D_8003CB04 & 0x10) {
            CutsceneSkip(self);
        }
    }
}

static s32 KoBossInit(Entity* self) {
    u8* script;

    if (g_CastleFlags[0xB4] || g_PlayableCharacter != PLAYER_ALUCARD ||
        g_DemoMode || (!self->params && KO_BOSS_FLAG)) {
        DestroyEntity(self);
        return 0;
    }
    script = self->params ? D_us_80181DC5 : D_us_80181CCC;
    if (!SetCutsceneScript(script)) {
        return 0;
    }
    self->flags |= FLAG_HAS_PRIMS | FLAG_UNK_2000;
    g_CutsceneFlags = 0;
    KO_BOSS_STARTED = 0;
    KO_BOSS_SKIP = 0;
    g_CutsceneHasControl = 1;
    self->step++;
    self->primIndex = g_Dialogue.primIndex[2];
    return 0;
}

static void KoBossEnd(Entity* self) {
    if (self->params) {
        g_CastleFlags[0xB4] = 1;
        g_CastleFlags[0x85] = 1;
        g_CastleFlags[0x62] = 1;
        g_Settings.D_8003CB04 |= 0x4300;
    } else {
        KO_BOSS_FLAG = 1;
        stopMusicFlag = 0;
        currentMusicId = 0x31D;
        g_Settings.D_8003CB04 |= 0x10;
    }
    DestroyEntity(self);
    g_CutsceneHasControl = 0;
}

#include "../ko_boss_dialogue.h"
#endif

#ifdef MOD_SATURN
// the PSP English script: room for the lines that do not fit the retail
// script's bytes (mods/sotn-ultimate/tools/psp_script.py; empty when not
// needed)
#endif
