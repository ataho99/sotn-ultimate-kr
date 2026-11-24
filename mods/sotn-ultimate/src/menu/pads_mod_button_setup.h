// SPDX-License-Identifier: AGPL-3.0-or-later
// SOTN Ultimate: included by src/dra/pads.c (at file scope, in place)
// Richter's / the Marias' own button set-up (include/game_mod.h
// MOD_PAD_WORD), in gameplay: each action's button reads as its default one
static u16 ModPadRemap(u16 p) {
    u32 v;
    s32 f;
    u16 out;

    if (g_GameEngineStep != Engine_Normal || !MOD_CHAR_MENU()) {
        return p;
    }
    v = MOD_PAD_WORD(MOD_PAD_CHAR());
    if (!v) {
        return p;
    }
    out = p & ~(PAD_SHAPES | PAD_SHOULDERS);
    for (f = 0; f < 8; f++) {
        if (p & g_ButtonMask[((v >> (f * 3)) & 7) ^ f]) {
            out |= g_ButtonMask[f];
        }
    }
    return out;
}
