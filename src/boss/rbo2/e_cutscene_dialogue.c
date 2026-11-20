// SPDX-License-Identifier: AGPL-3.0-or-later
#if defined(MOD_KO) && !defined(VERSION_PSP) && !defined(VERSION_PC) &&        \
    !defined(VERSION_HD)
/* Korean dialogue entity in C (src/boss/ko_boss_dialogue.h)
 * with 16 px line buffers (src/st/ko_font_params.h); other builds: INCLUDE_ASM */
#define KO_BOSS_C 1
#define KO_DLG16 1
#endif
#include "rbo2.h"
#include "mod_psp_script.h" // the PSP dub toggle's script copies
#include <cutscene.h>

extern Dialogue g_Dialogue;
extern const char* actor_names[];
#ifdef MOD_KO
static const u8 ko_name_0[] = {0xF0, 0x6F, 0xF2, 0x06, 0xF0, 0xB2};
static const u8 ko_name_1[] = {0xF1, 0x3F, 0xF0, 0x68};
static const u8* const actor_names_ko[] = {ko_name_0, ko_name_1};
static const u8 actor_names_ko_len[] = {6, 4};
#endif

#include "../../st/cutscene_unk1.h"

#include "../../st/set_cutscene_script.h"

#include "../../st/cutscene_unk3.h"

#include "../../st/cutscene_unk4.h"

#include "../../st/cutscene_actor_name.h"

#include "../../st/set_cutscene_events.h"

#define CUTSCENE_TILEMAP_SCROLL
#include "../../st/cutscene_events.h"

#include "../../st/cutscene_scale_avatar.h"

INCLUDE_RODATA("boss/rbo2/nonmatchings/e_cutscene_dialogue", D_us_8019AE04);

INCLUDE_RODATA("boss/rbo2/nonmatchings/e_cutscene_dialogue", D_us_8019AE0C);

#ifndef KO_BOSS_C
INCLUDE_ASM("boss/rbo2/nonmatchings/e_cutscene_dialogue", EntityCutsceneDialogue);

#else
// Death: Alucard meets Death before the Richter-castle fight
#define KO_BOSS_ENTITY EntityCutsceneDialogue
// the retail entity's data, in place (actor_names - 0xA8: u/v coords, portrait
// cluts, x_vals, text cluts, red lines[72], actor_names)
#define KO_BOSS_DATA(off) ((u8*)actor_names - (off))
#define KO_BOSS_U_COORDS KO_BOSS_DATA(0xA8)
#define KO_BOSS_V_COORDS KO_BOSS_DATA(0xA4)
#define KO_BOSS_PORTRAIT_CLUTS ((u16*)KO_BOSS_DATA(0xA0))
#define KO_BOSS_X_VALS ((u16*)KO_BOSS_DATA(0x9C))
#define KO_BOSS_TEXT_CLUTS ((u16*)KO_BOSS_DATA(0x98))
#define KO_BOSS_RED_LINES ((s16*)KO_BOSS_DATA(0x90))
#define KO_BOSS_PORTRAIT_TPAGE 0x90
// bss of the retail entity: g_SkipCutscene (g_Dialogue - 4, D_us_801AE850)
// and the started flag (g_CutsceneFlags + 4, D_us_801AE904), relative
#define KO_BOSS_SKIP (((s32*)&g_Dialogue)[-1])
#define KO_BOSS_STARTED (((s32*)&g_CutsceneFlags)[1])
#define KO_BOSS_SWITCH_INDEX(ptr) (*(u16*)(ptr))
extern u8 D_us_80181878[]; // the script (ko_entry_offsets.BOSS_SCRIPTS rbo2)

static void KoBossSkipCheck(Entity* self) {
    if (KO_BOSS_STARTED && !KO_BOSS_SKIP &&
        ((g_Settings.D_8003CB04 & 0x40000) || g_GameClearFlag)) {
        if (g_pads[0].tapped == PAD_START) {
            KO_BOSS_SKIP = 1;
            g_api.FreePrimitives(self->primIndex);
            self->flags ^= FLAG_HAS_PRIMS;
            if (g_Dialogue.primIndex[1] != -1) {
                g_api.FreePrimitives(g_Dialogue.primIndex[1]);
            }
            if (g_Dialogue.primIndex[0] != -1) {
                g_api.FreePrimitives(g_Dialogue.primIndex[0]);
            }
            g_api.PlaySfx(SET_STOP_MUSIC);
            self->step = 1;
            self->step_s = 0;
        }
    }
}

static s32 KoBossInit(Entity* self) {
    if (g_CastleFlags[0xDC] || g_PlayableCharacter != PLAYER_ALUCARD ||
        g_DemoMode) {
        DestroyEntity(self);
        return 0;
    }
    if (!SetCutsceneScript(D_us_80181878)) {
        return 0;
    }
    g_CutsceneHasControl = 1;
    g_CutsceneFlags = 0;
    KO_BOSS_STARTED = 0;
    KO_BOSS_SKIP = 0;
    self->flags |= FLAG_HAS_PRIMS | FLAG_UNK_2000;
    self->step++;
    self->primIndex = g_Dialogue.primIndex[2];
    return 0;
}

static void KoBossEnd(Entity* self) {
    DestroyEntity(self);
    g_CastleFlags[0xDC] = 1;
    g_Settings.D_8003CB04 |= 0x40000;
    g_CutsceneHasControl = 0;
}

#include "../ko_boss_dialogue.h"
#endif
