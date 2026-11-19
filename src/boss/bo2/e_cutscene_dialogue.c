// SPDX-License-Identifier: AGPL-3.0-or-later
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

INCLUDE_ASM("boss/bo2/nonmatchings/e_cutscene_dialogue", EntityCutsceneDialogue);

#ifdef MOD_SATURN
// the PSP English script: room for the lines that do not fit the retail
// script's bytes (mods/sotn-ultimate/tools/psp_script.py; empty when not
// needed)
#endif
