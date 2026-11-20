/* ko_equip_jp_load.c - loader, stubbed: the JP name bundle
 * (121 rows, 1669B) rides the ARC_KO boot chain into KO_JP_BUNDLE_BASE
 * (cd.c CopyKoGlyphsCallback block 14), so no menu-time CD read remains.
 * Kept as a stub: the KO_EQUIP_LOAD slot/symbol is link-asserted and stage
 * one-shot callers (e_shop, w_021) still call it absolutely.
 */
#include "game.h"

void KO_EquipJpLoad(s32 lba) {
    (void)lba;
}
