// SPDX-License-Identifier: AGPL-3.0-or-later
// SOTN Ultimate: included by src/st/e_nova_skeleton.h (at file scope, in place)
// step up onto a slope that starts on flat floor: retail's walk probes only
// 3 px under the feet, so it walked into such stairs at floor height (the
// Reverse Outer Wall's) until the block behind them stopped it. A slope tile
// just above the feet (their own row is the flat floor's) lifts it to the
// surface; plain blocks are left alone
static void NovaStepUpSlope(void) {
    Collider col;

    g_api.CheckCollision(g_CurrentEntity->posX.i.hi,
                         g_CurrentEntity->posY.i.hi + sensors2[1] - 1, &col, 0);
    if ((col.effects & (EFFECT_UNK_8000 | EFFECT_SOLID)) ==
            (EFFECT_UNK_8000 | EFFECT_SOLID) &&
        col.unk18 < 0) {
        g_CurrentEntity->posY.i.hi += col.unk18;
    }
}
