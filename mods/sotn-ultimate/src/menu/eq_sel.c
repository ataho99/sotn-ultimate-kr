// SPDX-License-Identifier: AGPL-3.0-or-later
// ko_equipreloc #128: selector entry. Must stay the first function
// (offset 0 of eq_sel.o .text = KO_EQUIP_SEL).
#include "ko_equip.h"

#if defined(MOD_KO)

void KO_EqSelDraw(const char** names, const s32* xs, const s32* ys,
                  s32 count, MenuContext* ctx) {
    KO_EqDraw(names, xs, ys, count, ctx, 1);
}

#endif
