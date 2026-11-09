// SPDX-License-Identifier: AGPL-3.0-or-later
#include "no3.h"

#if defined(MOD_SATURN_CASTLE)
// Saturn port: the retail tables stay as they are (NO3's data section must
// not change, see mods/sotn-ultimate/tools/check_stage_data.py; not static, so
// that they are kept although only the copies below are used)
#define STATIC_RETAIL
#else
#define STATIC_RETAIL static
#endif

STATIC_RETAIL u8 anim_1[] = {4, 1, 4, 2, 0, 0};
STATIC_RETAIL u8 anim_2[] = {4, 0, 4, 0, 0};
STATIC_RETAIL u8* g_eBreakableAnimations[8] = {anim_1, anim_2};
STATIC_RETAIL u8 g_eBreakableHitboxes[8] = {8, 8};
STATIC_RETAIL u8 g_eBreakableExplosionTypes[8] = {0, 0};

STATIC_RETAIL u16 g_eBreakableanimSets[8] = {ANIMSET_DRA(3), ANIMSET_DRA(3)};
STATIC_RETAIL u8 blend_modes[8] = {
    BLEND_TRANSP | BLEND_QUARTER,
    BLEND_TRANSP | BLEND_ADD,
};

// Stripped on PSP
static u8 unused[] = {0, 0, 0, 0, 0, 0, 0, 0};

#if defined(MOD_SATURN_CASTLE)
// the shaft's candelabras, as NP3's
#include "../../../mods/sotn-ultimate/src/castle/np3/e_breakable_mod_sconce.h"
#endif

#include "../e_breakable.h"
