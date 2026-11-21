// SPDX-License-Identifier: AGPL-3.0-or-later
#if defined(MOD_KO) && !defined(VERSION_PSP) && !defined(VERSION_PC) &&        \
    !defined(VERSION_HD)
/* Korean dialogue entity in C (src/boss/ko_boss_dialogue.h) with
 * 16 px line buffers (src/st/ko_font_params.h); other builds: INCLUDE_ASM */
#define KO_BOSS_C 1
#define KO_DLG16 1
#endif
#include "rbo6.h"
#include "mod_psp_script.h" // the PSP dub toggle's script copies
#include <cutscene.h>

extern Dialogue g_Dialogue;
extern const char* actor_names[];
#ifdef MOD_KO
static const u8 ko_name_0[] = {0xF0, 0x6F, 0xF2, 0x06, 0xF0, 0xB2};
static const u8 ko_name_1[] = {0xF0, 0xB2, 0xF0, 0x4B, 0xF2, 0x16, 0xF0, 0x4B};
static const u8* const actor_names_ko[] = {ko_name_0, ko_name_1};
static const u8 actor_names_ko_len[] = {6, 8};
#endif

#define CUTSCENE_UNK1_NEXT_X 2
#include "../../st/cutscene_unk1.h"

#include "../../st/set_cutscene_script.h"

#include "../../st/cutscene_unk3.h"

#include "../../st/cutscene_unk4.h"

#include "../../st/cutscene_actor_name.h"

#include "../../st/set_cutscene_events.h"

#define CUTSCENE_TILEMAP_SCROLL
#include "../../st/cutscene_events.h"

#include "../../st/cutscene_skip.h"

#include "../../st/cutscene_scale_avatar.h"

INCLUDE_RODATA("boss/rbo6/nonmatchings/e_cutscene_dialogue", D_us_8019CE94);

INCLUDE_RODATA("boss/rbo6/nonmatchings/e_cutscene_dialogue", D_us_8019CEA0);

#ifndef KO_BOSS_C
INCLUDE_ASM("boss/rbo6/nonmatchings/e_cutscene_dialogue", EntityCutscene);

#else
// Dracula (inverted keep): params 0 = before the fight, 1 = during it
#define KO_BOSS_ENTITY EntityCutscene
// the retail entity's data, in place (upstream .data e_cutscene_dialogue at
// 0x80180864 = actor_names - 0xA8: u/v coords, portrait cluts, x_vals,
// text cluts, red lines[72], actor_names)
#define KO_BOSS_DATA(off) ((u8*)actor_names - (off))
#define KO_BOSS_U_COORDS KO_BOSS_DATA(0xA8)
#define KO_BOSS_V_COORDS KO_BOSS_DATA(0xA4)
#define KO_BOSS_PORTRAIT_CLUTS ((u16*)KO_BOSS_DATA(0xA0))
#define KO_BOSS_X_VALS ((u16*)KO_BOSS_DATA(0x9C))
#define KO_BOSS_TEXT_CLUTS ((u16*)KO_BOSS_DATA(0x98))
#define KO_BOSS_RED_LINES ((s16*)KO_BOSS_DATA(0x90))
#define KO_BOSS_PORTRAIT_TPAGE 0x90
// g_IsCutsceneDone = g_CutsceneFlags + 4 (upstream 0x801B4000)
#define KO_BOSS_SKIP g_SkipCutscene
#define KO_BOSS_STARTED (((s32*)&g_CutsceneFlags)[1])
#define KO_BOSS_SWITCH_INDEX(ptr) (*(u16*)(ptr))
// the scripts (ko_entry_offsets.BOSS_SCRIPTS rbo6; 0x801819E7 is
// mods/sotn-ultimate/config/mod_rel_syms.us.borbo6.txt)
extern u8 D_us_80181720[];
extern u8 D_us_801819E7[];

static void KoBossSkipCheck(Entity* self) {
    if (KO_BOSS_STARTED && !g_SkipCutscene) {
        if (g_GameClearFlag) {
            CutsceneSkip(self);
        } else if (self->params) {
            if (g_Settings.D_8003CB04 & 0x400000) {
                CutsceneSkip(self);
            }
        } else if (g_Settings.D_8003CB04 & 0x200000) {
            CutsceneSkip(self);
        }
    }
}

static s32 KoBossInit(Entity* self) {
    s32 ok;

    if (self->params) {
        D_8003C73C = 2;
        ok = SetCutsceneScript(D_us_801819E7);
    } else {
        ok = SetCutsceneScript(D_us_80181720);
    }
    if (!ok) {
        return 0;
    }
    self->flags |= FLAG_HAS_PRIMS | FLAG_UNK_2000;
    self->primIndex = g_Dialogue.primIndex[2];
    g_CutsceneHasControl = 1;
    g_CutsceneFlags = 0;
    KO_BOSS_STARTED = 0;
    g_SkipCutscene = 0;
    self->step++;
    if (g_PlayableCharacter != PLAYER_ALUCARD || g_DemoMode) {
        KO_BOSS_STARTED = 1;
        g_SkipCutscene = 1;
        g_api.FreePrimitives(self->primIndex);
        self->flags ^= FLAG_HAS_PRIMS;
    }
    return 1; // retail falls into the run step
}

static void KoBossEnd(Entity* self) {
    g_CutsceneHasControl = 0;
    if (self->params) {
        g_CastleFlags[0xE1] = 1; // DRAC_FIGHT_CS_UNK (upstream castle_flags.h)
        g_Settings.D_8003CB04 |= 0x400000;
    } else {
        g_Settings.D_8003CB04 |= 0x200000;
    }
    g_unkGraphicsStruct.unk28 = 0;
    D_8003C73C = 0;
    DestroyEntity(self);
}

#include "../ko_boss_dialogue.h"
#endif
