// SPDX-License-Identifier: AGPL-3.0-or-later
// SOTN Ultimate: included by src/dra/5F60C.c (at file scope, in place)
// Maria's version changed from her pause menu (src/menu/menu_mod_charmenu.h
// ModCharSwap): the old HUD's primitives go (the PSP one is Richter's, with
// a second set; the Saturn one has only the first) and the new one is made
void ModHudSwap(void) {
    FreePrimitives(g_PlayerHud.primIndex1);
    if (!MOD_MARIA_PSP()) { // (it was the PSP one)
        FreePrimitives(g_PlayerHud.primIndex2);
    }
    DrawHud();
}
