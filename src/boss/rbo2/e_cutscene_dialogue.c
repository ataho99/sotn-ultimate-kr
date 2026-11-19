// SPDX-License-Identifier: AGPL-3.0-or-later
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

INCLUDE_ASM("boss/rbo2/nonmatchings/e_cutscene_dialogue", EntityCutsceneDialogue);
