// SPDX-License-Identifier: AGPL-3.0-or-later
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

#define CUTSCENE_UNK1_NEXT_X 0
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

INCLUDE_ASM("boss/rbo6/nonmatchings/e_cutscene_dialogue", EntityCutscene);
