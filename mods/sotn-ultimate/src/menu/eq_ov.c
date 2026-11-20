// SPDX-License-Identifier: AGPL-3.0-or-later
// ko_equipreloc: overview entry. Must stay the first function
// (offset 0 of eq_ov.o .text = KO_EQUIP_OV).
#include "ko_equip.h"

#if defined(MOD_KO)

void KO_EqOvDraw(const char** names, const s32* xs, const s32* ys,
                 s32 count, MenuContext* ctx) {
    KO_EqDraw(names, xs, ys, count, ctx, 0);
}

#endif
