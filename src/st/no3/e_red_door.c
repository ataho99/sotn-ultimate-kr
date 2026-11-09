// SPDX-License-Identifier: AGPL-3.0-or-later
#include "no3.h"

#include "../e_red_door.h"

#if defined(MOD_SATURN)
// Saturn port: Richter's and Maria's horse ride
// (mods/sotn-ultimate/tools/gen_no3_horse.py; e_outdoor_ents.c). Rodata: NO3's
// data section must not move.
#define MOD_HORSE_TABLES
#include "mod_horse.h"
#include "mod_layout.h"
#endif
#if defined(MOD_SATURN_CASTLE)
#include "../../../mods/sotn-ultimate/src/castle/no3/e_red_door_mod_shaft.h"
#endif
