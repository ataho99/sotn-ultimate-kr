// SPDX-License-Identifier: AGPL-3.0-or-later
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

INCLUDE_ASM("boss/bo6/nonmatchings/cutscene", func_us_801A7DC0);

#ifdef MOD_SATURN
// the PSP English script: room for the lines that do not fit the retail
// script's bytes (mods/sotn-ultimate/tools/psp_script.py; empty when not
// needed)
#endif
