// SPDX-License-Identifier: AGPL-3.0-or-later
#if defined(MOD_KO) && !defined(VERSION_PSP) && !defined(VERSION_PC) &&        \
    !defined(VERSION_HD)
/* fixBOSS: Korean dialogue entity in C (src/boss/ko_boss_dialogue.h) with
 * 16 px line buffers (src/st/ko_font_params.h); other builds: INCLUDE_ASM */
#define KO_BOSS_C 1
#define KO_DLG16 1
#endif
#include "bo6.h"
#include "mod_psp_script.h" // the PSP dub toggle's script copies
#include <cutscene.h>

extern Dialogue g_Dialogue;
extern const char* actor_names[];
#ifdef MOD_KO
static const u8 ko_name_0[] = {0xF0, 0x6F, 0xF2, 0x06, 0xF0, 0xB2};
static const u8 ko_name_1[] = {0xF0, 0x50, 0xF2, 0x4E, 0xF2, 0x1E};
static const u8 ko_name_2[] = {0xF2, 0xB9, 0xF0, 0xA7, 0xF0, 0x90};
static const u8* const actor_names_ko[] = {ko_name_0, ko_name_1, ko_name_2};
static const u8 actor_names_ko_len[] = {6, 6, 6};
#endif

#define CUTSCENE_UNK1_NEXT_X (DISP_STAGE_W + 2)
#include "../../st/cutscene_unk1.h"

#include "../../st/set_cutscene_script.h"

#define CUTSCENE_UNK3_RECT_X DISP_STAGE_W
#include "../../st/cutscene_unk3.h"

#define CUTSCENE_UNK4_TPAGE 0x14
#include "../../st/cutscene_unk4.h"

#include "../../st/cutscene_actor_name.h"

#include "../../st/set_cutscene_events.h"

#define CUTSCENE_TILEMAP_SCROLL
#include "../../st/cutscene_events.h"

#include "../../st/cutscene_skip.h"

#include "../../st/cutscene_scale_avatar.h"

#ifndef KO_BOSS_C
INCLUDE_ASM("boss/bo6/nonmatchings/cutscene", func_us_801A7DC0);
#else
// Richter (castle keep): params 0..3 pick script 0 / 1 / 2 (0 and 2/3 not
// once Shaft's orb is defeated); upstream EntityCutsceneDialogue
#define KO_BOSS_ENTITY func_us_801A7DC0
// the retail entity's data, in place (upstream .data e_cutscene_dialogue at
// 0x80180610 = actor_names - 0xAC: u/v coords, portrait cluts[4], x_vals,
// text cluts[4], red lines[72], actor_names)
#define KO_BOSS_DATA(off) ((u8*)actor_names - (off))
#define KO_BOSS_U_COORDS KO_BOSS_DATA(0xAC)
#define KO_BOSS_V_COORDS KO_BOSS_DATA(0xA8)
#define KO_BOSS_PORTRAIT_CLUTS ((u16*)KO_BOSS_DATA(0xA4))
#define KO_BOSS_X_VALS ((u16*)KO_BOSS_DATA(0x9C))
#define KO_BOSS_TEXT_CLUTS ((u16*)KO_BOSS_DATA(0x98))
#define KO_BOSS_RED_LINES ((s16*)KO_BOSS_DATA(0x90))
#define KO_BOSS_PORTRAIT_TPAGE 0x94
extern s32 g_IsCutsceneDone;
#define KO_BOSS_SKIP g_SkipCutscene
#define KO_BOSS_STARTED g_IsCutsceneDone
// bo6 reads the case index as a byte at the script address (+0x100000)
#define KO_BOSS_SWITCH_INDEX(ptr) (*CS_PTR(ptr))
// the scripts (ko_entry_offsets.BOSS_SCRIPTS bo6; 0x8019A2E1/0x8019A2FB are
// mods/sotn-ultimate/config/mod_rel_syms.us.bobo6.txt) and the two s8 flags
// in front of the first one (upstream D_us_8019A0C8 MARIA_FLAG,
// D_us_8019A0CC item flag)
extern u8 D_us_8019A0D0[];
extern u8 D_us_8019A2E1[];
extern u8 D_us_8019A2FB[];
#define KO_BO6_MARIA_FLAG (*(s8*)(D_us_8019A0D0 - 8))
#define KO_BO6_ITEM_FLAG (*(s8*)(D_us_8019A0D0 - 4))

static void KoBossSkipCheck(Entity* self) {
    if (g_IsCutsceneDone && !g_SkipCutscene) {
        if (g_GameClearFlag) {
            CutsceneSkip(self);
        } else {
            switch (self->params) {
            case 0:
                if (g_Settings.D_8003CB04 & 0x8000) {
                    CutsceneSkip(self);
                }
                break;
            case 2:
                if (g_Settings.D_8003CB04 & 0x10000) {
                    CutsceneSkip(self);
                }
                break;
            case 3:
                if (g_Settings.D_8003CB04 & 0x20000) {
                    CutsceneSkip(self);
                }
                break;
            }
        }
    }
}

static s32 KoBossInit(Entity* self) {
    s32 ok = 0;

    switch (self->params) {
    case 0:
        if (g_DemoMode != Demo_None || g_CastleFlags[SHAFT_ORB_DEFEATED] ||
            g_PlayableCharacter != PLAYER_ALUCARD) {
            DestroyEntity(self);
            return 0;
        }
        ok = SetCutsceneScript(D_us_8019A0D0);
        KO_BO6_MARIA_FLAG = 0;
        if (g_CastleFlags[MET_MARIA_IN_CEN]) {
            KO_BO6_MARIA_FLAG = 1;
        }
        KO_BO6_ITEM_FLAG = 0;
        if (g_api.CheckEquipmentItemCount(0x22, 1)) {
            KO_BO6_ITEM_FLAG = 1;
        }
        break;
    case 1:
        ok = SetCutsceneScript(D_us_8019A2E1);
        break;
    case 2:
    case 3:
        if (g_DemoMode != Demo_None || g_CastleFlags[SHAFT_ORB_DEFEATED]) {
            DestroyEntity(self);
            return 0;
        }
        ok = SetCutsceneScript(D_us_8019A2FB);
        KO_BO6_ITEM_FLAG = self->params - 2;
        break;
    }
    if (ok) {
        self->flags |= FLAG_HAS_PRIMS | FLAG_UNK_2000;
        self->primIndex = g_Dialogue.primIndex[2];
        g_CutsceneHasControl = 1;
        g_CutsceneFlags = 0;
        g_IsCutsceneDone = 0;
        g_SkipCutscene = 0;
        self->step++;
        self->flags |= FLAG_UNK_10000;
    }
    return 0;
}

static void KoBossEnd(Entity* self) {
    switch (self->params) {
    case 0:
        g_Settings.D_8003CB04 |= 0x8000;
        break;
    case 2:
        g_Settings.D_8003CB04 |= 0x10000;
        break;
    case 3:
        g_Settings.D_8003CB04 |= 0x20000;
        g_CastleFlags[SHAFT_ORB_DEFEATED] = 1;
        break;
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
