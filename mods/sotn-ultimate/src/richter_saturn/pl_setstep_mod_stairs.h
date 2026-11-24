// SPDX-License-Identifier: AGPL-3.0-or-later
// SOTN Ultimate: included by src/ric/pl_setstep.c (at file scope, in place)
// Ultimate: on stairs (a steep slope, not a gentle one) his stairs stance,
// facing up or down them (the slope rises to the left when vram_flag has
// 0x4000: GDB, 2026-11-20); NULL elsewhere, in Classic and the prologue.
// (RIC is only Richter: the balance flag alone)
AnimationFrame* RicStairsAnim(void) {
    if (MOD_PS1_BALANCE() || g_StageId == STAGE_ST0 ||
        (g_Player.vram_flag & (TOUCHING_ANY_SLOPE | TOUCHING_SLIGHT_SLOPE)) !=
            TOUCHING_ANY_SLOPE) {
        return NULL;
    }
    return !(g_Player.vram_flag & TOUCHING_RAISING_SLOPE) == !PLAYER.facingLeft
               ? ric_anim_stairs_up
               : ric_anim_stairs_down;
}
