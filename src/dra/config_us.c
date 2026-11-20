// SPDX-License-Identifier: AGPL-3.0-or-later
// Saturn port: only DRA's copies of the item tables get the modded entries;
// SEL includes the same header and must stay byte-identical to retail.
#define MOD_DRA_CONFIG
// Bytes of g_ModDataPad, see src/config_us.h
// MOD_KO trial: .data +4B absorbed (64 -> 60); retail otherwise.
// A3 (0.9.2): cd.c .text +24B is absorbed by MOD_DRA_PAD_KO_BASE (468 -> 444, src/dra/42398.c); MOD_DATA_PAD stays 36 (check_dra_layout.sh verified 3/3).
#if defined(MOD_KO)
#define MOD_DATA_PAD 36
#else
#define MOD_DATA_PAD 64
#endif
#include "../config_us.h"
