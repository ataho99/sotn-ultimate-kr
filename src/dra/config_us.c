// SPDX-License-Identifier: AGPL-3.0-or-later
// Saturn port: only DRA's copies of the item tables get the modded entries;
// SEL includes the same header and must stay byte-identical to retail.
#define MOD_DRA_CONFIG
// Bytes of g_ModDataPad, see src/config_us.h
// MOD_KO trial (P67): .data +4B absorbed (64 -> 60); retail otherwise.
#if defined(MOD_KO)
#define MOD_DATA_PAD 36
#else
#define MOD_DATA_PAD 64
#endif
#include "../config_us.h"
