// SPDX-License-Identifier: AGPL-3.0-or-later
// ko_equipreloc #128: menu-enter entry. Must stay the first function
// (offset 0 of eq_enter.o .text = KO_EQUIP_ENTER).
// SPDX-License-Identifier: AGPL-3.0-or-later
// ko_equipreloc #128: menu-enter entry. Must stay the first function
// (offset 0 of eq_enter.o .text = KO_EQUIP_ENTER).
// #246: EPOCH bump only (forces 8px re-upload from boot RAM). The menu-entry
// CD load is removed: the 8px block rides the ARC_KO boot chain (cd.c:102).
#include "ko_equip.h"

#if defined(MOD_KO)

void KO_EqOnMenuEnter(void) {
    KO_EQ_EPOCH++;
}

#endif
